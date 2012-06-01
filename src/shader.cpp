#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shader.hpp"
#include "globals.hpp"
#include "light.hpp"
#include "utils.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <cassert>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PP_INCLUDE "#include"
#define VERT_SHADER_EXTENTION ".vert"
#define FRAG_SHADER_EXTENTION ".frag"
#define GEOM_SHADER_EXTENTION ".geom"

static enum Vendor {
	VENDOR_NVIDIA,
	VENDOR_ATI,
	VENDOR_UNKNOWN,
} vendor;

struct state_data {
	float time;
	float width;
	float height;
};

const char * Shader::global_uniform_names_[] = {
	"projectionViewMatrices",
	"modelMatrices",
	"Camera",
	"Material",
	"LightsData",
	"StateData",
};

const char * Shader::local_uniform_names_[] = {
	"texture1",
	"texture2",
	"texture_array1"
};


const GLsizeiptr Shader::global_uniform_buffer_sizes_[] = {
	sizeof(glm::mat4)*3,
	sizeof(glm::mat4)*2,
	sizeof(glm::vec3),
	sizeof(Shader::material_t),
	sizeof(Shader::lights_data_t),
	sizeof(struct state_data),
};

const GLenum Shader::global_uniform_usage_[] = {
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_DRAW
};

GLuint Shader::global_uniform_buffers_[Shader::NUM_GLOBAL_UNIFORMS];

Shader* Shader::current = nullptr;

void Shader::initialize() {
	/* determine vendor */
	const char* vendor_id = (const char*)glGetString(GL_VENDOR);
	vendor = VENDOR_UNKNOWN;
	if ( strcmp(vendor_id, "ATI Technologies Inc.") == 0 ){
		vendor = VENDOR_ATI;
	} else if ( strcmp(vendor_id, "NVIDIA Corporation") == 0 ){
		vendor = VENDOR_NVIDIA;
	} else {
		fprintf(stderr, "Unknown vendor ID: `%s'\n", vendor_id);
	}

	//Generate global uniforms:
	glGenBuffers(NUM_GLOBAL_UNIFORMS, (GLuint*)&global_uniform_buffers_);

	checkForGLErrors("Generate global uniform buffers");

	for( int i = 0; i < NUM_GLOBAL_UNIFORMS; ++i) {
		//Allocate memory in the buffer:A
		glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[i]);
		glBufferData(GL_UNIFORM_BUFFER, global_uniform_buffer_sizes_[i], NULL, global_uniform_usage_[i]);
		//Bind buffers to range
		glBindBufferRange(GL_UNIFORM_BUFFER, i, global_uniform_buffers_[i], 0, global_uniform_buffer_sizes_[i]);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("Bind and allocate global uniforms");
}

Shader::Shader(const std::string &name_, GLuint program) : name(name_), program_(program) {
	glGetProgramiv(program_, GL_ACTIVE_ATTRIBUTES, &num_attributes_);
	fprintf(verbose, "Created shader %s\n"
	        "  ID %d\n"
	        "  Active attrib: %d\n",
	        name_.c_str(), program, num_attributes_);

	glUseProgram(program_);
	init_uniforms();
	glUseProgram(0);
}

std::string Shader::parse_shader(const std::string &filename, filemap& included_files){
	parser_context ctx = {"", 0};
	return parse_shader(filename, included_files, ctx);
}

std::string Shader::parse_shader(const std::string &filename, filemap& included_files, const parser_context& parent){
	char buffer[2048];

	if ( included_files.find(filename) != included_files.end() ){
		return "";
	}

	parser_context parser = {filename != "" ? filename.c_str() : nullptr, 0};
	std::stringstream parsed_content;

	std::ifstream fp(filename.c_str());
	if ( fp.fail() ){
		if ( parent.filename ){
			fprintf(stderr, "%s:%d: %s: No such file or directory\n", parent.filename, parent.linenr, parser.filename);
		} else {
			fprintf(stderr, "%s: No such file or directory\n", filename.c_str());
		}
		abort();
	}

	fprintf(verbose, "Loading %s\n", parser.filename);

	const int file_id = included_files.size() + 1;
	included_files[filename] = file_id;

	if ( file_id > 1 ){
		parsed_content << "#line 0 " << file_id << std::endl;
	}

	while(!fp.eof()) {
		parser.linenr++;
		fp.getline(buffer, 2048);
		std::string line(buffer);

		//Parse preprocessor:
		if(line.find(PP_INCLUDE) == 0) {
			line = line.substr(line.find_first_not_of(" ", strlen(PP_INCLUDE)));

			size_t first_quote = line.find_first_of('"');
			if(first_quote != std::string::npos) {
				size_t end_quote = line.find_last_of('"');
				if(end_quote == std::string::npos || end_quote == first_quote) {
					fprintf(stderr, "%s\nShader preprocessor error in %s:%d: Missing closing quote for #include command\n", buffer, parser.filename, parser.linenr);
					abort();
				}
				//Trim quotes
				line = line.substr(first_quote+1, (end_quote - first_quote)-1);
			}

			//Include the file:
			parsed_content << parse_shader(PATH_SHADERS+line, included_files, parser);
			parsed_content << "#line " << parser.linenr << " " << file_id << std::endl;
			continue;
		}

		parsed_content << line << std::endl;
	}

	return parsed_content.str();
}

