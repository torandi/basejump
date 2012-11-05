#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "techniques/blur.hpp"
#include "logging.hpp"

namespace Technique {
	Blur::Blur(const glm::ivec2& size, int num_passes_, GLenum format) throw()
		:	RenderTarget(size/(4*num_passes_), format, 0, GL_LINEAR) 
		, num_passes(num_passes_) {

		if(num_passes < 1) Logging::fatal("Invalid number of passes for blur technique: %d\n", num_passes);

		shader[0] = Shader::create_shader("/shaders/blur_vertical");
		shader[1] = Shader::create_shader("/shaders/blur_horizontal");
		
		for(int i=0; i<(num_passes * 2 - 1); ++i) {
			passes.push_back(new RenderTarget(size/(2 * (i+1)), format, 0, GL_LINEAR));
		}
	}

	Blur::~Blur() {
		for(RenderTarget * pass : passes) {
			delete pass;
		}
	}

	void Blur::render(const RenderTarget * target) {
		const RenderTarget * cur = target;
		for(int i=0; i < num_passes - 1; ++i) {
			passes[i*2]->transfer(shader[0], cur);
			passes[i*2 + 1]->transfer(shader[1], passes[i*2]);
			cur = passes[i*2 + 1];
		}
		passes[(num_passes - 1) * 2]->transfer(shader[0], cur);
		transfer(shader[1], passes[(num_passes - 1) * 2]);
	}
};
