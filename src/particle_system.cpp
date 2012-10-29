#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "particle_system.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "texture.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <ctime>
#include <memory>

#include "cl.hpp"
#include "globals.hpp"
#include "utils.hpp"

ParticleSystem::ParticleSystem(const int max_num_particles, TextureArray* texture, bool _auto_spawn)
	: avg_spawn_rate(static_cast<float>(max_num_particles)/10.f)
	, spawn_rate_var(avg_spawn_rate/100.f)
	, auto_spawn(_auto_spawn)
	,	max_num_particles_(max_num_particles)
	,	texture_(texture) {

	program_ = CL::create_program("/cl_programs/particles.cl");
	run_kernel_  = CL::load_kernel(program_, "run_particles");
	spawn_kernel_  = CL::load_kernel(program_, "spawn_particles");

	Logging::verbose("Created particle system with %d particles\n", max_num_particles);

	//Empty vec4s:
	vertex_t * empty = new vertex_t[max_num_particles];

	for(int i=0;i<max_num_particles; ++i) {
		empty[i].position = glm::vec4(0.f);
		empty[i].color = glm::vec4(0.f);
		empty[i].scale = 0.f;
		empty[i].texture_index = 0;
	}

	//Create VBO's
	glGenBuffers(1, &gl_buffer_);
	checkForGLErrors("[ParticleSystem] Generate GL buffer");

	glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t)*max_num_particles, empty, GL_DYNAMIC_DRAW);

	checkForGLErrors("[ParticleSystem] Buffer vertices");

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete[] empty;

	particle_t * initial_particles = new particle_t[max_num_particles];
	for(int i=0; i<max_num_particles; ++i) {
		initial_particles[i].dead = 1; //mark as dead
	}

	//Create cl buffers:
	cl_gl_buffers_.push_back(CL::create_gl_buffer(CL_MEM_READ_WRITE , gl_buffer_));

	particles_ = CL::create_buffer(CL_MEM_READ_WRITE, sizeof(particle_t)*max_num_particles);
	config_ = CL::create_buffer(CL_MEM_READ_ONLY, sizeof(config));
	spawn_rate_  = CL::create_buffer(CL_MEM_READ_WRITE, sizeof(cl_int));

	random_ = CL::create_buffer(CL_MEM_READ_ONLY, sizeof(float)*max_num_particles);

	Logging::verbose("  - Generating random numbers\n");
	std::unique_ptr<float[]> rnd(new float[max_num_particles]);
	for ( int i = 0; i < max_num_particles; ++i ) {
		rnd[i] = frand();
	}

	cl::Event lock[2];

	cl_int err = CL::queue().enqueueWriteBuffer(particles_, CL_FALSE, 0, sizeof(particle_t)*max_num_particles, initial_particles, NULL, &lock[0]);
	CL::check_error(err, "[ParticleSystem] Write particles buffer");
	err = CL::queue().enqueueWriteBuffer(random_, CL_FALSE, 0, sizeof(float)*max_num_particles, rnd.get(), NULL, &lock[1]);
	CL::check_error(err, "[ParticleSystem] Write random data buffer");

	CL::flush();

	lock[0].wait();
	lock[1].wait();

	delete[] initial_particles;

	err = run_kernel_.setArg(0, cl_gl_buffers_[0]);
	CL::check_error(err, "[ParticleSystem] run: Set arg 0");
	err = run_kernel_.setArg(1, particles_);
	CL::check_error(err, "[ParticleSystem] run: Set arg 1");
	err = run_kernel_.setArg(2, config_);
	CL::check_error(err, "[ParticleSystem] run: Set arg 2");
	err = run_kernel_.setArg(3, random_);
	CL::check_error(err, "[ParticleSystem] run: Set arg 3");

	err = spawn_kernel_.setArg(0, cl_gl_buffers_[0]);
	CL::check_error(err, "[ParticleSystem] spawn: Set arg 0");
	err = spawn_kernel_.setArg(1, particles_);
	CL::check_error(err, "[ParticleSystem] spawn: Set arg 1");
	err = spawn_kernel_.setArg(2, config_);
	CL::check_error(err, "[ParticleSystem] spawn: Set arg 2");
	err = spawn_kernel_.setArg(3, random_);
	CL::check_error(err, "[ParticleSystem] spawn: Set arg 3");
	err = spawn_kernel_.setArg(4, spawn_rate_);
	CL::check_error(err, "[ParticleSystem] spawn: Set arg 4");

	//Set default values in config:

	config.birth_color = glm::vec4(0.f, 1.f, 1.f, 1.f);;
	config.death_color = glm::vec4(1.f, 0.f, 0.f, 1.f);;

	config.motion_rand = glm::vec4(0.f, 0.f, 0.f, 0.f);

	config.avg_spawn_velocity = glm::vec4(1.f, 0.f, 0.f, 0.f);
	config.spawn_velocity_var = glm::vec4(0.f, 0.3f, 0.3f,0.f);

	config.spawn_position = glm::vec4(0.f, 0.f, 0.f, 0.f);
	config.spawn_area = glm::vec4(1.0f, 1.0f, 1.f, 0);

	config.wind_velocity = glm::vec4(0.f);
	config.gravity = glm::vec4(0, -1.f, 0, 0);

	//Time to live
	config.avg_ttl = 2.0;
	config.ttl_var = 1.0;

	//Scale
	config.avg_scale = 0.01f;
	config.scale_var = 0.005f;
	config.avg_scale_change = 0.f;
	config.scale_change_var = 0.f;

	//Rotation
	config.avg_rotation_speed = 0.f;
	config.rotation_speed_var = 0.f;

	config.avg_wind_influence = 0.1f;
	config.wind_influence_var = 0.f;
	config.avg_gravity_influence = 0.5f;
	config.gravity_influence_var = 0.f;

	config.start_texture = 0;
	config.num_textures = static_cast<unsigned int>(texture->num_textures());
	config.max_num_particles = max_num_particles;
	update_config();

}

