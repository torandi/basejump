#ifndef MOVABLE_LIGHT_H
#define MOVABLE_LIGHT_H

#include <functional>

#include <glm/glm.hpp>

#include "movable_object.hpp"
#include "aabb.hpp"

class MovableLight : public MovableObject {
	private:
		Light * data;
		Shader * shadowmap_shader;
	public:

		static glm::ivec2 shadowmap_resolution;
		static float shadowmap_far_factor;

		enum light_type_t {
			DIRECTIONAL_LIGHT, //position is direction instead
			POINT_LIGHT,
			SPOT_LIGHT
		};

		struct shadow_map_t {
			shadow_map_t(glm::ivec2 size);
			~shadow_map_t();

			void create_fbo();

			glm::ivec2 resolution;
			RenderTarget * fbo;
			TextureBase * texture;
			glm::mat4 matrix;
		} shadow_map;

		MovableLight(Light * light);
		MovableLight();
		MovableLight(const MovableLight &ml);
		virtual ~MovableLight();

		void update(); //Must be called to update position and type in light

		float &constant_attenuation;
		float &linear_attenuation;
		float &quadratic_attenuation;
		float &shadow_bias;
		glm::vec3 &intensity;
		light_type_t type;

		void render_shadow_map(const Camera &camera, const AABB &scene_aabb, std::function<void(const glm::mat4& m)> render_geometry);

	private:
		/* matrices for shadowmap calculations */
		glm::mat4 view_matrix, projection_matrix;
		bool matrices_dirty_;

		void recalculate_matrices();
		void compute_near_and_far(float &near, float &far, const glm::vec3 &min, const glm::vec3 &max, const std::vector<glm::vec3> &points);
	protected:
		virtual void matrix_becomes_dirty();
};

#endif
