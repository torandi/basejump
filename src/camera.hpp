#ifndef CAMERA_H
#define CAMERA_H

#include "movable_object.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>

class Camera : public MovableObject {
public:
	Camera(float fov, float aspect, float near, float far);
	Camera(float fov, const glm::ivec2& size, float near, float far);
	virtual ~Camera() {};

	const glm::vec3 look_at() const;

	const glm::mat4 view_matrix() const;
	const glm::mat4 projection_matrix() const;

	float fov() const;
	float aspect() const;
	float near() const;
	float far() const;
	const glm::vec3 up() const;

	void set_aspect(float aspect);
	void set_fov(float fov);
	void set_near_far(float near, float far);
	void look_at(const glm::vec3 &pos);

	/*
	 * Get a list of all frustrum corners
	 *
	 * @param points a array with 8 entries to store result
	 * @param near, far: Specify near and far, leave at -1 to use cameras values
	 */
	void frustrum_corners(glm::vec3 * points, float near = -1.f, float far = -1.f, float fov=-1.f) const;

	AABB aabb(float near = -1.f, float far = -1.f, float fov=-1.f) const;

	/*
	 * Debug function for rendering the view frustrum
	 * Expects /shaders/simple to be bound
	 */
	void render_frustrum(GLuint buffer) const;

private:
	float fov_;
	float aspect_, near_, far_;

	glm::mat4 projection_matrix_;
};

#endif
