#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "aabb2d.hpp"
#include <cstring>

#include <cstdio>

AABB_2D::AABB_2D()
	: min(FLT_MAX)
	, max(-FLT_MAX)
	, corners_dirty_(true)
	{ }

AABB_2D::AABB_2D(const glm::vec2 &min_, const glm::vec2 &max_) 
	: min(min_)
	, max(max_)
	, corners_dirty_(true)
	{ }

AABB_2D AABB_2D::operator+(const AABB_2D &other) const {
	corners_dirty_ = true;
	return AABB_2D(
				glm::min(min, other.min),
				glm::max(max, other.max)
			);
}

AABB_2D &AABB_2D::operator+=(const AABB_2D &other) {
	corners_dirty_ = true;
	min = glm::min(min, other.min);
	max = glm::max(max, other.max);
	return *this;
}

AABB_2D AABB_2D::operator*(const glm::mat4 &matrix) const {
	corners_dirty_ = true;
	glm::vec4 new_min = matrix * glm::vec4(min.x, 0.f, min.y, 1.f);
	glm::vec4 new_max = matrix * glm::vec4(max.x, 0.f, min.y, 1.f);
	return AABB_2D(
			glm::vec2(new_min.x, new_min.z) / new_min.w, 
			glm::vec2(new_max.x, new_max.z) / new_max.w
		);
}

void AABB_2D::add_point(const glm::vec2 &v) {
	corners_dirty_ = true;
	min = glm::min(min, v);
	max = glm::max(max, v);
}

const std::vector<glm::vec2> &AABB_2D::corners() const {
	if(corners_dirty_) {
		corners_.clear();
		corners_.push_back( min);
		corners_.push_back( glm::vec2(max.x, min.y) );

		corners_.push_back( glm::vec2(min.x, max.y) );
		corners_.push_back( max);
		corners_dirty_ = false;
	}

	return corners_;
}

bool AABB_2D::contains(const glm::vec2 &point) const {
	return (
				min.x < point.x && point.x < max.x &&
				min.y < point.y && point.y < max.y
			);
}

glm::vec2 AABB_2D::middle() const {
	return (min + max) / 2.f;
}

glm::vec2 AABB_2D::size() const {
	return (max - min);
}
