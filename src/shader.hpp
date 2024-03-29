#ifndef SHADER_H
#define SHADER_H

#include "camera.hpp"
#include "light.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <set>

//Must be same as in uniforms.glsl
#define MAX_NUM_LIGHTS 4

class LightsData;

/**
 * GLSL shader wrapper class.
 * Actual GLSL code is located in `shaders` subdirectory.
 *
 * Typical usage:
 *   Shader* shader = Shader::create_shader("foobar");
 *   ...
 *   Shader::upload_camera(..) or  Shader::upload_projection_view_matrices(..);
 *   Shader::upload_model_matrix(..);
 *   ...
 *   shader->bind();
 *   model->render();
 */
class Shader {
public:

	/**
	 * Create a new shader, or if the shader is already loaded the same instance
	 * is retrieved.
	 *
	 * @param base_name is used to build the filename: base_name + EXTENSION.
	 * @param cache If true it uses caching. Uncached shaders should be removed
	 *              with Shader::release(). Normally you do _not_ want to use
	 *              uncached shaders.
	 * @return A borrowed pointer. Do not delete yourself, call Shader::cleanup()
	 *         to remove all shaders.
	 */
	static Shader* create_shader(const std::string& base_name, bool cache = true);

	/**
	 * Preload shader into GPU memory.
	 */
	static void preload(const std::string& base_name);

	/**
	 * Write a usage report to dst with details about which shaders has been
	 * loaded and the files it depends.
	 */
	static void usage_report(FILE* dst = stderr);

	/**
	 * Initialize shader engine.
	 * Must be called before first call to create_shader.
	 */
	static void initialize();

	/**
	 * Release all resources owned by shader engine. No shaders is valid after call to this function.
	 */
	static void cleanup();

	enum global_uniforms_t {
		UNIFORM_PROJECTION_VIEW_MATRICES=0,
		UNIFORM_MODEL_MATRICES,

		UNIFORM_CAMERA,

		UNIFORM_MATERIAL,

		UNIFORM_LIGHTS,
		UNIFORM_RESOLUTION,
		UNIFORM_FRAMEINFO,
		UNIFORM_FOG,
		UNIFORM_SKY,
		NUM_GLOBAL_UNIFORMS
	};

	enum TextureUnit {
		TEXTURE_2D_0 = GL_TEXTURE0,
		TEXTURE_2D_1,
		TEXTURE_2D_2,
		TEXTURE_2D_3,
		TEXTURE_2D_4,
		TEXTURE_2D_5,
		TEXTURE_2D_6,
		TEXTURE_2D_7,
		TEXTURE_ARRAY_0,
		TEXTURE_ARRAY_1,
		TEXTURE_ARRAY_2,
		TEXTURE_ARRAY_3,
		TEXTURE_CUBEMAP_0,
		TEXTURE_CUBEMAP_1,
		TEXTURE_CUBEMAP_2,
		TEXTURE_CUBEMAP_3,
		TEXTURE_SHADOWMAP_0,
		TEXTURE_SHADOWMAP_1,
		TEXTURE_SHADOWMAP_2,
		TEXTURE_SHADOWMAP_3,

		/* Aliases */
		TEXTURE_COLORMAP = TEXTURE_2D_0,
		TEXTURE_NORMALMAP = TEXTURE_2D_1,
		TEXTURE_SPECULARMAP = TEXTURE_2D_2,
		TEXTURE_ALPHAMAP = TEXTURE_2D_3,
		TEXTURE_DEPTHMAP = TEXTURE_2D_7,
		TEXTURE_BLOOM = TEXTURE_2D_6,

		/* Blending aliases */
		TEXTURE_BLEND_0 = TEXTURE_2D_0,
		TEXTURE_BLEND_1 = TEXTURE_2D_1,
		TEXTURE_BLEND_2 = TEXTURE_2D_2,
		TEXTURE_BLEND_3 = TEXTURE_2D_3,
		TEXTURE_BLEND_S = TEXTURE_2D_4,
	};

