#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "globals.hpp"
#include <cstdint>
#include <cstddef>

#ifdef ENABLE_INPUT
Input input;
#endif

glm::ivec2 resolution(800,600);
CL * opencl;
