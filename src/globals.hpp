#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "shader.hpp"
#include "time.hpp"
#include <glm/glm.hpp>

#include "cl.hpp"

#ifdef ENABLE_INPUT
	#include "input.hpp"
	extern Input input;
#endif

extern FILE* verbose;                    /* stderr if verbose output is enabled or /dev/null if not */
extern Time global_time;                 /* current time */
extern glm::ivec2 resolution;            /* current resolution */
extern glm::mat4 screen_ortho;           /* orthographic projection for window */

enum shader_t {
	SHADER_SIMPLE=0,
	SHADER_NORMAL,
	SHADER_MODELVIEWER,                    /* Texture, no lighting */
	SHADER_PARTICLES,
	SHADER_PARTICLES_LIGHT, //Particles with lighting
	SHADER_DEBUG,
	SHADER_SKYBOX,
	SHADER_WATER,

	/* for rendering targets */
	SHADER_PASSTHRU,                       /* multiplies vertices with MVP and textures using unit 1 */
	SHADER_DISTORT,                        /* distorts the view by offsetting UV */
	SHADER_BLUR,                           /* gaussian blur */
	SHADER_FILMGRAIN,                      /* slightly random offsets + noise */
	SHADER_BLEND,                          /* mixes texture unit 1-4 using unit 5 */

	NUM_SHADERS
};

extern Shader* shaders[];                /* all shader programs */
extern CL * opencl;

void terminate(); //Implemented in main.cpp

#endif /* GLOBALS_H */
