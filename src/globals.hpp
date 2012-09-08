#ifndef GLOBALS_H
#define GLOBALS_H

#include "time.hpp"
#include <glm/glm.hpp>
#include <cstdio>

#ifdef ENABLE_INPUT
	#include "input.hpp"
	extern Input input;
#endif

extern FILE* verbose;                    /* stderr if verbose output is enabled or /dev/null if not */
extern Time global_time;                 /* current time */
extern glm::ivec2 resolution;            /* current resolution */
extern glm::mat4 screen_ortho;           /* orthographic projection for window */

extern CL * opencl;

#endif /* GLOBALS_H */
