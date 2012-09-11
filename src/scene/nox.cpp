#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glm/gtc/type_ptr.hpp>

#include "quad.hpp"
#include "scene.hpp"
#include "globals.hpp"
#include "light.hpp"
#include "render_object.hpp"
#include "particle_system.hpp"
#include "shader.hpp"
#include "timetable.hpp"
#include "skybox.hpp"

#define HOLOGRAM_SCALE 2.f
#define HOLOGRAM_FRAMERATE 30
#define HOLOGRAM_FRAMES 900

class NOX: public Scene {
public:
	NOX(const glm::ivec2& size)
		: Scene(size)
		, tunnel("/models/nox2/tunnel.obj", false)
		, logo("/models/nox.obj", false)
		, geometry(size, GL_RGB8, RenderTarget::DEPTH_BUFFER)
		, camera(75.f, size.x/(float)size.y, 0.1f, 100.0f)
		, cam_pos1("/src/scene/nox_cam1.txt")
		, cam_pos2("/src/scene/nox_cam2.txt")
		, light_pos("/src/scene/nox_extra_light.txt")
		, skybox("/textures/skydark")
		, normal_shader(nullptr)
		, water_shader(nullptr)
		, water_quad(glm::vec2(10.f, 10.0f), true, true)
		, water_texture(Texture2D::from_filename("/textures/water.png"))
		, particle_shader(nullptr)
		, fog(10000, TextureArray::from_filename("/textures/fog.png", nullptr))
		, video(glm::vec2(1.f), true, true)
	{

		logo.set_scale(0.1f);
		logo.set_rotation(glm::vec3(0,1,0), 90.0f);
		logo.set_position(glm::vec3(-30,0.3,0));

		normal_shader = Shader::create_shader("/shaders/normal");

		water_shader = Shader::create_shader("/shaders/water");
		water_quad.set_position(glm::vec3(-100.f, -0.6f, -50.f));
		water_quad.set_rotation(glm::vec3(1.f, 0, 0), 90.f);
		water_quad.set_scale(100.f);

		water_texture->texture_bind(Shader::TEXTURE_2D_0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
		water_texture->texture_unbind();

		u_wave1 = water_shader->uniform_location("wave1");
		u_wave2 = water_shader->uniform_location("wave2");

		wave1 = glm::vec2(0.01, 0);
		wave2 = glm::vec2(0.005, 0.03);

		particle_shader = Shader::create_shader("/shaders/particles");

		lights.ambient_intensity() = glm::vec3(0.01f);
		lights.num_lights() = 2;

		lights.lights[0]->set_position(glm::vec3(-5.5f, 0.4f, 0.0f));
		lights.lights[0]->intensity = glm::vec3(0.0f, 0.6f, 0.4f);
		lights.lights[0]->type = MovableLight::POINT_LIGHT;
		lights.lights[0]->constant_attenuation = 0.0f;
		lights.lights[0]->linear_attenuation = 0.1f;
		lights.lights[0]->quadratic_attenuation = 0.4f;

		lights.lights[1]->set_position(glm::vec3(-2.0f, 1.0f, 0.0f));
		lights.lights[1]->intensity = glm::vec3(0.3f, 0.6f, 0.8f);
		lights.lights[1]->type = MovableLight::POINT_LIGHT;

		lights.lights[2]->type = MovableLight::POINT_LIGHT;


		fog.avg_spawn_rate = 50000.f;
		fog.spawn_rate_var = 0.f;

		fog.config.spawn_position = glm::vec4(-50.f, -1.f, -30.f, 1.f);
	//	fog.config.spawn_position = glm::vec4(0.f, 0.f, 0.f, 1.f);
		fog.config.spawn_area = glm::vec4(50.0f, 0.f, 60.0f, 0.0f);
		fog.config.avg_spawn_velocity = glm::vec4(0, 0.f, 0.f, 1.f);
		fog.config.spawn_velocity_var = glm::vec4(0.3f, 0.3f, 0.3f, 0.f);
		fog.config.avg_ttl = 20.f;
		fog.config.ttl_var = 10.f;
		fog.config.avg_scale = 15.f;
		fog.config.scale_var = 5.f;
		fog.config.avg_scale_change = 0.5f;
		fog.config.scale_change_var = 0.1f;
		fog.config.avg_rotation_speed = 0.02f;
		fog.config.rotation_speed_var = 0.005f;
		fog.config.birth_color = glm::vec4(0.3f, 0.3f, 0.3f, 0.05);
		fog.config.death_color = glm::vec4(0.3f ,0.3f, 0.3f, 0.0f);
		fog.config.motion_rand = glm::vec4(0.001f, 0.f, 0.001f, 0);
		fog.update_config();

		hologram_shader = Shader::create_shader("hologram");
		u_video_index = hologram_shader->uniform_location("texture_index");
		video_index = 0;

		std::vector<std::string> frames;
		char buffer[128];
		for(int i=1;i<=HOLOGRAM_FRAMES; ++i) {
			snprintf(buffer, 128, "/textures/nox2/videos/entries%d.png", i);
			frames.push_back(std::string(buffer));
		}

		hologram = TextureArray::from_filename(frames, false);


		video.set_rotation(glm::vec3(0, 1.f, 0), 45.f);
	}

	virtual void render_geometry(const Camera& cam){
		clear(Color::black);
		Shader::upload_lights(lights);

		/* skybox.render(cam); */

		Shader::upload_camera(cam);
		normal_shader->bind();

		tunnel.render();
		logo.render();



		water_texture->texture_bind(Shader::TEXTURE_NORMALMAP);
		skybox.texture->texture_bind(Shader::TEXTURE_CUBEMAP_0);
		geometry.texture_bind(Shader::TEXTURE_2D_3);
		glActiveTexture(Shader::TEXTURE_2D_2);
		glBindTexture(GL_TEXTURE_2D, geometry.depthbuffer());

		water_shader->bind();
		{
			glUniform2fv(u_wave1, 1, glm::value_ptr(wave1));
			glUniform2fv(u_wave2, 1, glm::value_ptr(wave2));
			water_quad.render();
		}

		particle_shader->bind();
		fog.render();


		const float t = global_time.get();

		if( t  > 64 && t < 96) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
			hologram_shader->bind();
			glUniform1i(u_video_index, video_index);

			hologram->texture_bind(Shader::TEXTURE_ARRAY_0);

			video.render();

			glPopAttrib();
		}

	}

