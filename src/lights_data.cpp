#include "lights_data.hpp"
#include "shader.hpp"

LightsData::LightsData() {
	for(int i=0; i < MAX_NUM_LIGHTS; ++i) {
		data_.lights[i].intensity = glm::vec3(1.f, 0.f, 0.f);
		lights[i] = new MovableLight(&data_.lights[i]);
	}
	data_.num_lights = 0;
	data_.ambient_intensity = glm::vec3(0.1f);
}

LightsData::~LightsData() {
	for(int i=0; i < MAX_NUM_LIGHTS; ++i) {
		delete lights[i];
	}
}

const Shader::lights_data_t &LightsData::shader_data() {
	for(int i=0; i < data_.num_lights; ++i) {
		lights[i]->update();
	}
	return data_;
}