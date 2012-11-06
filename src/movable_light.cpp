#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "movable_light.hpp"
#include "light.hpp"
#include "texture.hpp"
#include "camera.hpp"
#include "rendertarget.hpp"
#include "camera.hpp"
#include "globals.hpp"
#include "logging.hpp"

#include <cfloat>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/swizzle.hpp>

glm::ivec2 MovableLight::shadowmap_resolution = glm::ivec2(4096, 4096);
float MovableLight::shadowmap_far_factor = 0.5f;

MovableLight::MovableLight(Light * light)
	: MovableObject(light->position)
	, data(light)
	, shadow_map(shadowmap_resolution)
	, constant_attenuation(data->constant_attenuation)
	, linear_attenuation(data->linear_attenuation)
	, quadratic_attenuation(data->quadratic_attenuation)
	, shadow_bias(data->shadow_bias)
	, intensity(data->intensity)
	, type(MovableLight::DIRECTIONAL_LIGHT)
	{
		shadowmap_shader = Shader::create_shader("/shaders/shadowmap");
		update();
	}

MovableLight::MovableLight() :
	  data(new Light())
	, shadow_map(shadowmap_resolution)
	, constant_attenuation(data->constant_attenuation)
	, linear_attenuation(data->linear_attenuation)
	, quadratic_attenuation(data->quadratic_attenuation)
	, shadow_bias(data->shadow_bias)
	, intensity(data->intensity)
	{
		shadowmap_shader = Shader::create_shader("/shaders/shadowmap");
	}

MovableLight::MovableLight(const MovableLight &ml)
	: MovableObject(ml.position())
	, data(ml.data)
	, shadow_map(shadowmap_resolution)
	, constant_attenuation(data->constant_attenuation)
	, linear_attenuation(data->linear_attenuation)
	, quadratic_attenuation(data->quadratic_attenuation)
	, shadow_bias(data->shadow_bias)
	, intensity(data->intensity)
	{
		shadowmap_shader = Shader::create_shader("/shaders/shadowmap");
	}

MovableLight::~MovableLight() { }

void MovableLight::update() {
	data->position = position_;
	data->is_directional = (type == DIRECTIONAL_LIGHT);
	data->matrix = shadow_map.matrix;
	data->shadowmap_scale = glm::vec2(1.f / (float)shadow_map.resolution.x, 1.f / (float)shadow_map.resolution.y);
	if(shadow_map.fbo != NULL) {
		shadow_map.fbo->depth_bind((Shader::TextureUnit) (Shader::TEXTURE_SHADOWMAP_0 + data->shadowmap_index));
	}
}

glm::vec3 MovableLight::calculateFrustrumData(const Camera &cam, float near, float far, glm::vec3 * points) const {
	//Near plane:
	float y = near * tanf(cam.fov() / 2.f);
	float x = y * cam.aspect();

	glm::vec3 lx, ly, lz;
	lx = glm::normalize(cam.local_x());
	ly = glm::normalize(cam.local_y());
	lz = glm::normalize(cam.local_z());

	glm::vec3 near_center = cam.position() + lz * near;
	glm::vec3 far_center = cam.position() + lz * far;

	points[0] = near_center + -x * lx + -y * ly;
	points[1] = near_center + -x * lx +  y * ly;
	points[2] = near_center +  x * lx +  y * ly;
	points[3] = near_center +  x * lx + -y * ly;

	y = far * tanf(cam.fov() / 2.f);
	x = y * cam.aspect();

	points[4] = far_center + -x * lx + -y * ly;
	points[5] = far_center + -x * lx +  y * ly;
	points[6] = far_center +  x * lx +  y * ly;
	points[7] = far_center +  x * lx + -y * ly;

	return cam.position() + far * 0.5f  * lz;
}


