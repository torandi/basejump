#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

in vec2 uv;
out vec4 ocolor;

void main(){
	float d = linear_depth(texture3);
	ocolor = vec4(d, d, d, 1.0);
}
