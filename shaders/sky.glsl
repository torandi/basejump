vec4 sky_color(in vec3 position, float sun_mix) {
	vec3 on_sphere = normalize(position);
	float s = clamp(( on_sphere.y + sky.lerp_offset)/ sky.lerp_size, 0.0, 1.0);
	vec4 skycolor = mix(sky.horizont_color, sky.zenit_color, s);

	s = sun_mix * (1.0 - step(sky.sun_radius, distance(sky.sun_position.xyz, on_sphere)));
	vec4 color = mix(skycolor, sky.sun_color, s);
	s = clamp((distance(sky.sun_position.xyz, on_sphere)) * sky.sun_aura_scale, 0.0, 1.0);
	return mix(sky.sun_aura_color, color, s);
	
}
