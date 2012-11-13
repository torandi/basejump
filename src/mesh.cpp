#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "aabb2d.hpp"
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

Mesh::Mesh(float partition_size) 
	: MovableObject()
	, vbos_generated_(false)
	, has_tangents_(false)
	, partition_size_(partition_size)
{
	submesh_tree = new QuadTree(AABB_2D(glm::vec2(0.f), glm::vec2(partition_size)));
	submesh_tree->data = new SubMesh(*this);
}

Mesh::Mesh(const std::vector<Shader::vertex_t> &vertices, const std::vector<unsigned int> &indices, float partition_size) :
	MovableObject()
	, aabb_dirty_(true)
	, vertices_(vertices)
	,	vbos_generated_(false),has_tangents_(false)
	, partition_size_(partition_size)
{

	submesh_tree = new QuadTree(AABB_2D(glm::vec2(0.f), glm::vec2(partition_size)));
	submesh_tree->data = new SubMesh(*this);

	if(!(indices.size()%3)==0) {
		Logging::fatal("Trying to create a Mesh with number of indices not a factor of 3\n");
	}
	add_indices(indices);
}

Mesh::~Mesh() {
	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) delete (SubMesh*) qt->data;
		return true;
	});

	delete submesh_tree;

	if(vbos_generated_) {
		glDeleteBuffers(1, &vertex_buffer);
	}
}

SubMesh::SubMesh(Mesh &mesh) : vbo_generated(false), parent(mesh) {

}

SubMesh::~SubMesh() {
	if(vbo_generated) {
		glDeleteBuffers(1, &index_buffer);
	}
}

void Mesh::add_vertices(const std::vector<Shader::vertex_t> &vertices) {
   verify_immutable("set_vertices");
	 vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
}

void Mesh::add_indices(const std::vector<unsigned int> &indices) {
   verify_immutable("set_indices");
	 if(indices.size() % 3 != 0) {
		Logging::fatal("Must add indices in groups of 3\n");
	 }

	 if(partition_size_ < 0.f) {
		//Ignore quad tree size, just insert in the quad tree node
		SubMesh * m = (SubMesh*) submesh_tree->data ;
		m->indices.insert(m->indices.end(), indices.begin(), indices.end());
	 } else {
		for(auto it = indices.begin(); it != indices.end(); it += 3) {
			glm::vec3 center = ( vertices_[*it].pos + vertices_[*(it + 1)].pos + vertices_[*(it + 2)].pos ) / 3.f;
			glm::vec2 center_2d = glm::vec2(center.x, center.z); //Ignore y
			//Find child:
			QuadTree * child = nullptr;
			while(child == nullptr) {
				child = submesh_tree->child(center_2d);
				if(child == nullptr) {
					//The point was outside the quad tree, enlarge!
					submesh_tree = submesh_tree->grow();
				}
			}
			if(child->data == nullptr) child->data = (void*) new SubMesh(*this);

			SubMesh * m = ((SubMesh*) child->data );
			m->indices.insert(m->indices.end(), it, it + 3);
		}
	 }
}

void Mesh::add_vertices(const float vertices[][5], const size_t num_vertices) {
   verify_immutable("set_vertices");
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
	verify_immutable("generate_normals()");

	if(vertices_.size() < 3) Logging::fatal("Mesh::generate_normals() called with vertices empty\n");

	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) ( (SubMesh*) qt->data )->generate_normals();
		return true;
	});

	has_normals_ = true;
}

void SubMesh::generate_normals() {
	if(indices.size() == 0) Logging::fatal("SubMesh::generate_normals() called with indices empty\n");
	if(indices.size() % 3 != 0) Logging::fatal("SubMesh::generate_normals() called with indices not a factor of 3\n");

	for(unsigned int i=0; i<indices.size(); i+=3) {
		unsigned int tri[3] = {indices[i], indices[i+1], indices[i+2]};
		Shader::vertex_t * face[3] = {&parent.vertices_[tri[0]], &parent.vertices_[tri[1]], &parent.vertices_[tri[2]]};
		glm::vec3 v1 = face[1]->pos - face[0]->pos;
		glm::vec3 v2 = face[2]->pos - face[0]->pos;
		glm::vec3 normal = glm::cross(v1, v2);
		for(int f=0; f<3;++f) {
			face[f]->normal += normal;
		}
	}
}

