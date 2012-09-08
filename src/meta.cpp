#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "meta.hpp"
#include "color.hpp"
#include "utils.hpp"
#include <sstream>
#include <cstring>

std::string MetaLightBase::get_string(Scene* instance, unsigned int offset) const {
	MovableLight * light = get(instance);

	std::stringstream str;
	switch (offset){
	case 1: str << light->type; break;
	case 2: str << light->constant_attenuation; break;
	case 3: str << light->intensity.r; break;
	case 4: str << light->intensity.g; break;
	case 5: str << light->intensity.b; break;
	case 6: str << light->position().x; break;
	case 7: str << light->position().y; break;
	case 8: str << light->position().z; break;
	}

	return str.str();
}

std::string MetaLightBase::set_string(Scene* instance, const std::string& str, unsigned int offset){
	MovableLight * light = get(instance);

	switch (offset){
	case 0: break;
	case 1: light->type = (Light::light_type_t)atoi(str.c_str()); break;
	case 2: light->constant_attenuation = atoff(str.c_str()); break;
	case 3: light->intensity.r = atoff(str.c_str()); break;
	case 4: light->intensity.g = atoff(str.c_str()); break;
	case 5: light->intensity.b = atoff(str.c_str()); break;
	case 6: light->set_position(glm::vec3(atof(str.c_str()), light->position().y, light->position().z)); break;
	case 7: light->set_position(glm::vec3(light->position().x, atof(str.c_str()), light->position().z)); break;
	case 8: light->set_position(glm::vec3(light->position().x, light->position().y, atof(str.c_str()))); break;
	}

	return get_string(instance, offset);
}

template <>
MetaVariableBase<float>::MetaVariableBase(): Meta(TYPE_FLOAT){

}

template <>
std::string MetaVariableBase<float>::get_string(Scene* instance, unsigned int offset) const {
	std::stringstream str;
	str << get(instance);
	return str.str();
}

template <>
std::string MetaVariableBase<float>::set_string(Scene* instance, const std::string& str, unsigned int offset){
	const float tmp = static_cast<float>(atof(str.c_str()));
	set(instance, tmp);
	return get_string(instance, 0);
}

template <>
MetaVariableBase<int>::MetaVariableBase(): Meta(TYPE_INT){

}

template <>
std::string MetaVariableBase<int>::get_string(Scene* instance, unsigned int offset) const {
	std::stringstream str;
	str << get(instance);
	return str.str();
}

template <>
std::string MetaVariableBase<int>::set_string(Scene* instance, const std::string& str, unsigned int offset){
	set(instance, atoi(str.c_str()));
	return get_string(instance, 0);
}

template <>
MetaVariableBase<glm::vec3>::MetaVariableBase(): Meta(TYPE_VEC3){

}

template <>
std::string MetaVariableBase<glm::vec3>::get_string(Scene* instance, unsigned int offset) const {
	std::stringstream str;
	glm::vec3 v = get(instance);

	switch (offset){
	case 0:
		str << v.x << "," << v.y << "," << v.z;
		break;
	case 1:
		str << v.x;
		break;
	case 2:
		str << v.y;
		break;
	case 3:
		str << v.z;
		break;
	}

	return str.str();
}

template <>
std::string MetaVariableBase<glm::vec3>::set_string(Scene* instance, const std::string& str, unsigned int offset){
	glm::vec3 v;

	if ( offset == 0 ){
		char* tmp = strdup(str.c_str());
		v.x = atoff(strtok(tmp,   ","));
		v.y = atoff(strtok(NULL,  ","));
		v.z = atoff(strtok(NULL,  ","));
		free(tmp);
	} else {
		float w = atoff(str.c_str());
		v = get(instance);
		switch (offset) {
		case 1: v.x = w; break;
		case 2: v.y = w; break;
		case 3: v.z = w; break;
		}
	}

	set(instance, v);
	return get_string(instance, offset);
}

template <>
MetaVariableBase<Color>::MetaVariableBase(): Meta(TYPE_COLOR){

}

template <>
std::string MetaVariableBase<Color>::get_string(Scene* instance, unsigned int offset) const {
	std::stringstream str;
	Color c = get(instance);

	switch (offset) {
	case 0:
		str << c.r << "," << c.g << "," << c.b << "," << c.a;
		break;
	case 1:
		str << c.r;
		break;
	case 2:
		str << c.g;
		break;
	case 3:
		str << c.b;
		break;
	case 4:
		str << c.a;
		break;
	}

	return str.str();
}

template <>
std::string MetaVariableBase<Color>::set_string(Scene* instance, const std::string& str, unsigned int offset){
	Color c;

	if ( offset == 0 ){
		char* tmp = strdup(str.c_str());
		c.r = atoff(strtok(tmp,   ","));
		c.g = atoff(strtok(NULL,  ","));
		c.b = atoff(strtok(NULL,  ","));
		c.a = atoff(strtok(NULL,  ","));
		free(tmp);
	} else {
		float v = atoff(str.c_str());
		c = get(instance);
		switch (offset) {
		case 1: c.r = v; break;
		case 2: c.g = v; break;
		case 3: c.b = v; break;
		case 4: c.a = v; break;
		}
	}

	set(instance, c);
	return get_string(instance, offset);
}
