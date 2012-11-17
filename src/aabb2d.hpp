#ifndef AABB_2D_HPP
#define AABB_2D_HPP

#include <glm/glm.hpp>
#include <vector>

struct AABB_2D {

	AABB_2D();

	AABB_2D(const glm::vec2 &min_, const glm::vec2 &max_);
	AABB_2D(const AABB_2D &aabb);

	glm::vec2 min, max;

	/*
	 * Return the aabb containing this and other aabb
	 */
	AABB_2D operator+(const AABB_2D &other) const;

	/*
	 * Same as operator+
	 */
	AABB_2D &operator+=(const AABB_2D &other);

	/**
	 * Multiply both points with given matrix (all changes in y (3d space) will be ignored)
	 */
	AABB_2D operator*(const glm::mat4 &matrix) const;

	/*
	 * Returns true if the given point is in the aabb
	 */
	bool contains(const glm::vec2 &point) const;

	/*
	 * Returns the middle point of this aabb
	 */
	glm::vec2 middle() const;


	/*
	 * Makes sure that point is inside aabb, modifies aabb if needed
	 */
	void add_point(const glm::vec2 &v);

	/**
	 * Returns vector with the 4 corners
	 */
	const std::vector<glm::vec2> &corners() const;

	glm::vec2 size() const;

	void mark_dirty();

private:
	mutable std::vector<glm::vec2> corners_;
	mutable bool corners_dirty_, middle_dirty_;
	mutable glm::vec2 middle_;
};

#endif
