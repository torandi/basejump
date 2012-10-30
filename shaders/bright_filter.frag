#version 330
#include "uniforms.glsl"

uniform float threshold;
uniform float bright_max;

in vec2 uv;
out vec4 color;

void main(){
	color = texture2D(texture0, uv);
	
	color.r = clamp((color.r - threshold) / bright_max, 0.0, 1.0);
	color.g = clamp((color.g - threshold) / bright_max, 0.0, 1.0);
	color.b = clamp((color.b - threshold) / bright_max, 0.0, 1.0);
}
