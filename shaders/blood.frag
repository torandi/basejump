#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

uniform float step;

in vec2 uv;
out vec4 ocolor;

void main(){
	vec4 wc = mix(texture2D(texture0, uv), vec4(1.0), step);
	vec4 blood = texture2D(texture1, uv) * 0.8;
	ocolor = wc + mix(blood, vec4(0.0), step(0.8, step));
}
