#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "engine.hpp"
#include "globals.hpp"
#include "loading.hpp"
#include "logging.hpp"
#include "render_object.hpp"
#include "rendertarget.hpp"
#include "utils.hpp"
#include "scene.hpp"
#include "shader.hpp"
#include "time.hpp"
#include "cl.hpp"
#include "texture.hpp"
#include "timetable.hpp"
#include "quad.hpp"

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <map>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "light.hpp"

static const unsigned int framerate = 60;
static const uint64_t per_frame = 1000000 / framerate;
Time global_time(per_frame);
glm::mat4 screen_ortho;

static volatile bool running = true;
static const char* program_name;
static bool resolution_given = false;
static int frames = 0;
static double seek = 0.0;
static bool fullscreen = FULLSCREEN;
static bool vsync = true;
static bool verbose_flag = false;
static bool skip_load_scene = false;
glm::ivec2 resolution(800, 600);

static void poll();

namespace Engine {
	void terminate() {
		::running = false;
	}
}

static void handle_sigint(int signum){
	if ( !running ){
		fprintf(stderr, "\rgot SIGINT again, aborting\n");
		abort();
	}

	running = false;
	fprintf(stderr, "\rgot SIGINT, terminating graceful\n");
}

static void show_fps(int signum){
	Logging::verbose("FPS: %d\n", frames);
	frames = 0;
}

static void init_window(const char* title){
	if ( SDL_Init(SDL_INIT_VIDEO) != 0 ){
		Logging::fatal("SDL_Init failed: %s\n", SDL_GetError());
	}

	const SDL_VideoInfo* vi = SDL_GetVideoInfo();
	if ( !vi ){
		Logging::fatal("SDL_GetVideoInfo() failed\n");
	}

	if ( fullscreen && !resolution_given ){
		resolution.x = vi->current_w;
		resolution.y = vi->current_h;
	}

	/* show configuration */
	Logging::info(PACKAGE_NAME "-" VERSION "\n"
	        "Configuration:\n"
	        "  Resolution: %dx%d (%s)\n"
#ifdef ENABLE_INPUT
	        "  Input is enabled\n"
#endif
	        , resolution.x, resolution.y, fullscreen?"fullscreen":"windowed");

	if(vsync) SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	SDL_SetVideoMode(resolution.x, resolution.y, 0, SDL_OPENGL|SDL_DOUBLEBUF|(fullscreen?SDL_FULLSCREEN:0));
	SDL_EnableKeyRepeat(0, 0);
	SDL_WM_SetCaption(title, NULL);

	if ( fullscreen ){
		SDL_ShowCursor(SDL_DISABLE);
	}

	GLenum ret;
	if ( (ret=glewInit()) != GLEW_OK ){
		Logging::fatal("Failed to initialize GLEW: %s\n", glewGetErrorString(ret));
	}

	/* setup window projection matrix */
	screen_ortho = glm::ortho(0.0f, (float)resolution.x, 0.0f, (float)resolution.y, -1.0f, 1.0f);
	screen_ortho = glm::scale(screen_ortho, glm::vec3(1.0f, -1.0f, 1.0f));
	screen_ortho = glm::translate(screen_ortho, glm::vec3(0.0f, -(float)resolution.y, 0.0f));

	/* show OpenGL info */
	GLint max_texture_units;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
	Logging::verbose("OpenGL Device: %s - %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	Logging::verbose("  - Supports %d texture units\n", max_texture_units);
}

static void init(const char* title){
	Logging::init();
	Logging::add_destination(verbose_flag ? Logging::VERBOSE : Logging::WARNING, stderr);
	Logging::add_destination(Logging::VERBOSE, "frob.log");
	Logging::info("FFS: Frobnicator Fubar System - Engine starting\n");
	init_window(title);
	Data::add_search_path(srcdir);
	Engine::setup_opengl();
	Shader::initialize();

	//Set default fog
	Shader::fog_t fog = { glm::vec4(0.584f, 0.698f, 0.698f, 1.f), 0.005f };
	Shader::upload_fog(fog);

	Loading::init(resolution);

	static const char* resources[] = {
		"texture:/textures/default.jpg",
		"texture:/textures/default_normalmap.jpg",
		"texture:/textures/white.jpg",
		"shader:/shaders/normal",
		"shader:/shaders/passthru",
	};
	Engine::preload(std::vector<std::string>(resources, resources + sizeof(resources)/sizeof(char*)), Loading::progress);
	Engine::autoload_scenes();
	CL::init();
	srand((unsigned int)time(0));

	Engine::init();
	Loading::cleanup();
	Engine::start(seek);
	checkForGLErrors("post init()");
}

static void cleanup(){
	CL::cleanup();
	Engine::cleanup();
	Texture2D::cleanup();
	Logging::cleanup();
	SDL_Quit();
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

#ifdef ENABLE_INPUT
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
#endif /* ENABLE_INPUT */
		}

#ifdef ENABLE_INPUT
		input.parse_event(event);
#endif /* ENABLE_INPUT */
	}
}

