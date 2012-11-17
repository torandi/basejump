#ifndef SKY_HPP
#define SKY_HPP

#include <color.hpp>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "camera.hpp"

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

		//TODO: All of them.
		const Color &horizont_color() const;
		const Color &zenit_color() const;

		//TODO: some kind of fog settings and calculations

	private:
		Shader* shader;
		GLuint vbo;
		static const float vertices[2*6*18];

		GLint u_zenit_color;
		GLint u_horizont_color;
		GLint u_sun_color;
		GLint u_sun_aura_color;
		GLint u_sun_position;
		GLint u_sun_radius;
		GLint u_sun_aura_scale;
		GLint u_lerp;

		glm::vec3 sun_position;
		float time_of_day;

		float sun_radius;

		struct sky_data_t {
			float time;
			Color zenit, horizont, sun, sun_aura, sunlight;
			float lerp[2];
			float sun_radius;
			float ambient_amount;
			float sun_aura_scale;
			bool operator<(const sky_data_t & d) const;
		};

		std::vector<sky_data_t> colors;

		sky_data_t current_sun_data;

};

#endif