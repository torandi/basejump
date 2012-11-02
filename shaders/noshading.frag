#version 150

#include "uniforms.glsl"

in vec3 position;
in vec2 texcoord;

#include "fog.glsl"

out vec4 ocolor;

void main() {

	ocolor = texture(texture0, texcoord);
	ocolor*=Mtl.diffuse;

}
