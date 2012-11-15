#ifndef LINE2D_HPP
#define LINE2D_HPP

#include <glm/glm.hpp>

struct Line2D {
	/*
	 * Line between point p1 and p2
	 */
	Line2D(const glm::vec2 &p1_, const glm::vec2 &p2_);

	glm::vec2 direction() const;

	glm::vec2 p1, p2;

	/*
	 * Returns data for line equation y = ax + b
	 * return (a, b) in a vec2
	 */
	glm::vec2 line_equation() const;

	glm::vec2 min() const;
	glm::vec2 max() const;
};

#endif
