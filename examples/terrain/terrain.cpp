#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "lights_data.hpp"
#include "rendertarget.hpp"
#include "terrain.hpp"
#include "time.hpp"
#include "globals.hpp"

static Shader* shader = nullptr;
static Shader* passthru = nullptr;
static Terrain* terrain = nullptr;
static RenderTarget* scene = nullptr;
static LightsData* lights = nullptr;
static Camera cam(75, 1.3f, 0.1f, 400.0f);
extern glm::mat4 screen_ortho;   /* defined in main.cpp */
extern Time global_time;         /* defined in main.cpp */
extern glm::ivec2 resolution;    /* defined in main.cpp */

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Data::add_search_path(srcdir "/examples/terrain");

		passthru  = Shader::create_shader("/shaders/passthru");
		shader    = Shader::create_shader("/shaders/normal");
		terrain   = new Terrain("/terrain.cfg");
		scene     = new RenderTarget(resolution, GL_RGB8, RenderTarget::DEPTH_BUFFER, GL_LINEAR);
		lights = new LightsData();

		terrain->set_position(glm::vec3(-7.5f, -2.0f, -7.5f));
		lights->ambient_intensity() = glm::vec3(0.5f);
		lights->num_lights() = 1;
		lights->lights[0]->set_position(glm::vec3(0, -1.0f, -0.5f));
		lights->lights[0]->intensity = glm::vec3(1.0f);
		lights->lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;

		Shader::fog_t fog = { glm::vec4(0.8, 0.8, 0.8, 1.0), 0.01f };

		Shader::upload_fog(fog);

		cam.set_position(glm::vec3(0.f, 32.f, 0.f));
		cam.look_at(glm::vec3(0.f, 32.f, 2.f));

#ifdef ENABLE_INPUT
		Input::movement_speed = 32.f;
#endif
	}

	void start(double seek){
	}

	void cleanup(){
		delete terrain;
		delete scene;
		Shader::cleanup();
	}

	void render(){
		Shader::upload_camera(cam);
		Shader::upload_model_matrix(glm::mat4());
		Shader::upload_lights(*lights);
		Shader::upload_blank_material();
		Shader::upload_resolution(resolution);

		RenderTarget::clear(Color::white);
		terrain->render();
	}

	void update(float t, float dt){
		const float s = t*0.5f;
		const float d = 15.5f;

#ifdef ENABLE_INPUT
		input.update_object(cam, dt);
#else
		cam.look_at(glm::vec3(0,0,0));
		cam.set_position(glm::vec3(cos(s)*d, 8.0f, sin(s)*d));
#endif
	}
}
