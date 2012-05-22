#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "globals.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "utils.hpp"
#include "scene.hpp"
#include "shader.hpp"
#include "time.hpp"

#include "cl.hpp"
#include "quad.hpp"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unistd.h>
#include <getopt.h>

static const unsigned int framerate = 60;
static const uint64_t per_frame = 1000000 / framerate;
Time global_time(per_frame);

static volatile bool running = true;
static RenderObject * tv_test;
static Camera * camera;
static Shader::lights_data_t lights;
static Light * light;

static int current_frame_rate;

static const char* shader_programs[] = {
	"simple",
	"normal",
   "particles",
   "debug"
};

Scene* scene[0] = {};

static double rotation = 0.0;

static void handle_sigint(int signum){
	if ( !running ){
		fprintf(stderr, "\rgot SIGINT again, aborting\n");
		abort();
	}

	running = false;
	fprintf(stderr, "\rgot SIGINT, terminating graceful\n");
}

static void load_shaders() {
   Shader::initialize();
	for(int i=0; i < NUM_SHADERS; ++i) {
		shaders[i] = Shader::create_shader(shader_programs[i]);
	}
}

static void init(bool fullscreen){
	if ( SDL_Init(SDL_INIT_VIDEO) != 0 ){
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		exit(1);	}

	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	if ( !vi ){ fprintf(stderr, "SDL_GetVideoInfo() failed\n"); abort(); }
	resolution.x = vi->current_w;
	resolution.y = vi->current_h;

	if ( !fullscreen ){
		resolution.x = 800;
		resolution.y = 600;
	}

	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	SDL_SetVideoMode(resolution.x, resolution.y, 0, SDL_OPENGL|SDL_DOUBLEBUF|(fullscreen?SDL_FULLSCREEN:0));
	SDL_EnableKeyRepeat(0, 0);

	SDL_WM_SetCaption("Speed 100%", NULL);

	int ret;
	if ( (ret=glewInit()) != GLEW_OK ){
		fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(ret));
		exit(1);
	}

	load_shaders();

	tv_test = new RenderObject("models/tv.obj");
   //tv_test->roll(M_PI_4);

	camera = new Camera(75.f, resolution.x/(float)resolution.y, 0.1f, 100.f);
	camera->set_position(glm::vec3(0.f, 0.f, -1.f));
	camera->look_at(glm::vec3(0.f, 0.f, 0.f));

   lights.num_lights = 1;
   lights.ambient_intensity = glm::vec3(0.1, 0.1, 0.1);
	light = new Light(Light::POINT_LIGHT, glm::vec3(0.8f, 0.8f, 0.8f), glm::vec3(0.f, 0.f, 1.f));
   lights.lights[0] = light->shader_light();
   Shader::upload_lights(lights);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glDepthRange(0.0f, 1.0f);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	screen_ortho = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
	screen_ortho = glm::scale(screen_ortho, glm::vec3(1.0f, -1.0f, 1.0f));
	screen_ortho = glm::translate(screen_ortho, glm::vec3(0.0f, -600.0f, 0.0f));

	checkForGLErrors("post init()");



}

static void cleanup(){
}

static void poll(){
	SDL_Event event;
	while ( SDL_PollEvent(&event) ){
		switch ( event.type ){
		case SDL_QUIT:
			running = false;
			break;

		case SDL_KEYDOWN:
			if ( event.key.keysym.sym == SDLK_ESCAPE ){
				running = false;
			}
			if ( event.key.keysym.sym == SDLK_q && event.key.keysym.mod & KMOD_CTRL ){
				running = false;
			}

			bool scale_updated = false;

			if ( event.key.keysym.sym == SDLK_SPACE ){
				global_time.toggle_pause();
				scale_updated = true;
			}

			if ( event.key.keysym.sym == SDLK_PERIOD ){
				global_time.step(1);
				scale_updated = true;
			}

			if ( event.key.keysym.sym == SDLK_COMMA ){
				global_time.adjust_speed(-10);
				scale_updated = true;
			} else if ( event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_p ){
				global_time.adjust_speed(10);
				scale_updated = true;
			}

			if ( scale_updated ){
				char title[64];
				sprintf(title, "Speed: %d%%", global_time.current_scale());
				SDL_WM_SetCaption(title, NULL);
			}
		}
	}
}

