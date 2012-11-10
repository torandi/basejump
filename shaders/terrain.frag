#version 330
#include "uniforms.glsl"

uniform float texture_fade_start;
uniform float texture_fade_length;

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec3 bitangent;
in vec2 texcoord;
in float texture_override;
in vec4 shadowmap_coord[maxNumberOfLights];

#include "light_calculations.glsl"
#include "fog.glsl"

out vec4 ocolor;

void main() {
	vec3 norm_normal, norm_tangent, norm_bitangent;
	norm_normal = normalize(normal);
	norm_tangent = normalize(tangent);
	norm_bitangent = normalize(bitangent);

	vec3 camera_direction = normalize(camera_pos - position);

	//Convert to tangent space:
	vec3 camera_dir;
	camera_dir.x = dot(camera_direction, norm_tangent);
	camera_dir.y = dot(camera_direction, norm_bitangent);
	camera_dir.z = dot(camera_direction, norm_normal);

	vec3 pos_tangent_space;
	pos_tangent_space.x = dot(position, norm_tangent);
	pos_tangent_space.y = dot(position, norm_bitangent);
	pos_tangent_space.z = dot(position, norm_normal);

	vec4 color1, color2;
	float color_mix, angle;
	angle = acos(abs(dot(norm_normal, vec3(0.0, 1.0, 0.0))));
	color_mix = clamp( ( angle - texture_fade_start ) / texture_fade_length, 0.0, 1.0);
	color1 = texture2DArray(texture_array0, vec3(texcoord, 0));
	color2 = texture2DArray(texture_array0, vec3(texcoord, 1));
	vec4 originalColor = mix(color1, color2, color_mix);

	color1 = texture2DArray(texture_array1, vec3(texcoord, 0));
	color2 = texture2DArray(texture_array1, vec3(texcoord, 1));
	vec3 normal_map = mix(color1, color2, color_mix).xyz;

	normal_map = normalize(normal_map * 2.0 - 1.0);

	float shininess = 18.f;
	vec4 accumLighting = originalColor * vec4(Lgt.ambient_intensity,1.f);

	for(int light = 0; light < Lgt.num_lights; ++light) {
			accumLighting += compute_lighting(
				Lgt.lights[light], originalColor,
				pos_tangent_space, normal_map, camera_dir,
				norm_normal, norm_tangent, norm_bitangent,
					Mtl.shininess, Mtl.specular) * 1.f;
			//shadow_coefficient(Lgt.lights[light], position, shadowmap_coord[light]);
	}

	ocolor = calculate_fog(accumLighting);

	ocolor.a = 1.f;
}