ParticleSystem::~ParticleSystem() {
	glDeleteBuffers(1, &gl_buffer_);
}

void ParticleSystem::update_config() {
	cl_int err = CL::queue().enqueueWriteBuffer(config_, CL_TRUE, 0, sizeof(config), &config, NULL, NULL);
	CL::check_error(err, "[ParticleSystem] Write config");
}


void ParticleSystem::callback_position(const glm::vec3 &position) {
	config.spawn_position = glm::vec4(position,1.f);
	update_config();
}

void ParticleSystem::spawn_particles(cl_int count, cl::Event * event) {
	cl_int err = CL::queue().enqueueWriteBuffer(spawn_rate_, CL_TRUE, 0, sizeof(cl_int), &count, NULL, NULL);
	CL::check_error(err, "[ParticleSystem] spawn: Write spawn count");

	//TODO: Optimize!
	err = CL::queue().enqueueNDRangeKernel(spawn_kernel_, cl::NullRange, cl::NDRange(max_num_particles_), cl::NullRange, NULL, event);
	CL::check_error(err, "[ParticleSystem] Execute spawn_kernel");
}

void ParticleSystem::update(float dt) {
	cl_int err;

	//Make sure opengl is done with our vbos
	glFinish();

	std::vector<cl::Event> lock(1,cl::Event());

	err = CL::queue().enqueueAcquireGLObjects((std::vector<cl::Memory>*) &cl_gl_buffers_, NULL, &lock[0]);
	CL::check_error(err, "[ParticleSystem] acquire gl objects");

	err = spawn_kernel_.setArg(5, (int)(time(0)%UINT_MAX));
	CL::check_error(err, "[ParticleSystem] spawn: set time");

	CL::flush();
	lock[0].wait(); //Wait to aquire gl objects

	bool restore_config = !spawn_list_.empty();

	/*
	 * Handle spawning
	 */
	while(!spawn_list_.empty()) {
		spawn_data &sd = spawn_list_.front();

		cl_int err = CL::queue().enqueueWriteBuffer(config_, CL_TRUE, 0, sizeof(config_t), &sd.first, NULL, NULL);
		CL::check_error(err, "[ParticleSystem] Write config");

		spawn_particles((cl_int) sd.second, &lock[0]);

		spawn_list_.pop_front();

		CL::flush();

		lock[0].wait();
	}

	if(restore_config) {
		update_config();
	}

	if(auto_spawn) {
		//Write number of particles to spawn this round:
		cl_int current_spawn_rate = (cl_int) round((avg_spawn_rate + 2.f*frand()*spawn_rate_var - spawn_rate_var)*dt);
		spawn_particles(current_spawn_rate, &lock[0]);
		CL::flush();
		lock[0].wait();
	}


	err = run_kernel_.setArg(4, dt);
	CL::check_error(err, "[ParticleSystem] run: set dt");
	err = run_kernel_.setArg(5, (int)(time(0)%UINT_MAX));
	CL::check_error(err, "[ParticleSystem] run: set time");

	err = CL::queue().enqueueNDRangeKernel(run_kernel_, cl::NullRange, cl::NDRange(max_num_particles_), cl::NullRange, NULL, &lock[0]);
	CL::check_error(err, "[ParticleSystem] Execute run_kernel");

	//render_blocking_events_.push_back(e2);
/*
	vertex_t * vertices = (vertex_t* ) CL::queue().enqueueMapBuffer(cl_gl_buffers_[0], CL_TRUE, CL_MAP_READ, 0, sizeof(particle_t)*max_num_particles_, NULL, NULL, &err);

	CL::finish();
	for(int i=0; i < max_num_particles_; ++i ) {
		printf("Dir: (%f, %f, %f), ttl: (%f/%f) speed: (%f) scale(%f->%f) rotation speed: %f\n", particles[i].direction.x, particles[i].direction.y,particles[i].direction.z, particles[i].ttl, particles[i].org_ttl, particles[i].speed, particles[i].initial_scale, particles[i].final_scale, particles[i].rotation_speed);
	}


*/
	/*
	vertex_t * vertices = (vertex_t*) CL::queue().enqueueMapBuffer(cl_gl_buffers_[0], CL_TRUE, CL_MAP_READ, 0, sizeof(particle_t)*max_num_particles_, NULL, NULL, &err);

	CL::finish();
	printf("---\n");
	for(int i=0;i<max_num_particles_; ++i) {
		printf("Vertex: pos:(%f, %f, %f, %f), color:(%f, %f, %f, %f) scale: %f, texture_index: %i\n", vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, vertices[i].position.w, vertices[i].color.r, vertices[i].color.g, vertices[i].color.b, vertices[i].color.a, vertices[i].scale, vertices[i].texture_index);
	}
	CL::queue().enqueueUnmapMemObject(cl_gl_buffers_[0], vertices, NULL, NULL); */

	err = CL::queue().enqueueReleaseGLObjects((std::vector<cl::Memory>*)&cl_gl_buffers_, &lock, NULL);
	CL::check_error(err, "[ParticleSystem] Release GL objects");

	CL::flush();

	//render_blocking_events_.push_back(e3);


	//BEGIN DEBUG

/*
	particle_t * particles = (particle_t*) CL::queue().enqueueMapBuffer(particles_, CL_TRUE, CL_MAP_READ, 0, sizeof(particle_t)*max_num_particles_, NULL, NULL, &err);

	CL::finish();
	for(int i=0; i < max_num_particles_; ++i ) {
		printf("Dir: (%f, %f, %f), ttl: (%f/%f) speed: (%f) scale(%f->%f) rotation speed: %f\n", particles[i].direction.x, particles[i].direction.y,particles[i].direction.z, particles[i].ttl, particles[i].org_ttl, particles[i].speed, particles[i].initial_scale, particles[i].final_scale, particles[i].rotation_speed);
	}

	CL::queue().enqueueUnmapMemObject(particles_, particles, NULL, NULL);
	*/
/*
	cl_int  * spawn_rate = (cl_int*) CL::queue().enqueueMapBuffer(spawn_rate_, CL_TRUE, CL_MAP_READ, 0, sizeof(cl_int), NULL, NULL, &err);

	CL::finish();

	printf("Spawn_rate: %d\n", *spawn_rate);

	CL::queue().enqueueUnmapMemObject(spawn_rate_, spawn_rate, NULL, NULL);

	CL::finish();*/

	//END DEBUG

	CL::finish();
}

