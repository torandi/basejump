#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "movable_light.hpp"

MovableLight::MovableLight(Light * light)
	: MovableObject(light->position)
	, constant_attenuation(light->constant_attenuation)
	, linear_attenuation(light->linear_attenuation)
	, quadratic_attenuation(light->quadratic_attenuation)
	, type(light->type)
	, intensity(light->intensity)
	, data(light)
{}

void MovableLight::update() {
	data->position = position_;
}
