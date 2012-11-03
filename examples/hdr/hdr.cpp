#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "lights_data.hpp"
#include "quad.hpp"
#include "material.hpp"
#include "techniques/hdr.hpp"

static RenderObject* obj = nullptr;
static RenderObject* sphere = nullptr;
static Quad* plane = nullptr;
static RenderTarget* scene = nullptr;
static Shader* shader = nullptr;
static Shader* shader_passthru = nullptr;
static Shader* shader_noshading = nullptr;
static Shader* shader_debug = nullptr;
static Camera cam(75, 1.3f, 0.1f, 10);
extern glm::mat4 screen_ortho; /* defined in main.cpp */
extern glm::ivec2 resolution; /* defined in main.cpp */
static LightsData * lights = nullptr;

static Technique::HDR * hdr;

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Data::add_search_path(srcdir "/examples/shadowmaps");

		lights = new LightsData();

		obj      = new RenderObject("/models/teapot.obj", true);
		sphere   = new RenderObject("/models/sphere.nff", true);
		plane    = new Quad();
		scene    = new RenderTarget(resolution, GL_RGB8, RenderTarget::DEPTH_BUFFER, GL_LINEAR);
		shader   = Shader::create_shader("/shaders/normal");
		shader_passthru   = Shader::create_shader("/shaders/passthru");
		shader_noshading   = Shader::create_shader("/shaders/noshading");
		shader_debug   = Shader::create_shader("/shaders/debug");
		sphere->materials[0].diffuse =  glm::vec4(10.f);


		hdr = new Technique::HDR(resolution, 2.1, 2.2, 1.8f);

		plane->set_rotation(glm::vec3(1.f, 0.f, 0.f), 90.f);
		plane->set_position(glm::vec3(-10.f, 0.0f, -10.f)); 
		plane->set_scale(20.f);
		
		obj->set_position(glm::vec3(0.f, 0.f, 0.f));

		lights->ambient_intensity() = glm::vec3(0.5f);
		lights->num_lights() = 1;
		lights->lights[0]->intensity = glm::vec3(0.8f);
		lights->lights[0]->type = MovableLight::POINT_LIGHT;

		sphere->set_scale(0.1f);
		sphere->add_position_callback(lights->lights[0]);
		sphere->set_position(glm::vec3(0.f, 2.f, 0.f));

		cam.set_position(glm::vec3(1.0, 0.0, 1.f));
		cam.look_at(glm::vec3(0,0,0));
	}

	void start(double seek) {

	}

	void cleanup(){
		delete scene;
		delete obj;
		delete hdr;
		Shader::cleanup();
	}

	static void render_scene(){
		Shader::upload_model_matrix(glm::mat4());

		Shader::upload_lights(*lights);

		scene->with([](){
				RenderTarget::clear(Color::black);
				shader->bind();
				Shader::upload_camera(cam);
				obj->render();
				//plane->render();
				shader_noshading->bind();
				sphere->render();
		});
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());

		hdr->render(scene);

		RenderTarget::clear(Color::magenta);
		hdr->draw(shader_passthru, glm::vec2(0,0), glm::vec2(resolution));
	}

	void render(){
		render_scene();
		render_blit();
	}

	static void print_values() {
		printf("Exposure: %f\n, bloom: %f\n, bright_max: %f\n", hdr->exposure(), hdr->bloom_factor(), hdr->bright_max());
	}

	void update(float t, float dt) {
		const float s = t*0.5f;
		const float d = 1.7f;

		sphere->set_position(glm::vec3(cos(s)*d, 0.00, sin(s)*d));

#ifdef ENABLE_INPUT
		input.update_object(cam, dt);
		if(input.down(Input::ACTION_0)) {
			hdr->set_exposure(hdr->exposure() - 0.1);
			print_values();
		}
		if(input.down(Input::ACTION_1)) {
			hdr->set_exposure(hdr->exposure() + 0.1);
			print_values();
		}

		if(input.down(Input::ACTION_2)) {
			hdr->set_bright_max(hdr->bright_max() - 0.1);
			print_values();
		}
		if(input.down(Input::ACTION_3)) {
			hdr->set_bright_max(hdr->bright_max() + 0.1);
			print_values();
		}

		if(input.down(Input::ACTION_4)) {
			hdr->set_bloom_factor(hdr->bloom_factor() - 0.1);
			print_values();
		}
		if(input.down(Input::ACTION_5)) {
			hdr->set_bloom_factor(hdr->bloom_factor() + 0.1);
			print_values();
		}

#endif
	}
}
