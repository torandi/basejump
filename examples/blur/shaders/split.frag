#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

in vec2 uv;
out vec4 ocolor;

void main(){
	if ( uv.x < 0.5 ){
		ocolor = texture(texture0, uv);
	} else {
		ocolor = texture(texture1, uv);
	}
}
