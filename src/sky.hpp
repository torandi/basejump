#ifndef SKY_HPP
#define SKY_HPP

#include <color.hpp>
#include <string>
#include <vector>
#include <GL/glew.h>

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

	private:
		Shader* shader;
		GLuint vbo;
		static const float vertices[2*3*36];

		GLint u_zenit_color;
		GLint u_horizont_color;
		GLint u_sun_color;
		GLint u_sun_aura_color;
		GLint u_sun_position;
		GLint u_sun_radius;

		float time_of_day;

		float sun_radius;

		struct sky_data_t {
			float time;
			Color zenit, horizont, sun, sunlight;
			bool operator<(const sky_data_t & d) const;
		};

		std::vector<sky_data_t> colors;

		sky_data_t current_sun_data;

};

#endif
