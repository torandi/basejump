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
#pragma managed(pop)

#include "Protagonist.hpp"



class Game {
	public:
		Game(const std::string &level, float near, float far, float fov);
		~Game();

		void update(float t, float dt);

		void render();

		void start();

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
		Technique::DoF dof;
		Shader * shader_normal, *shader_passthru;
		Sky * sky;

		Terrain * terrain;
		TextureArray * particle_textures;

		Color sky_color;
		Shader::fog_t fog;

		Controller* controller;
		Protagonist * protagonist;
		ParticleSystem * particles;

		Sound* wind_sound;
};

#endif
