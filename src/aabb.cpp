#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "aabb.hpp"
AABB::AABB()
	: min()
	, max() { }

AABB::AABB(const glm::vec3 &min_, const glm::vec3 &max_) 
	: min(min_)
	, max(max_) { }

AABB AABB::operator+(const AABB &other) const {
	return AABB(
				glm::vec3(glm::min(min.x, other.min.x),glm::min(min.y, other.min.y),glm::min(min.z, other.min.z)),
				glm::vec3(glm::max(max.x, other.max.x),glm::max(max.y, other.max.y),glm::max(max.z, other.max.z))
			);
}

AABB AABB::operator*(const glm::mat4 &matrix) const {
	glm::vec4 new_min = matrix * glm::vec4(min, 1.f);
	glm::vec4 new_max = matrix * glm::vec4(max, 1.f);
	return AABB(
			glm::vec3(new_min.x, new_min.y, new_min.z) / new_min.w, 
			glm::vec3(new_max.x, new_max.y, new_max.z) / new_max.w
		);
}

void AABB::add_point(const glm::vec3 &v) {
	min = glm::vec3(glm::min(min.x, v.x),glm::min(min.y, v.y),glm::min(min.z, v.z));
	max = glm::vec3(glm::max(max.x, v.x),glm::max(max.y, v.y),glm::max(max.z, v.z));
}
