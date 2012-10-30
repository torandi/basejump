#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "lights_data.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "terrain.hpp"
#include "time.hpp"

static Shader* shader = nullptr;
static Shader* passthru = nullptr;
static TextureArray* colormap = nullptr;
static TextureArray* normalmap = nullptr;
static Terrain* terrain = nullptr;
static RenderObject* obj = nullptr;
static RenderTarget* scene = nullptr;
static LightsData* lights = nullptr;
static Camera cam(75, 1.3f, 0.1f, 100.0f);
extern glm::mat4 screen_ortho;   /* defined in main.cpp */
extern Time global_time;         /* defined in main.cpp */
extern glm::ivec2 resolution;    /* defined in main.cpp */

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		passthru  = Shader::create_shader("/shaders/passthru");
		shader    = Shader::create_shader("/shaders/normal");
		colormap  = TextureArray::from_filename("/color0.png", "/color1.png", nullptr);
		normalmap = TextureArray::from_filename("/textures/default_normalmap.jpg", "/textures/default_normalmap.jpg", nullptr);
		terrain   = new Terrain("/nx15/terrain.png", 15.0f, 4.0f, colormap, normalmap);
		scene     = new RenderTarget(resolution, GL_RGB8, RenderTarget::DEPTH_BUFFER, GL_LINEAR);
		lights    = new LightsData();
		obj       = new RenderObject("/models/bench.obj", true);

		terrain->set_position(glm::vec3(-7.5f, -2.0f, -7.5f));
		lights->ambient_intensity() = glm::vec3(0.1f);
		lights->num_lights() = 1;
		lights->lights[0]->set_position(glm::vec3(0, -0.3f, -1.f));
		lights->lights[0]->intensity = glm::vec3(0.8f);
		lights->lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;
	}

	void start(double seek){
	}

	void cleanup(){
		delete terrain;
		delete scene;
		Shader::cleanup();
	}

	static void render_geometry(){
		RenderTarget::clear(Color::magenta);
		terrain->render();
	}

	static void render_scene(){
		lights->lights[0]->render_shadow_map(cam, [](const glm::mat4 &m) -> void  {
			render_geometry();
		});

		Shader::upload_projection_view_matrices(cam.projection_matrix(), cam.view_matrix());
		Shader::upload_model_matrix(glm::mat4());
		Shader::upload_lights(*lights);
		Shader::upload_blank_material();
		Shader::upload_resolution(resolution);
		RenderTarget::clear(Color::blue);

		scene->with([](){
			render_geometry();
		});
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());
		RenderTarget::clear(Color::magenta);
		scene->draw(passthru, glm::vec2(0,0), glm::vec2(resolution));
	}

	void render(){
		render_scene();
		render_blit();
	}

	void update(float t, float dt){
		const float s = t*0.2f;
		const float d = 7.5f;

		cam.look_at(glm::vec3(0,0,0));
		cam.set_position(glm::vec3(cos(s)*d, 2.5f, sin(s)*d));
	}
}
