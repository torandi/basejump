#ifndef GLOBALS_H
#define GLOBALS_H

#include "time.hpp"
#include <glm/glm.hpp>
#include <cstdio>

#ifdef ENABLE_INPUT
	#include "input.hpp"
	extern Input input;
#endif

extern glm::ivec2 resolution;            /* current resolution */

#endif /* GLOBALS_H */
