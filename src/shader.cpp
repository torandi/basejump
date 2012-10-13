#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shader.hpp"
#include "globals.hpp"
#include "light.hpp"
#include "logging.hpp"
#include "utils.hpp"
#include "lights_data.hpp"
#include "data.hpp"

#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <memory>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PP_INCLUDE "#include"
#define VERT_SHADER_EXTENTION ".vert"
#define FRAG_SHADER_EXTENTION ".frag"
#define GEOM_SHADER_EXTENTION ".geom"

struct resolution_data {
	float width;
	float height;
};

struct frame_data {
	float time;
};

struct UBO {
	const char* name;
	size_t size;
	GLenum usage;
};

static const struct UBO ubo[Shader::NUM_GLOBAL_UNIFORMS] = {
	{"projectionViewMatrices", sizeof(glm::mat4)*3,           GL_DYNAMIC_DRAW},
	{"modelMatrices",          sizeof(glm::mat4)*2,           GL_DYNAMIC_DRAW},
	{"Camera",                 sizeof(glm::vec3),             GL_DYNAMIC_DRAW},
	{"Material",               sizeof(Shader::material_t),    GL_DYNAMIC_DRAW},
	{"LightsData",             sizeof(Shader::lights_data_t), GL_DYNAMIC_DRAW},
	{"Resolution",             sizeof(struct resolution_data),GL_DYNAMIC_DRAW},
	{"Frame",                  sizeof(struct frame_data),     GL_DYNAMIC_DRAW},
	{"Fog",                    sizeof(Shader::fog_t),         GL_DYNAMIC_DRAW},
};

static GLuint global_uniform_buffers_[Shader::NUM_GLOBAL_UNIFORMS];
const Shader* Shader::current = nullptr;

typedef std::map<std::string, Shader*> ShaderMap;
typedef std::pair<std::string, Shader*> ShaderPair;
static ShaderMap shadercache;
static bool initialized = false;

void Shader::initialize() {
	//Generate global uniforms:
	glGenBuffers(NUM_GLOBAL_UNIFORMS, global_uniform_buffers_);

	checkForGLErrors("Generate global uniform buffers");

	for( int i = 0; i < NUM_GLOBAL_UNIFORMS; ++i) {
		//Allocate memory in the buffer:
		glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[i]);
		glBufferData(GL_UNIFORM_BUFFER, ubo[i].size, NULL, ubo[i].usage);
		//Bind buffers to range
		glBindBufferRange(GL_UNIFORM_BUFFER, i, global_uniform_buffers_[i], 0, ubo[i].size);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("Bind and allocate global uniforms");


	/* Enable all attribs for Shader::vertex_x */
	for ( int i = 0; i < NUM_ATTR; ++i ) {
		glEnableVertexAttribArray(i);
	}

	initialized = true;
}

void Shader::cleanup(){
	glDeleteBuffers(NUM_GLOBAL_UNIFORMS, global_uniform_buffers_);

	/* remove all shaders */
	for ( ShaderPair p: shadercache ){
		delete p.second;
	}
}

Shader::Shader(const std::string &name_, GLuint program) :
	program_(program)
	,	name(name_) {
	glGetProgramiv(program_, GL_ACTIVE_ATTRIBUTES, &num_attributes_);
	Logging::verbose("Created shader %s\n"
	                 "  - ID %d\n"
	                 "  - Active attrib: %d\n",
	                 name_.c_str(), program, num_attributes_);

	bind();
	init_uniforms();
}

Shader::~Shader(){
	glDeleteProgram(program_);
}

void Shader::release(){
	delete this;
}

void Shader::load_file(const std::string &filename, std::stringstream &shaderData, std::string included_from) {
	Data * file = Data::open(filename);
	if(file == nullptr) {
		if(included_from.empty()) {
			Logging::fatal("Shader preprocessor error: File %s not found\n", filename.c_str());
		} else {
			Logging::fatal("Shader preprocessor error: File %s not found (included from %s)\n", filename.c_str(), included_from.c_str());
		}
	}
	shaderData << file;
	delete file;
	Logging::verbose("    - Source: \"%s\"\n", filename.c_str());
}

