#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "loading.hpp"
#include "logging.hpp"
#include "globals.hpp"
#include "texture.hpp"
#include "quad.hpp"
#include "utils.hpp"

static const unsigned long fade_time = 1e6; /* in µs */

namespace Loading {

	static Shader* shader = nullptr;
	static Texture2D* texture[3] = {nullptr, };
	static Quad* quad[2] = {nullptr, };
	static GLint u_fade = -1;

	static void render(float s) {
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_blank_material();

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		shader->bind();
		glUniform1f(u_fade, s);

		texture[0]->texture_bind(Shader::TEXTURE_2D_0);
		texture[1]->texture_bind(Shader::TEXTURE_2D_1);
		quad[0]->render();

		glUniform1f(u_fade, 0.0f);
		texture[2]->texture_bind(Shader::TEXTURE_2D_0);
		quad[1]->render();

		SDL_GL_SwapBuffers();
	}

	void init(const glm::ivec2& resolution){
		Logging::verbose("Preparing loading scene\n");

		shader = Shader::create_shader("/shaders/loading");
		u_fade = shader->uniform_location("fade");

		texture[0] = Texture2D::from_filename("/textures/frob_nocolor.png");
		texture[1] = Texture2D::from_filename("/textures/frob_color.png");
		texture[2] = Texture2D::from_filename("/textures/loading.png");

		quad[0] = new Quad(glm::vec2(1.f, -1.f), false);
		quad[1] = new Quad(glm::vec2(1.f, -1.f), false);

		float scale = resolution.x/1280.f;

		quad[0]->set_scale(glm::vec3(1024*scale,512*scale,1));
		quad[0]->set_position(glm::vec3(resolution.x/2.f - (1024*scale)/2.f, 3.f*resolution.y/10.f - (512*scale)/2.f,1.f));

		quad[1]->set_scale(glm::vec3(512*scale,128*scale,1));
		quad[1]->set_position(glm::vec3(resolution.x/2.f - (512*scale)/2.f, 7.f*resolution.y/10.f - (128*scale)/2.f,1.f));

		render(0.0f);
	}

	void cleanup(){
		long t = util_utime();
		float s = 0.0f;
		while ( s < 1.0 ){
			render(1.0 - s);
			s = static_cast<float>(util_utime() - t ) / fade_time;
		}

		for( auto ptr : quad) {
			delete ptr;
		}
	}

	void progress(const std::string& name, int elem, int total){
		const float s = (float)elem / total;
		render(s);
	}
}
