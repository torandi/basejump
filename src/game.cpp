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
#include "sound.hpp"
#include "particle_system.hpp"
#include "Prng.hpp"
#include "utils.hpp"
#include "logging.hpp"

#include "Controller.hpp"

#ifdef WIN32
#include "Kinect.hpp"
#endif

Game::Game(const std::string &level, float near, float far, float fov)
	: camera(fov, (float)resolution.x/(float)resolution.y, near, far)
	, hdr(resolution, /* exposure = */ 2.0f, /* bright_max = */ 3.6f, /* bloom_amount = */ 2.0f)
	//, dof(resolution, 1, GL_RGBA32F)
	, controller(nullptr)
{
	scene = new RenderTarget(resolution, GL_RGBA32F, RenderTarget::DEPTH_BUFFER);
	//shader_normal = Shader::create_shader("/shaders/normal");
	shader_passthru = Shader::create_shader("/shaders/passthru");

	printf("Loading level %s\n", level.c_str());

	std::string base_dir = std::string(srcdir) + "/basejump/levels/" + level;

	Data::add_search_path(srcdir "/basejump");
	Data::add_search_path(base_dir);

	Config config = Config::parse("/level.cfg");
	terrain = new Terrain("/terrain.cfg");

	std::vector<std::string> texture_paths;
	for(const ConfigEntry * entry : config["/particles/textures"]->as_list()) {
		texture_paths.push_back(entry->as_string());
	}
	
	fog.density = config["/environment/fog/density"]->as_float();

	lights.num_lights() = 1;

	Input::movement_speed = 10.f;

	scene_aabb = terrain->aabb();

	initPhysics();

	wind_sound = new Sound("/sound/34338__erh__wind.wav",1);

	particle_textures = TextureArray::from_filename(texture_paths);

	particles = new ParticleSystem(config["/particles/count"]->as_int(), scene_aabb, particle_textures);
	particles->read_config(config["/particles"]);

	particle_spawn_far = glm::min(config["/particles/spawn_far"]->as_float(), far);
	particle_keep_far = glm::min(config["/particles/keep_far"]->as_float(), far);

	particles->update_config();

	//Check for different controllers and init them if found
#ifdef WIN32
	controller = new Kinect();
#endif
	//TODO: check controller->active() and if false try other controllers

	setup();

	state = STATE_GAME;
}

void Game::setup() {
	char seed[16];
	for(int i=0; i<16; ++i) {
		seed[i] = static_cast<char>(static_cast<float>(time(0)) * frand()) % 256 + i;
	}

	Prng prng(seed);

	float time = 0.1f + static_cast<float>(prng.random()) * 0.8f;

	sky = new Sky("/sky.cfg", time);

	lights.ambient_intensity() = sky->ambient_intensity();
	sky->configure_light(lights.lights[0]);

	glm::vec3 pos = glm::vec3(terrain->horizontal_size()/2.f, 32.f, terrain->horizontal_size()/2.f);
	pos.y = terrain->height_at(pos.x, pos.z) + 600.f;

	protagonist = new Protagonist(pos);
	dynamicsWorld->addRigidBody(protagonist->rigidBody);
	protagonist->syncTransform(&camera);
}

void Game::start() {
	wind_sound->play();
}

void Game::restart() {
	delete sky;
	dynamicsWorld->removeRigidBody(protagonist->rigidBody);
	delete protagonist;

	/* clear particles */
	particles->set_bounds(AABB());
	particles->update_config();

	particles->update(0.1f);

	setup();
	start();
}

void Game::initPhysics()
{
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0,-9.82f,0));
}


Game::~Game() {
	cleanupPhysics();

	delete protagonist;

	delete scene;
	delete sky;
	delete terrain;
	if(controller != nullptr){
		delete controller;
	}

	delete particles;
	delete particle_textures;

	if(wind_sound->is_playing())
		wind_sound->stop();
	delete wind_sound;
}


void Game::cleanupPhysics()
{
	dynamicsWorld->removeRigidBody(protagonist->rigidBody);
	
	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;
}


void Game::render_scene(){
	Shader::upload_model_matrix(glm::mat4());
	/*
	lights.lights[0]->render_shadow_map(camera, scene_aabb, [&](const AABB &aabb) -> void  {
			terrain->render_geometry_cull(camera, aabb);

			protagonist->draw();
	});*/
	Shader::upload_model_matrix(glm::mat4());


	Shader::upload_lights(lights);

	Shader::upload_fog(fog);

	scene->with([&](){
			RenderTarget::clear(Color::black);
			sky->render(camera);
			Shader::upload_camera(camera);

			/* Render scene here */
			terrain->render_cull(camera);

			//protagonist->draw();

			particles->render();
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

void Game::run_particles(float dt) {
	AABB cam_aabb = camera.aabb(camera.near(), particle_spawn_far);
	AABB cam_bounds = camera.aabb(camera.near(), particle_keep_far);

	cam_aabb.min.y = scene_aabb.min.y;
	cam_bounds.min.y = scene_aabb.min.y;

	particles->config.spawn_position = glm::vec4(cam_aabb.min, 1.f);
	particles->config.spawn_area = glm::vec4(cam_aabb.max - cam_aabb.min , 1.f);
	particles->set_bounds(cam_bounds);
	particles->update_config();

	particles->update(dt);

}

void Game::update(float t, float dt) {
	/* Update game logic */

	dynamicsWorld->stepSimulation(dt, 30, 1.f/300.f);
	protagonist->update();

	glm::vec3 body_pos = protagonist->position();

	if(terrain->height_at(body_pos.x, body_pos.z) > body_pos.y) {
		restart();
	}

	camera.set_matrix(protagonist->matrix());

	run_particles(dt);

	//Check Controller for input
	//We should probably not need to do this check, as the init should fail if no controller can be found
	//(or simply fall back on keyboard and mouse)
	if(controller != nullptr && controller->active()){
		controller->update_object(*protagonist, dt);
	} else {
		input.update_object(*protagonist, dt);
	}

	//Debug stuff
//	input.update_object(camera, dt);
	
	if(input.current_value(Input::ACTION_0) > 0.9f) {
		protagonist->activateThruster();
	}

	if(input.down(Input::ACTION_3)) {
		restart();
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
