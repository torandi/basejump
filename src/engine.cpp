#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "engine.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "scene.hpp"
#include "shader.hpp"
#include "utils.hpp"

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

	void load_timetable(const std::string& filename){
		int ret;
		const char* tablename = filename.c_str();
		Logging::verbose("Loading timetable from `%s'\n", tablename);

		auto func = [](const std::string& name, float begin, float end){
			RenderTarget* target = rendertarget_by_name("scene:" + name);
			Scene* scene = nullptr;

			if ( !target ){
				Logging::error("Timetable entry for missing scene `%s', ignored.\n", name.c_str());
				return;
			} else if ( !(scene=dynamic_cast<Scene*>(target)) ){
				Logging::warning("Timetable entry for RenderTarget `%s', ignored.\n", name.c_str());
				return;
			}

			scene->add_time(begin, end);
		};
		if ( (ret=timetable_parse(tablename, func)) != 0 ){
			Logging::fatal("Failed to read `%s': %s\n", tablename, strerror(ret));
		}
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

			progress(filename, index++, names.size());

			if ( prefix == "texture" ){
				Texture2D::preload(filename);
			} else {
				Logging::warning("Resource `%s' has an unknown prefix, preloading ignored.\n", resource.c_str());
			}
		}
	}
}
