#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "techniques/temporalblur.hpp"
#include "logging.hpp"

namespace Technique {
	TemporalBlur::TemporalBlur(const glm::ivec2& size, float factor, GLenum format) throw()
		:	RenderTarget(size, format, RenderTarget::DOUBLE_BUFFER, GL_LINEAR) 
		, _factor(factor) {

		shader = Shader::create_shader("/shaders/blur_temporal");
		u_factor = shader->uniform_location("factor");

		shader->bind();
		glUniform1f(u_factor, _factor);
	}

	TemporalBlur::~TemporalBlur() { }

	void TemporalBlur::render(const RenderTarget * target) {
		shader->bind();
		texture_bind(Shader::TEXTURE_2D_1);
		transfer(shader, target);
	}

	void TemporalBlur::set_factor(float factor) {
		shader->bind();	
		glUniform1f(u_factor, _factor);
	}
};
