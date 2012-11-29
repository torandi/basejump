#ifndef DEBUG_MESH_HPP
#define DEBUG_MESH_HPP

#include "movable_object.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class DebugMesh : public MovableObject {
	public:
		struct vertex_t {
			glm::vec3 pos;
			glm::vec4 color;
		};

		DebugMesh(GLenum draw_mode);
		DebugMesh(GLenum draw_mode, const std::vector<vertex_t> vertices);
		DebugMesh(GLenum draw_mode, const vertex_t * vertices, size_t count);
		virtual ~DebugMesh();

		void set_vertices(const std::vector<vertex_t> &vertices);
		void set_vertices(const vertex_t * vertices, size_t count);

		void set_indices(const std::vector<unsigned int> &indices);
		void set_indices(const unsigned int * indices, size_t count);

		void set_draw_mode(GLenum new_draw_mode);

		virtual void render(const glm::mat4& m = glm::mat4()) const;
	protected:
		Shader * shader;
		GLuint buffer;
		GLenum draw_mode;
		std::vector<unsigned int> indices;
	private:
		void init();
};

#endif
