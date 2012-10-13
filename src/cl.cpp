#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cl.hpp"
#include "data.hpp"
#include "logging.hpp"
#include "texture.hpp"

#ifdef HAVE_GL_GLX_H
#include <GL/glx.h>
#endif

#include <sstream>
#include <map>

#define PP_INCLUDE "#include"

namespace CL {
static void CL_CALLBACK cl_error_callback(const char * errorinfo, const void * private_info_size, size_t cb, void * user_data);
static const char* errorString(cl_int error);

static std::map<std::string, cl::Program> cache;
static std::vector<cl::Device> devices_;
static cl::Platform platform_;
static cl::Context context_;
static cl::CommandQueue queue_;
static cl::Device context_device_;

void init(){
	cl_int err;

	std::vector<cl::Platform> platforms;
	if(cl::Platform::get(&platforms) == CL_INVALID_VALUE) {
		Logging::fatal("[OpenCL] No platforms available\n");
	}

	platform_ = platforms[0]; //Just select the first platform

	std::string name, version, extensions;

	platform_.getInfo(CL_PLATFORM_NAME, &name);
	platform_.getInfo(CL_PLATFORM_VERSION, &version);
	platform_.getInfo(CL_PLATFORM_EXTENSIONS, &extensions);

	Logging::verbose("[OpenCL]\n"
	                 "  - Platform: %s %s\n"
	                 "  - Extensions: %s\n", name.c_str(), version.c_str() ,extensions.c_str());

#if defined (__APPLE__) || defined(MACOSX)
	CGLContextObj kCGLContext = CGLGetCurrentContext();
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
	cl_context_properties properties[] =
	{
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
		0
	};
#elif defined WIN32
	HGLRC current_context = wglGetCurrentContext();
	HDC current_dc = wglGetCurrentDC();
	if(current_dc == NULL || current_context == NULL) {
		Logging::fatal("[OpenCL] No OpenGL context active\n");
	}

	cl_context_properties properties[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_(),
		CL_WGL_HDC_KHR, (intptr_t) current_dc,
		CL_GL_CONTEXT_KHR, (intptr_t) current_context,
		0
	};
#else
	if(glXGetCurrentContext() == NULL) {
		Logging::fatal("[OpenCL] glXGetCurrentContex() return NULL. Make sure to create OpenGL context before create the CL-context\n");
	}
	cl_context_properties properties[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)(platform_)(),
		0
	};
#endif

	static CL_API_ENTRY cl_int (CL_API_CALL
	                            *clGetGLContextInfoKHR)(const cl_context_properties *properties,
	                                                    cl_gl_context_info param_name,
	                                                    size_t param_value_size,
	                                                    void *param_value,
	                                                    size_t *param_value_size_ret)=NULL;

	clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddress("clGetGLContextInfoKHR");

	cl_device_id devices[32];
	size_t deviceSize = 0;
	err = clGetGLContextInfoKHR(properties,
	                            CL_DEVICES_FOR_GL_CONTEXT_KHR,
	                            32 * sizeof(cl_device_id),
	                            devices,
	                            &deviceSize);

	if(deviceSize == 0) {
		Logging::fatal("[OpenCL] Interop not possible\n");
	}

	cl_bool image_support, available;
	size_t max_width, max_height;
	cl_uint num_cores, frequency;
	cl_device_type _type;
	std::string type;

	Logging::verbose("  - Available devices:\n");

	for(unsigned int i=0; i< (deviceSize / sizeof(cl_device_id)); ++i) {
		cl::Device device(devices[i]);
		device.getInfo(CL_DEVICE_VENDOR, &name);
		device.getInfo(CL_DEVICE_VERSION, &version);
		device.getInfo(CL_DEVICE_EXTENSIONS, &extensions);
		device.getInfo(CL_DEVICE_AVAILABLE, &available);
		device.getInfo(CL_DEVICE_IMAGE_SUPPORT, &image_support);
		device.getInfo(CL_DEVICE_IMAGE2D_MAX_WIDTH, &max_width);
		device.getInfo(CL_DEVICE_IMAGE2D_MAX_HEIGHT, &max_height);
		device.getInfo( CL_DEVICE_MAX_COMPUTE_UNITS , &num_cores);
		device.getInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY, &frequency);
		device.getInfo(CL_DEVICE_TYPE, &_type);

