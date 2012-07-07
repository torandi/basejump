#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "movable_object.hpp"
#include "texture.hpp"

#include "cl.hpp"
#include <glm/glm.hpp>

class ParticleSystem : public MovableObject {
	public:

		ParticleSystem(const int max_num_particles, TextureArray* texture);
		~ParticleSystem();

		void update(float dt);
		void render();

		void update_config();

		//Change values in this struct and call update_config() to update
		struct {

			glm::vec4 birth_color;

			glm::vec4 death_color;

			glm::vec4 motion_rand;

			glm::vec4 spawn_direction;
			glm::vec4 direction_var;

			glm::vec4 spawn_position;
			glm::vec4 spawn_area;

			glm::vec4 directional_speed;
			glm::vec4 directional_speed_var;

			//Time to live
			float avg_ttl;
			float ttl_var;
			//Spawn speed
			float avg_spawn_speed;
			float spawn_speed_var;

			//Acceleration
			float avg_acc;
			float acc_var;
			//Scale
			float avg_scale;
			float scale_var;

			float avg_scale_change;
			float scale_change_var;

			//Rotation
			float avg_rotation_speed;
			float rotation_speed_var;

			//These two should not be manually changed!
			int num_textures;
			int max_num_particles;

		} config __attribute__ ((aligned (16)));

		float avg_spawn_rate; //Number of particles to spawn per second
		float spawn_rate_var;

		struct vertex_t {
			glm::vec4 position;
			glm::vec4 color;
			float scale;
			int texture_index;
		} __attribute__ ((aligned (16)));

		virtual void callback_position(const glm::vec3 &position);

	private:

		const int max_num_particles_;

		//Texture * texture_;

		// Buffer 0: position buffer 1: color.
		// Both are set in the opencl-kernel
		GLuint gl_buffer_;
		std::vector<cl::Memory> cl_gl_buffers_;
		cl::Buffer particles_, config_, random_, spawn_rate_;

		cl::Program program_;
		cl::Kernel run_kernel_, spawn_kernel_;

		struct particle_t {
			glm::vec4 direction;

			float ttl;
			float speed;
			float acc;
			float rotation_speed;

			float initial_scale;
			float final_scale;
			float org_ttl;
			int dead;
		} __attribute__ ((aligned (16))) ;

		TextureArray* texture_;
};


#endif
