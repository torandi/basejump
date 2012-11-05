#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "techniques/blur.hpp"
#include "techniques/temporalblur.hpp"
#include "utils.hpp"

static RenderObject* obj = nullptr;
static RenderTarget* scene = nullptr;
static Technique::Blur * blur = nullptr;
static Technique::TemporalBlur * temporal_blur = nullptr;
static Shader* shader = nullptr;
static Shader* split = nullptr;
static Camera cam(75, 1.3f, 0.1f, 10);
extern glm::mat4 screen_ortho; /* defined in main.cpp */
extern glm::ivec2 resolution; /* defined in main.cpp */

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Data::add_search_path(srcdir "/examples/blur");

		obj      = new RenderObject("/models/nox.obj", true);
		scene    = new RenderTarget(resolution, GL_RGB8, 0, GL_LINEAR);
		shader   = Shader::create_shader("/shaders/normal");
		split    = Shader::create_shader("/shaders/split");

		blur = new Technique::Blur(resolution);
		temporal_blur = new Technique::TemporalBlur(resolution);
	}

	void start(double seek) {

	}

	void cleanup(){
		delete scene;
		delete obj;
		delete blur;
		delete temporal_blur;
		Shader::cleanup();
	}

	static void render_scene(){
		Shader::upload_projection_view_matrices(cam.projection_matrix(), cam.view_matrix());
		Shader::upload_model_matrix(glm::mat4());
		scene->with([](){
			RenderTarget::clear(Color::green);
			shader->bind();
			obj->render();
		});
	}

	static void render_blur(){
		blur->render(scene);

		temporal_blur->render(blur);
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());
		RenderTarget::clear(Color::magenta);
		scene->texture_bind(Shader::TEXTURE_2D_1); /* for comparing */
		temporal_blur->draw(split, glm::vec2(0,0), glm::vec2(resolution));
	}

	void render(){
		render_scene();
		render_blur();
		render_blit();
	}

	void update(float t, float dt){
		const float s = t*0.5f;
		const float d = 0.7f;

		cam.look_at(glm::vec3(0,0,0));
		cam.set_position(glm::vec3(cos(s)*d, 0.05, sin(s)*d));
	}
}
