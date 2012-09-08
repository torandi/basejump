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

enum shader_t {
	SHADER_NORMAL = 0,
	SHADER_PARTICLES,
	SHADER_WATER,

	/* for rendering targets */
	SHADER_PASSTHRU,                       /* multiplies vertices with MVP and textures using unit 1 */
	SHADER_BLUR,                           /* gaussian blur */
	SHADER_BLEND,                          /* mixes texture unit 1-4 using unit 5 */

	NUM_SHADERS
};

extern Shader* shaders[];                /* all shader programs */
extern CL * opencl;

#endif /* GLOBALS_H */
