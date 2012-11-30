#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "engine.hpp"
#include "config.hpp"
#include "game.hpp"

Game * game;

namespace Engine {
	/* Not used in basejump */
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Config config = Config::parse("/graphics.cfg");
		/* TODO: Maybee have a level selection screen */
		game = new Game("default", 
				config["/camera/near"]->as_float(),
				config["/camera/far"]->as_float(),
				config["/camera/fov"]->as_float()
			);
	}

	void start(double seek) {
		game->start();
	}

	void cleanup(){
		delete game;
		Shader::cleanup();
	}

	void render(){
		game->render();
	}

	void update(float t, float dt){
		game->update(t, dt);
	}
}
