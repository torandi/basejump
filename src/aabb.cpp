#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "aabb.hpp"
#include <cstring>

#include <cstdio>

AABB::AABB()
	: min()
	, max()
	, corners_dirty_(true)
	{ }

AABB::AABB(const glm::vec3 &min_, const glm::vec3 &max_) 
	: min(min_)
	, max(max_)
	, corners_dirty_(true)
	{ }

AABB AABB::operator+(const AABB &other) const {
	corners_dirty_ = true;
	return AABB(
				glm::vec3(glm::min(min.x, other.min.x),glm::min(min.y, other.min.y),glm::min(min.z, other.min.z)),
				glm::vec3(glm::max(max.x, other.max.x),glm::max(max.y, other.max.y),glm::max(max.z, other.max.z))
			);
}

AABB AABB::operator*(const glm::mat4 &matrix) const {
	corners_dirty_ = true;
	glm::vec4 new_min = matrix * glm::vec4(min, 1.f);
	glm::vec4 new_max = matrix * glm::vec4(max, 1.f);
	return AABB(
			glm::vec3(new_min.x, new_min.y, new_min.z) / new_min.w, 
			glm::vec3(new_max.x, new_max.y, new_max.z) / new_max.w
		);
}

void AABB::add_point(const glm::vec3 &v) {
	corners_dirty_ = true;
	min = glm::vec3(glm::min(min.x, v.x),glm::min(min.y, v.y),glm::min(min.z, v.z));
	max = glm::vec3(glm::max(max.x, v.x),glm::max(max.y, v.y),glm::max(max.z, v.z));
}

const std::vector<glm::vec3> &AABB::corners() const {
	if(corners_dirty_) {
		corners_.clear();
		corners_.push_back( min);
		corners_.push_back( glm::vec3(min.x, min.y, max.z));
		corners_.push_back( glm::vec3(max.x, min.y, max.z));
		corners_.push_back( glm::vec3(max.x, min.y, min.z));

		corners_.push_back( glm::vec3(min.x, max.y, min.z));
		corners_.push_back( glm::vec3(min.x, max.y, max.z));
		corners_.push_back( max);
		corners_.push_back( glm::vec3(max.x, max.y, min.z));
		corners_dirty_ = false;
	}

	return corners_;
}