void Mesh::activate_normals() {
   has_normals_ = true;
}

void Mesh::activate_tangents_and_bitangents() {
   has_tangents_ = true;
}

void Mesh::ortonormalize_tangent_space() {
	verify_immutable("ortonormalize_tangent_space");

	if(! (has_normals_ && has_tangents_)) {
		Logging::fatal("Mesh::ortonormalize_tangent_space() called with normals or tangents inactive\n");
	}

	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) ( (SubMesh*) qt->data )->ortonormalize_tangent_space();
		return true;
	});
}

//This method orgonormalizes the tangent space
void SubMesh::ortonormalize_tangent_space() {

	for(std::vector<Shader::vertex_t>::iterator it=parent.vertices_.begin(); it!=parent.vertices_.end(); ++it) {
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
	if(vertices_.size() < 3) Logging::fatal("Mesh::generate_normals() called with vertices empty\n");

	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) ( (SubMesh*) qt->data )->generate_tangents_and_bitangents();
		return true;
	});

	has_tangents_ = true;
}

void SubMesh::generate_tangents_and_bitangents() {
	if(indices.size() == 0) Logging::fatal("SubMesh::generate_normals() called with indices empty\n");

	for(unsigned int i=0; i<indices.size(); i+=3) {
		unsigned int tri[3] = {indices[i], indices[i+1], indices[i+2]};
		Shader::vertex_t * face[3] = {&parent.vertices_[tri[0]], &parent.vertices_[tri[1]], &parent.vertices_[tri[2]]};
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
}

void Mesh::verify_immutable(const char * where) {
	if(vbos_generated_) {
		Logging::fatal("Mesh::%s can not be used after vertex buffers have been generated\n", where);
	}
}

void Mesh::generate_vbos() {
	verify_immutable("generate_vbos()");

	glGenBuffers(1, &vertex_buffer);
	checkForGLErrors("Mesh::generate_vbos(): gen buffers");

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Shader::vertex_t)*vertices_.size(), vertices_.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	checkForGLErrors("Mesh::generate_vbos(): fill vertex buffer");

	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) ( (SubMesh*) qt->data )->generate_vbos();
		return true;
	});

	raw_aabb_.min = vertices_[0].pos;
	raw_aabb_.max = vertices_[0].pos;

	for(const Shader::vertex_t &v : vertices_) {
		raw_aabb_.add_point(v.pos);
	}

	vbos_generated_ = true;
}

void SubMesh::generate_vbos() {
	//Upload data:
	glGenBuffers(1, &index_buffer);
	checkForGLErrors("SubMesh::generate_vbos(): gen buffers");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*indices.size(), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	checkForGLErrors("SubMesh::generate_vbos(): fill index buffer");

	num_faces = static_cast<unsigned int>( static_cast<float>(indices.size()) / 3.f );
	vbo_generated = true;
}

void Mesh::prepare_submesh_rendering(const glm::mat4& m) {
	Shader::upload_model_matrix(m * matrix());

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
}

void Mesh::render(const glm::mat4& m) {

	prepare_submesh_rendering(m);

	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) ( (SubMesh*) qt->data )->render();
		return true;
	});
}

void Mesh::render_geometry(const glm::mat4& m) {
	prepare_submesh_rendering(m);

	submesh_tree->traverse([](QuadTree * qt) -> bool {
		if(qt->data != nullptr) ( (SubMesh*) qt->data )->render_geometry();
		return true;
	});
}

void SubMesh::render() {
	render_geometry();
}

void SubMesh::render_geometry() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	checkForGLErrors("SubMesh::render(): Bind buffers");

	Shader::push_vertex_attribs(6);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, uv));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, normal));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, tangent));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, bitangent));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*) offsetof(Shader::vertex_t, color));

	checkForGLErrors("SubMesh::render(): Set vertex attribs");

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(num_faces * 3), GL_UNSIGNED_INT, 0);

	checkForGLErrors("SubMesh::render(): glDrawElements()");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Shader::pop_vertex_attribs();
	checkForGLErrors("SubMesh::render(): Teardown ");
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
