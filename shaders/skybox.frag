#version 330
#include "uniforms.glsl"
#include "skybox_color.glsl"

in vec3 texcoord;

void main() {
	ocolor = vec4(skybox_color(texcoord), 1.0);
}
