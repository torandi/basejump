#version 330
#include "uniforms.glsl"

out vec3 position;
out vec2 texcoord;

void main() {
	vec4 w_pos = modelMatrix * in_position;

	position = w_pos.xyz;
	texcoord = in_texcoord.st;
	gl_Position = projectionViewMatrix * w_pos;
}
