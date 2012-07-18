#version 330
#include "uniforms.glsl"

uniform int texture_index;

in vec3 position;
in vec2 texcoord;

void main() {
	vec4 texture_color = texture(texture_array0, vec3(texcoord, texture_index));
	ocolor = vec4(1.f, 1.f, 1.f, 0.8f)*texture_color;
}