void ParticleSystem::render(const glm::mat4&  m) {

	Shader::push_vertex_attribs();

	glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	Shader::upload_model_matrix(matrix() * m);

	glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);

	//DEBUG
/*
	vertex_t * vertices = (vertex_t* )glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

	printf("---\n");
	for(int i=0;i<max_num_particles_; ++i) {
		printf("Vertex: pos:(%f, %f, %f, %f), color:(%f, %f, %f, %f) scale: %f, texture_index: %i\n", vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, vertices[i].position.w, vertices[i].color.r, vertices[i].color.g, vertices[i].color.b, vertices[i].color.a, vertices[i].scale, vertices[i].texture_index);
	}

	printf("Sizeof(vertex_t): %d\n", sizeof(vertex_t));

	if(!glUnmapBuffer(GL_ARRAY_BUFFER)) {
		printf("glUnmapBuffer returned False\n");
		abort();
	}
	*/


	//END DEBUG

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*) sizeof(glm::vec4));
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*) (2*sizeof(glm::vec4)));
	glVertexAttribPointer(3, 1, GL_INT, GL_FALSE, sizeof(vertex_t), (GLvoid*)		(2*sizeof(glm::vec4)+sizeof(float)));
	texture_->texture_bind(Shader::TEXTURE_ARRAY_0);

	glDrawArrays(GL_POINTS, 0, max_num_particles_);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopAttrib();

	Shader::pop_vertex_attribs();

}

