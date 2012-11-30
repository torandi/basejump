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

#include "Controller.hpp"


#ifdef WIN32
#include "Kinect.hpp"
#endif

Game::Game(const std::string &level, float near, float far, float fov)
	: camera(fov, (float)resolution.x/(float)resolution.y, near, far)
	, hdr(resolution, /* exposure = */ 2.5f, /* bright_max = */ 3.6f, /* bloom_amount = */ 1.0f)
	, dof(resolution, 1, GL_RGBA32F)
	, controller(nullptr)
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

	std::vector<std::string> texture_paths;
	for(const ConfigEntry * entry : config["/particles/textures"]->as_list()) {
		texture_paths.push_back(entry->as_string());
	}


	lights.num_lights() = 1;
	lights.ambient_intensity() = sky->ambient_intensity();
	sky->configure_light(lights.lights[0]);

	fog.density = config["/environment/fog/density"]->as_float();

	//TODO: Remove debug hack
	camera.set_position(glm::vec3(terrain->horizontal_size()/2.f, 32.f, terrain->horizontal_size()/2.f-1000));

	glm::vec3 pos = camera.position();
	pos.y = terrain->height_at(pos.x, pos.z) + 900.f;

	camera.set_position(pos);
	//camera.look_at(camera.position() + glm::vec3(0.f, 0.f, 1.f));

	Input::movement_speed = 10.f;

	scene_aabb = terrain->aabb();


	initPhysics();
	protagonist = new Protagonist(pos);
	dynamicsWorld->addRigidBody(protagonist->rigidBody);

	wind_sound = new Sound("/sound/34338__erh__wind.wav",1);

	particle_textures = TextureArray::from_filename(texture_paths);

	particles = new ParticleSystem(config["/particles/count"]->as_int(), scene_aabb, particle_textures);
	particles->read_config(config["/particles"]);
	particles->update_config();

	//Prerun particles a bit:
	for(int i = 0; i < 100; ++i) {
		run_particles(0.1f);
	}

	//Check for different controllers and init them if found
#ifdef WIN32
	controller = new Kinect();
#endif
	//TODO: check controller->active() and if false try other controllers
}

void Game::start() {
	wind_sound->play();
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
	delete scene;
	delete sky;
	delete terrain;
	if(controller != nullptr){
		delete controller;
	}

	delete particles;
	delete particle_textures;

	dynamicsWorld->removeRigidBody(protagonist->rigidBody);
	delete protagonist;

	cleanupPhysics();

	if(wind_sound->is_playing())
		wind_sound->stop();
	delete wind_sound;
}


void Game::cleanupPhysics()
{
	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;
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

			protagonist->draw();

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

static void print_values(const Technique::HDR &hdr) {
	printf("Exposure: %f\n, bloom: %f\n, bright_max: %f\n", hdr.exposure(), hdr.bloom_factor(), hdr.bright_max());
}

void Game::run_particles(float dt) {
	AABB cam_aabb = camera.aabb(camera.near(), 300.f);
	AABB cam_bounds = camera.aabb();

	cam_aabb.min.y = scene_aabb.min.y;
	cam_bounds.min.y = scene_aabb.min.y;

	particles->config.spawn_position = glm::vec4(cam_aabb.min, 1.f);
	particles->config.spawn_area = glm::vec4(cam_aabb.max - cam_aabb.min , 1.f);
	//particles->config.spawn_position = glm::vec4(camera.position() + camera.local_z(), 1.f);
	//particles->config.spawn_area = glm::vec4(1.f, 1.f, 1.f, 0.f);
	particles->set_bounds(cam_bounds);
	particles->update_config();

	particles->update(dt);

}

void Game::update(float t, float dt) {
	/* Update game logic */

	dynamicsWorld->stepSimulation(1/60.f, 1);
	protagonist->update();

	// sync camera transform with protagonist transform
	camera.set_matrix(protagonist->matrix());

	run_particles(dt);

	//Check Controller for input
	//We should probably not need to do this check, as the init should fail if no controller can be found
	//(or simply fall back on keyboard and mouse)
	if(controller != nullptr && controller->active()){
		controller->update_object(*protagonist, dt);
	}

	//Debug stuff
//	input.update_object(camera, dt);
	input.update_object(*protagonist, dt);

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
