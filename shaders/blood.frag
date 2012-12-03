#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

uniform float step;

in vec2 uv;
out vec4 ocolor;

void main(){
	vec4 wc = mix(texture2D(texture0, uv), vec4(1.0), step * 2.0);
	vec4 blood = texture2D(texture1, uv);
	ocolor = wc * 0.7 + mix(blood, wc, step) * 0.3;
}
