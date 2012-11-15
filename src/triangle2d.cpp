#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "triangle2d.hpp"

Triangle2D::Triangle2D(const glm::vec2 &p1_, const glm::vec2 &p2_, const glm::vec2 &p3_)
	: p1(p1_)
	, p2(p2_)
	, p3(p3_)
{ }

bool Triangle2D::contains(const glm::vec2 &vec) const {
	/*
	 * Barycentric Technique: http://www.blackpawn.com/texts/pointinpoly/default.html
	 *
	 * Base at p1, direction dv, d2 and d3 to v, p2 and p3 resp:
	 */
	glm::vec2 d2 = p2 - p1;
	glm::vec2 d3 = p3 - p1;
	glm::vec2 dv = vec  - p1;

	//Compute dot products:
	float dot22 = glm::dot(d2, d2);
	float dot23 = glm::dot(d2, d3);
	float dot2v = glm::dot(d2, dv);
	float dot33 = glm::dot(d3, d3);
	float dot3v = glm::dot(d3, dv);

	//Compute barycentric coordinates:
	float inv_denom = 1.f / (dot22 * dot33 - dot23 * dot23);
	float u = (dot33 * dot2v - dot23 * dot3v) * inv_denom;
	float v = (dot22 * dot3v - dot23 * dot2v) * inv_denom;

	//Check if point is in triangle
	return (u >= 0) && (v >=0) && ((u + v) < 1.f);
//	return true;
}
