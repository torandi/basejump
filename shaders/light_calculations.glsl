float attenuation(in light_data light, float d){
	return 1.0 / ( light.constant_attenuation + light.linear_attenuation * d + light.quadratic_attenuation * d * d);
}

vec4 computeLighting(
	in light_data light, in vec4 originalColor,
	in vec3 N, in vec3 L,
	in vec3 camera_dir, in float distance,
	float shininess, vec4 specular,
	bool use_diffuse, bool use_specular
	) {
	vec3 lightIntensity;

	//Light attenuation on if type == 1 (point light)
	if(light.type == 0) {
		lightIntensity = light.intensity.rgb;
	} else {
		float lightAttenuation = attenuation(light, distance);
		lightIntensity =  lightAttenuation * light.intensity.rgb;
	}

	float LambertTerm = max( dot(L, N), 0.0);
	float specular_amount = 0.0;

	if( LambertTerm > 0.0) {
		//Apply specular
		specular_amount = pow(clamp(dot(reflect(-L, N), camera_dir), 0.0, 1.0), shininess);
	}

	vec3 diffuse = originalColor.rgb * LambertTerm * lightIntensity;
	vec3 specular_color = specular.rgb * specular_amount * length(diffuse);

	vec4 color = vec4(0.0);
	if(use_diffuse)
		color.rgb += diffuse;
	if(use_specular)
		color.rgb += specular_color;
	color.a = 1.0;

	return color;
}