		switch(_type) {
		case CL_DEVICE_TYPE_GPU:
			type = "GPU";
			break;
		case CL_DEVICE_TYPE_CPU:
			type = "CPU";
			break;
		case CL_DEVICE_TYPE_ACCELERATOR:
			type =  "Accelerator";
			break;
		}

		Logging::verbose("    - Device (%p): %s %s (%s)\n"
		                 "      Cores: %u, Frequency: %u MHz\n"
		                 "      Available: %s\n"
		                 "      Image support: %s, max size: %lux%lu\n"
		                 "      Extensions: %s\n", (device)(),  name.c_str(), version.c_str(), type.c_str(), num_cores, frequency,available?"YES":"NO",image_support?"YES":"NO", max_width, max_height, extensions.c_str());
		devices_.push_back(device);
	}

	cl_device_id device_id;

	context_ = cl::Context(devices_, properties, cl_error_callback, nullptr, &err);

	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to create context: %s\n", errorString(err));
	}

	err = clGetGLContextInfoKHR(properties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(device_id), &device_id, NULL);
	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to get current device for context: %s\n", errorString(err));
	}

	context_device_ = cl::Device(device_id);

	context_device_.getInfo(CL_DEVICE_VENDOR, &name);
	context_device_.getInfo(CL_DEVICE_VERSION, &version);
	Logging::verbose("[OpenCL] Context Device (%p): %s %s\n",(context_device_)(),  name.c_str(), version.c_str());

	queue_ = cl::CommandQueue(context_, context_device_, 0, &err);

	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to create a command queue: %s\n", errorString(err));
	}
}

void cleanup(){

}

static void load_file(const std::string &filename, std::stringstream &data, const std::string &included_from) {
	Data * file = Data::open(filename);
	if(file == nullptr) {
		if(included_from.empty())
			Logging::fatal("[OpenCL] Kernel preprocessor error: File %s not found\n", filename.c_str());
		else
			Logging::fatal("[OpenCL] Kernel preprocessor error: File %s not found (included from %s)\n", filename.c_str(), included_from.c_str());
	}
	data << file;
	delete file;
	Logging::verbose("[OpenCL] Loaded %s\n", filename.c_str());
}

static std::string parse_file(
	const std::string &filename,
	std::set<std::string> included_files,
	const std::string &included_from
	) {

	std::pair<std::set<std::string>::iterator, bool> ret = included_files.insert(filename);
	if(ret.second == false) {
		Logging::fatal("[OpenCL] Kernel preprocessor error: Found include loop when including %s from %s\n", filename.c_str(), included_from.c_str());
	}

	std::stringstream raw_content, parsed_content;

	load_file(filename, raw_content, included_from);

	int linenr = 0;
	char buffer[2048];
	while(!raw_content.eof()) {
		++linenr;
		raw_content.getline(buffer, 2048);
		std::string line(buffer);
		if(line.find(PP_INCLUDE) == 0) {
			line = line.substr(line.find_first_not_of(" ", strlen(PP_INCLUDE)));

			size_t first_quote = line.find_first_of('"');
			if(first_quote != std::string::npos) {
				size_t end_quote = line.find_last_of('"');
				if(end_quote == std::string::npos || end_quote == first_quote) {
					Logging::fatal("%s\n[OpenCL] Kernel preprocessor error in %s:%d: Missing closing quote for #include command\n", buffer, filename.c_str(),  linenr);
				}
				//Trim quotes
				line = line.substr(first_quote+1, (end_quote - first_quote)-1);
			}

			//Include the file:
			char loc[256];
			sprintf(loc, "%s:%d", filename.c_str(), linenr);
			parsed_content << parse_file("/cl_programs/" + line, included_files, std::string(loc));
		} else {
			parsed_content << line << std::endl;
		}
	}
	return parsed_content.str();
}

cl::Program create_program(const std::string &source_file){
	auto it = cache.find(source_file);
	if(it != cache.end()) {
		return it->second;
	}

	Logging::verbose("Building CL program %s\n", source_file.c_str());

	std::string src = parse_file(source_file, std::set<std::string>(), "");

	cl_int err;
	cl::Program::Sources source(1, std::make_pair(src.c_str(), src.size()));

	cl::Program program = cl::Program(context_, source, &err);

	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Program creation error: %s\n", errorString(err));
	}

	err = program.build(devices_);


	std::string build_log;
	program.getBuildInfo(context_device_, CL_PROGRAM_BUILD_LOG, &build_log);

	if(build_log.size() > 1) { /* 1+ because nvidia likes to put a single LF in the log */
		Logging::error("[OpenCL] Build log: %s\n", build_log.c_str());
	}

	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to build program: %s\n", errorString(err));
	}

	cache[source_file] = program;

	return program;
}

