#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "game.hpp"

#include <GL/glew.h>
#include <SDL/SDL.h>
#include "globals.hpp"
#include "camera.hpp"
#include "rendertarget.hpp"
#include "shader.hpp"
#include "config.hpp"
#include "terrain.hpp"
#include "engine.hpp"
#include "data.hpp"

Game::Game(const std::string &level, float near, float far, float fov)
	: camera(fov, (float)resolution.x/(float)resolution.y, near, far)
	,	hdr(resolution, /* exposure = */ 1.8f, /* bright_max = */ 1.6f, /* bloom_amount = */ 1.0f)
{
	scene = new RenderTarget(resolution, GL_RGBA32F, RenderTarget::DEPTH_BUFFER);
	shader_normal = Shader::create_shader("/shaders/normal");
	shader_passthru = Shader::create_shader("/shaders/passthru");

	printf("Loading level %s\n", level.c_str());

	std::string base_dir = std::string(srcdir) + "/basejump/levels/" + level;

	Data::add_search_path(srcdir "/basejump");
	Data::add_search_path(base_dir);

	Config config = Config::parse("/level.cfg");
	terrain = new Terrain("/terrain.cfg");

	lights.ambient_intensity() = config["/environment/light/ambient"]->as_vec3();
	lights.num_lights() = 1;

	lights.lights[0]->set_position(glm::normalize(glm::vec3(0.f) - config["/environment/light/sun_position"]->as_vec3())); /* subtract to get direction from sun position */
	lights.lights[0]->intensity = config["/environment/light/sunlight"]->as_vec3();
	lights.lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;

	sky_color = config["/environment/sky_color"]->as_color();

	fog.color = config["/environment/fog/color"]->as_color().to_vec4();
	fog.density = config["/environment/fog/density"]->as_float();

	//TODO: Remove debug hack
	camera.set_position(glm::vec3(100.f, 32.f, 100.f));
	camera.look_at(glm::vec3(100.f, 32.f, 102.f));

	Input::movement_speed = 32.f;
}

Game::~Game() {
	delete scene;
}

void Game::render_geometry() {
	/* This should render all geometry in the scene for the shadow map */
	terrain->render_geometry();
}

void Game::render_scene(){
	Shader::upload_model_matrix(glm::mat4());

	lights.lights[0]->render_shadow_map(camera, scene_aabb, [&](const glm::mat4 &m) -> void  {
			render_geometry();
	});

	Shader::upload_camera(camera);
	Shader::upload_lights(lights);

	Shader::upload_fog(fog);

	scene->with([&](){
			RenderTarget::clear(sky_color);
			/* Render scene here */
			terrain->render();
	});
}

void Game::render_blit(){
	Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
	Shader::upload_model_matrix(glm::mat4());

	hdr.render(scene);

	RenderTarget::clear(Color::magenta);
	hdr.draw(shader_passthru, glm::vec2(0,0), glm::vec2(resolution));
}

void Game::render(){
	render_scene();
	render_blit();
}

static void print_values(const Technique::HDR &hdr) {
	printf("Exposure: %f\n, bloom: %f\n, bright_max: %f\n", hdr.exposure(), hdr.bloom_factor(), hdr.bright_max());
}

void Game::update(float t, float dt) {
	/* Update game logic */


	//Debug stuff
	input.update_object(camera, dt);

	//Update hdr
	if(input.down(Input::ACTION_0)) {
		hdr.set_exposure(hdr.exposure() - 0.1f);
		print_values(hdr);
	}
	if(input.down(Input::ACTION_1)) {
		hdr.set_exposure(hdr.exposure() + 0.1f);
		print_values(hdr);
	}

	if(input.down(Input::ACTION_2)) {
		hdr.set_bright_max(hdr.bright_max() - 0.1f);
		print_values(hdr);
	}
	if(input.down(Input::ACTION_3)) {
		hdr.set_bright_max(hdr.bright_max() + 0.1f);
		print_values(hdr);
	}

	if(input.down(Input::ACTION_4)) {
		hdr.set_bloom_factor(hdr.bloom_factor() - 0.1f);
		print_values(hdr);
	}
	if(input.down(Input::ACTION_5)) {
		hdr.set_bloom_factor(hdr.bloom_factor() + 0.1f);
		print_values(hdr);
	}
}
