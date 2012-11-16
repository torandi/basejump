#version 330
#include "uniforms.glsl"

uniform vec4 zenit_color;
uniform vec4 horizont_color;
uniform vec4 sun_color;
uniform vec4 sun_aura_color;
uniform vec3 sun_position;
uniform float sun_radius;
uniform float sun_aura_scale;
uniform float lerp[2]; /* size, offset */

in vec3 position;
in vec3 texcoord;
out vec4 ocolor;

void main() {
	vec3 on_sphere = normalize(position);

	vec4 skycolor = mix(horizont_color, zenit_color, clamp(( on_sphere.y + lerp[1])/ lerp[0], 0.0, 1.0));

	ocolor = mix(sun_color, skycolor, step(sun_radius, distance(sun_position, on_sphere)));
	ocolor = mix(sun_aura_color, ocolor, clamp((distance(sun_position, on_sphere)) * sun_aura_scale, 0.0, 1.0));
}