	enum attribute_t {
		ATTR_POSITION=0,
		ATTR_TEXCOORD,
		ATTR_NORMAL,
		ATTR_TANGENT,
		ATTR_BITANGENT,
		ATTR_COLOR,

		NUM_ATTR,
	};

	struct vertex {
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
		glm::vec4 color;
	};
	typedef struct vertex vertex_t;

	struct material_t {
		float shininess;
		glm::vec4 __ALIGNED__(16) diffuse;
		glm::vec4 specular;
		glm::vec4 ambient;
		glm::vec4 emission;
	};

	struct lights_data_t {
		int num_lights;
		float padding[3];
		glm::vec3 ambient_intensity;
		float padding_2;
		Light lights[MAX_NUM_LIGHTS];
	};

	struct __ALIGNED__(16) fog_t {
		glm::vec4 color;
		float density;
	};

	struct sky_data_t {
		glm::vec4 zenit_color;
		glm::vec4 horizont_color;
		glm::vec4 sun_color;
		glm::vec4 sun_aura_color;
		glm::vec4 sun_position;
		float sun_radius;
		float sun_aura_scale;
		float lerp_size;
		float lerp_offset;
	};

	/**
	 * Used *ONLY* for uncached shaders. Will delete the pointer. Shader is not
	 * valid for any use after this call and user should pointer to nullptr.
	 */
	void release();

	struct camera_t {
		glm::vec3 pos;
		float near;
		float far;
	};

	private:

	Shader(const std::string &name_, GLuint program);
	~Shader();

	static GLuint load_shader(GLenum eShaderType, const std::string &strFilename);
	static GLuint create_program(const std::string &shader_name, const std::vector<GLuint> &shaderList);

	static void load_file(const std::string &filename, std::stringstream &shaderData, std::string included_from);
	static std::string parse_shader(const std::string &filename, std::set<std::string> included_files=std::set<std::string>(), std::string included_from="");

	GLint global_uniform_block_index_[NUM_GLOBAL_UNIFORMS];

	void init_uniforms();

	const GLuint program_;

	GLint num_attributes_;

	static const Shader* current; /* current bound shader or null */

public:

	const std::string name;

	Shader &operator= (const Shader &shader);

	void bind() const;
	static void unbind();

	GLint num_attributes() const;

	/***************************
	 * Uniform helpers
	 */

	/*
	 * Get uniform location in shader from name
	 */
	GLint uniform_location(const char * uniform_name) const;

	/*
	 * Uniform uploaders
	 */

	void uniform_upload(GLint uniform, const glm::ivec4 v) const;
	void uniform_upload(GLint uniform, const glm::ivec3 v) const;
	void uniform_upload(GLint uniform, const glm::ivec2 v) const;
	void uniform_upload(GLint uniform, const glm::vec4 v) const;
	void uniform_upload(GLint uniform, const glm::vec3 v) const;
	void uniform_upload(GLint uniform, const glm::vec2 v) const;

	void uniform_upload(GLint uniform, float f) const;
	void uniform_upload(GLint uniform, int i) const;

	void uniform_upload(GLint uniform, const Color &color) const;

	/**
	 * Upload lights
	 */
	static void upload_lights(const lights_data_t &lights);
	static void upload_lights(LightsData &lights);


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
	 * Uploads camera data, and projection and view matrix
	 */
	static void upload_camera(const Camera &camera);

	/**
	 * Upload current resolution (of window or RenderTarget).
	 */
	static void upload_resolution(const glm::ivec2& size);

	/**
	 * Upload per-frame into such as time.
	 */
	static void upload_frameinfo(float t);

	/**
	 * Upload fog data
	 */
	static void upload_fog(const fog_t &fog);

	/**
	 * Upload sky data
	 */
	static void upload_sky(const sky_data_t  &sky);

	/**
	 * Upload white material
	 */
	static void upload_blank_material();

	/**
	 * Push vertex attribs and disable all.
	 *
	 * @param Disable attrib from (inclusive) offset.
	 */
	static void push_vertex_attribs(int offset = 0);

	/**
	 * Restore all vertex attribs
	 */
	static void pop_vertex_attribs();
};
#endif
