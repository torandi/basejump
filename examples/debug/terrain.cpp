#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "lights_data.hpp"
#include "rendertarget.hpp"
#include "terrain.hpp"
#include "time.hpp"
#include "globals.hpp"
#include "triangle2d.hpp"

static Shader* shader = nullptr;
static Shader* passthru = nullptr;
static Terrain* terrain = nullptr;
static RenderTarget* scene = nullptr;
static LightsData* lights = nullptr;
static Camera cam(75, 1.3f, 0.1f, 400.0f), cam2(75, 1.3f, 0.1f, 200.0f);
extern glm::mat4 screen_ortho;   /* defined in main.cpp */
extern Time global_time;         /* defined in main.cpp */
extern glm::ivec2 resolution;    /* defined in main.cpp */
static Camera * move;
static GLuint debug_buffers[2];
static Shader * simple = nullptr;

namespace Engine {
	RenderTarget* rendertarget_by_name(const std::string& fullname){
		return nullptr;
	}

	void init(){
		Data::add_search_path(srcdir "/examples/terrain");

		passthru  = Shader::create_shader("/shaders/passthru");
		shader    = Shader::create_shader("/shaders/normal");
		simple		= Shader::create_shader("/shaders/simple");
		terrain   = new Terrain("/terrain.cfg");
		scene     = new RenderTarget(resolution, GL_RGB8, RenderTarget::DEPTH_BUFFER, GL_LINEAR);
		lights = new LightsData();

		terrain->set_position(glm::vec3(-7.5f, -2.0f, -7.5f));
		lights->ambient_intensity() = glm::vec3(0.2f);
		lights->num_lights() = 1;
		lights->lights[0]->set_position(glm::vec3(0, -1.0f, -0.5f));
		lights->lights[0]->intensity = glm::vec3(1.0f);
		lights->lights[0]->type = MovableLight::DIRECTIONAL_LIGHT;

		Shader::fog_t fog = { glm::vec4(0.8, 0.8, 0.8, 1.0), 0.01f };

		Shader::upload_fog(fog);

		cam.set_position(glm::vec3(20.f,  128.f, 20.f));
		cam.look_at(glm::vec3(20.f, 128.f, 22.f));

		cam2.set_position(glm::vec3(0.f, 32.f, 0.f));
		cam2.look_at(glm::vec3(0.f, 32.f, 2.f));

		move = &cam;

		glGenBuffers(2, debug_buffers);

#ifdef ENABLE_INPUT
		Input::movement_speed = 32.f;
#endif
	}

	void start(double seek){
	}

	void cleanup(){
		delete terrain;
		delete scene;

		glDeleteBuffers(2, debug_buffers);

		Shader::cleanup();
	}

	void render(){
		Shader::upload_camera(cam);
		Shader::upload_model_matrix(glm::mat4());
		Shader::upload_lights(*lights);
		Shader::upload_blank_material();
		Shader::upload_resolution(resolution);

		const Camera &cull_cam = cam;

		RenderTarget::clear(Color::white);
		terrain->render_cull(cull_cam);

		/*
		glLineWidth(2.f);

		Shader::upload_model_matrix(glm::mat4());
		simple->bind();
		cull_cam.render_frustrum(debug_buffers[0]);

		//Render culling triangle:

		static const unsigned int indices[] = {
			0, 1, 2, 0
		};

		struct point_data_t {
			glm::vec3 corner;
			glm::vec4 color;
		};

		Triangle2D tri = Terrain::calculate_camera_tri(cull_cam);

		glm::vec4 c = glm::vec4(1.f, 0.f, 0.f, 1.f);
		point_data_t vertices[] = {
			{ glm::vec3(tri.p1.x, cull_cam.position().y, tri.p1.y) , c },
			{ glm::vec3(tri.p2.x, cull_cam.position().y, tri.p2.y), c },
			{ glm::vec3(tri.p3.x, cull_cam.position().y, tri.p3.y), c },
		};

		glBindBuffer(GL_ARRAY_BUFFER, debug_buffers[1]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBufferData(GL_ARRAY_BUFFER, sizeof(point_data_t) * 3, vertices, GL_DYNAMIC_DRAW);

		Shader::push_vertex_attribs(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_data_t), (const GLvoid*) offsetof(point_data_t, corner));
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(point_data_t), (const GLvoid*) offsetof(point_data_t, color));

		glDrawElements(GL_LINE_STRIP, 4, GL_UNSIGNED_INT, indices);

		Shader::pop_vertex_attribs();
		*/
	}

	void update(float t, float dt){
		const float s = t*0.5f;
		const float d = 15.5f;

#ifdef ENABLE_INPUT
		input.update_object(*move, dt);
		if(input.down(Input::ACTION_0)) {
			if(move == &cam) move = &cam2;
			else move = &cam;
		}

		if(input.down(Input::ACTION_1)) {
			Input::movement_speed += 5.f;
		}

		if(input.down(Input::ACTION_2)) {
			Input::movement_speed -= 5.f;
		}
#else
		cam.look_at(glm::vec3(0,0,0));
		cam.set_position(glm::vec3(cos(s)*d, 8.0f, sin(s)*d));
#endif
	}
}
