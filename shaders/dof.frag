#version 330
#include "uniforms.glsl"
#include "screenspace.glsl"

in vec2 uv;
out vec4 ocolor;

void main(){
	float depth = clamp(linear_depth(texture7), 0.0, 1.0);

	vec3 t0 = texture(texture0, uv).rgb;
	vec3 t1 = texture(texture1, uv).rgb;

	ocolor.rgb = mix(t0, t1, depth);
	ocolor.a = 1.0f;
}