void MovableLight::render_shadow_map(const Camera &camera, const AABB &scene_aabb, std::function<void(const glm::mat4& m)> render_geometry) {
	if(shadow_map.fbo == nullptr) shadow_map.create_fbo();

	float near, far;
	near = camera.near();
	far = camera.far() * shadowmap_far_factor;

	glm::mat4 view_matrix, projection_matrix;

	switch(type) {
		case DIRECTIONAL_LIGHT:
			{
				glm::vec3 lightv[3];
				lightv[0] = glm::normalize(position());

				glm::vec3 hint;
				if(fabs(lightv[0].x) <= fabs(lightv[0].y)) {
					if(fabs(lightv[0].x) <= fabs(lightv[0].z)) {
						hint = glm::vec3(1.f, 0.f, 0.f);
					} else {
						hint = glm::vec3(0.f, 0.f, 1.f);
					}
				} else {
					if(fabs(lightv[0].y) <= fabs(lightv[0].z)) {
						hint = glm::vec3(0.f, 1.f, 0.f);
					} else {
						hint = glm::vec3(0.f, 0.f, 1.f);
					}
				}

				lightv[1] = glm::normalize(glm::cross(lightv[0], hint));
				lightv[2] = glm::normalize(glm::cross(lightv[1], lightv[0]));

				glm::vec3 frustrum_corners[8];
				glm::vec3 frustrum_center = calculateFrustrumData(camera, near, far, frustrum_corners);


				glm::vec3 min = glm::vec3(FLT_MAX);
				glm::vec3 max = glm::vec3(-FLT_MAX);

				glm::vec3 diagonal = frustrum_corners[0] - frustrum_corners[6];
				float bound = glm::length(diagonal);

				glm::vec3 eye = frustrum_center - lightv[0] * bound;
				view_matrix = glm::lookAt(glm::vec3(0.f), position(), lightv[2]);

				for(glm::vec3 &corner : frustrum_corners) {
					glm::vec4 tmp = view_matrix * glm::vec4(corner, 1.f);
					glm::vec3 tmp2 = glm::vec3(tmp) / tmp.w;
					min = glm::min(min, tmp2);
					max = glm::max(max, tmp2);
				}

				/*
				 * Padd the ortho transform to prevent it from changing size between frames
				 */
				diagonal = glm::vec3(bound);
				glm::vec3 offset = ( glm::vec3(diagonal) - ( max - min ) ) * 0.5f;
				offset.z = 0.f;
				min -= offset;
				max += offset;

				/* 
				 * Round to shadow map texels
				 */
				glm::vec3 world_units_per_texel = glm::vec3(bound / (float) shadow_map.resolution.x, bound / (float) shadow_map.resolution.y, 1.f);
				min = world_units_per_texel * glm::floor(min / world_units_per_texel);
				max = world_units_per_texel * glm::floor(max / world_units_per_texel);

				/* Now we must find near and far plane */
				float near_plane, far_plane;

				std::vector<glm::vec3> scene_corners = scene_aabb.corners();

				/* Convert corners to light space */
				for(glm::vec3 &v : scene_corners) {
					glm::vec4 tmp = view_matrix * glm::vec4(v, 1.f);
					v = glm::vec3(tmp.x, tmp.y, tmp.z) / tmp.w;
				}


				compute_near_and_far(near_plane, far_plane, min, max, scene_corners);

				projection_matrix = glm::ortho(min.x, max.x, max.y, min.y, near_plane, far_plane);


				break;
			}
		case POINT_LIGHT:
			{
				view_matrix = glm::lookAt(position(), position() + local_z(), local_y());
				projection_matrix = glm::perspective(90.f, 1.f, 0.1f, 100.f);
				break;
			}
		default:
			Logging::fatal("Shadowmaps are only implemented for directional lights at the moment\n");
	}

	shadow_map.matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.5f))
		* glm::scale(glm::mat4(1.f),glm::vec3(0.5f))
		* projection_matrix * view_matrix;

	Shader::upload_projection_view_matrices(projection_matrix, view_matrix);

	shadow_map.fbo->bind();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	shadowmap_shader->bind();
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	render_geometry(glm::mat4());

	shadow_map.fbo->unbind();
}

MovableLight::shadow_map_t::shadow_map_t(glm::ivec2 size) : resolution(size), fbo(nullptr), matrix(1.f){
	texture = Texture2D::from_filename("/textures/white.jpg");
}

MovableLight::shadow_map_t::~shadow_map_t() {
	if(fbo) delete fbo;
}

