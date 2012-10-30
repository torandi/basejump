#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

uniform float q;

in vec2 uv;
out vec4 ocolor;

void main(){
	float t = q;
	if ( t < 0.0f ){
		t = 0.0f;
	}

	ocolor.r = texture2D(texture0, vec2(uv.x * (1.0f - t / 7), uv.y + sin(uv.x * 15 + 1) * 0.2 * uv.x * t)).r;
	ocolor.g = texture2D(texture0, vec2(uv.x * (1.0f - t / 7) + cos(uv.y) * 0.1 * t, uv.y + sin(uv.x * 15 + 2) * 0.2 * uv.x * t)).g;
	ocolor.b = texture2D(texture0, vec2(uv.x * (1.0f - t / 7) - cos(uv.y * 1.2) * 0.1 * t, uv.y + sin(uv.x * 15 + 3) * 0.2 * uv.x * t)).b;
	ocolor.a = 1.0 - (t / 4);
}
