#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

#include "aabb.hpp"
#include "shader.hpp"
#include "movable_object.hpp"
#include "quadtree.hpp"

class Mesh;

class SubMesh {
	public:
		friend class Mesh;

		SubMesh(Mesh &mesh);
		virtual ~SubMesh();

		void generate_vbos();

		std::vector<unsigned int> indices;

		unsigned long num_faces;

		virtual void render();
		virtual void render_geometry();

	protected:
		void generate_normals();
		void generate_tangents_and_bitangents();

		GLuint index_buffer;
		bool vbo_generated;

		Mesh &parent;
};

class Mesh : public MovableObject {
	public:
		friend class SubMesh;
		/*
		 * partition size is how large each submesh is (in x and z), not considering any matrices.
		 * Negative number meens dont partition
		 */
		Mesh(float partition_size=-1.f);
		Mesh(const std::vector<Shader::vertex_t> &vertices, const std::vector<unsigned int> &indices, float partition_size=-1.f);
		virtual ~Mesh();

		/* Call these methods to activate these features.
		 * Automaticaly activated if the corresponding activate function is called
		 * Note though that the data is still uploaded to the GPU, just not used
		 */
		void activate_normals();
		void activate_tangents_and_bitangents();

		void add_vertices(const std::vector<Shader::vertex_t> &vertices);
		void add_indices(const std::vector<unsigned int> &indices);

		/*
		 * Loads vertex data from array of floats (x, y, z, u, v)
		 */
		void add_vertices(const float vertices[][5], const size_t num_vertices);
		void generate_normals();
		void generate_tangents_and_bitangents();
		void ortonormalize_tangent_space();

		/*
		 * The mesh becommes immutable when vbos have been generated
		 */
		void generate_vbos();

		/*
		 * This must be called before rendering submeshes (if not using Mesh::render or Mesh::render_geometri)
		 *
		 * Uploads model matrix and binds vertex buffer
		 */
		virtual void prepare_submesh_rendering(const glm::mat4& m = glm::mat4());

		virtual void render(const glm::mat4& m = glm::mat4());
		virtual void render_geometry(const glm::mat4& m = glm::mat4());

		const AABB &aabb();
		const bool &aabb_dirty();

	protected:
		AABB aabb_, raw_aabb_; /* raw_aabb is aabb unmodified by model matrix */
		bool aabb_dirty_;
		
		virtual void calculate_aabb();
		virtual void matrix_becomes_dirty();

		QuadTree * submesh_tree;

		std::vector<Shader::vertex_t> vertices_;

		bool vbos_generated_, has_normals_, has_tangents_;
		float partition_size_;

		glm::vec3 scale_;

		GLuint vertex_buffer;

		void verify_immutable(const char * where); //Checks that vbos_generated == false

		/*
		 * Warning! Calling this will remove any currently added indices
		 */
		void set_partition_size(float size);

	private:
		void free_submesh_tree();


};

#endif
