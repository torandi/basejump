#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

uniform float sun;
uniform float t;

in vec2 uv;
out vec4 ocolor;

void main(){
	ocolor = vec4(0,1,0,1);
  ocolor.r = 1.0;
  ocolor.g = clamp(sin(uv.y * 15 + t * 2) * 0.5 + 0.5 * sin(uv.x * 5 + t * 5) * 0.5 + 0.5 + 0.1, 0,1);
  ocolor.b = 0.0;
}