void MovableLight::shadow_map_t::create_fbo() {
	fbo = new RenderTarget(resolution, GL_RGB8, RenderTarget::DEPTH_BUFFER);
	texture = fbo;

	//Bind depth and set GL_LINEAR
	fbo->depth_bind(Shader::TEXTURE_SHADOWMAP_0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void MovableLight::compute_near_and_far(float &near, float &far, const glm::vec3 &min, const glm::vec3 &max, const std::vector<glm::vec3> &points) {
	struct triangle_t {
		glm::vec3 pt[3];
		bool culled;
	};
	
	//Initialize near and far
	near = FLT_MAX;
	far = -FLT_MAX;

	triangle_t triangle_list[16];

	static const int aabb_tri_indices[] = {
			0,1,2,  1,2,3,
			4,5,6,  5,6,7,
			0,2,4,  2,4,6,
			1,3,5,  3,5,7,
			0,1,4,  1,4,5,
			2,3,6,  3,6,7
	};

	int point_passes_collision[3];

	/* At a high level:
	 * 1. Iterate over all 12 triangles of the AABB.
	 * 2. Clip the triangles against each plane. Create new triangles as needed.
	 * 3. Find the min and max z values as the near and far plane.
	 *
	 * This is mainly converted copy pasta from the msdn-example code for http://msdn.microsoft.com/en-us/library/windows/desktop/ee416324%28v=vs.85%29.aspx
	 */

	 for( int aabb_tri_index = 0; aabb_tri_index < 12; ++aabb_tri_index ) {
        triangle_list[0].pt[0] = points[ aabb_tri_indices[ aabb_tri_index*3 + 0 ] ];
        triangle_list[0].pt[1] = points[ aabb_tri_indices[ aabb_tri_index*3 + 1 ] ];
        triangle_list[0].pt[2] = points[ aabb_tri_indices[ aabb_tri_index*3 + 2 ] ];
        int triangle_cnt = 1;
        triangle_list[0].culled = false;

        // Clip each invidual triangle against the 4 frustum planes.  When ever a triangle is clipped into new triangles,
        //add them to the list.
        for( int frustum_plane_index = 0; frustum_plane_index < 4; ++frustum_plane_index ) {
            float edge;
						glm::vec3 component;

            if( frustum_plane_index == 0 ) {
                edge = min.x;
								component = glm::vec3(1.f, 0.f, 0.f);
            }
            else if( frustum_plane_index == 1 ) {
                edge = max.x;
								component = glm::vec3(1.f, 0.f, 0.f);
            }
            else if( frustum_plane_index == 2 ) {
                edge = min.y;
								component = glm::vec3(0.f, 1.f, 0.f);
            } else {
                edge = max.y;
								component = glm::vec3(0.f, 1.f, 0.f);
            }

            for( int tri_index=0; tri_index < triangle_cnt; ++tri_index ) {
                // We don't delete triangles, so we skip those that have been culled.
                if( !triangle_list[tri_index].culled ) {
                    int iInsideVertCount = 0;
                    glm::vec3 tempOrder;
                    // Test against the correct frustum plane.
                    // This could be written more compactly, but it would be harder to understand.

                    if( frustum_plane_index == 0 ) {
                        for( int triPtIter=0; triPtIter < 3; ++triPtIter ) {
                            if( triangle_list[tri_index].pt[triPtIter].x > min.x ) {
                                point_passes_collision[triPtIter] = 1;
                            } else {
                                point_passes_collision[triPtIter] = 0;
                            }
                            iInsideVertCount += point_passes_collision[triPtIter];
                        }
                    } else if( frustum_plane_index == 1 ) {
                        for( int triPtIter=0; triPtIter < 3; ++triPtIter ) {
                            if( triangle_list[tri_index].pt[triPtIter].x < max.x ) {
                                point_passes_collision[triPtIter] = 1;
                            } else {
                                point_passes_collision[triPtIter] = 0;
                            }
                            iInsideVertCount += point_passes_collision[triPtIter];
                        }
                    } else if( frustum_plane_index == 2 ) {
                        for( int triPtIter=0; triPtIter < 3; ++triPtIter ) {
                            if( triangle_list[tri_index].pt[triPtIter].y > min.y ) {
                                point_passes_collision[triPtIter] = 1;
                            } else {
                                point_passes_collision[triPtIter] = 0;
                            }
                            iInsideVertCount += point_passes_collision[triPtIter];
                        }
                    } else {
                        for( int triPtIter=0; triPtIter < 3; ++triPtIter ) {
                            if( triangle_list[tri_index].pt[triPtIter].y <    max.y ) {
                                point_passes_collision[triPtIter] = 1;
                            } else {
                                point_passes_collision[triPtIter] = 0;
                            }
                            iInsideVertCount += point_passes_collision[triPtIter];
                        }
                    }

                    // Move the points that pass the frustum test to the begining of the array.
                    if( point_passes_collision[1] && !point_passes_collision[0] ) {
                        tempOrder =  triangle_list[tri_index].pt[0];
                        triangle_list[tri_index].pt[0] = triangle_list[tri_index].pt[1];
                        triangle_list[tri_index].pt[1] = tempOrder;
                        point_passes_collision[0] = true;
                        point_passes_collision[1] = false;
                    }

                    if( point_passes_collision[2] && !point_passes_collision[1] ) {
                        tempOrder =  triangle_list[tri_index].pt[1];
                        triangle_list[tri_index].pt[1] = triangle_list[tri_index].pt[2];
                        triangle_list[tri_index].pt[2] = tempOrder;
                        point_passes_collision[1] = true;
                        point_passes_collision[2] = false;
                    }
                    if( point_passes_collision[1] && !point_passes_collision[0] ) {
                        tempOrder =  triangle_list[tri_index].pt[0];
                        triangle_list[tri_index].pt[0] = triangle_list[tri_index].pt[1];
                        triangle_list[tri_index].pt[1] = tempOrder;
                        point_passes_collision[0] = true;
                        point_passes_collision[1] = false;
                    }

                    if( iInsideVertCount == 0 ) { // All points failed. We're done,
                        triangle_list[tri_index].culled = true;
                    } else if( iInsideVertCount == 1 ) {// One point passed. Clip the triangle against the Frustum plane
                        triangle_list[tri_index].culled = false;

                        //
                        glm::vec3 vVert0ToVert1 = triangle_list[tri_index].pt[1] - triangle_list[tri_index].pt[0];
                        glm::vec3 vVert0ToVert2 = triangle_list[tri_index].pt[2] - triangle_list[tri_index].pt[0];

                        // Find the collision ratio.
                        float fHitPointTimeRatio = edge - glm::dot(triangle_list[tri_index].pt[0], component);
                        // Calculate the distance along the vector as ratio of the hit ratio to the component.
                        float fDistanceAlongVector01 = fHitPointTimeRatio / glm::dot(vVert0ToVert1, component );
                        float fDistanceAlongVector02 = fHitPointTimeRatio / glm::dot(vVert0ToVert2, component );
                        // Add the point plus a percentage of the vector.
                        vVert0ToVert1 *= fDistanceAlongVector01;
                        vVert0ToVert1 += triangle_list[tri_index].pt[0];
                        vVert0ToVert2 *= fDistanceAlongVector02;
                        vVert0ToVert2 += triangle_list[tri_index].pt[0];

                        triangle_list[tri_index].pt[1] = vVert0ToVert2;
                        triangle_list[tri_index].pt[2] = vVert0ToVert1;

                    } else if( iInsideVertCount == 2 ) { // 2 in  // tesselate into 2 triangles


                        // Copy the triangle\(if it exists) after the current triangle out of
                        // the way so we can override it with the new triangle we're inserting.
                        triangle_list[triangle_cnt] = triangle_list[tri_index+1];

                        triangle_list[tri_index].culled = false;
                        triangle_list[tri_index+1].culled = false;

                        // Get the vector from the outside point into the 2 inside points.
                        glm::vec3 vVert2ToVert0 = triangle_list[tri_index].pt[0] - triangle_list[tri_index].pt[2];
                        glm::vec3 vVert2ToVert1 = triangle_list[tri_index].pt[1] - triangle_list[tri_index].pt[2];

                        // Get the hit point ratio.
                        float fHitPointTime_2_0 =  edge - glm::dot(triangle_list[tri_index].pt[2], component );
                        float fDistanceAlongVector_2_0 = fHitPointTime_2_0 / glm::dot(vVert2ToVert0, component );
                        // Calcaulte the new vert by adding the percentage of the vector plus point 2.
                        vVert2ToVert0 *= fDistanceAlongVector_2_0;
                        vVert2ToVert0 += triangle_list[tri_index].pt[2];

                        // Add a new triangle.
                        triangle_list[tri_index+1].pt[0] = triangle_list[tri_index].pt[0];
                        triangle_list[tri_index+1].pt[1] = triangle_list[tri_index].pt[1];
                        triangle_list[tri_index+1].pt[2] = vVert2ToVert0;

                        //Get the hit point ratio.
                        float fHitPointTime_2_1 =  edge - glm::dot(triangle_list[tri_index].pt[2], component );
                        float fDistanceAlongVector_2_1 = fHitPointTime_2_1 / glm::dot( vVert2ToVert1,component );
                        vVert2ToVert1 *= fDistanceAlongVector_2_1;
                        vVert2ToVert1 += triangle_list[tri_index].pt[2];
                        triangle_list[tri_index].pt[0] = triangle_list[tri_index+1].pt[1];
                        triangle_list[tri_index].pt[1] = triangle_list[tri_index+1].pt[2];
                        triangle_list[tri_index].pt[2] = vVert2ToVert1;
                        // Cncrement triangle count and skip the triangle we just inserted.
                        ++triangle_cnt;
                        ++tri_index;


                    } else { // all in
                        triangle_list[tri_index].culled = false;

                    }
                }// end if !culled loop
            }
        }

        for( int index=0; index < triangle_cnt; ++index ) {
            if( !triangle_list[index].culled ) {
                // Set the near and far plan and the min and max z values respectivly.
                for( int vertind = 0; vertind < 3; ++ vertind ) {
                    float tri_z = triangle_list[index].pt[vertind].z;
                    if( near > tri_z ) {
                        near = tri_z;
                    }
                    if( far < tri_z ) {
                        far = tri_z;
                    }
                }
            }
        }
    }

}
