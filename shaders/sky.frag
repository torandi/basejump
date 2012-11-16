#version 330
#include "uniforms.glsl"

uniform vec4 zenit_color;
uniform vec4 horizont_color;
uniform vec4 sun_color;
uniform vec4 sun_aura_color;
uniform vec2 sun_position;
uniform vec2 sun_radius;

in vec3 position;
in vec3 texcoord;
out vec4 ocolor;

void main() {
	vec3 on_sphere = normalize(position);
	vec2 spherical = vec2(acos(on_sphere.y), atan(on_sphere.z, on_sphere.x));

	ocolor = mix(horizont_color, zenit_color, on_sphere.y);
	ocolor.a = 1.f;
}
