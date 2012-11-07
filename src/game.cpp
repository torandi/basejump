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

Game::Game(const std::string &level, float near, float far, float fov)
	: camera(fov, (float)resolution.x/(float)resolution.y, near, far)
	,	hdr(resolution, /* exposure = */ 1.8f, /* bright_max = */ 1.6f, /* bloom_amount = */ 1.0f)
{
	scene = new RenderTarget(resolution, GL_RGBA32F, RenderTarget::DEPTH_BUFFER);
	shader_normal = Shader::create_shader("/shaders/normal");
	shader_passthru = Shader::create_shader("/shaders/passthru");

	printf("Loading level %s\n", level.c_str());

	std::string base_dir = "/levels/" + level;

	Config config = Config::parse(base_dir + "/level.cfg");

	lights.ambient_intensity() = config["/environment/light/ambient"]->as_vec3();
	lights.num_lights() = 1;

	lights.lights[0]->set_position(glm::normalize(glm::vec3(0.f) - config["/environment/light/sun_position"]->as_vec3())); /* subtract to get direction from sun position */
	lights.lights[0]->intensity = config["/environment/light/sunlight"]->as_vec3();
	lights.lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;

	sky_color = config["/environment/sky_color"]->as_color();

	fog.color = config["/environment/fog/color"]->as_color().to_vec4();
	fog.density = config["/environment/fog/density"]->as_float();
	
}

Game::~Game() {
	delete scene;
}

void Game::render_geometry() {
	/* This should render all geometry in the scene for the shadow map */
}

void Game::render_scene(){
	Shader::upload_model_matrix(glm::mat4());

	lights.lights[0]->render_shadow_map(camera, scene_aabb, [&](const glm::mat4 &m) -> void  {
			render_geometry();
	});

	Shader::upload_camera(camera);
	Shader::upload_lights(lights);

	scene->with([](){
			RenderTarget::clear(Color::green);
			/* Render scene here */
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

void Game::update(float t, float dt) {
	/* Update game logic */
}
