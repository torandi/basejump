#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "movable_object.hpp"
#include "cl.hpp"
#include <glm/glm.hpp>

class ParticleSystem : public MovableObject {
	public:

		ParticleSystem(const int max_num_particles, TextureArray* texture, bool oneshot=false);
		~ParticleSystem();

		void update(float dt);
		void render(const glm::mat4& m = glm::mat4());

		void update_config();

		//Change values in this struct and call update_config() to update
		struct __ALIGNED__(16) {

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
			cl_float avg_ttl;
			cl_float ttl_var;
			//Spawn speed
			cl_float avg_spawn_speed;
			cl_float spawn_speed_var;

			//Acceleration
			cl_float avg_acc;
			cl_float acc_var;
			//Scale
			cl_float avg_scale;
			cl_float scale_var;

			cl_float avg_scale_change;
			cl_float scale_change_var;

			//Rotation
			cl_float avg_rotation_speed;
			cl_float rotation_speed_var;

			//These two should not be manually changed!
			cl_int num_textures;
			cl_int max_num_particles;

		} config;

		float avg_spawn_rate; //Number of particles to spawn per second
		float spawn_rate_var;

		struct __ALIGNED__(16) vertex_t {
			glm::vec4 position;
			glm::vec4 color;
			cl_float scale;
			cl_int texture_index;
		};

		virtual void callback_position(const glm::vec3 &position);

	private:

		const int max_num_particles_;

		bool spawn_; //set to false to stop spawning, must not be changed to true from false (but other way is ok)

		//Texture * texture_;

		// Buffer 0: position buffer 1: color.
		// Both are set in the opencl-kernel
		GLuint gl_buffer_;
		std::vector<cl::BufferGL> cl_gl_buffers_;
		cl::Buffer particles_, config_, random_, spawn_rate_;

		cl::Program program_;
		cl::Kernel run_kernel_, spawn_kernel_;

		struct __ALIGNED__(16) particle_t {
			glm::vec4 direction;

			cl_float ttl;
			cl_float speed;
			cl_float acc;
			cl_float rotation_speed;

			cl_float initial_scale;
			cl_float final_scale;
			cl_float org_ttl;
			cl_int dead;
		};

		TextureArray* texture_;
};


#endif
