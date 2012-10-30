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
#include "globals.hpp"
#include "quad.hpp"
#include "config.hpp"
#include "particle_system.hpp"
#include <glm/glm.hpp>

static Shader* shader = nullptr;
static Shader* passthru = nullptr;
static Shader* tonemap = nullptr;
static Shader* bright_filter = nullptr;
static GLuint u_exposure, u_bloom_factor, u_bright_max[2], u_threshold, u_sun;

static float exposure = 1.0f;
static float bloom_factor = 0.45f;
static float bright_max = 1.20f;
static float bright_threshold = 1.00f;

static Shader* normal = nullptr;
static Shader* blend = nullptr;
static Shader* blendmix = nullptr;
static Shader* logoshader = nullptr;
static TextureArray* colormap = nullptr;
static TextureArray* normalmap = nullptr;
static Terrain* terrain = nullptr;
static RenderObject* obj = nullptr;
static RenderTarget* scene = nullptr;
static RenderTarget* logo = nullptr;
static RenderTarget* blendmap = nullptr;
static RenderTarget* ldr = nullptr;
static LightsData* lights = nullptr;
static Texture2D* crap = nullptr;
static Texture2D* white = nullptr;
static Quad* quad = nullptr;
static Quad* fsquad = nullptr;

static TextureArray *smoke_textures = nullptr;
static TextureArray *fire_textures = nullptr;
static ParticleSystem *smoke = nullptr;
static ParticleSystem *fire = nullptr;

static Camera cam(75, 1.3f, 0.1f, 100.0f);
extern glm::mat4 screen_ortho;   /* defined in main.cpp */
extern Time global_time;         /* defined in main.cpp */
extern glm::ivec2 resolution;    /* defined in main.cpp */