cl::Kernel load_kernel(const cl::Program &program, const char * kernel_name){
	cl_int err;
	cl::Kernel kernel = cl::Kernel(program, kernel_name, &err);
	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to create kernel %s: %s\n", kernel_name, errorString(err));
	}

	return kernel;
}

cl::Buffer create_buffer(cl_mem_flags flags, size_t size){
	cl_int err;
	cl::Buffer buffer = cl::Buffer(context_, flags, size, NULL, &err);
	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to create buffer: %s\n", errorString(err));
	}
	return buffer;
}

cl::BufferGL create_gl_buffer(cl_mem_flags flags, GLuint gl_buffer){
	cl_int err;
	cl::BufferGL buffer(context_, flags, gl_buffer, &err);
	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] Failed to create gl buffer: %s\n", errorString(err));
	}
	return buffer;
}

cl::CommandQueue &queue() {
	return queue_;
}

cl_int finish(){
	return queue().finish();
}

cl_int flush(){
	return queue().flush();
}

static void CL_CALLBACK cl_error_callback(const char * errorinfo, const void * private_info_size, size_t cb, void * user_data) {
	Logging::fatal("[OpenCL] Got error callback: %s\n", errorinfo);
}

void check_error(const cl_int &err, const char* context){
	if(err != CL_SUCCESS) {
		Logging::fatal("[OpenCL] %s: %s\n", context, errorString(err));
	}
}

static const char* errorString(cl_int error){
	static const char* errorString[] = {
		"CL_SUCCESS",
		"CL_DEVICE_NOT_FOUND",
		"CL_DEVICE_NOT_AVAILABLE",
		"CL_COMPILER_NOT_AVAILABLE",
		"CL_MEM_OBJECT_ALLOCATION_FAILURE",
		"CL_OUT_OF_RESOURCES",
		"CL_OUT_OF_HOST_MEMORY",
		"CL_PROFILING_INFO_NOT_AVAILABLE",
		"CL_MEM_COPY_OVERLAP",
		"CL_IMAGE_FORMAT_MISMATCH",
		"CL_IMAGE_FORMAT_NOT_SUPPORTED",
		"CL_BUILD_PROGRAM_FAILURE",
		"CL_MAP_FAILURE",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"CL_INVALID_VALUE",
		"CL_INVALID_DEVICE_TYPE",
		"CL_INVALID_PLATFORM",
		"CL_INVALID_DEVICE",
		"CL_INVALID_CONTEXT",
		"CL_INVALID_QUEUE_PROPERTIES",
		"CL_INVALID_COMMAND_QUEUE",
		"CL_INVALID_HOST_PTR",
		"CL_INVALID_MEM_OBJECT",
		"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
		"CL_INVALID_IMAGE_SIZE",
		"CL_INVALID_SAMPLER",
		"CL_INVALID_BINARY",
		"CL_INVALID_BUILD_OPTIONS",
		"CL_INVALID_PROGRAM",
		"CL_INVALID_PROGRAM_EXECUTABLE",
		"CL_INVALID_KERNEL_NAME",
		"CL_INVALID_KERNEL_DEFINITION",
		"CL_INVALID_KERNEL",
		"CL_INVALID_ARG_INDEX",
		"CL_INVALID_ARG_VALUE",
		"CL_INVALID_ARG_SIZE",
		"CL_INVALID_KERNEL_ARGS",
		"CL_INVALID_WORK_DIMENSION",
		"CL_INVALID_WORK_GROUP_SIZE",
		"CL_INVALID_WORK_ITEM_SIZE",
		"CL_INVALID_GLOBAL_OFFSET",
		"CL_INVALID_EVENT_WAIT_LIST",
		"CL_INVALID_EVENT",
		"CL_INVALID_OPERATION",
		"CL_INVALID_GL_OBJECT",
		"CL_INVALID_BUFFER_SIZE",
		"CL_INVALID_MIP_LEVEL",
		"CL_INVALID_GLOBAL_WORK_SIZE",
	};
	static const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

	const int index = -error;
	return (index >= 0 && index < errorCount) ? errorString[index] : "";
}
}
