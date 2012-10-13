#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glm/gtc/type_ptr.hpp>

#include "quad.hpp"
#include "scene.hpp"
#include "globals.hpp"
#include "light.hpp"
#include "render_object.hpp"
#include "particle_system.hpp"
#include "shader.hpp"
#include "timetable.hpp"
#include "skybox.hpp"
#include "material.hpp"
#include <memory>

static glm::vec3 offset(0,1.5,0);

static float arc[][3] = {
	{0.000000f,  3.966543f, 0.00f},
	{1.050000f,  3.956699f, 0.05f},
	{2.189144f,  3.951092f, 0.10f},
	{3.407661f,  3.878636f, 0.15f},
	{4.586273f,  3.562828f, 0.20f},
	{5.609610f,  2.898265f, 0.25f},
	{6.377500f,  1.950000f, 0.30f},
	{6.814777f,  0.810856f, 0.35f},
	{6.878636f, -0.407661f, 0.40f},
	{6.562828f, -1.586273f, 0.45f},
	{5.898265f, -2.609610f, 0.50f},
	{4.950000f, -3.377500f, 0.55f},
	{3.810856f, -3.814777f, 0.60f},
	{2.592339f, -3.950274f, 0.65f},
	{1.413727f, -3.954323f, 0.70f},
	{0.000000f, -3.962843f, 0.75f},
	{0.000000f, -1.950001f, 0.80f},
	{0.000000f, -0.810856f, 0.85f},
	{0.000000f,  0.407661f, 0.90f},
	{0.000000f,  1.586274f, 0.95f},
	{0.000000f,  3.966543f, 1.00f},
};
static const unsigned int per_segment = sizeof(arc) / (3*sizeof(float));

static float noise_x(float s, unsigned int i){
	return sinf(s * 28.0f + static_cast<float>(i % (per_segment-1))) * 0.2f + sinf(s * 943.0f) * 0.7f;
}

static float noise_y(float s, unsigned int i){
	return sinf(s * 138.0f + static_cast<float>(i % (per_segment-1))) * 0.2f + sinf(s * 24.0f) * 0.3f;
}

static float noise_z(float s, unsigned int i){
	return 0.0f;
}

class TunnelScene: public Scene {
public:
	TunnelScene(const glm::ivec2& size)
		: Scene(size)
		, camera(75.f, size, 0.1f, 100.0f)
		, position("scene/tunnel.txt")
		, shader(nullptr) {

		lights.ambient_intensity() = glm::vec3(1.0f);
		lights.num_lights() = 0;

		camera.set_position(glm::vec3(-25,3,-25));
		camera.look_at(glm::vec3(0,0,0));

		shader = Shader::create_shader("/shaders/normal");

		const size_t num_segment = 8;
		const size_t num_vertices = num_segment * per_segment;
		num_indices = (per_segment-1)*4*(num_segment-1);
		std::unique_ptr<Shader::vertex_t[]> vertices(new Shader::vertex_t[num_vertices]);
		std::unique_ptr<unsigned int[]> indices(new unsigned int[num_indices]);

		/* generate vertices */
		for ( unsigned int seg = 0; seg < num_segment; seg++ ){
			for ( unsigned int c = 0; c < per_segment; c++ ){
				const float s = static_cast<float>(seg);
				const unsigned int i = seg * per_segment + c;

				vertices[i].pos.x = arc[c][1]  + noise_x(s, i);
				vertices[i].pos.y = arc[c][0]  + noise_y(s, i);
				vertices[i].pos.z = s * 5.0f + noise_z(s, i);
				vertices[i].uv.x = arc[c][2];
				vertices[i].uv.y = s * 0.25f;
			}
		}

		/* generate indicies */
		unsigned int i = 0;
		for ( unsigned int seg = 0; seg < (num_segment-1); seg++ ){
			for ( unsigned int cur = 0; cur < (per_segment-1); i += 4, cur++ ){
				const unsigned offset = seg * per_segment + cur;
				indices[i+0] = offset;
				indices[i+1] = offset + per_segment;
				indices[i+2] = offset + per_segment + 1;
				indices[i+3] = offset + 1;
			}
		}

		/* upload to gpu */
		glGenBuffers(2, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Shader::vertex_t) * num_vertices, vertices.get(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices, indices.get(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	virtual ~TunnelScene(){
		glDeleteBuffers(2, vbo);
	}

	virtual void render_geometry(const Camera& cam){
		clear(Color::yellow);

		Shader::upload_lights(lights);
		Shader::upload_camera(cam);
		shader->bind();

		glm::mat4 model(1.f);
		Shader::upload_model_matrix(model);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
		glVertexAttribPointer(Shader::ATTR_POSITION,  3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, pos));
		glVertexAttribPointer(Shader::ATTR_TEXCOORD,  2, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, uv));
		glVertexAttribPointer(Shader::ATTR_NORMAL,    3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, normal));
		glVertexAttribPointer(Shader::ATTR_TANGENT,   3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, tangent));
		glVertexAttribPointer(Shader::ATTR_BITANGENT, 3, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, bitangent));
		glVertexAttribPointer(Shader::ATTR_COLOR,     4, GL_FLOAT, GL_FALSE, sizeof(Shader::vertex_t), (const GLvoid*)offsetof(Shader::vertex_t, color));

		material.bind();
		glDrawElements(GL_QUADS, static_cast<GLsizei>(num_indices), GL_UNSIGNED_INT, 0);
	}

	virtual const Camera& get_current_camera(){
		return camera;
	}

	virtual void update(float t, float dt){
		camera.set_position(position.at(t) + offset);
		camera.look_at(position.at(t+0.1f)  + offset);
	}

	Camera camera;
	PointTable position;
	Material material;
	Shader* shader;

	GLuint vbo[2];
	size_t num_indices;
};

REGISTER_SCENE_TYPE(TunnelScene, "Tunnelbana", "tunnel.meta");
