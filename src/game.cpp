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
#include "sky.hpp"
#include "quad.hpp"

Game::Game(const std::string &level, float near, float far, float fov)
	: camera(fov, (float)resolution.x/(float)resolution.y, near, far)
	,	hdr(resolution, /* exposure = */ 2.5f, /* bright_max = */ 3.6f, /* bloom_amount = */ 1.0f)
	, dof(resolution, 1, GL_RGBA32F)
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

	sky = new Sky("/sky.cfg", 0.5f);

	lights.num_lights() = 1;
	lights.ambient_intensity() = sky->ambient_intensity();
	sky->configure_light(lights.lights[0]);



	fog.density = config["/environment/fog/density"]->as_float();

	//TODO: Remove debug hack
	camera.set_position(glm::vec3(terrain->horizontal_size()/2.f, 32.f, terrain->horizontal_size()/2.f));

	glm::vec3 pos = camera.position();
	pos.y = terrain->height_at(pos.x, pos.z) + 2.f;
	camera.set_position(pos);
	camera.look_at(camera.position() + glm::vec3(0.f, 0.f, 1.f));

	Input::movement_speed = 10.f;

	scene_aabb = terrain->aabb();
}

Game::~Game() {
	delete scene;
	delete sky;
	delete terrain;
}

void Game::render_scene(){
	Shader::upload_model_matrix(glm::mat4());

	lights.lights[0]->render_shadow_map(camera, scene_aabb, [&](const AABB &aabb) -> void  {
			terrain->render_geometry_cull(camera, aabb);
	});
	Shader::upload_model_matrix(glm::mat4());


	Shader::upload_lights(lights);

	Shader::upload_fog(fog);

	scene->with([&](){
			RenderTarget::clear(Color::black);
			sky->render(camera);
			Shader::upload_camera(camera);

			/* Render scene here */
			terrain->render_cull(camera);
	});
}

void Game::render_blit(){
	Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
	Shader::upload_model_matrix(glm::mat4());

	//dof.render(scene);

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

	if(input.current_value(Input::ACTION_4) > 0.9) {
		sky->set_time_of_day(sky->time() + (dt / 10.f));
		sky->configure_light(lights.lights[0]);
	}
	if(input.current_value(Input::ACTION_5) > 0.9) {
		sky->set_time_of_day(sky->time() - (dt / 10.f));
		sky->configure_light(lights.lights[0]);
	}

	/*if(input.down(Input::ACTION_4)) {
		hdr.set_bloom_factor(hdr.bloom_factor() - 0.1f);
		print_values(hdr);
	}
	if(input.down(Input::ACTION_5)) {
		hdr.set_bloom_factor(hdr.bloom_factor() + 0.1f);
		print_values(hdr);
	}*/
}
