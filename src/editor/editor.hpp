#ifndef EDITOR_GLOBALS_H
#define EDITOR_GLOBALS_H

#include "time.hpp"
#include <string>
#include <gtk/gtk.h>

/* general globals */
extern Time global_time;
extern bool running;
extern Scene* scene;

/* editor widgets */
extern GtkWidget* drawing;
extern GtkLabel* timelabel;
extern GtkLabel* message;
extern GtkToggleButton* playbutton;
extern GtkTreeStore* scenestore;
extern GtkTreeStore* propstore;
extern RenderTarget* frame;

/* icons */
extern GdkPixbuf* icon_cat_scene;
extern GdkPixbuf* icon_cat_model;
extern GdkPixbuf* icon_scene;
extern GdkPixbuf* icon_model;
extern GdkPixbuf* icon_path;
extern GdkPixbuf* icon_light;

namespace Editor {

	enum MODE {
		MODE_BLANK,
		MODE_SCENE,
		MODE_PATH,
		MODE_MODEL,
	};

	enum TYPE: gint {
		TYPE_UNKNOWN, /* should be first */
		TYPE_CAT = 0,
		TYPE_CAT_SCENE,
		TYPE_CAT_COMPOSITION,
		TYPE_CAT_MODELS,
		TYPE_PATH,
		TYPE_MODEL,
		TYPE_LIGHT,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_VEC2,
		TYPE_VEC3,
		TYPE_VEC4,
		TYPE_STRING,
		TYPE_MAX, /* should be last */
	};

	enum CAMERA_MODE {
		CAMERA_AUTO,
		CAMERA_MANUAL,
	};

	extern MODE mode;
	extern CAMERA_MODE camera_control;
	extern unsigned int frames;
	extern std::string scene_name;

	TYPE classify_name(const std::string&name, std::string& data);

	void scenelist_populate();
	void sceneprops_populate(Scene* scene);

	void reset();
	void load_model(const std::string& filename);
}

#endif /* EDITOR_GLOBALS_H */
