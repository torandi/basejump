#version 330
#include "uniforms.glsl"

in vec2 uv;

void main(){
	ocolor = texture2D(texture0, uv);
}
