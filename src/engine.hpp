#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <vector>
#include <functional>

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
	 * Register all scene types.
	 */
	void autoload_scenes();

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

	/**
	 * Load timetable from file. Uses rendertaget_by_name to locate scenes.
	 */
	void load_timetable(const std::string& filename);
};

#endif /* ENGINE_H */
