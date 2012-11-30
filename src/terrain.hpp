#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <SDL/SDL.h>

#include "shader.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "texture.hpp"

#pragma managed(push,off)
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#pragma managed(pop)

#include "PerlinNoise.hpp"



#define TERRAIN_LOD_LEVELS 8



class Terrain : public Mesh
{
	PerlinNoise perlin;

	btTransform trans_;
	btMatrix3x3 & rot_;
	btVector3 & pos_;

	btHeightfieldTerrainShape * shape;
	btDefaultMotionState * motionState;
	btRigidBody * rigidBody;
	btCollisionObject * collisionObject;

	float * collision_map_data;

	double H, lacunarity, octaves, offset, gain;
	double amplitude, density, offsetX, offsetY;

	void init_physics();
	void cleanup_physics();

	inline int clamp(int x, int min, int max);
	void update_collision_map(const btVector3 & protagonist_position);



	float horizontal_scale_;
	float vertical_scale_;
	SDL_Surface * data_map_;
	float uv_scale_;
	Shader * shader_;
	glm::ivec2 size_;
	float * map_;

	float texture_selection_[2];
	GLuint u_texture_selection_[2];

	glm::vec4 material_specular_[3];
	float material_shininess_[3];
	GLuint u_material_specular_;
	GLuint u_material_shininess_;

	void generate_terrain();

	float height_from_color(const glm::vec4 &color) const ;

	TextureArray * diffuse_textures_, *normal_textures_;

	float height_at(int x, int y) const;
	const glm::vec3 &normal_at(int x, int y) const;

	const glm::ivec2& heightmap_size() const;

	static bool cull_or_render(const Triangle2D &cam_tri, const AABB_2D &near_aabb, const AABB_2D &limiting_box, QuadTree * node);

	/**
	 * Calculates camera 2d triangle approximation, and fills near_aabb aabb for near
	 */
	static Triangle2D calculate_camera_tri(const Camera& cam, AABB_2D &near_aabb);

	static float lod_distance[TERRAIN_LOD_LEVELS];

	unsigned int extra_vertex(std::map<glm::vec2, unsigned int, bool(*)(const glm::vec2&, const glm::vec2&)> &extra_vertices, const Shader::vertex_t &v, const glm::vec3 &normal);

	float lod_base_step;

	public:


		float vertical_scale() { return vertical_scale_; };

		/**
		 * Create a new terrain-mesh.
		 *
		 * @param file Filename with config.
		 *	heighmap is:
		 *              - red is height
		 *              - green is forced mix in of second texture
		 *              - blue is reserved for future use
		 */
		Terrain(const std::string &file);
		virtual ~Terrain();

		virtual void render(const glm::mat4& m = glm::mat4());

		void render_cull(const Camera &cam, const glm::mat4& m = glm::mat4());
		void render_geometry_cull( const Camera &cam, const glm::mat4& m = glm::mat4());
		void render_geometry_cull( const Camera &cam, const AABB &aabb, const glm::mat4& m = glm::mat4());

		static glm::vec4 get_pixel_color(int x, int y, SDL_Surface * surface, const glm::ivec2 &size);

		float height_at(float x, float y) const;
		glm::vec3 normal_at(float x, float y) const;

		float horizontal_size() const;

		void prepare_shader();

		/*
		 * Once this has been called get_pixel_color can not be called
		 */
		void free_surface();


		/*
		 * Configuration for culling. If you don't intend to use full roll (or maybee even then)
		 * the default values will probably do
		 */
		static float culling_fov_factor; /* How much fov is taken into account when culling (multiplied with fov)*/
		static float culling_near_padding; /* How far back camera point (near plan single point in triangle) 
																					is moved from camera position */
};

#endif
