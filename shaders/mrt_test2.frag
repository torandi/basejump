#version 330
#include "uniforms.glsl"

in vec2 uv;

void main(){
	vec4 t0 = vec4(texture2D(texture0, uv));
	vec4 t1 = vec4(texture2D(texture1, uv));

	ocolor = mix(t0, t1, uv.x);
}
