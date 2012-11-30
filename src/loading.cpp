#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "loading.hpp"
#include "logging.hpp"
#include "globals.hpp"
#include "texture.hpp"
#include "quad.hpp"
#include "utils.hpp"
#include "engine.hpp"
#include <glm/gtc/matrix_transform.hpp>

static const unsigned long fade_time = static_cast<float>(1e6); /* in µs */

namespace Loading {

	static Shader* shader = nullptr;
	static Texture2D* texture = nullptr;
	static Quad* quad = nullptr;
	static glm::mat4 projection;

	static void render(float s) {
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_blank_material();
		Shader::upload_model_matrix(glm::mat4());

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		shader->bind();

		texture->texture_bind(Shader::TEXTURE_2D_0);
		quad->render();

		SDL_GL_SwapBuffers();
	}

	void init(const glm::ivec2& resolution){
		Logging::verbose("Preparing loading scene\n");

		/* setup window projection matrix */
		projection = glm::ortho(0.0f, static_cast<float>(resolution.x), 0.0f, static_cast<float>(resolution.y), -1.0f, 1.0f);
		projection = glm::scale(projection, glm::vec3(1.0f, -1.0f, 1.0f));
		projection = glm::translate(projection, glm::vec3(0.0f, -static_cast<float>(resolution.y), 0.0f));

		shader = Shader::create_shader("/shaders/passthru");

		texture = Texture2D::from_filename("/basejump/loading.jpg");

		quad = new Quad(glm::vec2(1.f, -1.f), false);
		float scale = static_cast<float>(resolution.x)/1280.f;

		quad->set_scale(glm::vec3(resolution,1));

		render(0.0f);
	}

	void cleanup(){
		long t = util_utime();
		float s = 0.0f;
		while ( s < 1.0 ){
			render(1.0f - s);
			s = static_cast<float>(util_utime() - t ) / fade_time;
		}

		delete quad;
	}

	void progress(const std::string& name, int elem, int total){
		const float s = static_cast<float>(elem) / static_cast<float>(total);
		render(s);
	}
}
