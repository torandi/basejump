#ifndef SHADER_H
#define SHADER_H

#include "light.hpp"
#include "camera.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

//Muste be same as in uniforms.glsl
#define MAX_NUM_LIGHTS 4

class Shader {
public:

	static Shader * create_shader(std::string base_name);

	//Must be called before first call to create_shader
	static void initialize();

	enum global_uniforms_t {
		UNIFORM_PROJECTION_VIEW_MATRICES=0,
		UNIFORM_MODEL_MATRICES,

		UNIFORM_CAMERA,

		UNIFORM_MATERIAL,

		UNIFORM_LIGHTS,
		UNIFORM_STATE,
		NUM_GLOBAL_UNIFORMS
	};

	enum local_uniforms_t {
		UNIFORM_TEXTURE1=0,
		UNIFORM_TEXTURE2,
		UNIFORM_TEXTURE_ARRAY1,

		NUM_LOCAL_UNIFORMS
	};

	enum attribute_t {
		ATTR_POSITION=0,
		ATTR_TEXCOORD,
		ATTR_NORMAL,
		ATTR_TANGENT,
		ATTR_BITANGENT,
		ATTR_COLOR
	};

	struct material_t {
		float shininess;
		float padding[3];
		glm::vec4 specular;
		glm::vec4 diffuse;
		glm::vec4 ambient;
		glm::vec4 emission;
	};

	struct lights_data_t {
		int num_lights;
		float padding[3];
		glm::vec3 ambient_intensity;
		float padding_2;
		Light::shader_light_t lights[MAX_NUM_LIGHTS];
	};

private:

	Shader(const std::string &name_, GLuint program);

	static const char *global_uniform_names_[];
	static const char *local_uniform_names_[];
	static const GLsizeiptr global_uniform_buffer_sizes_[];
	static const GLenum global_uniform_usage_[];

	static GLuint load_shader(GLenum eShaderType, const std::string &strFilename);
	static GLuint create_program(const std::string &shader_name, const std::vector<GLuint> &shaderList);

	typedef std::pair<std::string, unsigned int> filepair;
	typedef std::map<std::string, unsigned int> filemap;
	typedef struct { const char* filename; unsigned int linenr; } parser_context;
	static std::string parse_shader(const std::string &filename, filemap& included_files);
	static std::string parse_shader(const std::string &filename, filemap& included_files, const parser_context& ctx);

	static void print_log(GLint object, const filemap& included_files, const char* fmt, ...) __attribute__((format(printf, 3,4)));

	GLint local_uniform_locations_[NUM_LOCAL_UNIFORMS];
	GLint global_uniform_block_index_[NUM_GLOBAL_UNIFORMS];
	static GLuint global_uniform_buffers_[NUM_GLOBAL_UNIFORMS];

	void init_uniforms();

	const GLuint program_;

	GLint num_attributes_;

	static Shader* current; /* current bound shader or null */

public:

	std::string name;

	Shader &operator= (const Shader &shader);

	void bind();
	static void unbind();

	const GLint num_attributes() const;

	/**
	 * Upload lights
	 */
	static void upload_lights(const lights_data_t &lights);

	/*
	 * Uploads the camera position
	 */
	static void upload_camera_position(const Camera &camera);

	/**
	 * Upload the material
	 */
	static void upload_material(const material_t &material);

	/**
	 * Upload projection and view matrices
	 */
	static void upload_projection_view_matrices(
		const glm::mat4 &projection,
		const glm::mat4 &view
		);

	/**
	 * Uploads the model and normal matrix
	 */
	static void upload_model_matrix( const glm::mat4 &model);

	/**
	 * Uploads camera position, and projection and view matrix
	 */
	static void upload_camera(const Camera &camera);

	/**
	 * Upload current state.
	 */
	static void upload_state(const glm::ivec2& size);

	/**
	 * Upload white material
	 */
	static void upload_blank_material();
};
#endif
