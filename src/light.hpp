#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <GL/glew.h>

struct Light {
	Light();

	float constant_attenuation;
	float linear_attenuation;
	float quadratic_attenuation;
	float is_directional;
	glm::vec3 __ALIGNED__(16) intensity;
	glm::vec3 __ALIGNED__(16) position;
	glm::mat4 __ALIGNED__(16) matrix;
	glm::vec2 __ALIGNED__(16) shadowmap_scale; // 1/width, 1/height of shadowmap resolution
	GLint shadowmap_index;
	float shadow_bias;
};

#endif
