#ifndef TECHNIQUE_BLUR_HPP
#define TECHNIQUE_BLUR_HPP

#include "rendertarget.hpp"
#include <vector>

namespace Technique {
	class Blur : public RenderTarget {
		public:
			/**
			 * Creates a blur technique for bluring a fbo of given size
			 * @param size: Size of the fbo to blur (probably resolution)
			 * @param format: format of the internal fbos
			 * @param passes: Number of passes. One pass includes both horizontal and vertical
			 */
			explicit Blur(const glm::ivec2& size, int num_passes=1, GLenum format=GL_RGB8) throw();
			virtual ~Blur();

			/**
			 * Render the target to the internal fbo with blur
			 */
			void render(const RenderTarget * target);
		private:
			Shader * shader[2];
			std::vector<RenderTarget*> passes;
			const int num_passes;
	};
};

#endif
