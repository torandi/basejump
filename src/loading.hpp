#ifndef LOADING_H
#define LOADING_H

#include <string>
#include <glm/glm.hpp>

namespace Loading {
	void init(const glm::ivec2& resolution);
	void cleanup();
	void progress(const std::string& name, int elem, int total);
}

#endif /* LOADING_H */
