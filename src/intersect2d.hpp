#ifndef INTERSECT_HPP
#define INTERSECT_HPP

namespace intersect2d {
	bool aabb_triangle(const AABB_2D &aabb, const Triangle2D &tri);
	bool aabb_line(const AABB_2D &aabb, const Line2D &line);
	bool aabb_aabb(const AABB_2D &aabb1, const AABB_2D &aabb2);
};

#endif