GLuint Shader::load_shader(GLenum eShaderType, const std::string &strFilename) {
	GLint gl_tmp;

	filemap included_files;
	std::string source = parse_shader(strFilename, included_files);

	GLuint shader = glCreateShader(eShaderType);

	const char * source_ptr = source.c_str();

	glShaderSource(shader, 1,&source_ptr , NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &gl_tmp);

	if(!gl_tmp) {
		char buffer[2048];

		fprintf(stderr, "Shader compile error (%s). Preproccessed source: \n", strFilename.c_str());
		std::stringstream code(source);
		int linenr=0;
		while(!code.eof()) {
			code.getline(buffer, 2048);
			fprintf(stderr, "%d %s\n", ++linenr, buffer);
		}
		glGetShaderInfoLog(shader, 2048, NULL, buffer);
		fprintf(stderr, "Error in shader %s:\n%s\n",strFilename.c_str(),  buffer);
		abort();
	}
	return shader;
}

GLuint Shader::create_program(const std::string &shader_name, const std::vector<GLuint> &shaderList) {
	GLint gl_tmp;
	GLuint program = glCreateProgram();
	checkForGLErrors("glCreateProgram");

	for(GLuint shader : shaderList) {
		glAttachShader(program, shader);
		checkForGLErrors("glAttachShader");
	}

	glLinkProgram(program);
	checkForGLErrors("glLinkProgram");

	std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

	glGetProgramiv(program, GL_LINK_STATUS, &gl_tmp);

	if(!gl_tmp) {
		char buffer[2048];
		glGetProgramInfoLog(program, 2048, NULL, buffer);
		fprintf(stderr, "Link error in shader %s: %s\n", shader_name.c_str(), buffer);
		abort();
	}

#ifdef VALIDATE_SHADERS
	glValidateProgram(program);

	glGetProgramiv(program, GL_VALIDATE_STATUS, &gl_tmp);

	if(!gl_tmp) {
		char buffer[2048];
		glGetProgramInfoLog(program, 2048, NULL, buffer);
		fprintf(stderr, "Validate error in shader %s: %s\n", shader_name.c_str(), buffer);
		abort();
	}

#endif

	return program;
}

Shader * Shader::create_shader(std::string base_name) {
	fprintf(verbose, "Compiling shader %s\n", base_name.c_str());

	const std::string vs = PATH_SHADERS+base_name+VERT_SHADER_EXTENTION;
	const std::string gs = PATH_SHADERS+base_name+GEOM_SHADER_EXTENTION;
	const std::string fs = PATH_SHADERS+base_name+FRAG_SHADER_EXTENTION;

	std::vector<GLuint> shader_list;

	//Load shaders:
	shader_list.push_back(load_shader(GL_VERTEX_SHADER,   file_exists(vs) ? vs : PATH_SHADERS"default.vert"));
	shader_list.push_back(load_shader(GL_FRAGMENT_SHADER, file_exists(fs) ? fs : PATH_SHADERS"default.frag"));
	if ( file_exists(gs) ){
		shader_list.push_back(load_shader(GL_GEOMETRY_SHADER, gs));
	}

	return new Shader(base_name, create_program(base_name, shader_list));
}

