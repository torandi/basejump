#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sky.hpp"
#include "camera.hpp"
#include "utils.hpp"
#include "shader.hpp"
#include "config.hpp"
#include "movable_light.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Sky::Sky(const std::string &file, float t) : time_of_day(t) {
	//Generate skybox buffers:
	shader = Shader::create_shader("/shaders/sky");
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Config config = Config::parse(file);

	for(ConfigEntry * c : config["/colors"]->as_list()) {
		sky_data_t d;
		d.zenit = c->find("/zenit", true)->as_color();
		d.horizont = c->find("/horizont", true)->as_color();
		d.sun = c->find("/sun", true)->as_color();
		d.sun_aura = c->find("/sun_aura", true)->as_color();
		d.sunlight = c->find("/sunlight", true)->as_color();
		d.lerp_size = c->find("/lerp_size", true)->as_float();
		d.lerp_offset = c->find("/lerp_offset", true)->as_float();
		d.sun_radius = c->find("/sun_radius", true)->as_float();
		d.ambient_amount = c->find("/ambient_amount", true)->as_float();
		/* Store scale inverse, since smaller number => bigger gradient, but it's more logical
		 * to specify bigger number => larger aura.
		 * Also, for scales > 1.0 the colors go bananas, so don't allow that
		 */
		d.sun_aura_scale = glm::min(1.f / c->find("/sun_aura_scale", true)->as_float(), 1.f);
		const ConfigEntry * time = c->find("/time", true);
		if(time->type == ConfigEntry::ENTRY_LIST) {
			for(const ConfigEntry * t : time->as_list()) {
				d.time = t->as_float();
				colors.push_back(d);
			}
		} else {
			d.time = time->as_float();
			colors.push_back(d);
		}
	}

	std::sort(colors.begin(), colors.end()); //Sort on time

	current_sun_data.sun_position.w = 1.f; //Never changes, set once

	set_time_of_day(t);
}

Sky::~Sky() {
	glDeleteBuffers(1, &vbo);
}

void Sky::render(const Camera &camera) const{
	shader->bind();

	glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	Shader::upload_projection_view_matrices(
			camera.projection_matrix(),
			glm::lookAt(glm::vec3(0.0), camera.look_at()-camera.position(), camera.up())
	);

	/* Disable most attribs from Shader::vertex_x */
	Shader::push_vertex_attribs(2);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)*3*36) );

	glDrawArrays(GL_TRIANGLES, 0, 36);

	checkForGLErrors("Sky::render(): render");

	Shader::pop_vertex_attribs();
	glPopAttrib();

	checkForGLErrors("Sky::render(): post");
}

void Sky::set_time_of_day(float t) {
	time_of_day = static_cast<float>(fmod(t, 1.f));

	auto prev = colors.end() - 1;
	auto cur = colors.begin();
	for(; cur != colors.end(); ++cur) {
		if(time_of_day < cur->time) break;
		prev = cur;
	}
	if(cur == colors.end()) {
		prev = colors.end() - 1;
		cur = colors.begin();
	}

	float s = (cur->time - time_of_day) / (cur->time - prev->time);

	if(prev->time > cur->time) {
		float max = 1.f + cur->time;
		float v = time_of_day;
		if(v < prev->time) {
			v += 1.f;
		}
		s = (max - v) / (max - prev->time);
	}

	s = 1.f - s;

	//Data to upload to shader
	current_sun_data.zenit_color = Color::mix(prev->zenit, cur->zenit, s).to_vec4();
	current_sun_data.horizont_color = Color::mix(prev->horizont, cur->horizont, s).to_vec4();
	current_sun_data.sun_color = Color::mix(prev->sun, cur->sun, s).to_vec4();
	current_sun_data.sun_aura_color = Color::mix(prev->sun_aura, cur->sun_aura, s).to_vec4();
	current_sun_data.sun_radius = glm::mix(prev->sun_radius, cur->sun_radius, s);
	current_sun_data.sun_aura_scale = glm::mix(prev->sun_aura_scale, cur->sun_aura_scale, s);
	current_sun_data.lerp_size = glm::mix(prev->lerp_size, cur->lerp_size, s);
	current_sun_data.lerp_offset = glm::mix(prev->lerp_offset, cur->lerp_offset, s);

	//Internal data
	sunlight = Color::mix(prev->sunlight, cur->sunlight, s).to_vec3();
	ambient_amount = glm::mix(prev->ambient_amount, cur->ambient_amount, s);

	glm::vec2 sp_sun;

	//TODO: Don't put the sun in the middle of the sky
	float two_pi = 2.f * static_cast<float>(M_PI);
	sp_sun.x = two_pi * (1.f - time_of_day) + static_cast<float>(M_PI); 
	sp_sun.y = 0.5f;//two_pi * time_of_day;

	/* Convert to cartesian */
	current_sun_data.sun_position.x = glm::sin(sp_sun.x) * glm::cos(sp_sun.y);
	current_sun_data.sun_position.z = glm::sin(sp_sun.x) * glm::sin(sp_sun.y);
	current_sun_data.sun_position.y = glm::cos(sp_sun.x);

	Shader::upload_sky(current_sun_data);
	
	checkForGLErrors("Sky::set_time_of_day(): Post");
}

bool Sky::sky_data_t::operator<(const Sky::sky_data_t & d) const {
	return time < d.time;
}


void Sky::configure_light(MovableLight *light) const {
	light->type = MovableLight::DIRECTIONAL_LIGHT;
	light->intensity = sunlight;
	light->set_position(glm::normalize(glm::vec3(-current_sun_data.sun_position)));
}

glm::vec3 Sky::ambient_intensity() const {
		return ambient_amount * sunlight;
}

const float Sky::vertices[] = {
	#include "cube_vertices.hpp"
};
