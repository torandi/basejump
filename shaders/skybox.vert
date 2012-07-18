#version 330
#include "uniforms.glsl"

out vec3 texcoord;

void main() {
	gl_Position = projectionViewMatrix * in_position;
	texcoord = in_texcoord.xyz;
}
