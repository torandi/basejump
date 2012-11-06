#ifndef AABB_HPP
#define AABB_HPP

#include <glm/glm.hpp>

struct AABB {

	AABB();

	AABB(const glm::vec3 &min_, const glm::vec3 &max_);

	glm::vec3 min, max;

	/*
	 * Return the aabb containing this and other aabb
	 */
	AABB operator+(const AABB &other) const;

	/**
	 * Multiply both points with given matrix
	 */
	AABB operator*(const glm::mat4 &matrix) const;

	/*
	 * Makes sure that point is inside aabb, modifies aabb if needed
	 */
	void add_point(const glm::vec3 &v);
};

#endif
