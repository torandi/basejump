#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.hpp"
#include "logging.hpp"
#include "shader.hpp"
#include "utils.hpp"
#include "aabb.hpp"

#include <GL/glew.h>

#include <cstdio>
#include <glm/glm.hpp>
#include <vector>
#include <cassert>

Mesh::Mesh() : MovableObject(), vbos_generated_(false), has_tangents_(false){ }

Mesh::Mesh(const std::vector<Shader::vertex_t> &vertices, const std::vector<unsigned int> &indices) :
	MovableObject()
	, vertices_(vertices), indices_(indices)
	, aabb_dirty_(true)
	,	vbos_generated_(false),has_tangents_(false)
{
	assert((indices.size()%3)==0);
}

Mesh::~Mesh() {
	if(vbos_generated_)
		glDeleteBuffers(2, buffers_);
}

void Mesh::set_vertices(const std::vector<Shader::vertex_t> &vertices) {
   verify_immutable("set_vertices");
   vertices_ = vertices;
}

void Mesh::set_indices(const std::vector<unsigned int> &indices) {
   verify_immutable("set_indices");
   indices_ = indices;
}

void Mesh::set_vertices(const float vertices[][5], const size_t num_vertices) {
   verify_immutable("set_vertices");
   vertices_.clear();
   Shader::vertex_t v;

   v.normal = glm::vec3();
   v.tangent = glm::vec3();
   v.bitangent = glm::vec3();

   for(unsigned int i=0; i < num_vertices; ++i) {
      v.pos = glm::vec3(vertices[i][0], vertices[i][1], vertices[i][2]);
      v.uv = glm::vec2(vertices[i][3], vertices[i][4]);
      vertices_.push_back(v);
   }
}

void Mesh::generate_normals() {
	if(vertices_.size() == 3 || indices_.size() == 0) {
		Logging::fatal("Mesh::generate_normals() called with vertices or indices empty\n");
	}

	verify_immutable("calculate_normals()");

	for(unsigned int i=0; i<indices_.size(); i+=3) {
		unsigned int tri[3] = {indices_[i], indices_[i+1], indices_[i+2]};
		Shader::vertex_t * face[3] = {&vertices_[tri[0]], &vertices_[tri[1]], &vertices_[tri[2]]};
		glm::vec3 v1 = face[1]->pos-face[0]->pos;
		glm::vec3 v2 = face[2]->pos-face[0]->pos;
		glm::vec3 normal = glm::cross(v1, v2);
		for(int f=0; f<3;++f) {
			face[f]->normal += normal;
		}
	}
   has_normals_ = true;
}

void Mesh::activate_normals() {
   has_normals_ = true;
}

void Mesh::activate_tangents_and_bitangents() {
   has_tangents_ = true;
}


//This method orgonormalizes the tangent space
void Mesh::ortonormalize_tangent_space() {
	if(! (has_normals_ && has_tangents_)) {
		Logging::fatal("Mesh::ortonormalize_tangent_space() called with normals or tangents inactive\n");
	}

	for(std::vector<Shader::vertex_t>::iterator it=vertices_.begin(); it!=vertices_.end(); ++it) {
		it->normal = glm::normalize(it->normal);
		//Make sure tangent is ortogonal to normal (and normalized)
		it->tangent = glm::normalize(it->tangent - it->normal*glm::dot(it->normal, it->tangent));
		//Normalize bitangent
		it->bitangent = glm::normalize(it->bitangent);
		//Make sure tangent space is right handed:
		glm::vec3 new_bitangent = glm::cross(it->normal, it->tangent);
		if(glm::dot(glm::cross(it->normal, it->tangent), new_bitangent) < 0.0f) {
			it->tangent *= -1.0f;
		}
		it->bitangent = new_bitangent;

	}
}

void Mesh::generate_tangents_and_bitangents() {
	if(vertices_.size() == 3 || indices_.size() == 0) {
		Logging::fatal("Mesh::generate_tangents_and_bitangents() called with vertices or indices empty\n");
	}

	for(unsigned int i=0; i<indices_.size(); i+=3) {
		unsigned int tri[3] = {indices_[i], indices_[i+1], indices_[i+2]};
		Shader::vertex_t * face[3] = {&vertices_[tri[0]], &vertices_[tri[1]], &vertices_[tri[2]]};
		glm::vec3 v1 = face[1]->pos-face[0]->pos;
		glm::vec3 v2 = face[2]->pos-face[0]->pos;
		glm::vec2 uv1 = face[1]->uv-face[0]->uv;
		glm::vec2 uv2 = face[2]->uv-face[0]->uv;

		float r=1.f / (uv1.x * uv2.y - uv1.y * uv2.x);
		glm::vec3 tangent = (v1 * uv2.y - v2 * uv1.y)*r;
		glm::vec3 bitangent = (v2 * uv1.x - v1 * uv2.x)*r;

		for(int f=0; f<3; ++f) {
			face[f]->tangent += tangent;
			face[f]->bitangent += bitangent;
		}
	}
   has_tangents_ = true;
}

void Mesh::verify_immutable(const char * where) {
	if(vbos_generated_) {
		Logging::fatal("Mesh::%s can not be used after vertex buffers have been generated\n", where);
	}
}

void Mesh::generate_vbos() {
	verify_immutable("generate_vbos()");

	//Upload data:
	glGenBuffers(2, buffers_);
	checkForGLErrors("Mesh::generate_vbos(): gen buffers");

	glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Shader::vertex_t)*vertices_.size(), &vertices_.front(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	checkForGLErrors("Mesh::generate_vbos(): fill array buffer");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers_[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*indices_.size(), &indices_.front(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	checkForGLErrors("Mesh::generate_vbos(): fill element array buffer");

	num_faces_ = indices_.size();

	raw_aabb_.min = vertices_[0].pos;
	raw_aabb_.max = vertices_[0].pos;

	/* Calculate aabb */
	for(const Shader::vertex_t &v : vertices_) {
		raw_aabb_.add_point(v.pos);
	}

	vbos_generated_ = true;
}

void Mesh::render(const glm::mat4& m) {
	render_geometry(m);
}

void Mesh::render_geometry(const glm::mat4& m) {
	Shader::upload_model_matrix(m * matrix());

	glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers_[1]);

	checkForGLErrors("Mesh::render(): Bind buffers");

	Shader::push_vertex_attribs(6);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, uv));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, normal));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, tangent));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, bitangent));
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, color));

	checkForGLErrors("Mesh::render(): Set vertex attribs");

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(num_faces_), GL_UNSIGNED_INT, 0);

	checkForGLErrors("Mesh::render(): glDrawElements()");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Shader::pop_vertex_attribs();
	checkForGLErrors("Mesh::render(): Teardown ");
}

void Mesh::calculate_aabb() {
	aabb_ = raw_aabb_ * matrix();
	aabb_dirty_ = false;
}

const AABB &Mesh::aabb() {
	if(aabb_dirty()) calculate_aabb();

	return aabb_;
}

const bool &Mesh::aabb_dirty() {
	return aabb_dirty_;
}

void Mesh::matrix_becomes_dirty() {
	aabb_dirty_ = true;
}
