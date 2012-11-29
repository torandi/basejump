#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "camera.hpp"
#include "utils.hpp"
#include "shader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cmath>
#include <cassert>

Camera::Camera(float fov, float aspect, float near, float far)
	: fov_(fov)
	, roll_(0.f)
	, aspect_(aspect)
	, near_(near)
	, far_(far) {

	projection_matrix_ = glm::perspective(fov_, aspect_, near_, far_);
}

Camera::Camera(float fov, const glm::ivec2& size, float near, float far)
	: fov_(fov)
	, roll_(0.f)
	, aspect_((float)size.x / (float)size.y)
	, near_(near)
	, far_(far) {

	projection_matrix_ = glm::perspective(fov_, aspect_, near_, far_);
}

const glm::vec3 Camera::look_at() const {
	return look_at_;
}

void Camera::look_at(const glm::vec3 &lookat) {
	look_at_ = lookat;

	recalculate();
}

/**
 * Recalculates rotation
 */
void Camera::recalculate() {
	orientation_ = glm::fquat(1.f, 0.f, 0.f, 0.f); //Reset rotation

	glm::vec3 direction = look_at_ - position_;
	glm::vec2 xz_projection = glm::vec2(direction.x,direction.z);
	float dot = glm::dot(xz_projection, glm::vec2(0.f, 1.f));

	float rotation = acosf(glm::clamp(dot / glm::length(xz_projection), -1.f, 1.f));
	MovableObject::absolute_rotate(glm::vec3(0.f, 1.f, 0.f), rotation*glm::sign(direction.x));

	glm::vec3 lz = local_z();

	rotation = acosf(glm::clamp(glm::dot(direction, lz) / (glm::length(direction) * glm::length(lz)), -1.f, 1.f));

	MovableObject::relative_rotate(glm::vec3(1.f, 0.f, 0.f),-rotation*glm::sign(direction.y));

	MovableObject::relative_rotate(glm::vec3(0.f, 0.f, 1.f), roll_);
}


const glm::mat4 Camera::view_matrix() const {
	return glm::lookAt(position_, look_at_, local_y());
}

const glm::mat4 Camera::projection_matrix() const { return projection_matrix_; }

float Camera::fov() const { return fov_; }
float Camera::aspect() const { return aspect_; }
float Camera::near() const { return near_; }
float Camera::far() const { return far_; }
float Camera::roll() const { return roll_; }
const glm::vec3 &Camera::position() const { return position_; }
const glm::vec3 Camera::up() const { return local_y(); };

void Camera::set_aspect(float aspect){
	aspect_ = aspect;
	projection_matrix_ = glm::perspective(fov_, aspect_, near_, far_);
}

void Camera::set_fov(float fov) {
	fov_ = fov;
	projection_matrix_ = glm::perspective(fov_, aspect_, near_, far_);
}

void Camera::set_near_far(float near, float far) {
	near_ = near;
	far_ = far;
	projection_matrix_ = glm::perspective(fov_, aspect_, near_, far_);
}

void Camera::set_roll(const float r) {
	MovableObject::roll(r-roll_);
	roll_ = r;
}

void Camera::roll(const float r) {
	roll_+=r;
	MovableObject::roll(r);
}

void Camera::relative_move(const glm::vec3 &move) {
	MovableObject::relative_move(move);
	look_at_ += orient_vector(move);
	recalculate();
}

void Camera::set_position(const glm::vec3 &pos) {
	MovableObject::set_position(pos);
	recalculate();
}

void Camera::absolute_move(const glm::vec3 &move) {
	MovableObject::absolute_move(move);
	look_at_ += move;
	recalculate();
}

void Camera::relative_rotate(const glm::vec3 &axis, const float &angle) {
	MovableObject::relative_rotate(axis, angle);
	look_at_ = position() + local_z();
}

void Camera::absolute_rotate(const glm::vec3 &axis, const float &angle) {
	MovableObject::absolute_rotate(axis, angle);
	look_at_ = position() + local_z();
}

void Camera::frustrum_corners(glm::vec3 * points, float near, float far, float fov) const {
	if(near < 0.f) near = near_;
	if(far < 0.f) far = far_;
	if(fov < 0.f) fov = fov_;
	//Near plane:
	float y = near * tanf(glm::radians(fov) / 2.f);
	float x = y * aspect();

	glm::vec3 lx, ly, lz;
	lx = glm::normalize(local_x());
	ly = glm::normalize(local_y());
	lz = glm::normalize(local_z());

	glm::vec3 near_center = position() + lz * near;
	glm::vec3 far_center = position() + lz * far;

	points[0] = near_center + -x * lx + -y * ly;
	points[1] = near_center + -x * lx +  y * ly;
	points[2] = near_center +  x * lx +  y * ly;
	points[3] = near_center +  x * lx + -y * ly;

	y = far * tanf(glm::radians(fov) / 2.f);
	x = y * aspect();

	points[4] = far_center + -x * lx + -y * ly;
	points[5] = far_center + -x * lx +  y * ly;
	points[6] = far_center +  x * lx +  y * ly;
	points[7] = far_center +  x * lx + -y * ly;
}

void Camera::render_frustrum(GLuint buffer) const{
	static const unsigned int indices[] = {
		0, 1, 2, 3, 0, 4, 5, 6, 7, 4, 5, 1, 2, 6, 7, 3
	};
	glm::vec3 corners[8];
	frustrum_corners(corners);

	struct corner_data_t {
		glm::vec3 corner;
		glm::vec4 color;
	};

	corner_data_t vertices[8];
	for(int i=0; i< 8; ++i) {
		vertices[i].corner = corners[i];
		vertices[i].color = glm::vec4(0.f, 0.f, 0.f, 1.f);
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBufferData(GL_ARRAY_BUFFER, sizeof(corner_data_t) * 8, vertices, GL_DYNAMIC_DRAW);

	Shader::push_vertex_attribs(2);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(corner_data_t), (const GLvoid*) offsetof(corner_data_t, corner));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(corner_data_t), (const GLvoid*) offsetof(corner_data_t, color));

	glDrawElements(GL_LINE_STRIP, 16, GL_UNSIGNED_INT, indices);

	Shader::pop_vertex_attribs();
}
