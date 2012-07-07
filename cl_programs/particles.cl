#include "particles_structs.cl"
#include "particles_random.cl"

__kernel void run_particles (
														 __global vertex_t * vertices, 
														 __global particle_t * particles, 
														 read_only image2d_t heightmap,
														 image2d_t hitmap,
														 __constant config_t * config, 
														 __global const float * rnd,
														 float dt,
														 uint time
														 )
{
	uint id = get_global_id(0);
	const sampler_t mSampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
	if(particles[id].dead == 0) {
		particles[id].ttl -= dt;
		if(particles[id].ttl > 0) {
			float life_progression = 1.0 - (particles[id].ttl/particles[id].org_ttl);
			particles[id].speed += particles[id].acc;
			if(particles[id].speed < 0) particles[id].speed = 0.0;
			vertices[id].position.xyz += particles[id].direction*particles[id].speed + random3(config->motion_rand, true) + config->directional_speed + random3(config->directional_speed_var, true);
			vertices[id].position.w += particles[id].rotation_speed;
			//vertices[id].color = mix(config->birth_color, config->death_color, life_progression);
			vertices[id].scale = mix(particles[id].initial_scale, particles[id].final_scale, life_progression);
			//Get image position:
			float4 rel_pos = vertices[id].position;// + (128, 10.f, 128, 0);  // + config->map_offset;
			float2 image_pos = rel_pos.xz / (256, 256);

			image_pos.x = 1.f - image_pos.x;
			image_pos.y = 1.f - image_pos.y;

			float4 sample = read_imagef(heightmap, mSampler, image_pos)*20.f;

			if(rel_pos.y < sample.x) {
				vertices[id].color.w = 0.f;
				//write_imagef(
				particles[id].dead = 1;
			}

		} else {
			//Dead!
			vertices[id].color.w = 0.0;
			particles[id].dead = 1;
		}
	} else if(particles[id].dead == 2) {
		//Marked for respawn
		vertices[id].position.xyz = config->spawn_position + random3(config->spawn_area.xyz, false);

		float a = random1(2*M_PI, false);
		vertices[id].position.x += random1(config->spawn_area.w,false) * cos(a);
		vertices[id].position.z += random1(config->spawn_area.w,false) * sin(a);

		vertices[id].position.w = 0.f;
		vertices[id].texture_index = (int)floor(random1((float)config->num_textures-0.1, false));

		vertices[id].color = config->birth_color;
		particles[id].direction = normalize(config->spawn_direction + random3(config->direction_var, true));
		particles[id].org_ttl = particles[id].ttl = config->avg_ttl + random1(config->ttl_var, true);
		particles[id].speed = config->avg_spawn_speed + random1(config->spawn_speed_var, true);
		particles[id].acc = config->avg_acc + random1(config->acc_var, true);
		particles[id].rotation_speed = config->avg_rotation_speed + random1(config->rotation_speed_var, true);
		particles[id].initial_scale = config->avg_scale + random1(config->scale_var, true);
		particles[id].final_scale = particles[id].initial_scale + config->avg_scale_change + random1(config->scale_change_var, true);
		particles[id].dead = 0;
	}

}

__kernel void spawn_particles (
														 __global vertex_t * vertices, 
														 __global particle_t * particles, 
														 __constant config_t * config, 
														 __global const float * rnd,
														 __global int * to_spawn, //Number of particles left to spawn, use atomic operations!
														 uint time
														 )
{
	uint id = get_global_id(0);

	if (particles[id].dead == 1 && to_spawn[0] > 0 && atomic_dec(&to_spawn[0]) > 0 ) {
		particles[id].dead = 2;	
	}
}

