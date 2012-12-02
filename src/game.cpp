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

static Quad * fullscreen_quad;

Game::Game(const std::string &level, float near, float far, float fov)
	: camera(fov, (float)resolution.x/(float)resolution.y, near, far)
	, hdr(resolution, /* exposure = */ 1.8f, /* bright_max = */ 5.0f, /* bloom_amount = */ 2.0f)
	, temporal(resolution, 0.2)
	, controller(nullptr)
{
	scene = new RenderTarget(resolution, GL_RGBA32F, RenderTarget::DEPTH_BUFFER);
	shader_blood = Shader::create_shader("/shaders/blood");
	shader_passthru = Shader::create_shader("/shaders/passthru");

	u_dead_step = shader_blood->uniform_location("step");

	printf("Loading level %s\n", level.c_str());

	fullscreen_quad = new Quad(glm::vec2(1.f, -1.f), false);
	fullscreen_quad->set_scale(glm::vec3(resolution, 1));

	std::string base_dir = std::string(srcdir) + "/basejump/levels/" + level;

	Data::add_search_path(srcdir "/basejump");
	Data::add_search_path(base_dir);

	Config config = Config::parse("/level.cfg");

	blood = Texture2D::from_filename("/blood.jpg");
	menu = Texture2D::from_filename("/menu.jpg");

	dead_time = config["/dead_time"]->as_float();
	dead_time_left = 0.f;

	temporal.set_factor(config["/environment/temporal_amount"]->as_float());

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

	state = STATE_MENU;
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
	pos = glm::vec3(0.f);
	pos.y = terrain->height_at(pos.x, pos.z) + 600.f;

	protagonist = new Protagonist(pos);
	dynamicsWorld->addRigidBody(protagonist->rigidBody);
	protagonist->syncTransform(&camera);

	death = new Sound("/sound/death.wav",2);

	wind_sound = new Sound("/sound/wind_medium.mp3",5);
	strong_wind_sound = new Sound("/sound/wind_strong.mp3",100);
	strong_wind_sound->set_volume(0);
}

void Game::start() {
	wind_sound->play();
	strong_wind_sound->play();
	state = STATE_GAME;
}

void Game::restart() {
	delete sky;
	dynamicsWorld->removeRigidBody(protagonist->rigidBody);
	delete protagonist;
	delete wind_sound;
	delete strong_wind_sound;
	delete death;

	/* clear particles */
	particles->set_bounds(AABB());
	particles->update_config();

	particles->update(0.1f);

	setup();
	start();
}

void Game::die() {
	state = STATE_DEAD;
	dead_time_left = dead_time;
	//TODO: Death sound
	wind_sound->stop();
	strong_wind_sound->stop();
	death->play();
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
	if(wind_sound->is_playing())
		wind_sound->stop();
	delete wind_sound;
	if(strong_wind_sound->is_playing())
		strong_wind_sound->stop();
	delete strong_wind_sound;
	if(death->is_playing())
		death->stop();
	delete death;
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

	if(state != STATE_MENU) {
		hdr.render(scene);
		temporal.render(&hdr);
	}

	RenderTarget::clear(Color::magenta);
	Shader * s = shader_passthru;
	switch(state) {
		case STATE_DEAD:
			s = shader_blood;
			blood->texture_bind(Shader::TEXTURE_2D_1);
			shader_blood->uniform_upload(u_dead_step, 1.f - (dead_time_left / dead_time));
		case STATE_GAME:
			temporal.draw(s, glm::vec2(0,0), glm::vec2(resolution));
			break;
		case STATE_MENU:
			shader_passthru->bind();
			menu->texture_bind(Shader::TEXTURE_COLORMAP);
			fullscreen_quad->render();
			break;
	}
}

void Game::render(){
	if(state != STATE_MENU) render_scene();
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
	switch(state) {
	case STATE_GAME:
		{
		dynamicsWorld->stepSimulation(dt, 10, 1.f/60.f);
		protagonist->update(dt);

		glm::vec3 body_pos = protagonist->position() + protagonist->local_z();

		if(terrain->height_at(body_pos.x, body_pos.z) > body_pos.y) {
			protagonist->set_position(protagonist->position() - protagonist->local_z());
			camera.set_matrix(protagonist->matrix());
			die();
		}

		camera.set_matrix(protagonist->matrix());

		run_particles(dt);

		if(controller != nullptr && controller->active()){
			controller->update_object(*protagonist, dt);
		} else {
			input.update_object(*protagonist, dt);
		}
	
		if(input.current_value(Input::ACTION_0) > 0.9f) {
			protagonist->activateThruster();
		}

		if(input.down(Input::ACTION_3)) {
			restart();
		}
	
		//Update sound

		//Set volume of strong_wind 
		float v = 30/(protagonist->position().y - terrain->height_at(protagonist->position().x, protagonist->position().z));
		if(v > 1){
			v = 1;
		}
		strong_wind_sound->set_volume(v);
	}
		break;
	case STATE_DEAD:
		dead_time_left -= dt;
		death->set_volume(glm::clamp(1.f - (dead_time_left /dead_time) + 0.2f, 0.f, 1.f));
		if(dead_time_left < 0) {
			if(death->is_playing())
			death->stop();
			state = STATE_MENU;
		}
		break;
	case STATE_MENU:
		if(input.down(Input::ACTION_0)) {
			restart();
		}
		break;
	}
}
