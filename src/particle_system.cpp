#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "particle_system.hpp"
#include "globals.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "cl.hpp"
#include "globals.hpp"
#include "utils.hpp"


ParticleSystem::ParticleSystem(const int max_num_particles) : max_num_particles_(max_num_particles) {
	program_ = opencl->create_program("cl_programs/particles.cl");
	kernel_  = opencl->load_kernel(program_, "run_particles");

	//Empty vec4s:
	vertex_t * empty = new vertex_t[max_num_particles];

	for(int i=0;i<max_num_particles; ++i) {
		empty[i].position = glm::vec4(0.f,0.f,0.f,0.1f);
		empty[i].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
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
		initial_particles[i].ttl = -1.f; //mark as dead
	}

	//Create cl buffers:
	cl_gl_buffers_.push_back(opencl->create_gl_buffer(CL_MEM_READ_WRITE, gl_buffer_));

	particles_ = opencl->create_buffer(CL_MEM_READ_WRITE, sizeof(particle_t)*max_num_particles);
	config_ = opencl->create_buffer(CL_MEM_READ_ONLY, sizeof(config));

	random_ = opencl->create_buffer(CL_MEM_READ_ONLY, sizeof(float)*max_num_particles);
	srand(time(0));

	float * rnd = new float[max_num_particles];

	fprintf(verbose, "Generating random numbers\n");
	for(int i = 0; i<max_num_particles; ++i) {
		rnd[i] = frand();
	}

	cl::Event e;

	cl_int err = opencl->queue().enqueueWriteBuffer(particles_, CL_TRUE, 0, sizeof(particle_t)*max_num_particles, initial_particles, NULL, &e);
	CL::check_error(err, "[ParticleSystem] Write particles buffer");
	update_blocking_events_.push_back(e);
	err = opencl->queue().enqueueWriteBuffer(random_, CL_TRUE, 0, sizeof(float)*max_num_particles, rnd, NULL, &e);
	CL::check_error(err, "[ParticleSystem] Write random data buffer");
	update_blocking_events_.push_back(e);

	opencl->queue().flush();

	delete[] initial_particles;
	delete[] rnd;

	err = kernel_.setArg(0, cl_gl_buffers_[0]);
	CL::check_error(err, "[ParticleSystem] Set arg 0");
	err = kernel_.setArg(1, particles_);
	CL::check_error(err, "[ParticleSystem] Set arg 1");
	err = kernel_.setArg(2, config_);
	CL::check_error(err, "[ParticleSystem] Set arg 2");
	err = kernel_.setArg(3, random_);
	CL::check_error(err, "[ParticleSystem] Set arg 3");
	err = kernel_.setArg(4, max_num_particles);
	CL::check_error(err, "[ParticleSystem] Set arg 4");

	//Set default values in config:

	config.birth_color = glm::vec4(0.f, 0.f, 1.f, 1.f);;
	config.death_color = glm::vec4(1.f, 0.f, 0.f, 1.f);;

	config.motion_rand = glm::vec4(0.001f, 0.001f, 0.001f, 0.f);

	config.spawn_direction = glm::vec4(1.f, 0.f, 0.f, 0.f);
	config.direction_var = glm::vec4(0.f, 0.3f, 0.3f,0.f);

	config.spawn_position = glm::vec4(0, 0, 0, 0);
	config.spawn_area = glm::vec4(0.2f, 0.2f, 0.f, 0);

	//Time to live
	config.avg_ttl = 2.0;
	config.ttl_var = 1.0;
	//Spawn speed
	config.avg_spawn_speed = 0.1f;
	config.spawn_speed_var = 0.05f;

	//Acceleration
	config.avg_acc = -0.01f;
	config.acc_var = 0.005f;
	//Scale
	config.avg_scale = 0.01f;
	config.scale_var = 0.005f;

	config.max_num_particles = max_num_particles;
	update_config();

}

ParticleSystem::~ParticleSystem() {
	glDeleteBuffers(1, &gl_buffer_);
}

void ParticleSystem::update_config() {

	cl::Event e;


	cl_int err = opencl->queue().enqueueWriteBuffer(config_, CL_TRUE, 0, sizeof(config), &config, NULL, &e);
	CL::check_error(err, "[ParticleSystem] Write config");

	update_blocking_events_.push_back(e);
	opencl->queue().flush();
}

void ParticleSystem::update(float dt) {
	cl_int err;
	//Ensure there are no pending writes active
	CL::waitForEvent(update_blocking_events_);
	update_blocking_events_.clear();


	//Make sure opengl is done with our vbos
	glFinish();

	update_blocking_events_.push_back(cl::Event());

	err = opencl->queue().enqueueAcquireGLObjects(&cl_gl_buffers_, NULL, &update_blocking_events_[0]);
	CL::check_error(err, "[ParticleSystem] acquire gl objects");

	err = kernel_.setArg(5, dt);
	CL::check_error(err, "[ParticleSystem] set dt");
	err = kernel_.setArg(6, (int)(time(0)%UINT_MAX));
	CL::check_error(err, "[ParticleSystem] set time");

	cl::Event e, e2;

	err = opencl->queue().enqueueNDRangeKernel(kernel_, cl::NullRange, cl::NDRange(max_num_particles_), cl::NullRange, &update_blocking_events_, &e);
	CL::check_error(err, "[ParticleSystem] Execute kernel");

	update_blocking_events_.clear();
	update_blocking_events_.push_back(e);

	err = opencl->queue().enqueueReleaseGLObjects(&cl_gl_buffers_, &update_blocking_events_, &e2);
	CL::check_error(err, "[ParticleSystem] Release GL objects");

	update_blocking_events_.clear();
	render_blocking_events_.push_back(e2);

	opencl->queue().flush();
/*
	//BEGIN DEBUG
	particle_t * particles = (particle_t*) opencl->queue().enqueueMapBuffer(particles_, CL_TRUE, CL_MAP_READ, 0, sizeof(particle_t)*max_num_particles_, NULL, NULL, &err);

	opencl->queue().finish();

	for(int i=0; i < max_num_particles_; ++i ) {
		printf("Dir: (%f, %f, %f), ttl: (%f/%f)\n", particles[i].direction.x, particles[i].direction.y,particles[i].direction.z, particles[i].ttl, particles[i].org_ttl);
	}

	opencl->queue().enqueueUnmapMemObject(particles_, particles, NULL, NULL);
	opencl->queue().finish();

	//END DEBUG
	*/
}

void ParticleSystem::render() {
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);

	Shader::upload_model_matrix(matrix());

	//Ensure there are no pending updates
	CL::waitForEvent(update_blocking_events_);
	render_blocking_events_.clear();

	glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);
/*
	//Debug!

	vertex_t * vertices = (vertex_t* )glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

	printf("---\n");
	for(int i=0;i<max_num_particles_; ++i) {
		printf("Vertex: (%f, %f, %f, %f), (%f, %f, %f, %f)\n", vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, vertices[i].position.w, vertices[i].color.r, vertices[i].color.g, vertices[i].color.b, vertices[i].color.a);
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);
	//END debug
*/

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*) sizeof(glm::vec4));
	glDrawArrays(GL_POINTS, 0, max_num_particles_);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glPopAttrib();
}

void ParticleSystem::limit_particles(int limit) {
	cl_int err;
	if(limit <= max_num_particles_) {
		err = kernel_.setArg(4, limit);
		CL::check_error(err, "[ParticleSystem] Set particle limit");
	} else {
		fprintf(stderr,"[ParticleSystem] Can set particle limit higher than initial limit\n");
		abort();
	}
}
