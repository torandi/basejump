#ifndef TECHNIQUE_HDR_HPP
#define TECHNIQUE_HDR_HPP

#include "rendertarget.hpp"
#include "techniques/blur.hpp"

namespace Technique {
	class HDR : public RenderTarget {
		public:
			/**
			 * Creates a hdr technique for doing hdr with bloom on a fbo
			 * @param size: Size of the fbo (probably resolution)
			 */
			explicit HDR(const glm::ivec2& size, float exposure, float bright_max, float bloom_factor = 0.5f) throw();
			virtual ~HDR();

			/**
			 * Render the target to the internal fbo with tonemapping and blur
			 */
			void render(const RenderTarget * target);

			float exposure() const { return _exposure; };
			float bright_max() const { return _bright_max; };
			float bloom_factor() const { return _bloom_factor; };

			void set_exposure(float exposure);
			void set_bright_max(float bright_max);
			void set_bloom_factor(float bloom_factor);

		private:
			Shader * tonemap, *bright_filter;
			RenderTarget * bright_spots;
			Technique::Blur * bloom;

			float _exposure, _bright_max, _bloom_factor;
			GLuint u_exposure[2], u_bright_max[2], u_bloom_factor;
	};
};

#endif
