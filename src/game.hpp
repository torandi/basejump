#ifndef GAME_CPP
#define GAME_CPP

#include "aabb.hpp"
#include "lights_data.hpp"
#include "techniques/hdr.hpp"
#include "techniques/dof.hpp"

#include <string>

#pragma managed(push,off)
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#pragma managed(pop)

#include "GLDebugDrawer.hpp"
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

	private:
		btDefaultCollisionConfiguration * collisionConfiguration;
        btCollisionDispatcher * dispatcher;
        btBroadphaseInterface * broadphase;
        btConstraintSolver * solver;
        btDynamicsWorld * dynamicsWorld;

		GLDebugDrawer * glDebugDrawer;

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
		//Technique::DoF dof;
		Shader * shader_normal, *shader_passthru;
		Sky * sky;

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

		enum {
			STATE_MENU,
			STATE_GAME
		} state;
};

#endif