void ParticleSystem::push_config() {
	config_stack_.push_back(config);
}

void ParticleSystem::pop_config() {
	config = config_stack_.back();
	config_stack_.pop_back();
}

void ParticleSystem::spawn(int count) {
	spawn_data sd;
	sd.first = config;
	sd.second = count;
	spawn_list_.push_back(sd);
}

void ParticleSystem::read_config(const ConfigEntry * cfg) {
	config.spawn_position = glm::vec4(cfg->find("spawn_position", true)->as_vec3(), 1.f);
	config.spawn_area = cfg->find("spawn_area", true)->as_vec4();
	config.birth_color = cfg->find("birth_color", true)->as_vec4();
	config.death_color = cfg->find("death_color", true)->as_vec4();
	config.motion_rand = glm::vec4(cfg->find("motion_rand", true)->as_vec3(), 1.f);
	config.spawn_velocity_var = glm::vec4(cfg->find("spawn_velocity_var", true)->as_vec3(), 1.f);

	config.wind_velocity = glm::vec4(cfg->find("wind_velocity", true)->as_vec3(), 1.f);
	config.gravity = glm::vec4(cfg->find("gravity", true)->as_vec3(), 1.f);

	config.avg_ttl = cfg->find("avg_ttl", true)->as_float();
	config.ttl_var = cfg->find("ttl_var", true)->as_float();
	config.avg_scale = cfg->find("avg_scale", true)->as_float();
	config.scale_var = cfg->find("scale_var", true)->as_float();
	config.avg_scale_change = cfg->find("avg_scale_change", true)->as_float();
	config.scale_change_var = cfg->find("scale_change_var", true)->as_float();
	config.avg_rotation_speed = cfg->find("avg_rotation_speed", true)->as_float();
	config.rotation_speed_var = cfg->find("rotation_speed_var", true)->as_float();
	config.avg_wind_influence = cfg->find("avg_wind_influence", true)->as_float();
	config.wind_influence_var = cfg->find("wind_influence_var", true)->as_float();
	config.avg_gravity_influence = cfg->find("avg_gravity_influence", true)->as_float();
	config.gravity_influence_var = cfg->find("gravity_influence_var", true)->as_float();
	config.start_texture = cfg->find("start_texture", true)->as_int();
	config.num_textures = cfg->find("num_textures", true)->as_int();
}
