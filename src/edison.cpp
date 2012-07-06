#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "engine.hpp"
#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "utils.hpp"
#include "scene.hpp"
#include "shader.hpp"
#include "time.hpp"
#include "cl.hpp"
#include "texture.hpp"
#include "timetable.hpp"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unistd.h>
#include <getopt.h>
#include <map>

#include "light.hpp"
#include "music.hpp"

static RenderTarget* composition = nullptr;
static RenderTarget* blend = nullptr;
static std::map<std::string, Scene*> scene;

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		const size_t offset = fullname.find_first_of(":");
		if ( offset == std::string::npos ){
			return nullptr;
		}

		const std::string prefix = fullname.substr(0, offset);
		const std::string name   = fullname.substr(offset+1);

		if ( prefix == "scene" ){
			auto it = scene.find(name);
			if ( it == scene.end() ) return nullptr;
			return it->second;
		}

		return nullptr;
	}

	void init(){
		scene["Winter"] = SceneFactory::create("Winter", glm::ivec2(resolution.x, resolution.y));
		composition = new RenderTarget(resolution,           GL_RGB8, false);
		blend = new RenderTarget(glm::ivec2(1,1), GL_RGBA8, false);

		load_timetable(PATH_SRC "edison.txt");
	}

	void start(double seek) {
/*		music->play();
		if(global_time.sync_to_music(music)) {
			fprintf(verbose, "Syncinc to music!\n");
		} else {
			printf("Warning! Syncing disabled!\n");
		}
		if(seek > 0.1) {
			music->seek(seek);
		}*/
	}

	void cleanup(){
		for ( std::pair<std::string,Scene*> p : scene ){
			delete p.second;
		}
		//delete music;
	}

	static void render_scene(){
		for ( std::pair<std::string,Scene*> p: scene ){
			p.second->render_scene();
		}
	}

	static void render_composition(){
		RenderTarget::clear(Color::black);

		Shader::upload_state(resolution);
		Shader::upload_projection_view_matrices(composition->ortho(), glm::mat4());
		glViewport(0, 0, resolution.x, resolution.y);

		blend->texture_bind(Shader::TEXTURE_BLEND_S);
		scene["grass"]->draw(shaders[SHADER_BLEND]);

		const float t = global_time.get();
	}

	static void render_display(){
		RenderTarget::clear(Color::magenta);
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
		composition->draw(shaders[SHADER_PASSTHRU]);
	}

	void render(){
		render_scene();

		const float t = global_time.get();
		float s = glm::min(t / 2.5f + 0.2f, 1.0f);
		if(t > 115) {
			s = 1.f - (t - 115.f)/5.f;
		}
		blend->with([s](){ RenderTarget::clear(Color(s, 0.0f, 0.0f, 0.0f)); });
		composition->with(render_composition);

		render_display();
	}

	void update(float t, float dt){
		if(t >= 120) {
			terminate();
		}
		for ( std::pair<std::string,Scene*> p: scene ){
			p.second->update_scene(t, dt);
		}
	}
}
