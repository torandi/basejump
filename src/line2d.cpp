#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "line2d.hpp"

Line2D::Line2D(const glm::vec2 &p1_, const glm::vec2 &p2_)
	: p1(p1_)
	, p2(p2_)
{ }

glm::vec2 Line2D::direction() const {
	return glm::normalize(p2 - p1);
}

glm::vec2 Line2D::line_equation() const {
	glm::vec2 ret;
	ret.x = (p2.y - p1.y) / (p2.x - p1.x);
	ret.y = p1.y - (ret.x * p1.x);
	return ret;
}

glm::vec2 Line2D::min() const { return glm::min(p1, p2); }
glm::vec2 Line2D::max() const { return glm::max(p1, p2); }
