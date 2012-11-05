#ifndef TECHNIQUE_TEMPORAL_BLUR_HPP
#define TECHNIQUE_TEMPORAL_BLUR_HPP

#include "rendertarget.hpp"
#include <vector>

namespace Technique {
	class TemporalBlur : public RenderTarget {
		public:
			/**
			 * Creates a temporal blur technique for bluring a fbo of given size
			 * @param size: Size of the fbo to blur
			 * @param format: format of the internal fbos
			 * @param factor: how much of the old frame to mix in
			 */
			explicit TemporalBlur(const glm::ivec2& size, float factor = 0.5, GLenum format=GL_RGB8) throw();
			virtual ~TemporalBlur();

			/**
			 * Render the target to the internal fbo with blur
			 */
			void render(const RenderTarget * target);

			float factor() const { return _factor; };

			/**
			 * Sets factor
			 * Note that this changes the active shader
			 */
			void set_factor(float factor);
		private:
			Shader * shader;
			float _factor;
			GLuint u_factor;
	};
};

#endif
