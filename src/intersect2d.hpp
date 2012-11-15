#ifndef INTERSECT_HPP
#define INTERSECT_HPP

namespace intersect2d {
	bool aabb_triangle(const AABB_2D &aabb, const Triangle2D &tri);
	bool aabb_line(const AABB_2D &aabb, const Line2D &line);
};

#endif
