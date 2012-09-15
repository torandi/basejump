#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "color.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <functional>

class RenderTarget: public TextureBase {
public:
	static RenderTarget* stack;

	enum Flags: int {
		DEPTH_BUFFER  = (1<<0),           /** Enable depth buffer/write. */
		DOUBLE_BUFFER = (1<<1),           /** Use doublebuffering so you can render the previous frame in the current frame. */
		MULTIPLE_RENDER_TARGETS = (1<<2), /** Enable usage of multiple rendertargets, remember to call MRT(N) */
	};

	/**
	 * RenderTarget is a framebuffer object which you can render to.
	 *
	 * Typical usage:
	 *
	 *   RenderTarget foo(ivec2(200,200), GL_RGB8, true);
	 *   foo.bind();
	 *   // render code here..
	 *   foo.unbind();
	 *   foo.draw()
	 *
	 * Color attachment is double-buffered so it is safe to bind itself as texture
	 * unit when rendering. Depth-buffer is not double buffered and should not be
	 * used the same way.
	 *
	 * @param size Size of the final textures.
	 * @param format Format of the color attachment.
	 * @param flags A bitmask of parameters for the rendertarget.
	 * @param filter Texture filtering of color attachment.
	 * @see Flags
	 */
	explicit RenderTarget(const glm::ivec2& size, GLenum format, int flags = 0, GLenum filter = GL_NEAREST) throw();
	~RenderTarget();

	/**
	 * Enable MRT support for this rendertarget.
	 * Doublebuffering cannot be enabled when using MRT. (because I'm lazy)
	 *
	 * @param target Number of color targets.
	 */
	RenderTarget* MRT(unsigned int targets);

	void bind();
	void unbind();

	/**
	 * Get orthographic projection for rendering on this target.
	 */
	const glm::mat4& ortho() const;

	/**
	 * Call func while target is bound.
	 * short for: bind(); func(); unbind();
	 */
	void with(const std::function<void()>& func);

	/**
	 * Get texture id of current frontbuffer.
	 * @param target Use 0 for normal targets or index when using MRT.
	 */
	GLuint texture(unsigned int target = 0) const;

	/**
	 * Bind texture from color buffer target.
	 * @param target Use 0 for normal targets or index when using MRT.
	 */
	virtual void texture_bind(Shader::TextureUnit unit, unsigned int target) const;

	/**
	 * Calls texture_bind(unit, 0)
	 */
	virtual void texture_bind(Shader::TextureUnit unit) const;
	virtual void texture_unbind() const;

	void depth_bind(Shader::TextureUnit unit) const;
	void depth_unbind() const;

	/**
	 * Get texture id of depthbuffer.
	 */
	GLuint depthbuffer() const;

	/**
	 * Short for: glClearColor(..); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	 */
	static void clear(const Color& color);

	/**
	 * Render the RenderTarget on current framebuffer. Caller should ensure an
	 * orthographic projection is bound before calling draw.
	 *
	 * Color-buffers is passed as texture unit 0..N and if depth is enabled it is
	 * passed in unit 7.
	 */
	void draw(const Shader* shader) const;
	void draw(const Shader* shader, const glm::vec2& pos) const;
	void draw(const Shader* shader, const glm::ivec2& pos) const;
	void draw(const Shader* shader, const glm::ivec2& pos, const glm::ivec2& size) const;
	void draw(const Shader* shader, const glm::vec2& pos, const glm::vec2& size) const;

	/**
	 * Transfer other target to this render target using shader.
	 *
	 * Roughly equivalent to:
	 *   this->bind();
	 *   target->draw(shader);
	 *   this->unbind();
	 *
	 * @param shader The shader to use.
	 * @param target Which rendertarget to transfer onto this.
	 */
	void transfer(const Shader* shader, const RenderTarget* target);

private:
	static GLuint vbo[2];
	static void init_vbo();

	glm::mat4 projection;

	/* parameters */
	int flags;
	GLenum format;
	GLenum filter;

	GLuint color_buffers;
	GLuint id;
	GLuint front;
	GLuint back;
	GLuint max;
	GLuint color[2];
	GLuint depth;
};

#endif /* RENDER_TARGET_H */
