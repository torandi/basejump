#version 330
#include "uniforms.glsl"

uniform float fade;
in vec2 uv;
out vec4 ocolor;

void main(){
	ocolor = mix(texture2D(texture0, uv), texture2D(texture1, uv), fade);
}