std::string Shader::parse_shader(
	const std::string &filename,
	std::set<std::string> included_files,
	std::string included_from
	) {
	char buffer[2048];

	std::pair<std::set<std::string>::iterator, bool> ret = included_files.insert(filename);
	if(ret.second == false) {
		Logging::fatal("Shader preprocessor error: Found include loop when including %s from %s\n", filename.c_str(), included_from.c_str());
	}

	std::stringstream raw_content;
	load_file(filename, raw_content, included_from);
	std::stringstream parsed_content;
	int linenr = 0;
	while(!raw_content.eof()) {
		++linenr;
		raw_content.getline(buffer, 2048);
		std::string line(buffer);
		//Parse preprocessor:
		if(line.find(PP_INCLUDE) == 0) {
			line = line.substr(line.find_first_not_of(" ", strlen(PP_INCLUDE)));

			size_t first_quote = line.find_first_of('"');
			if(first_quote != std::string::npos) {
				size_t end_quote = line.find_last_of('"');
				if(end_quote == std::string::npos || end_quote == first_quote) {
					Logging::fatal("%s\nShader preprocessor error in %s:%d: Missing closing quote for #include command\n", buffer, filename.c_str(),  linenr);
				}
				//Trim quotes
				line = line.substr(first_quote+1, (end_quote - first_quote)-1);
			}

			//Include the file:
			char loc[256];
			snprintf(loc, sizeof(loc), "%s:%d", filename.c_str(), linenr);
			parsed_content << parse_shader("/shaders/" + line, included_files, std::string(loc));
		} else {
			parsed_content << line << std::endl;
		}
	}
	return parsed_content.str();
}

static const char* str_shader_type(GLenum type){
	switch ( type ){
	case GL_VERTEX_SHADER: return "vertex";
	case GL_FRAGMENT_SHADER: return "fragment";
	case GL_GEOMETRY_SHADER: return "geometry";
	case GL_TESS_EVALUATION_SHADER: return "tesselation evaluation";
	case GL_TESS_CONTROL_SHADER: return "tesselation control";
	default: return "unknown";
	}
}

GLuint Shader::load_shader(GLenum eShaderType, const std::string &strFilename) {
	const GLuint shader = glCreateShader(eShaderType);
	Logging::verbose("  - Compiling %s shader (shader_%d)\n", str_shader_type(eShaderType), shader);

	const std::string source = parse_shader(strFilename);
	const char* source_ptr = source.c_str();
	glShaderSource(shader, 1,&source_ptr , NULL);
	glCompileShader(shader);

	GLint compile_status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

	if ( compile_status == GL_FALSE ) {
		char buffer[2048];

		Logging::error("Shader compile error (%s). Preproccessed source: \n", strFilename.c_str());
		std::stringstream code(source);
		int linenr=0;
		while(!code.eof()) {
			code.getline(buffer, 2048);
			Logging::error("%d %s\n", ++linenr, buffer);
		}
		glGetShaderInfoLog(shader, 2048, NULL, buffer);
		Logging::error("Error in shader %s: %s\n", strFilename.c_str(),  buffer);
		checkForGLErrors("shader");
		abort();
	}

	return shader;
}

GLuint Shader::create_program(const std::string &shader_name, const std::vector<GLuint> &shaderList) {
	GLint gl_tmp;
	GLuint program = glCreateProgram();
	Logging::verbose("  - Linking program (program_%d)\n", program);

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
		Logging::fatal("Link error in shader %s: %s\n", shader_name.c_str(), buffer);
	}

#ifdef VALIDATE_SHADERS
	glValidateProgram(program);

	glGetProgramiv(program, GL_VALIDATE_STATUS, &gl_tmp);

	if(!gl_tmp) {
		char buffer[2048];
		glGetProgramInfoLog(program, 2048, NULL, buffer);
		Logging::fatal("Validate error in shader %s: %s\n", shader_name.c_str(), buffer);
	}

#endif

	return program;
}

Shader* Shader::create_shader(const std::string& base_name, bool cache) {
	Logging::verbose("Loading shader \"%s\"\n", base_name.c_str());

	/* sanity check */
	if ( !initialized ){
		Logging::fatal("Shader::create_shader(..) called before Shader::initialize()\n");
	}

	const auto it = shadercache.find(base_name);
	if ( cache && it != shadercache.end() ){
		Logging::verbose("  - Cache hit.\n");
		return it->second;
	}

	Logging::verbose("  - Cache miss.\n");
	Logging::verbose("Compiling shader \"%s\"\n", base_name.c_str());

	const std::string vs = base_name+VERT_SHADER_EXTENTION;
	const std::string gs = base_name+GEOM_SHADER_EXTENTION;
	const std::string fs = base_name+FRAG_SHADER_EXTENTION;

	std::vector<GLuint> shader_list;

	//Load shaders:
	shader_list.push_back(load_shader(GL_VERTEX_SHADER,   Data::file_exists(vs) ? vs : "/shaders/default.vert"));
	shader_list.push_back(load_shader(GL_FRAGMENT_SHADER, Data::file_exists(fs) ? fs : "/shaders/default.frag"));
	if ( Data::file_exists(gs) ){
		shader_list.push_back(load_shader(GL_GEOMETRY_SHADER, gs));
	}

	Shader* shader = new Shader(base_name, create_program(base_name, shader_list));
	shadercache[base_name] = shader;
	return shader;
}

