#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "engine.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "shader.hpp"
#include "utils.hpp"
#include "texture.hpp"

namespace Engine {

	void setup_opengl(){
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glEnable(GL_BLEND);
		glCullFace(GL_BACK);
		glDepthFunc(GL_LEQUAL);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}

	void preload(const std::vector<std::string>& names, std::function<void(const std::string&, int, int)> progress){
		int index = 1;

		for ( auto resource : names ){
			const size_t delimiter = resource.find(':');
			if ( delimiter == std::string::npos ){
				Logging::warning("Resource `%s' does not contain prefix, preloading ignored.\n", resource.c_str());
				continue;
			}

			const std::string prefix = resource.substr(0, delimiter);
			const std::string filename = resource.substr(delimiter+1);

			progress(filename, index++, static_cast<int>(names.size()));

			if ( prefix == "texture" ){
				Texture2D::preload(filename);
			} else if ( prefix == "shader" ){
				Shader::create_shader(filename);
			} else {
				Logging::warning("Resource `%s' has an unknown prefix, preloading ignored.\n", resource.c_str());
			}
		}
	}
}
