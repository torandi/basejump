#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rendertarget.hpp"
#include <cstdio>

RenderTarget* RenderTarget::stack = nullptr;

RenderTarget::RenderTarget(const glm::ivec2& size, bool alpha)
	: size(size)
	, id(0)
	, current(0){

	glGenFramebuffersEXT(1, &id);
	glGenTextures(2, color);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	for ( int i = 0; i < 2; i++ ){
		glBindTexture(GL_TEXTURE_2D, color[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, alpha ? GL_RGBA8 : GL_RGB8, size.x, size.y, 0, alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color[current], 0);

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status != GL_FRAMEBUFFER_COMPLETE_EXT){
		fprintf(stderr, "Framebuffer incomplete: %s\n", gluErrorString(status));
		abort();
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

RenderTarget::~RenderTarget(){
	glDeleteFramebuffersEXT(1, &id);
	glDeleteTextures(2, color);
}

void RenderTarget::bind(){
	if ( stack ){
		fprintf(stderr, "Nesting problem with RenderTarget, another target already bound.\n");
		abort();
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color[current], 0);

	stack = this;
}

void RenderTarget::unbind(){
	if ( !stack ){
		fprintf(stderr, "Nesting problem with RenderTarget, no target is bound\n");
		abort();
	}

	current = 1 - current;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	stack = nullptr;
}

GLuint RenderTarget::texture() const {
	return color[1-current];
}

void RenderTarget::clear(const Color& color) const {
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}