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
	GLint __ALIGNED__(16) shadowmap_index;
};

#endif