	virtual const Camera& get_current_camera(){
		return camera;
	}

	virtual void update(float t, float dt){
		camera.set_position(cam_pos1.at(t));

		camera.look_at(cam_pos2.at(t));


		fog.update(dt);


		//Extra light
		if(t > 30 && t < 40) {
			float s = (t-30.f)/10.f;
			lights.lights[2]->intensity = glm::vec3(0.1f, 0.3f, 0.2f)*s;
		} else if( t > 50 && t < 60) {
			float s = (1.f-(t-50.f)/10.f);
			lights.lights[2]->intensity = glm::vec3(0.1f, 0.3f, 0.2f)*s;
		}

		if(t > 30 && t < 60) {
			lights.num_lights() = 3;
			lights.lights[2]->set_position(light_pos.at(t));
		} else if( t > 60) {
			lights.num_lights() = 2;
		}

		//Hologram
		if( t > 65 && t < 95) {
			const float s = (t - 65);
			video_index = s*HOLOGRAM_FRAMERATE;
		}
		if( t > 64 && t < 65) {
			const float s = (t - 64);
			video.set_scale(glm::vec3(HOLOGRAM_SCALE, HOLOGRAM_SCALE*s, HOLOGRAM_SCALE));
			video.set_position(glm::vec3(-29.59,-(HOLOGRAM_SCALE/2.f)*s,3.10));
		} else if(t > 74.5 && t < 75) {
			const float s = 1.f - (t - 74.5)*2.f;
			video.set_scale(glm::vec3(HOLOGRAM_SCALE, HOLOGRAM_SCALE*s, HOLOGRAM_SCALE));
			video.set_position(glm::vec3(-29.59,-(HOLOGRAM_SCALE/2.f)*s,3.10));
		} else if ( t > 75 && t < 75.5) {
			const float s = (t - 75.f)*2.f;
			video.set_scale(glm::vec3(HOLOGRAM_SCALE, HOLOGRAM_SCALE*s, HOLOGRAM_SCALE));
			video.set_position(glm::vec3(-29.59,-(HOLOGRAM_SCALE/2.f)*s,3.10));

		} else if(t > 84.5 && t < 85) {
			const float s = 1.f - (t - 84.5)*2.f;
			video.set_scale(glm::vec3(HOLOGRAM_SCALE, HOLOGRAM_SCALE*s, HOLOGRAM_SCALE));
			video.set_position(glm::vec3(-29.59,-(HOLOGRAM_SCALE/2.f)*s,3.10));
		} else if ( t > 85 && t < 85.5) {
			const float s = (t - 85)*2.f;
			video.set_scale(glm::vec3(HOLOGRAM_SCALE, HOLOGRAM_SCALE*s, HOLOGRAM_SCALE));
			video.set_position(glm::vec3(-29.59,-(HOLOGRAM_SCALE/2.f)*s,3.10));

		} else if ( t > 95 && t < 96) {
			const float s = 1.f - (t - 95);
			video.set_scale(glm::vec3(HOLOGRAM_SCALE, HOLOGRAM_SCALE*s, HOLOGRAM_SCALE));
			video.set_position(glm::vec3(-29.59,-(HOLOGRAM_SCALE/2.f)*s,3.10));
		}
	}

	void render_scene(){
		if ( !match ) return;

		geometry.bind();
		geometry.clear(Color::black);
		normal_shader->bind();
		Shader::upload_camera(get_current_camera());
		tunnel.render();
		logo.render();
		geometry.unbind();

		with(std::bind(&Scene::render, this));
	}

	RenderObject tunnel;
	RenderObject logo;
	RenderTarget geometry;
	Camera camera;

	PointTable cam_pos1;
	PointTable cam_pos2;
	PointTable light_pos;
	Skybox skybox;

	Shader* normal_shader;
	Shader* water_shader;
	Quad water_quad;
	Texture2D* water_texture;
	glm::vec2 wave1, wave2;
	int video_index;
	GLint u_wave1, u_wave2, u_video_index;
	Shader* particle_shader;
	ParticleSystem fog;
	TextureArray * hologram;
	Quad video;
	Shader * hologram_shader;
};

template <>
SceneFactory::Metadata* SceneTraits<NOX>::metadata(){
	SceneFactory::Metadata* _ = new SceneFactory::Metadata;
	SceneFactory::Metadata& m = *_;
	m["Camera 1"]   = new MetaPath("nox_cam1.txt");
	m["Tunnel"]     = new MetaModel("nox2/tunnel.obj");
	m["Logo"]       = new MetaModel("nox.obj");
	m["Light[0]"]   = new MetaLight<NOX>(&NOX::lights, 0, "tv_light0.txt");
	m["Light[1]"]   = new MetaLight<NOX>(&NOX::lights, 1, "tv_light1.txt");
	m["Light[2]"]   = new MetaLight<NOX>(&NOX::lights, 2, "nox_extra_light.txt");
	//m["fog"]				= new MetaParticles<NOX>(&NOX::fog
	return _;
}

REGISTER_SCENE_TYPE(NOX, "NördtroXy II", "/src/scene/nox.meta");
