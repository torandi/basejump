#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "techniques/hdr.hpp"
#include "logging.hpp"

namespace Technique {
	HDR::HDR(const glm::ivec2& size, float exposure, float bright_max, float bloom_factor) throw()
		:	RenderTarget(size, GL_RGB8, 0, GL_LINEAR)
		,	_exposure(exposure)
		, _bright_max(bright_max)
		, _bloom_factor(bloom_factor)
		{
			bright_spots = new RenderTarget(size, GL_RGB8, 0, GL_LINEAR);
			bloom = new Technique::Blur(size, 1, GL_RGB8);

			tonemap = Shader::create_shader("/shaders/tonemap");
			bright_filter = Shader::create_shader("/shaders/bright_filter");
			u_exposure[0] = tonemap->uniform_location("exposure");
			u_bright_max[0] = tonemap->uniform_location("bright_max");
			u_bloom_factor = tonemap->uniform_location("bloom_factor");

			u_exposure[1] = bright_filter->uniform_location("exposure");
			u_bright_max[1] = bright_filter->uniform_location("bright_max");


			tonemap->bind();
			glUniform1f(u_exposure[0], _exposure);
			glUniform1f(u_bright_max[0], _bright_max);
			glUniform1f(u_bloom_factor, _bloom_factor);

			bright_filter->bind();
			glUniform1f(u_exposure[1], _exposure);
			glUniform1f(u_bright_max[1], _bright_max);
	}

	HDR::~HDR() {
		delete bright_spots;
		delete bloom;
	}

	void HDR::render(const RenderTarget * target) {
		bright_spots->with([&]() {
					RenderTarget::clear(Color::black);
		});
	
		bright_spots->transfer(bright_filter, target);
		bloom->render(bright_spots);

		bloom->texture_bind(Shader::TEXTURE_BLOOM);
		transfer(tonemap, target);
	}

	void HDR::set_exposure(float exposure) {
		_exposure = exposure;
		tonemap->bind();
		glUniform1f(u_exposure[0], _exposure);

		bright_filter->bind();
		glUniform1f(u_exposure[1], _exposure);
	}

	void HDR::set_bright_max(float bright_max) {
		_bright_max = bright_max;

		tonemap->bind();
		glUniform1f(u_bright_max[0], _bright_max);

		bright_filter->bind();
		glUniform1f(u_bright_max[1], _bright_max);
	}

	void HDR::set_bloom_factor(float bloom_factor) {
		_bloom_factor = bloom_factor;

		tonemap->bind();
		glUniform1f(u_bloom_factor, _bloom_factor);

	}
};