static void render(){
	glClearColor(1,0,1,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	static int x = 0;
	//shaders[SHADER_NORMAL]->bind();

//	shaders[SHADER_NORMAL]->upload_projection_view_matrices(camera->projection_matrix(), camera->view_matrix());


	//tv_test->render(shaders[SHADER_NORMAL]);

	checkForGLErrors("model render");

   //shaders[SHADER_NORMAL]->unbind();

   shaders[SHADER_PARTICLES]->bind();

   static const float vertices[][4] = {
      {0, 0, 0, 1.f},
      {0, 1, 0, 1.f}
   };

   static const float colors[][4] = {
      {1, 0, 0, 1},
      {0, 1, 0, 1}
   };

   glDisable(GL_CULL_FACE);

   Shader::upload_model_matrix(glm::mat4(1.f));

   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, &vertices[0][0]);   
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, &colors[0][0]);   

   glDrawArrays(GL_POINTS, 0, 2);

	shaders[SHADER_PARTICLES]->unbind();

	SDL_GL_SwapBuffers();

	checkForGLErrors("SDL_GL_SwapBuffer");
}

static void update(float dt){
	float t = global_time.get();
	for ( Scene* s: scene ){
		s->update_scene(t, dt);
	}
	tv_test->yaw(M_PI_4*dt);

   rotation += dt*M_PI_4/4.f;
   rotation = fmod(rotation, 2.f*M_PI);

   glm::vec3 pos = glm::vec3((1.f+rotation)*cos(rotation), 0.f, (1.f+rotation)*sin(rotation));

   camera->set_position(pos);

   Shader::upload_camera(*camera);
	
}

static void magic_stuff(){
	/* for calculating dt */
	struct timeval t, last;
	gettimeofday(&t, NULL);
   gettimeofday(&last, NULL);

   current_frame_rate = 0;
   
   int show_fps = 0;

	while ( running ){
		poll();

		/* calculate dt */
		struct timeval cur;
		gettimeofday(&cur, NULL);
		const uint64_t delta = (cur.tv_sec - t.tv_sec) * 1000000 + (cur.tv_usec - t.tv_usec);
      current_frame_rate = 1000000/ ( (cur.tv_sec - last.tv_sec) * 1000000 + (cur.tv_usec - last.tv_usec) );
		const  int64_t delay = per_frame - delta;

      last = cur;

      if(show_fps%100 == 0)
         printf("FPS: %d\n", current_frame_rate);

      show_fps = show_fps%100;
      show_fps++;
		global_time.update();
		update(global_time.dt());
		render();

		/* move time forward */
		t.tv_usec += per_frame;
		if ( t.tv_usec > 1000000 ){
			t.tv_usec -= 1000000;
			t.tv_sec++;
		}

		/* fixed framerate */
		if ( delay > 0 ){
			usleep(delay);
		}
	}
}

int main(int argc, char* argv[]){
	const bool fullscreen = argc >= 2;
	signal(SIGINT, handle_sigint);

   CL ocl;

   //TEST CL:
   cl::Program clprogram = ocl.create_program("cl_programs/test.cl");
   cl::Kernel kernel = ocl.load_kernel(clprogram, "part1");

   cl::CommandQueue &queue = ocl.queue();

    int num = 10;
    float *a = new float[num];
    float *b = new float[num];
    float *c = new float[num];
    for(int i=0; i < num; i++)
    {
        a[i] = 1.0f * i;
        b[i] = 1.0f * i;
        c[i] = 0.0f;
    }

    size_t array_size = sizeof(float) * num;

   cl::Buffer cl_a, cl_b, cl_c;
   cl_int err;
   cl::Event event; 

    cl_a = ocl.create_buffer(CL_MEM_READ_ONLY, array_size);
    cl_b = ocl.create_buffer(CL_MEM_READ_ONLY, array_size);
    cl_c = ocl.create_buffer(CL_MEM_WRITE_ONLY, array_size);

    printf("Pushing data to the GPU\n");
    err = queue.enqueueWriteBuffer(cl_a, CL_TRUE, 0, array_size, a, NULL, &event);
    err = queue.enqueueWriteBuffer(cl_b, CL_TRUE, 0, array_size, b, NULL, &event);
    err = queue.enqueueWriteBuffer(cl_c, CL_TRUE, 0, array_size, c, NULL, &event);
    

    err = kernel.setArg(0, cl_a);
    err = kernel.setArg(1, cl_b);
    err = kernel.setArg(2, cl_c);
    queue.finish();

    delete a;
    delete b;


    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event);
    printf("clEnqueueNDRangeKernel: %s\n", CL::errorString(err));
    queue.finish();

    err = queue.enqueueReadBuffer(cl_c, CL_TRUE, 0, sizeof(float) * num, c, NULL, &event);
    printf("clEnqueueReadBuffer: %s\n", CL::errorString(err));

    for(int i=0; i < num; i++)
    {
        printf("c_done[%d] = %g\n", i, c[i]);
    }

   delete c;

	init(fullscreen);
	magic_stuff();
	cleanup();

	return 0;
}
