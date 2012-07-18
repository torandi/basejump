#version 330
#include "uniforms.glsl"

out vec3 position;

void main() {
	vec4 w_pos = modelMatrix * in_position;
	gl_Position = projectionViewMatrix * w_pos;

	position = w_pos.xyz;
}
