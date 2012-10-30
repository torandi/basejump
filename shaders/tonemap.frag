#version 330
#include "uniforms.glsl"

uniform float exposure;
uniform float bloom_factor;
uniform float bright_max;

in vec2 uv;
out vec4 color;

void main(){
	color = texture2D(texture0, uv);
	vec4 color_bloom = texture2D(texture6, uv);

	color += color_bloom * bloom_factor;
	float Y = dot(vec4(0.30, 0.59, 0.11, 0.0), color);
	float YD = exposure * (exposure/bright_max + 1.0) / (exposure + 1.0);
	color *= YD;
}
