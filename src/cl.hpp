#ifndef FROB_CL_H
#define FROB_CL_H

#include <GL/glew.h>
#include <CL/cl.hpp>
#include <glm/glm.hpp>

#include <string>
#include <GL/glew.h>

namespace CL {
	void init();
	void cleanup();

	cl::Program create_program(const std::string &file_name);
	cl::Kernel load_kernel(const cl::Program &program, const char* kernel_name);

	cl::Buffer create_buffer(cl_mem_flags flags, size_t size);
	cl::BufferGL create_gl_buffer(cl_mem_flags flags, GLuint gl_buffer);

	void check_error(const cl_int &err, const char* context);

	cl_int finish();
	cl_int flush();

	cl::CommandQueue& queue();
};

#endif
