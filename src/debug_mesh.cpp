#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "debug_mesh.hpp"
#include "shader.hpp"

DebugMesh::DebugMesh(GLenum draw_mode) 
	: MovableObject()
	, draw_mode(draw_mode)
{ init(); };

DebugMesh::DebugMesh(GLenum draw_mode, const std::vector<vertex_t> vertices)
	: MovableObject()
	, draw_mode(draw_mode)
{
	init();
	set_vertices(vertices);
}

DebugMesh::DebugMesh(GLenum draw_mode, const vertex_t * vertices, size_t count)
	: MovableObject()
	, draw_mode(draw_mode)
{
	init();
	set_vertices(vertices, count);
}

void DebugMesh::set_vertices(const std::vector<vertex_t> &vertices) {
	set_vertices(vertices.data(), vertices.size());
}

void DebugMesh::set_vertices(const vertex_t * vertices, size_t count) {
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * count, static_cast<const GLvoid*>( vertices ),GL_STATIC_DRAW);
}

void DebugMesh::set_indices(const std::vector<unsigned int> &indices_) {
	indices = indices_;
}

void DebugMesh::set_indices(const unsigned int * indices_, size_t count) {
	indices.clear();
	indices.insert(indices.begin(), indices_, indices_ + count);
}

void DebugMesh::set_draw_mode(GLenum new_draw_mode) {
	draw_mode = new_draw_mode;
}

DebugMesh::~DebugMesh() {
	glDeleteBuffers(1, &buffer);
}

void DebugMesh::init() {
	shader = Shader::create_shader("/shaders/simple");
	glGenBuffers(1, &buffer);
}

void DebugMesh::render(const glm::mat4& m) const {
	shader->bind();

	Shader::upload_model_matrix(m * matrix());

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Shader::push_vertex_attribs(2);
	glPushAttrib(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (const GLvoid*) offsetof(vertex_t, pos));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (const GLvoid*) offsetof(vertex_t, color));

	glDrawElements(draw_mode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, indices.data());

	glPopAttrib();

	Shader::pop_vertex_attribs();
}
