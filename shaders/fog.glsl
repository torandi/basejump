#define FOG_USE_SKY_COLOR

#ifdef FOG_USE_SKY_COLOR
#include "sky.glsl"
#endif

const float LOG2 = 1.442695;

vec4 calculate_fog(in vec4 original_color , in vec3 camera_dir) {
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = exp2(-fog_density * fog_density * z * z * LOG2);
	fogFactor = 1.0 - clamp(fogFactor, 0.0, 1.0);
	vec4 color = fog_color;
#ifdef FOG_USE_SKY_COLOR
	color = sky_color(-camera_dir);
#endif
	return mix(original_color, color, fogFactor);
}
