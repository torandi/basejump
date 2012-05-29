#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "scene.hpp"
#include "globals.hpp"
#include "shader.hpp"
#include "particle_system.hpp"

class ParticlesScene: public Scene {
public:
	ParticlesScene(const glm::ivec2 &size)
		: Scene(size)
		, fire(1000, Texture::array(3, PATH_BASE "textures/fire1.png", PATH_BASE "textures/fire2.png", PATH_BASE "textures/fire3.png"))
		, smoke(1000, Texture::array(1, PATH_BASE "textures/fog.png"), 20)
		, camera(75.f, size.x/(float)size.y, 0.1f, 100.f) {
			camera.set_position(glm::vec3(0.f, 0.f, -1));
			camera.look_at(glm::vec3(0.f, 0.f, 0.f));
			fire.config.spawn_position = glm::vec4(0.f, -0.5f, -0.2f, 1.f);
			fire.config.spawn_area = glm::vec4(0.4f, 0.f, 0.4f, 0.f);
			fire.config.spawn_direction = glm::vec4(0, 1.f, 0.f, 0.f);
			fire.config.direction_var = glm::vec4(0.1f, 0.f, 0.1f, 0.f);
			fire.config.avg_spawn_speed= 0.001f;
			fire.config.spawn_speed_var = 0.0005f;
			fire.config.avg_ttl = 7.f;
			fire.config.ttl_var = 2.5f;
			fire.config.avg_spawn_delay = 0.f;
			fire.config.spawn_delay_var = 0.f;
			fire.config.avg_scale = 0.4f;
			fire.config.scale_var = 0.05f;
			fire.config.avg_scale_change = 0.2f;
			fire.config.scale_change_var = 0.05f;
			fire.config.avg_rotation_speed = 0.01f;
			fire.config.rotation_speed_var = 0.01f;
			fire.config.birth_color = glm::vec4(1.0, 0.8, 0.0, 1.0);
			fire.config.death_color = glm::vec4(0.8 ,0.2, 0.1, 0.f);
			fire.config.motion_rand = glm::vec4(0.0, 0.f, 0.0, 0);
			fire.update_config();

			smoke.config.spawn_position = glm::vec4(0.f, -0.5f, -0.2f, 1.f);
			smoke.config.spawn_area = glm::vec4(0.4f, 0.f, 0.4f, 0.f);
			smoke.config.spawn_direction = glm::vec4(0, 1.f, 0.f, 0.f);
			smoke.config.direction_var = glm::vec4(0.1f, 0.f, 0.1f, 0.f);
			smoke.config.avg_spawn_speed= 0.003f;
			smoke.config.spawn_speed_var = 0.0005f;
			smoke.config.avg_ttl = 20.f;
			smoke.config.ttl_var = 5.f;
			smoke.config.avg_spawn_delay = 0.f;
			smoke.config.spawn_delay_var = 0.f;
			smoke.config.avg_scale = 0.6f;
			smoke.config.scale_var = 0.05f;
			smoke.config.avg_scale_change = 2.f;
			smoke.config.scale_change_var = 0.5f;
			smoke.config.avg_rotation_speed = 0.02f;
			smoke.config.rotation_speed_var = 0.005f;
			smoke.config.birth_color = glm::vec4(0.2, 0.2, 0.2, 0.5);
			smoke.config.death_color = glm::vec4(0.8 ,0.8, 0.8, 0.f);
			smoke.config.motion_rand = glm::vec4(0.001f, 0.f, 0.001f, 0);
			smoke.update_config();
	}

	virtual void render(){
		clear(Color::black);
		Shader::upload_camera(camera);

		shaders[SHADER_PARTICLES]->bind();
		{
			smoke.render();
			fire.render();
		}
		shaders[SHADER_PARTICLES]->unbind();

	}

	virtual void update(float t, float dt){
		fire.update(dt);
		smoke.update(dt);
	}

private:
	ParticleSystem fire, smoke;
	Camera camera;
};

REGISTER_SCENE_TYPE(ParticlesScene, "Particles");
