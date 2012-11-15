#ifndef TECHNIQUE_DOF_HPP
#define TECHNIQUE_DOF_HPP

#include "rendertarget.hpp"
#include "techniques/blur.hpp"

namespace Technique {
	class DoF : public RenderTarget {
		public:
			/**
			 * Creates a blur technique for bluring a fbo of given size
			 * @param size: Size of the fbo (probably resolution)
			 * @param passes: Number of blur passes. One pass includes both horizontal and vertical
			 * @param format: format of the internal fbos
			 */
			explicit DoF(const glm::ivec2& size, int blur_passes = 1, GLenum format=GL_RGB8) throw();
			virtual ~DoF();

			/**
			 * Render the target to the internal fbo with depth of field
			 */
			void render(const RenderTarget * target);

		private:
			Shader * shader;
			Technique::Blur blur;
	};
};

#endif