void Shader::init_uniforms() {
	const int tmp = program_;
	for(int i=0; i<NUM_LOCAL_UNIFORMS; ++i) {
		local_uniform_locations_[i] = glGetUniformLocation(program_, local_uniform_names_[i]);
		checkForGLErrors((std::string("load uniform ")+local_uniform_names_[i]+" from shader "+name).c_str());
	}

	/* setup samplers */
	if ( local_uniform_locations_[UNIFORM_TEXTURE1] != -1 )	glUniform1i(local_uniform_locations_[UNIFORM_TEXTURE1], 0);
	if ( local_uniform_locations_[UNIFORM_TEXTURE2] != -1 )	glUniform1i(local_uniform_locations_[UNIFORM_TEXTURE2], 1);
	if ( local_uniform_locations_[UNIFORM_TEXTURE_ARRAY1] != -1 )	glUniform1i(local_uniform_locations_[UNIFORM_TEXTURE_ARRAY1], 0);

	checkForGLErrors("Upload texture locations");

	//Bind global uniforms to blocks:
	for(int i = 0; i < NUM_GLOBAL_UNIFORMS; ++i) {
		assert(tmp == program_);
		global_uniform_block_index_[i] = glGetUniformBlockIndex(program_, global_uniform_names_[i]);
		if(global_uniform_block_index_[i] != -1) {
			glUniformBlockBinding(program_, global_uniform_block_index_[i], i);
		} else {
			fprintf(verbose, "Not binding global uniform %s, probably not used\n", global_uniform_names_[i]);
		}
	}

	checkForGLErrors("Bind global uniforms to buffers");
}

void Shader::bind() {
	if ( this == current ){
		return; /* do nothing */
	} else if ( current ){
		unbind();
	}

	glUseProgram(program_);
	checkForGLErrors("Bind shader");

	for(int i=0; i<num_attributes_; ++i) {
		glEnableVertexAttribArray(i);
		checkForGLErrors("Enable vertex attrib");
	}

	current = this;
}

void Shader::unbind() {
	if ( !current ){
		fprintf(stderr, "Shader nesting problem, no shader is bound.\n");
		abort();
	}

	for( int i = 0; i < current->num_attributes_; ++i ) {
		glDisableVertexAttribArray(i);
		checkForGLErrors("Shader::unbind glDisableVertexAttribArray");
	}

	glUseProgram(0);
	checkForGLErrors("Shader::unbind");
	current = nullptr;
}

void Shader::upload_lights(const Shader::lights_data_t &lights) {
	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_LIGHTS]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights_data_t), &lights);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("upload lights");
}

void Shader::upload_camera_position(const Camera &camera) {
	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_CAMERA]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec3), glm::value_ptr(camera.position()));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("upload camera position");
}

void Shader::upload_projection_view_matrices(
	const glm::mat4 &projection,
	const glm::mat4 &view
	) {
	glm::mat4 projection_view = projection * view;

	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_PROJECTION_VIEW_MATRICES]);

	static const size_t s = sizeof(glm::mat4);
	glBufferSubData(GL_UNIFORM_BUFFER, 0*s, s, glm::value_ptr(projection_view));
	glBufferSubData(GL_UNIFORM_BUFFER, 1*s, s, glm::value_ptr(projection));
	glBufferSubData(GL_UNIFORM_BUFFER, 2*s, s, glm::value_ptr(view));

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("upload projection view matrices");
}

void Shader::upload_model_matrix(const glm::mat4 &model) {
	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_MODEL_MATRICES]);

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(model));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(glm::transpose(glm::inverse(model))));

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("upload model matrices");
}

void Shader::upload_material(const Shader::material_t &material) {
	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_MATERIAL]);

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(material_t), &material);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("upload material");
}

void Shader::upload_blank_material() {
	material_t m;
	m.shininess = 0;
	m.specular = glm::vec4(0,0,0,0);
	m.diffuse = glm::vec4(1.f,1.f,1.f,1.f);
	m.ambient = glm::vec4(1.f,1.f,1.f,1.f);
	m.emission = glm::vec4(0,0,0,0);

	upload_material(m);
}

void Shader::upload_camera(const Camera &camera) {
	upload_camera_position(camera);
	upload_projection_view_matrices(camera.projection_matrix(), camera.view_matrix());
}

void Shader::upload_state(const glm::ivec2& size){
	struct state_data data = {
		global_time.get(),
		(float)size.x,
		(float)size.y
	};

	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_STATE]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(struct state_data), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("Shader::upload_state");
}

const GLint Shader::num_attributes() const { return num_attributes_; }
