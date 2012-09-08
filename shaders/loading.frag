#version 330
#include "uniforms.glsl"

uniform float fade;

in vec2 uv;
out vec4 ocolor;

void main(){
	ocolor = texture2D(texture0, uv);

	ocolor.a *= min(fade/2.0, 1.0);
}
