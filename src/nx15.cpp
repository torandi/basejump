#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "engine.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "quad.hpp"
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
#include <map>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "light.hpp"
#include "sound.hpp"

static Shader* passthru = nullptr;
extern glm::mat4 screen_ortho;   /* defined in main.cpp */
extern Time global_time;         /* defined in main.cpp */
extern glm::ivec2 resolution;    /* defined in main.cpp */

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		passthru = Shader::create_shader("/shaders/passthru");
	}

	void start(double seek){
		// sound->play();
		// if(global_time.sync_to_music(sound)) {
		// 	Logging::verbose("Syncinc to music!\n");
		// } else {
		// 	Logging::warning("Warning! Syncing disabled!\n");
		// }
		// if(seek > 0.1) {
		// 	sound->seek(seek);
		// }
	}

	void cleanup(){

	}

	static void render_display(){
		RenderTarget::clear(Color::magenta);
		Shader::upload_projection_view_matrices(screen_ortho, glm::mat4());
	}

	void render(){
		render_display();
	}

	void update(float t, float dt){
		if(t >= 120) {
			Engine::terminate();
		}
	}
}