static void render(){
	checkForGLErrors("Frame begin");

	Engine::render();

	SDL_GL_SwapBuffers();
	checkForGLErrors("Frame end");
}

static void update(float dt){
	float t = global_time.get();
	Shader::upload_frameinfo(t);
	Engine::update(t, dt);
}

static void magic_stuff(){
	/* for calculating dt */
	long t = util_utime();

	while ( running ){
		/* calculate dt */
		const long cur = util_utime();
		const long delta = cur - t;
		const long delay = per_frame - delta;

		poll();
		global_time.update();
		update(global_time.dt());
		render();

		/* move time forward */
		frames++;
		t += per_frame;

		/* fixed framerate */
		if ( delay > 0 ){
			util_usleep(static_cast<useconds_t>(delay));
		}
	}
}

static void __UNUSED__  set_resolution(const char* str){
	glm::ivec2 tmp;
	int n = sscanf(str, "%dx%d", &tmp.x, &tmp.y);
	if ( n != 2 || tmp.x <= 0 || tmp.y <= 0 ){
		Logging::error("%s: Malformed resolution `%s', must be WIDTHxHEIGHT. Option ignored\n", program_name, str);
		return;
	}

	resolution = tmp;
	resolution_given = true;
}

#ifdef HAVE_GETOPT_H
void show_usage(){
	printf("%s (" PACKAGE_NAME "-" VERSION ")\n"
	       "usage: %s [OPTIONS]\n"
	       "\n"
	       "  -r, --resolution=SIZE   Set window resultion (default: 800x600 in windowed and\n"
	       "                          current resolution in fullscreen.)\n"
	       "  -f, --fullscreen        Enable fullscreen mode (default: %s)\n"
	       "  -w, --windowed          Inverse of --fullscreen.\n"
	       "  -s, --seek=TIME         Seek to the given time in seconds.\n"
	       "  -n, --no-vsync          Disable vsync.\n"
	       "  -v, --verbose           Enable verbose output to stdout (redirected to logfile otherwise)\n"
	       "  -q, --quiet             Inverse of --verbose.\n"
				 "  -l, --no-loading        Don't show loading scene (faster load).\n"
	       "  -h, --help              This text\n",
	       program_name, program_name, FULLSCREEN ? "true" : "false");
}

static const char* shortopts = "r:s:fwnvqlh";
static struct option longopts[] = {
	{"resolution",   required_argument, 0, 'r'},
	{"seek",         required_argument, 0, 's'},
	{"fullscreen",   no_argument,       0, 'f'},
	{"windowed",     no_argument,       0, 'w'},
	{"no-vsync",     no_argument,       0, 'n'},
	{"verbose",      no_argument,       0, 'v'},
	{"quiet",        no_argument,       0, 'q'},
	{"no-loading",   no_argument,       0, 'l'},
	{"help",         no_argument,       0, 'h'},
	{0,0,0,0} /* sentinel */
};

static void parse_argv(int argc, char* argv[]){
	int op, option_index;
	while ( (op = getopt_long(argc, argv, shortopts, longopts, &option_index)) != -1 ){
		switch ( op ){
		case 0:   /* long opt*/
		case '?': /* invalid */
			break;

		case 'r': /* --resolution */
			set_resolution(optarg);

		case 'f': /* --fullscreen */
			fullscreen = false;
			break;

		case 'l':
			skip_load_scene = true;
			break;

		case 'w': /* --windowed */
			fullscreen = false;
			break;

		case 's': /* --seek */
			seek = atof(optarg);
			break;

		case 'n': /* --no-vsync */
			vsync = false;
			break;

		case 'v': /* --verbose */
			verbose_flag = true;
			break;

		case 'q': /* --quiet */
			verbose_flag = false;
			break;

		case 'h': /* --help */
			show_usage();
			exit(0);

		default:
			fprintf(stderr, "%s: declared but unhandled argument '%c' (0x%02X)\n", program_name, op, op);
			abort();
		}
	};
}

#else /* HAVE_GETOPT_H */
static void parse_argv(int argc, char* argv[]){
	if ( argc > 1 ){
		Logging::warning("%s: warning: argument parsing has been disabled at compile-time.\n", program_name);
	}
}
#endif

static void setup_fps_timer(){
#ifdef HAVE_SETITIMER
	struct itimerval difftime;
	difftime.it_interval.tv_sec = 1;
	difftime.it_interval.tv_usec = 0;
	difftime.it_value.tv_sec = 1;
	difftime.it_value.tv_usec = 0;
	signal(SIGALRM, show_fps);
	setitimer(ITIMER_REAL, &difftime, NULL);
#else
	Logging::warning("%s: warning: no framerate timer available on this platform. FPS report will be diabled.\n");
#endif
}

int main(int argc, char* argv[]){
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	parse_argv(argc, argv);

	if(verbose_flag) setup_fps_timer();

	/* proper termination */
	signal(SIGINT, handle_sigint);

	/* let the magic begin */
	init(program_name);
	magic_stuff();
	cleanup();

	return 0;
}
