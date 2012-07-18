#version 330
#include "uniforms.glsl"

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec3 bitangent;
in vec2 texcoord;

layout(location=0) out vec4 ocolor0;
layout(location=1) out vec4 ocolor1;

void main() {
	ocolor0 = vec4(0,0,1,1);
	ocolor1 = vec4(0,1,1,1);
}
