#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "engine.hpp"
#include "globals.hpp"
#include "rendertarget.hpp"

static RenderTarget* mrt = nullptr;
static RenderTarget* fs = nullptr;
static Shader* shader1 = nullptr;
static Shader* shader2 = nullptr;

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		mrt = (new RenderTarget(resolution, GL_RGB8, RenderTarget::MULTIPLE_RENDER_TARGETS))->MRT(2);
		fs  = (new RenderTarget(resolution, GL_RGB8));
		shader1 = Shader::create_shader("mrt_test1");
		shader2 = Shader::create_shader("mrt_test2");
	}

	void start(double seek) {

	}

	void cleanup(){
		delete fs;
		delete mrt;

		Shader::cleanup();
	}

	void render(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());

		shader1->bind();
		mrt->with([](){
				RenderTarget::clear(Color::green);
				fs->draw(shader1);
		});

		RenderTarget::clear(Color::magenta);
		mrt->draw(shader2);
	}

	void update(float t, float dt){

	}
}
