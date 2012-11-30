vec4 sky_color(in vec3 position, float sun_mix) {
	vec3 on_sphere = normalize(position);

	vec4 skycolor = mix(sky.horizont_color, sky.zenit_color, clamp(( on_sphere.y + sky.lerp_offset)/ sky.lerp_size, 0.0, 1.0));

	vec4 color = mix(skycolor, sky.sun_color, sun_mix * (1.0 - step(sky.sun_radius, distance(sky.sun_position, on_sphere))));
	return mix(sky.sun_aura_color, color, clamp((distance(sky.sun_position, on_sphere)) * sky.sun_aura_scale, 0.0, 1.0));
}
