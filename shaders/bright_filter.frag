#version 330
#include "uniforms.glsl"

uniform float exposure;
uniform float bright_max;

in vec2 uv;
out vec4 color;

void main(){
	color = texture2D(texture0, uv);

	float Y = dot(vec4(0.30, 0.59, 0.11, 0.0), color);
	float YD = exposure * (exposure/bright_max + 1.0) / (exposure + 1.0);
	color *= YD;

	color.r = clamp(color.r - 1.0, 0.0, 1.0);
	color.g = clamp(color.g - 1.0, 0.0, 1.0);
	color.b = clamp(color.b - 1.0, 0.0, 1.0);
}
