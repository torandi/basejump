#ifndef GAME_CPP
#define GAME_CPP

#include "aabb.hpp"
#include "lights_data.hpp"
#include "techniques/hdr.hpp"
#include "techniques/temporalblur.hpp"

#include <string>

#pragma managed(push,off)
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#pragma managed(pop)

#include "Protagonist.hpp"

class Game {
	public:
		Game(const std::string &level, float near, float far, float fov);
		~Game();

		void update(float t, float dt);

		void render();

		void setup();

		void start();
		void restart();
		void die();

	private:
		btDefaultCollisionConfiguration * collisionConfiguration;
        btCollisionDispatcher * dispatcher;
        btBroadphaseInterface * broadphase;
        btConstraintSolver * solver;
        btDynamicsWorld * dynamicsWorld;

		void initPhysics();
		void cleanupPhysics();

		void render_blit();
		void render_scene();

		void run_particles(float dt);

		LightsData lights;

		Camera camera;
		RenderTarget *scene;

		AABB scene_aabb;
		Technique::HDR hdr;
		Technique::TemporalBlur temporal;
		Texture2D * blood, *menu;
		Shader * shader_blood, *shader_passthru;
		Sky * sky;

		float dead_time_left;
		float dead_time;

		GLuint u_dead_step;

		Terrain * terrain;
		TextureArray * particle_textures;

		Color sky_color;
		Shader::fog_t fog;

		Controller* controller;
		Protagonist * protagonist;
		ParticleSystem * particles;

		float particle_spawn_far;
		float particle_keep_far;

		Sound* wind_sound;
		Sound* strong_wind_sound;
		Sound * death;

		enum {
			STATE_MENU,
			STATE_DEAD,
			STATE_GAME
		} state;
};

#endif
