/**
 * Calculate screenspace UV based on texture.
 */
vec2 screenspace_uv(sampler2D t){
	ivec2 size = textureSize(t, 0);
	return vec2(gl_FragCoord.x / size.x, gl_FragCoord.y / size.y);
}

/**
 * Linearize depth from a depthbuffer to [0, 1].
 */
float linear_depth(sampler2D depthbuffer){
	float z = texture(depthbuffer, screenspace_uv(depthbuffer)).x;
	return (2.0 * camera_near) / (camera_far + camera_near - z * (camera_far - camera_near));
}
