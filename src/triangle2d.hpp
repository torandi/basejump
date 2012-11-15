#ifndef TRIANGLE2D_HPP
#define TRIANGLE2D_HPP

#include <glm/glm.hpp>

struct Triangle2D {
	Triangle2D(const glm::vec2 &p1_, const glm::vec2 &p2_, const glm::vec2 &p3_);

	bool contains(const glm::vec2 &v) const;

	glm::vec2 p1, p2, p3;
};

#endif
