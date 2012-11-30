#version 330
#include "uniforms.glsl"
#include "sky.glsl"

in vec3 position;
in vec3 texcoord;
out vec4 ocolor;

void main() {
	ocolor = sky_color(position, 1.0);
}
