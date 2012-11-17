#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "terrain.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "color.hpp"
#include "triangle2d.hpp"
#include "line2d.hpp"
#include "intersect2d.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <glm/gtx/norm.hpp>

#define RENDER_DEBUG 0

#if RENDER_DEBUG
	static Shader * debug_shader;
#endif

float Terrain::culling_fov_factor = 1.2f;
float Terrain::culling_near_padding = 1.5f;
float Terrain::lod_distance[TERRAIN_LOD_LEVELS];

int Terrain::LOD = 1;

Terrain::~Terrain() {
	if(map_ != NULL)
		delete map_;
	free_surface();
}

Terrain::Terrain(const std::string &file) : Mesh(32.f) {
	Config config = Config::parse(file);

	data_map_  = TextureBase::load_image(config["/heightmap"]->as_string() , &size_);
	horizontal_scale_ = config["/horizontal_size"]->as_float() / static_cast<float>(size_.x);
	vertical_scale_ = config["/vertical_size"]->as_float();;
	uv_scale_ = config["/uv_repeat"]->as_float();
	set_partition_size(config["/submesh_size"]->as_float());
	const std::vector<ConfigEntry*> texture_selection = config["/texture_selection"]->as_list();
	texture_selection_[0] = glm::radians(texture_selection[0]->as_float());
	texture_selection_[1] = glm::radians(texture_selection[1]->as_float()) - texture_selection_[0];
	/* 
	 * second value is given as end, but algorithm handles length (delta)
	*/

	diffuse_textures_ = TextureArray::from_filename(
			config["/materials/flat/diffuse"]->as_string().c_str(),
			config["/materials/steep/diffuse"]->as_string().c_str(),
			config["/materials/override/diffuse"]->as_string().c_str(),
			nullptr
		);

	normal_textures_ = TextureArray::from_filename(
			config["/materials/flat/normal"]->as_string().c_str(),
			config["/materials/steep/normal"]->as_string().c_str(),
			config["/materials/override/normal"]->as_string().c_str(),
			nullptr
		);

	material_shininess_[0] = config["/materials/flat/shininess"]->as_float();
	material_shininess_[1] = config["/materials/steep/shininess"]->as_float();
	material_shininess_[2] = config["/materials/override/shininess"]->as_float();

	material_specular_[0] = config["/materials/flat/specular"]->as_color().to_vec4();
	material_specular_[1] = config["/materials/steep/specular"]->as_color().to_vec4();
	material_specular_[2] = config["/materials/override/specular"]->as_color().to_vec4();

	diffuse_textures_->texture_bind(Shader::TEXTURE_ARRAY_0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	normal_textures_->texture_bind(Shader::TEXTURE_ARRAY_0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	normal_textures_->texture_unbind();

	shader_ = Shader::create_shader("/shaders/terrain");
	u_texture_selection_[0] = shader_->uniform_location("texture_fade_start");
	u_texture_selection_[1] = shader_->uniform_location("texture_fade_length");

	u_material_shininess_ = shader_->uniform_location("material_shininess");
	u_material_specular_ = shader_->uniform_location("material_specular");

	generate_terrain();

#if RENDER_DEBUG
	debug_shader = Shader::create_shader("/shaders/debug");
#endif
}

void Terrain::free_surface() {
	if(data_map_ != nullptr) {
		SDL_FreeSurface(data_map_);
		data_map_ = nullptr;
	}
}

const glm::ivec2 &Terrain::heightmap_size() const {
	return size_;
}

void Terrain::generate_terrain() {
	unsigned long numVertices = size_.x*size_.y;

	map_ = new float[numVertices];

	Logging::verbose("Generating terrain...\n"
	                 "  - World size: %dx%d\n"
	                 "  - scale: %fx%f\n", size_.x, size_.y, horizontal_scale_, vertical_scale_);

	vertices_ = std::vector<Shader::vertex_t>(numVertices);


	int filter_offset_x[] = {
		-1, 0, 1,
		-1, 0, 1,
		-1, 0, 1,
	};

	int filter_offset_y[] = {
		-1, -1, -1,
		 0,  0,  0,
		 1,  1,  1,
	};

	float filter_kernel[] = {
		1.f, 1.f, 1.f,
		1.f, 1.f, 1.f,
		1.f, 1.f, 1.f,
	};

	int filter_size = sizeof(filter_offset_x) / sizeof(int);

	float inverse_filter_sum = 0.f;
	for(int i=0; i<filter_size; ++i) {
		inverse_filter_sum += filter_kernel[i];
	}
	inverse_filter_sum = 1.f/inverse_filter_sum;

	for(int y=0; y<size_.y; ++y) {
		for(int x=0; x<size_.x; ++x) {
			Shader::vertex_t v;
			int i = y * size_.x + x;

			float h = 0.f;
			if(x == 0 || y == 0 || x == size_.x || y == size_.y) {
				//On edge, can't run box filter:
				h = height_from_color(get_pixel_color(x, y, data_map_, size_));
			} else {
				for(int f = 0; f<filter_size; ++f) {
					h += height_from_color(get_pixel_color(x + filter_offset_x[f], y + filter_offset_y[f], data_map_, size_)) * filter_kernel[f] * inverse_filter_sum;
				}
			}

			v.pos = glm::vec3(horizontal_scale_*static_cast<float>(x), h*vertical_scale_, horizontal_scale_*static_cast<float>(y));
			v.uv = glm::vec2(static_cast<float>(x) / static_cast<float>(size_.x), 1.f - static_cast<float>(y) / static_cast<float>(size_.y)) * uv_scale_;
			vertices_[i] = v;
			map_[i] =  h*vertical_scale_;
		}
	}
	unsigned long indexCount[TERRAIN_LOD_LEVELS];
	indexCount[0] = (size_.y - 1 ) * (size_.x -1) * 6;

	glm::ivec2 sizes[TERRAIN_LOD_LEVELS];
	int scale_factor[TERRAIN_LOD_LEVELS];
	scale_factor[0] = 1;
	sizes[0] = size_;

	//build indices
	std::vector<unsigned int> indices[TERRAIN_LOD_LEVELS];
	indices[0] = std::vector<unsigned int>(indexCount[0]);
	for(int i=1; i<TERRAIN_LOD_LEVELS; ++i) {
		scale_factor[i] = scale_factor[i - 1] * 2;
		sizes[i] = sizes[i-1] / 2;
		indexCount[i] = (sizes[i].y - 1 ) * (sizes[i].x -1) * 6;
		indices[i] = std::vector<unsigned int>(indexCount[i]);
	}

	/*
	 * Oh the complexity :(
	 * Well, it's only loading phase anyway
	 */
	for(int lod=0; lod<TERRAIN_LOD_LEVELS; ++lod) {
		for(int x=0; x<sizes[lod].x- 1; ++x) {
			for(int y=0; y<sizes[lod].y- 1; ++y) {
				int i = y * (sizes[lod].x-1) + x;
				int vx, vy;
				vx = x * scale_factor[lod];
				vy = y * scale_factor[lod];
				indices[lod][6 * i + 0] = vx + vy*size_.x;
				indices[lod][6 * i + 1] = vx + (vy+scale_factor[lod])*size_.x;
				indices[lod][6 * i + 2] = (vx + scale_factor[lod]) + vy*size_.x;

				indices[lod][6 * i + 3] = vx + (vy+scale_factor[lod])*size_.x;
				indices[lod][6 * i + 4] = (vx+scale_factor[lod]) + (vy+scale_factor[lod])*size_.x;
				indices[lod][6 * i + 5] = (vx + scale_factor[lod]) + vy*size_.x;
			}
		}
	}

	Logging::info("[Terrain] Partitioning mesh\n");

	for(int i=0; i<TERRAIN_LOD_LEVELS; ++i) {
		add_indices(indices[i], i);
		lod_distance[i] = glm::pow(400.f * static_cast<float>(scale_factor[i]), 2.f);
	}

	Logging::info("[Terrain] Generate normals.\n");
	generate_normals();
	Logging::info("[Terrain] Generate tangent space.\n");
	generate_tangents_and_bitangents();
	Logging::info("[Terrain] Ortonormalize tangent space.\n");
	ortonormalize_tangent_space();
	Logging::info("[Terrain] Create buffers.\n");
	generate_vbos();
	Logging::info("[Terrain] Generating LOD.\n");

	Logging::info("[Terrain] Terrain loaded.\n");
}

float Terrain::height_from_color(const glm::vec4 &color) const {
	return color.r + color.g;
}

float Terrain::height_at(int x, int y) const {
	return map_[y*size_.x + x];
}

float Terrain::height_at(float x_, float y_) const {
	printf("%f, %f\n", x_, y_);
	int x = (int) (x_/horizontal_scale_);
	int y = (int) (y_/horizontal_scale_);
	if(x > size_.x || x < 0 || y > size_.y || y_ < 0)
		return 0.f;
	float dx = (x_/horizontal_scale_) - (float)x;
	float dy = (y_/horizontal_scale_) - (float)y;
	float height=0;
	height += (1.f-dx) * (1.f-dy) * height_at(x,y);
	height += dx * (1.f-dy) * height_at(y,(x+1));
	height += (1.f-dx) * dy * height_at((y+1),x);
	height += dx * dy * height_at((y+1), (x+1));
	return height;
}

const glm::vec3 &Terrain::normal_at(int x, int y) const {
	return vertices_[y*size_.x + x].normal;
}

glm::vec3 Terrain::normal_at(float x_, float y_) const {
	if(x_ > static_cast<float>(size_.x) * horizontal_scale_|| x_ < 0 || y_ > static_cast<float>(size_.y)*horizontal_scale_ || y_ < 0)
		return glm::vec3(0.f, 1.f, 0.f);
	int x = (int) (x_/horizontal_scale_);
	int y = (int) (y_/horizontal_scale_);
	float dx = (x_/horizontal_scale_) - (float)x;
	float dy = (y_/horizontal_scale_) - (float)y;
	glm::vec3 normal(0.f);
	normal += (1.f-dx) * (1.f-dy) * normal_at(x,y);
	normal += dx * (1.f-dy) * normal_at(y,(x+1));
	normal += (1.f-dx) * dy * normal_at((y+1),x);
	normal += dx * dy * normal_at((y+1), (x+1));
	return normal;
}

glm::vec4 Terrain::get_pixel_color(int x, int y, SDL_Surface * surface, const glm::ivec2 &size) {
	glm::ivec4 c = TextureBase::get_pixel_color(x, y, surface, size);
	glm::vec4 color;
	color.r = (float)c.x/0xFF;
	color.g = (float)c.y/0xFF;
	color.b = (float)c.z/0xFF;
	color.a = (float)c.w/0xFF;

	return color;
}

void Terrain::render(const glm::mat4& m) {
	prepare_shader();

	Mesh::render();

#if RENDER_DEBUG
	//Render debug:
	glLineWidth(2.0f);
	debug_shader->bind();

	Mesh::render();
#endif
}

void Terrain::prepare_shader() {
	shader_->bind();
	glUniform1f(u_texture_selection_[0], texture_selection_[0]);
	glUniform1f(u_texture_selection_[1], texture_selection_[1]);
	glUniform1fv(u_material_shininess_, 3, material_shininess_);
	glUniform4fv(u_material_specular_, 3, (float*)material_specular_);

	diffuse_textures_->texture_bind(Shader::TEXTURE_ARRAY_0);
	normal_textures_->texture_bind(Shader::TEXTURE_ARRAY_1);
}

void Terrain::render_cull(const Camera &cam, const glm::mat4& m) {

	prepare_shader();

	prepare_submesh_rendering(m);
	//We assume no roll (we could widen the frustrum a bit to take some roll into account)
	
	Triangle2D cam_tri = calculate_camera_tri(cam);
	
	AABB_2D aabb2d;
	aabb2d.add_point(cam_tri.p1);
	aabb2d.add_point(cam_tri.p2);
	aabb2d.add_point(cam_tri.p3);

	submesh_tree->traverse(std::bind(&Terrain::cull_or_render, cam_tri, aabb2d, std::placeholders::_1));
}

void Terrain::render_geometry_cull( const Camera &cam, const glm::mat4& m) {
	render_geometry_cull(cam, aabb(), m);
}

void Terrain::render_geometry_cull( const Camera &cam, const AABB &aabb, const glm::mat4& m) {
	prepare_submesh_rendering(m);

	Triangle2D cam_tri = calculate_camera_tri(cam);

	AABB_2D aabb2d(glm::vec2(aabb.min.x, aabb.min.z), glm::vec2(aabb.max.x, aabb.max.z));

	submesh_tree->traverse(std::bind(&Terrain::cull_or_render, cam_tri, aabb2d, std::placeholders::_1));
}

Triangle2D Terrain::calculate_camera_tri(const Camera& cam) {
	glm::vec3 corners[8];

	cam.frustrum_corners(corners, cam.near(), cam.far(), cam.fov() * culling_fov_factor);

	glm::vec2 points[3];

	points[0] = glm::vec2(cam.position().x, cam.position().z) - glm::normalize(glm::vec2(cam.local_z().x, cam.local_z().z)) * culling_near_padding;
	points[1] = glm::vec2(corners[4].x, corners[4].z);
	points[2] = glm::vec2(corners[6].x, corners[6].z);
	
	return Triangle2D(
			points[0],
			points[1],
			points[2]
			);

}

bool Terrain::cull_or_render(const Triangle2D &cam_tri, const AABB_2D &limiting_box, QuadTree * node) {
	//if(intersect2d::aabb_aabb(node->aabb, limiting_box) && intersect2d::aabb_triangle(node->aabb, cam_tri)) {
		float d = glm::distance2(node->aabb.middle(), cam_tri.p1);
		int lod = TERRAIN_LOD_LEVELS - 1;
		for(int i=0; i< TERRAIN_LOD_LEVELS; ++i) {
			if(d < lod_distance[i]) {
				lod = i;
				break;
			}
		}

		if(node->level() <= lod) {
			if(node->data != nullptr) ( (SubMesh*) node->data )->render_geometry();
			return false;
		}
		return true;
	/*} else {
		return false;
	}*/
}
