#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "techniques/dof.hpp"
#include "logging.hpp"

namespace Technique {
	DoF::DoF(const glm::ivec2& size, int blur_passes, GLenum format) throw()
		:	RenderTarget(size, format, 0, GL_LINEAR)
		, blur(size, blur_passes, format)
		{
			shader = Shader::create_shader("/shaders/dof");
}

	DoF::~DoF() { }

	void DoF::render(const RenderTarget * target) {
		blur.render(target);

		blur.texture_bind(Shader::TEXTURE_BLEND_1);
		target->depth_bind(Shader::TEXTURE_DEPTHMAP);
		transfer(shader, target);
	}
};
