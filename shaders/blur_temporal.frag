#version 330
#include "uniforms.glsl"

in vec2 uv;
out vec4 ocolor;

uniform float factor = 0.5;

void main() {
	ocolor = mix(texture(texture0, uv), texture(texture1, uv), factor);
}