void Shader::preload(const std::string& base_name){
	create_shader(base_name);
}

void Shader::usage_report(FILE* dst){
	fprintf(dst, "Shader usage\n"
	             "============\n");

	for ( ShaderPair p: shadercache ){
		const GLuint id = p.second->program_;

		GLint num_attached;
		glGetProgramiv(id, GL_ATTACHED_SHADERS, &num_attached);

		std::unique_ptr<GLuint[]> shader(new GLuint[num_attached]);
		glGetAttachedShaders(id, num_attached, nullptr, shader.get());

		for ( int i = 0; i < num_attached; i++ ){
			static const std::string extlut[] = { VERT_SHADER_EXTENTION, FRAG_SHADER_EXTENTION, GEOM_SHADER_EXTENTION, ".<unknown>" };
			unsigned int extension;

			GLint type;
			glGetShaderiv(shader[i], GL_SHADER_TYPE, &type);
			switch ( type ){
			case GL_VERTEX_SHADER:   extension = 0; break;
			case GL_FRAGMENT_SHADER: extension = 1; break;
			case GL_GEOMETRY_SHADER: extension = 2; break;
			default:                 extension = 3; break;
			}

			std::string filename = p.first+extlut[extension];
			if ( !Data::file_exists(filename) ) filename = "/shaders/default"+extlut[extension];

			fprintf(dst, "%s:%s\n", p.first.c_str(), filename.c_str());
		}
	}
}

void Shader::init_uniforms() {
	//Bind global uniforms to blocks:
	for(int i = 0; i < NUM_GLOBAL_UNIFORMS; ++i) {
		global_uniform_block_index_[i] = glGetUniformBlockIndex(program_, ubo[i].name);
		if(global_uniform_block_index_[i] != -1) {
			glUniformBlockBinding(program_, global_uniform_block_index_[i], i);
		} else {
			Logging::debug("Not binding global uniform %s, probably not used\n", ubo[i].name);
		}
	}

	checkForGLErrors("Bind global uniforms to buffers");
}

void Shader::bind() const {
	if ( this == current ){
		return; /* do nothing */
	} else if ( current ){
		unbind();
	}

	glUseProgram(program_);
	checkForGLErrors("Bind shader");
	current = this;
}

void Shader::unbind() {
	if ( !current ){
		Logging::fatal("Shader nesting problem, no shader is bound.\n");
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

void Shader::upload_lights(LightsData &lights) {
	upload_lights(lights.shader_data());
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

void Shader::upload_resolution(const glm::ivec2& size){
	struct resolution_data data = {
		(float)size.x,
		(float)size.y,
	};

	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_RESOLUTION]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(struct resolution_data), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("Shader::upload_resolution");
}

void Shader::upload_frameinfo(float t){
	struct frame_data data = {
		t,
	};

	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_FRAMEINFO]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(struct frame_data), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	checkForGLErrors("Shader::upload_frameinfo");
}

void Shader::upload_fog(const fog_t &fog) {
	glBindBuffer(GL_UNIFORM_BUFFER, global_uniform_buffers_[UNIFORM_FOG]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(fog_t), &fog);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GLint Shader::num_attributes() const { return num_attributes_; }

GLint Shader::uniform_location(const char * uniform_name) const{
	GLint l = glGetUniformLocation(program_, uniform_name);
	checkForGLErrors((std::string("uniform_location")+std::string(uniform_name)+" from shader "+name).c_str());
	return l;
}

void Shader::push_vertex_attribs(int offset) {
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	for ( int i = offset; i < NUM_ATTR; ++i ) {
		glDisableVertexAttribArray(i);
	}
}

void Shader::pop_vertex_attribs() {
	glPopClientAttrib();
}