static RenderTarget* pass[3] = {nullptr, nullptr, nullptr};
static Shader* blur[2] = {nullptr, nullptr};

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		normal    = Shader::create_shader("/shaders/normal");
		passthru    = Shader::create_shader("/shaders/passthru");
		tonemap   = Shader::create_shader("/shaders/tonemap");
		bright_filter  = Shader::create_shader("/shaders/bright_filter");
		shader    = Shader::create_shader("/shaders/terrain");
		blend     = Shader::create_shader("/shaders/blend");
		blendmix  = Shader::create_shader("/nx15/blendmix");
		logoshader= Shader::create_shader("/nx15/logo");
		colormap  = TextureArray::from_filename("/nx15/color0.png", "/nx15/color1.png", nullptr);
		normalmap = TextureArray::from_filename("/nx15/normal0.png", "/nx15/normal1.png", nullptr);
		terrain   = new Terrain("/nx15/terrain.jpg", 30.0f, 4.0f, colormap, normalmap);
		scene     = new RenderTarget(resolution, GL_RGBA32F, RenderTarget::DEPTH_BUFFER | RenderTarget::DOUBLE_BUFFER, GL_LINEAR);
		logo      = new RenderTarget(resolution, GL_RGB8, GL_LINEAR);
		blendmap  = new RenderTarget(resolution, GL_RGBA8, GL_LINEAR);
		ldr       = new RenderTarget(resolution, GL_RGB8, GL_LINEAR);
		lights    = new LightsData();
		obj       = new RenderObject("/models/bench.obj", true);
		crap      = Texture2D::from_filename("/nx15/craptastic.png");
		white     = Texture2D::from_filename("/textures/white.jpg");

		pass[0]  = new RenderTarget(resolution, GL_RGB8, 0, GL_LINEAR);
		pass[1]  = new RenderTarget(resolution/2, GL_RGB8, 0, GL_LINEAR);
		pass[2]  = new RenderTarget(resolution/4, GL_RGB8, 0, GL_LINEAR);

		blur[0]  = Shader::create_shader("/shaders/blur_vertical");
		blur[1]  = Shader::create_shader("/shaders/blur_horizontal");

		terrain->set_position(glm::vec3(-7.5f, -2.0f,  -7.5f));
		lights->ambient_intensity() = glm::vec3(0.1f);
		lights->num_lights() = 1;
		lights->lights[0]->set_position(glm::normalize(glm::vec3(0, -0.5f, -1.0f)));
		lights->lights[0]->intensity = glm::vec3(0.8f);
		lights->lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;

		fire_textures = TextureArray::from_filename("/textures/fire1.png", "/textures/fire2.png", "/textures/fire3.png", nullptr);
		smoke_textures = TextureArray::from_filename("/nx15/smoke.png", nullptr);

		Config particle_config = Config::parse("/nx15/particles.cfg");

		smoke = new ParticleSystem(20000, smoke_textures);
		smoke->read_config(particle_config["/particles/smoke"]);
		smoke->update_config();

		u_exposure = tonemap->uniform_location("exposure");
		u_bloom_factor = tonemap->uniform_location("bloom_factor");
		u_bright_max[0] = tonemap->uniform_location("bright_max");
		u_bright_max[1] = bright_filter->uniform_location("bright_max");
		u_threshold = bright_filter->uniform_location("threshold");
		u_sun = blendmix->uniform_location("sun");

		/* setup logo */
		quad = new Quad();
		quad->set_scale(glm::vec3(resolution.x, resolution.x / 2, 1));
		quad->set_position(glm::vec3(0.0, (resolution.y - resolution.x / 2) / 2 , 0.0f));

		/* fullscreen quad */
		fsquad = new Quad();
		fsquad->set_scale(glm::vec3(resolution.x, resolution.y, 1));

		cam.set_position(glm::vec3(7.267048, 2.076503, 14.047379));
		cam.look_at(glm::vec3(9.275319, 1.763391, 14.40433));
	}

	void start(double seek){
	}

	void cleanup(){
		delete terrain;
		delete scene;
		delete pass[0];
		delete pass[1];
		delete pass[2];
		Shader::cleanup();
	}

	static void render_logo(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());

		RenderTarget::clear(Color::black);
		logoshader->bind();
		crap->texture_bind(Shader::TEXTURE_2D_0);
		quad->render();
	}

	static void render_geometry(){
		terrain->render();

		normal->bind();
		Shader::upload_blank_material();
		Shader::upload_projection_view_matrices(cam.projection_matrix(), cam.view_matrix());
		obj->render();
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

		scene->with([](){
			const glm::vec3 dir = glm::normalize(cam.position() - cam.look_at());
			const float dot = glm::dot(dir, lights->lights[0]->position());
			const float s = glm::clamp(dot*fabsf(dot), 0.0f, 1.0f);

			static Color sky = Color::from_hex("a0c8db");
			RenderTarget::clear(Color::lerp(sky, Color::white, s));
			render_geometry();
			smoke->render();
		});
	}

	static void render_bloom() {
		bright_filter->bind();
		glUniform1f(u_threshold, bright_threshold);
		glUniform1f(u_bright_max[1], bright_max);

		pass[0]->transfer(bright_filter, scene);
		pass[1]->transfer(blur[0], pass[0]);
		pass[2]->transfer(blur[1], pass[1]);
	}

	static void render_blit(){
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		Shader::upload_model_matrix(glm::mat4());
		RenderTarget::clear(Color::magenta);

		blendmap->with([](){
			RenderTarget::clear(Color::black);
			blendmix->bind();
			fsquad->render();
		});

		tonemap->bind();
		glUniform1f(u_exposure, exposure);
		glUniform1f(u_bloom_factor, bloom_factor);
		glUniform1f(u_bright_max[0], bright_max);

		pass[2]->texture_bind(Shader::TEXTURE_BLOOM);
		ldr->transfer(tonemap, scene);

		logo->texture_bind(Shader::TEXTURE_BLEND_1);
		white->texture_bind(Shader::TEXTURE_BLEND_2);
		blendmap->texture_bind(Shader::TEXTURE_BLEND_S);
		ldr->draw(blend, glm::vec2(0,0), glm::vec2(resolution));
	}

	void render(){
		float t = global_time.get();

		if ( t < 10.0f ){
			logo->with(render_logo);
		}

		render_logo();
		render_scene();
		render_bloom();
		render_blit();
	}

#ifdef ENABLE_INPUT
	void static print_values() {
		printf("Exposure: %f\n, bloom: %f\n, bright: [%f, %f]\n", exposure, bloom_factor, bright_max, bright_threshold);
		printf("Position: %f, %f, %f\n", cam.position().x, cam.position().y, cam.position().z) ;
	}
#endif

	void update(float t, float dt){
		smoke->update(dt);

#ifdef ENABLE_INPUT
		input.update_object(cam, dt);
		if(input.down(Input::ACTION_0)) {
			exposure -= 0.1;
			print_values();
		}
		if(input.down(Input::ACTION_1)) {
			exposure += 0.1;
			print_values();
		}

		if(input.down(Input::ACTION_2)) {
			bloom_factor -= 0.1;
			print_values();
		}
		if(input.down(Input::ACTION_3)) {
			bloom_factor += 0.1;
			print_values();
		}

		if(input.down(Input::ACTION_4)) {
			bright_max -= 0.1;
			print_values();
		}
		if(input.down(Input::ACTION_5)) {
			bright_max += 0.1;
			print_values();
		}

		if(input.down(Input::ACTION_6)) {
			bright_threshold -= 0.1;
			print_values();
		}
		if(input.down(Input::ACTION_7)) {
			bright_threshold += 0.1;
			print_values();
		}
#else
		const float s = t*0.2f;
		const float d = 7.5f;

		cam.look_at(glm::vec3(0,0,0));
		cam.set_position(glm::vec3(cos(s)*d, 2.5f, sin(s)*d));
#endif
	}
}
