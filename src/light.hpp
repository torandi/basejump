#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

struct Light {
	Light();

	float constant_attenuation;
	float linear_attenuation;
	float quadratic_attenuation;
	float is_directional;
	glm::vec3 intensity;
	glm::vec3 __ALIGNED__(16) position;
};

#endif
