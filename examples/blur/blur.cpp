#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "techniques/blur.hpp"
#include "utils.hpp"

static RenderObject* obj = nullptr;
static RenderTarget* scene = nullptr;
static RenderTarget* pass = nullptr;
static Technique::Blur * blur = nullptr;
static Shader* shader = nullptr;
static Shader* split = nullptr;
static Shader* blur_temporal = nullptr;
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
		pass  = new RenderTarget(resolution/4, GL_RGB8, 0, GL_LINEAR);
		shader   = Shader::create_shader("/shaders/normal");
		split    = Shader::create_shader("/shaders/split");
		blur_temporal  = Shader::create_shader("/shaders/blur_temporal");

		blur = new Technique::Blur(resolution);

		blur_temporal->bind();
		glUniform1f(blur_temporal->uniform_location("factor"), 0.5f);
	}

	void start(double seek) {

	}

	void cleanup(){
		delete pass;
		delete scene;
		delete obj;
		delete blur;
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

		pass->texture_bind(Shader::TEXTURE_2D_1); /* for temporal blur */
		pass->transfer(blur_temporal, blur);
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());
		RenderTarget::clear(Color::magenta);
		scene->texture_bind(Shader::TEXTURE_2D_1); /* for comparing */
		pass->draw(split, glm::vec2(0,0), glm::vec2(resolution));
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
