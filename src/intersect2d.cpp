#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glm/glm.hpp>

#include "intersect2d.hpp"
#include "triangle2d.hpp"
#include "line2d.hpp"
#include "aabb2d.hpp"

bool intersect2d::aabb_triangle(const AABB_2D &aabb, const Triangle2D &tri) {
	//First check if any of the triangles corners is in the aabb:
	if(aabb.contains(tri.p1) || aabb.contains(tri.p2) || aabb.contains(tri.p3)) return true;

	//Then if any of the aabb corners are in the triangle
	for(const glm::vec2 & c: aabb.corners()) {
		if(tri.contains(c)) return true;
	}

	//Then test if any lines intersect:
	return	aabb_line(aabb, Line2D(tri.p1, tri.p2)) ||
					aabb_line(aabb, Line2D(tri.p2, tri.p3)) ||
					aabb_line(aabb, Line2D(tri.p3, tri.p1));

}

bool intersect2d::aabb_line(const AABB_2D &aabb, const Line2D &line) {
	glm::vec2 l_min = line.min();
	glm::vec2 l_max = line.max();

	l_min = glm::max(l_min, aabb.min);
	l_max = glm::min(l_max, aabb.max);

	return !( (l_min.x > l_max.x) || (l_min.y > l_max.y));
}
