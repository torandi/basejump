#ifndef SKY_HPP
#define SKY_HPP

#include <color.hpp>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "camera.hpp"
#include "shader.hpp"

class Sky {
	public:
		Sky(const std::string &file, float t=0.5f);
		~Sky();

		void render(const Camera &camera) const;

		/*
		 * @param t Time of day, 0 - 1: 0 midnight, 0.5 noon
		 */
		void set_time_of_day(float t);

		float time() const { return time_of_day; };

		/*
		 * Configure light to be sun
		 */
		void configure_light(MovableLight *light) const;

		/*
		 * Get current ambient light
		 */
		glm::vec3 ambient_intensity() const;

		const Shader::sky_data_t &sky_data() const { return current_sun_data; };


	private:
		Shader* shader;
		GLuint vbo;
		static const float vertices[2*6*18];

		float time_of_day;

		float sun_radius;

		struct sky_data_t {
			float time;
			Color zenit, horizont, sun, sun_aura, sunlight;
			float lerp_size;
			float lerp_offset;
			float sun_radius;
			float ambient_amount;
			float sun_aura_scale;
			bool operator<(const sky_data_t & d) const;
		};

		std::vector<sky_data_t> colors;

		Shader::sky_data_t current_sun_data;
		float ambient_amount;
		glm::vec3 sunlight;

};

#endif
