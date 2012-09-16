#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"

static RenderObject* obj = nullptr;
static RenderTarget* scene = nullptr;
static RenderTarget* pass[3] = {nullptr, nullptr, nullptr};
static Shader* shader = nullptr;
static Shader* split = nullptr;
static Shader* blur[3] = {nullptr, nullptr, nullptr};
static Camera cam(75, 1.3, 0.1, 10);
extern glm::mat4 screen_ortho; /* defined in main.cpp */
extern glm::ivec2 resolution; /* defined in main.cpp */

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Data::add_search_path(PATH_BASE "examples/blur");

		obj      = new RenderObject("/models/nox.obj", true);
		scene    = new RenderTarget(resolution, GL_RGB8);
		pass[0]  = new RenderTarget(resolution/2, GL_RGB8, 0, GL_LINEAR);
		pass[1]  = new RenderTarget(resolution/4, GL_RGB8, 0, GL_LINEAR);
		pass[2]  = new RenderTarget(resolution/4, GL_RGB8, 0, GL_LINEAR);
		shader   = Shader::create_shader("/shaders/normal");
		split    = Shader::create_shader("/shaders/split");
		blur[0]  = Shader::create_shader("/shaders/blur_vertical");
		blur[1]  = Shader::create_shader("/shaders/blur_horizontal");
		blur[2]  = Shader::create_shader("/shaders/blur_temporal");

		glUniform1f(blur[2]->uniform_location("factor"), 0.75f);
	}

	void start(double seek) {

	}

	void cleanup(){
		delete scene;
		delete obj;
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
		pass[2]->texture_bind(Shader::TEXTURE_2D_1); /* for temporal blur */

		pass[0]->transfer(blur[0], scene);
		pass[1]->transfer(blur[1], pass[0]);
		pass[2]->transfer(blur[2], pass[1]);
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());
		RenderTarget::clear(Color::magenta);
		scene->texture_bind(Shader::TEXTURE_2D_1); /* for comparing */
		pass[2]->draw(split, glm::vec2(0,0), glm::vec2(resolution));
	}

	void render(){
		render_scene();
		render_blur();
		render_blit();
	}

	void update(float t, float dt){
		const float s = t*0.5;
		const float d = 0.7f;

		cam.look_at(glm::vec3(0,0,0));
		cam.set_position(glm::vec3(cos(s)*d, 0.05, sin(s)*d));
	}
}
