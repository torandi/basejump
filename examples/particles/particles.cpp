#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "particle_system.hpp"
#include "lights_data.hpp"
#include "quad.hpp"
#include "input.hpp"
#include "config.hpp"

static RenderObject* obj = nullptr;
static Quad* plane = nullptr;
static RenderTarget* scene = nullptr;
static Shader* shader = nullptr;
static Shader* shader_passthru = nullptr;
static Shader* shader_particles = nullptr;
static Camera cam(75, 1.3f, 0.1f, 10);
extern glm::mat4 screen_ortho; /* defined in main.cpp */
extern glm::ivec2 resolution; /* defined in main.cpp */
static LightsData * lights = nullptr;
static ParticleSystem * particles = nullptr;
static TextureArray * particle_textures = nullptr;

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Data::add_search_path(srcdir "/examples/particles");

		lights = new LightsData();

		obj      = new RenderObject("/models/bench.obj", true);
		plane    = new Quad();
		scene    = new RenderTarget(resolution, GL_RGB8, RenderTarget::DEPTH_BUFFER, GL_LINEAR);
		shader   = Shader::create_shader("/shaders/normal");
		shader_passthru   = Shader::create_shader("/shaders/passthru");
		shader_particles   = Shader::create_shader("/shaders/particles");

		plane->set_rotation(glm::vec3(1.f, 0.f, 0.f), 90.f);
		plane->set_position(glm::vec3(-10.f, -0.5f, -10.f));
		plane->set_scale(20.f);

		particle_textures = TextureArray::from_filename("/textures/fog.png", nullptr);

		particles = new ParticleSystem(10000, particle_textures);
		Config cfg = Config::parse("/particles.cfg");
		particles->read_config(cfg["particles"]);
		particles->update_config();

		lights->ambient_intensity() = glm::vec3(0.1f);
		lights->num_lights() = 1;
		lights->lights[0]->set_position(glm::vec3(0, -1.f, -1.f));
		lights->lights[0]->intensity = glm::vec3(0.8f);
		lights->lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;

		cam.set_position(glm::vec3(0.0f, 0.1, 0.7));
		cam.look_at(glm::vec3(0,0,0));
		cam.yaw(1.f);
	}

	void start(double seek) {

	}

	void cleanup(){
		delete scene;
		delete obj;
		Shader::cleanup();
	}

	static void render_geometry() {
		RenderTarget::clear(Color::green);
		shader->bind();
		obj->render();
		Shader::upload_blank_material();
		plane->render();
	}

	static void render_scene(){
		lights->lights[0]->render_shadow_map(cam, [](const glm::mat4 &m) -> void  {
				render_geometry();
		});

		Shader::upload_projection_view_matrices(cam.projection_matrix(), cam.view_matrix());
		Shader::upload_model_matrix(glm::mat4());
		Shader::upload_lights(*lights);
		Shader::upload_blank_material();

		scene->with([](){
				render_geometry();
				shader_particles->bind();
				particles->render();
		});
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());
		RenderTarget::clear(Color::magenta);
		scene->draw(shader_passthru, glm::vec2(0,0), glm::vec2(resolution));
	}

	void render(){
		render_scene();
		render_blit();
	}

	void update(float t, float dt){
		obj->yaw(dt);
		particles->update(dt);
	}
}