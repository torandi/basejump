#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rendertarget.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "utils.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

RenderTarget* RenderTarget::stack = nullptr;
GLuint RenderTarget::vbo[2] = {0,0};

RenderTarget::RenderTarget(const glm::ivec2& size, GLenum format, int flags, GLenum filter) throw()
	: TextureBase()
	, flags(flags)
	, format(format)
	, filter(filter)
	, color_buffers(flags & DOUBLE_BUFFER ? 2 : 1)
	, id(0)
	, front(0)
	, back(0)
	, max(1) {

	checkForGLErrors("RenderTarget()");
	this->size = size;

	/* doublebuffer and MRT does not work together, yet. Because I'm to lazy to implement it */
	if ( (flags & DOUBLE_BUFFER) && (flags & MULTIPLE_RENDER_TARGETS) ){
		Logging::fatal("MRT with RenderTarget does not support doublebuffering, because I'm lazy.\n");
	}

	/* init_vbo is a no-op if it already is initialized */
	init_vbo();

	/* generate projection matrix for this target */
	projection = glm::ortho(0.0f, (float)size.x, 0.0f, (float)size.y, -1.0f, 1.0f);
	projection = glm::scale(projection, glm::vec3(1.0f, -1.0f, 1.0f));
	projection = glm::translate(projection, glm::vec3(0.0f, -(float)size.y, 0.0f));

	glGenFramebuffers(1, &id);
	glGenTextures(color_buffers, color);
	glGenTextures(1, &depth);

	glBindFramebuffer(GL_FRAMEBUFFER, id);
	Engine::setup_opengl();

	/* enable doublebuffering */
	if ( flags & DOUBLE_BUFFER ){
		max = 2;
	}

	/* bind color buffers */
	for ( unsigned int i = 0; i < max; i++ ){
		glBindTexture(GL_TEXTURE_2D, color[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format == GL_RGB8 ? GL_RGB : GL_RGBA, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	}
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color[front], 0);
	checkForGLErrors("glFramebufferTexture2D::color");

	/* bind depth buffer */
	if ( flags & DEPTH_BUFFER ){
		glBindTexture(GL_TEXTURE_2D, depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
		checkForGLErrors("glFramebufferTexture2D::depth");
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE){
		switch( status ) {
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			Logging::fatal("Framebuffer object format is unsupported by the video hardware. (GL_FRAMEBUFFER_UNSUPPORTED_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			Logging::fatal("Framebuffer incomplete attachment. (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			Logging::fatal("Framebuffer incomplete missing attachment. (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			Logging::fatal("Framebuffer incomplete dimensions. (GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			Logging::fatal("Framebuffer incomplete formats. (GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			Logging::fatal("Framebuffer incomplete draw buffer. (GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			Logging::fatal("Framebuffer incomplete read buffer. (GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT)\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
			Logging::fatal("Framebuffer incomplete multisample buffer. (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT)\n");
			break;
		default:
			Logging::fatal("Framebuffer incomplete: %s\n", gluErrorString(status));
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkForGLErrors("RenderTarget() fin");

	with([this](){
		RenderTarget::clear(Color::black);
	} );
}

RenderTarget::~RenderTarget(){
	glDeleteFramebuffers(1, &id);
	glDeleteTextures(color_buffers, color);
	glDeleteTextures(1, &depth);
}

RenderTarget* RenderTarget::MRT(unsigned int targets){
	static const GLenum mrt[] = {
		GL_COLOR_ATTACHMENT0_EXT,
		GL_COLOR_ATTACHMENT1_EXT,
		GL_COLOR_ATTACHMENT2_EXT,
		GL_COLOR_ATTACHMENT3_EXT,
	};

	/* sanity check */
	if ( !(flags & MULTIPLE_RENDER_TARGETS) ){
		Logging::fatal("RenderTarget::MRT(..) called without MRT enabled on target. Set flag RenderTarget::MULTIPLE_RENDER_TARGETS when allocating RenderTarget.\n");
	}
	if ( targets > 4 ){
		Logging::fatal("RenderTarget::MRT only supports 4 units, can be raised in code.\n");
	}

	/* generate new textures */
	glDeleteTextures(color_buffers, color);
	glGenTextures(targets, color);
	color_buffers = targets;

	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glDrawBuffers(targets, mrt);

	/* bind color buffers */
	for ( unsigned int i = 0; i < color_buffers; i++ ){
		glBindTexture(GL_TEXTURE_2D, color[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format == GL_RGB8 ? GL_RGB : GL_RGBA, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color[i], 0);
		checkForGLErrors("glFramebufferTexture2D::color");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return this;
}

void RenderTarget::init_vbo(){
	static const Shader::vertex_t vertices[4] = {
		{/* .pos = */ glm::vec3(0, 0, 0), /* .uv = */ glm::vec2(0, 1), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec4(1)},
		{/* .pos = */ glm::vec3(0, 1, 0), /* .uv = */ glm::vec2(0, 0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec4(1)},
		{/* .pos = */ glm::vec3(1, 1, 0), /* .uv = */ glm::vec2(1, 0), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec4(1)},
		{/* .pos = */ glm::vec3(1, 0, 0), /* .uv = */ glm::vec2(1, 1), glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec4(1)},
	};
	static const unsigned int indices[4] = {0,1,2,3};

	/* don't reinitialize */
	static int initialized = false;
	if ( initialized ) return;
	initialized = true;

	/** @todo memory leak when closing application */
	glGenBuffers(2, vbo);

	/* upload data */
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RenderTarget::bind(){
	if ( stack ){
		Logging::fatal("Nesting problem with RenderTarget, another target already bound.\n");
	}

	glViewport(0, 0, size.x, size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color[back], 0);

	stack = this;
}

void RenderTarget::unbind(){
	if ( !stack ){
		Logging::fatal("Nesting problem with RenderTarget, no target is bound\n");
	}

	front = back;
	back = (back + 1) % max;
	glViewport(0, 0, resolution.x, resolution.y);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	stack = nullptr;
}

void RenderTarget::with(const std::function<void()>& func){
	bind();
	func();
	unbind();
}

GLuint RenderTarget::texture(unsigned int target) const {
	return color[front + target];
}

GLuint RenderTarget::depthbuffer() const {
	return depth;
}

const glm::mat4& RenderTarget::ortho() const {
	return projection;
}

void RenderTarget::texture_bind(Shader::TextureUnit unit, unsigned int target) const {
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, texture(target));
}

void RenderTarget::texture_bind(Shader::TextureUnit unit) const {
	texture_bind(unit, 0);
}

void RenderTarget::texture_unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderTarget::depth_bind(Shader::TextureUnit unit) const {
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, depthbuffer());
}

void RenderTarget::depth_unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderTarget::clear(const Color& color){
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void RenderTarget::draw(const Shader* shader) const {
	draw(shader, glm::ivec2(0,0), size);
}

void RenderTarget::draw(const Shader* shader, const glm::ivec2& pos) const {
	draw(shader, pos, size);
}

void RenderTarget::draw(const Shader* shader, const glm::vec2& pos) const {
	draw(shader, pos, glm::vec2(size.x, size.y));
}

void RenderTarget::draw(const Shader* shader, const glm::ivec2& pos, const glm::ivec2& size) const {
	draw(shader, glm::vec2(pos.x, pos.y), glm::vec2(size.x, size.y));
}

void RenderTarget::draw(const Shader* shader, const glm::vec2& pos, const glm::vec2& size) const {
	glm::mat4 model(1.f);

	model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
	model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

	Shader::upload_model_matrix(model);
	Shader::upload_resolution(glm::ivec2(size));

	shader->bind();

	/* bind colorbuffers */
	const unsigned int units = flags & DOUBLE_BUFFER ? 1 : color_buffers;
	for ( unsigned int i = 0; i < units; i++ ){
		texture_bind((Shader::TextureUnit)(Shader::TEXTURE_2D_0 + i), i);
	}

	/* bind depth buffer */
	if ( flags & DEPTH_BUFFER ){
		depth_bind(Shader::TEXTURE_DEPTHMAP);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);

	glVertexAttribPointer(Shader::ATTR_POSITION,  3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, pos));
	glVertexAttribPointer(Shader::ATTR_TEXCOORD,  2, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, uv));
	glVertexAttribPointer(Shader::ATTR_NORMAL,    3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, normal));
	glVertexAttribPointer(Shader::ATTR_TANGENT,   3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, tangent));
	glVertexAttribPointer(Shader::ATTR_BITANGENT, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, bitangent));
	glVertexAttribPointer(Shader::ATTR_COLOR,     4, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, color));

	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);
}

void RenderTarget::transfer(const Shader* shader, const RenderTarget* target){
	Shader::upload_projection_view_matrices(ortho(), glm::mat4());
	bind();
	target->draw(shader, glm::vec2(0,0), glm::vec2(size));
	unbind();
}
