#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shader.hpp"
#include "scene.hpp"
#include "globals.hpp"
#include "particle_system.hpp"
#include "quad.hpp"
#include "texture.hpp"
#include "terrain.hpp"

static const Color skycolor = Color::rgb(149.0f / 255.0f, 178.0f / 255.0f, 178.0f / 255.0f);

class WinterScene : public Scene {
public:
	WinterScene (const glm::ivec2 &size)
		: Scene(size)
		, camera(75.f, size.x/(float)size.y, 0.1f, 100.f)
		, particle_shader(nullptr)
		, snow(500000, TextureArray::from_filename("snow1.png", nullptr))
	 {
			camera.set_position(glm::vec3(35.750710, 17.926385, 6.305542));
			camera.look_at(glm::vec3(35.750710, 17.926385, 7.305542));
			terrain_shader = Shader::create_shader("terrain");
			TextureArray * color = TextureArray::from_filename("dirt.png","grass.png", nullptr);
			color->texture_bind(Shader::TEXTURE_ARRAY_0);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
			//TextureArray * normal = TextureArray::from_filename("dirt_normal.png","grass_normal.png");
			TextureArray * normal = TextureArray::from_filename("default_normalmap.jpg","default_normalmap.jpg", nullptr);
			normal->texture_bind(Shader::TEXTURE_ARRAY_0);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
			terrain = new Terrain("/textures/park_map.png", 1.f, 20.f, color, normal);
			//terrain->absolute_move(glm::vec3(0.f, -10.f, 0.f));

			lights.ambient_intensity() = glm::vec3(0.0f);
			lights.num_lights() = 1;
			lights.lights[0]->set_position(glm::vec3(10, 50.f, 10.f));
			lights.lights[0]->intensity = glm::vec3(0.8f);
			lights.lights[0]->type = Light::POINT_LIGHT;
			lights.lights[0]->quadratic_attenuation = 0.00002f;

			particle_shader = Shader::create_shader("/shaders/particles");
			snow.avg_spawn_rate = 500000.f;
			snow.spawn_rate_var = 100000.f;

			snow.config.spawn_position = glm::vec4(-100.f, 0.f, -100.f, 1.f);
			snow.config.spawn_area = glm::vec4(200.0f, 20.f, 200.0f, 0.0f);
      snow.config.avg_spawn_velocity = glm::vec4(0, -1.f, 0.f, 1.f);
      snow.config.spawn_velocity_var = glm::vec4(0.3f, 0.0f, 0.3f, 0.f);
			snow.config.avg_ttl = 10.f;
			snow.config.ttl_var = 5.f;
			snow.config.avg_scale = 1.0f;
			snow.config.scale_var = 0.3f;
			snow.config.avg_scale_change = 0.05f;
			snow.config.scale_change_var = 0.01f;
			snow.config.avg_rotation_speed = 0.004f;
			snow.config.rotation_speed_var = 0.001f;
			snow.config.birth_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.8);
			snow.config.death_color = glm::vec4(1.0f ,1.0f, 1.0f, 0.6f);
			snow.config.motion_rand = glm::vec4(0.001f, 0.f, 0.001f, 0);
			snow.config.wind_velocity = glm::vec4(0.1f, 0.f, .1f, 0.f);
			snow.update_config();
			snow.update(1.f);
			snow.update(1.f);
			snow.update(1.f);

			snow.avg_spawn_rate = 50000.f;
			snow.spawn_rate_var = 20000.f;
			snow.config.spawn_position.y = 20.f;
			snow.config.spawn_area.y = 0.f;
			/*snow.avg_spawn_rate = 30000.f;
			snow.spawn_rate_var = 10000.f;*/
			snow.update_config();
	}

	virtual void render_geometry(const Camera& cam){
		terrain_shader->bind();
		Shader::upload_lights(lights);
		Shader::upload_camera(cam);
		terrain->render();
	}

	virtual void render(){
		clear(skycolor);
		glDisable(GL_CULL_FACE);
		render_geometry(camera);
		particle_shader->bind();
		snow.render();
		Shader::unbind();
	}

	virtual const Camera& get_current_camera(){
		return camera;
	}

	virtual void update(float t, float dt){
		snow.update(dt);
		#ifdef ENABLE_INPUT
			Input::movement_speed = 5.f;
			input.update_object(camera, dt);
			if(input.current_value(Input::ACTION_1) > 0.5f) {
				printf("Current position: (%f, %f, %f)\n", camera.position().x, camera.position().y, camera.position().z);
			}
		#endif
	}

private:
	Camera camera;
	Shader * terrain_shader;
	Terrain * terrain;
	Shader* particle_shader;
	ParticleSystem snow;
};

REGISTER_SCENE_TYPE(WinterScene, "Winter", "winter.meta");
