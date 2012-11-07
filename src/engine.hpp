#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <vector>
#include <functional>

#include <glm/glm.hpp>

extern glm::mat4 screen_ortho; /* defined in main.cpp */
extern glm::ivec2 resolution; /* defined in main.cpp */

namespace Engine {

	void init();
	void cleanup();
	void update(float t, float dt);
	void render();

	void terminate(); //Implemented in main.cpp

	/**
	 * Called when the loading scene has faded
	 * @param seek Time to start at
	 */
	void start(double seek);

	/**
	 * Enable and disable settings.
	 */
	void setup_opengl();

	/**
	 * Preload resources.
	 *
	 * @param names List of resources with "type:" prefix, e.g. "texture:foo.jpg".
	 * @param progress Optional callback run before loading a resource.
	 */
	void preload(const std::vector<std::string>& names, std::function<void(const std::string&, int, int)> progress = nullptr);

	/**
	 * Get a rendertarget by name.
	 * @return nullptr if not found.
	 */
	RenderTarget* rendertarget_by_name(const std::string& name);

};

#endif /* ENGINE_H */
