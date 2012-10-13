ACLOCAL_AMFLAGS = -I m4
AM_DEFAULT_SOURCE_EXT = .cpp

engine_CXXFLAGS = ${GL_CFLAGS} ${GLU_CFLAGS} ${GLEW_CFLAGS} ${GLM_CFLAGS} ${SDL_CFLAGS} ${CL_CFLAGS}
engine_LIBS = ${VENDOR_LIBS} ${GL_LIBS} ${GLU_LIBS} ${GLEW_LIBS} ${SDL_LIBS} ${CL_LIBS} -lSDL_image -lfmodex -lassimp
AM_CXXFLAGS = -Wall -Woverloaded-virtual -Wvla -Wconversion -I${top_srcdir}/src ${engine_CXXFLAGS} ${VENDOR_CFLAGS}

noinst_LIBRARIES = libfrob.a
bin_PROGRAMS = demo nox2 examples_mrt examples_blur
TESTS = test/utils test/data

if BUILD_EDITOR
bin_PROGRAMS += editor
TESTS += test/editor_classify
endif

if BUILD_TESTS
check_PROGRAMS = ${TESTS}
endif

libfrob_a_SOURCES = \
	src/bindable.hpp \
	src/camera.cpp src/camera.hpp \
	src/cl.cpp src/cl.hpp \
	src/color.cpp src/color.hpp \
	src/data.cpp src/data.hpp \
	src/engine.cpp src/engine.hpp \
	src/globals.cpp src/globals.hpp \
	src/light.cpp src/light.hpp \
	src/lights_data.cpp src/lights_data.hpp \
	src/loading.cpp src/loading.hpp \
	src/logging.cpp src/logging.hpp \
	src/material.cpp src/material.hpp \
	src/mesh.cpp src/mesh.hpp \
	src/meta.cpp src/meta.hpp \
	src/movable_light.cpp src/movable_light.hpp \
	src/movable_object.cpp src/movable_object.hpp \
	src/particle_system.cpp src/particle_system.hpp \
	src/path.cpp src/path.hpp \
	src/rails.cpp src/rails.hpp \
	src/rendertarget.cpp src/rendertarget.hpp \
	src/render_object.cpp src/render_object.hpp \
	src/scene.cpp src/scenefactory.cpp src/scene.hpp \
	src/shader.cpp src/shader.hpp \
	src/skybox.cpp src/skybox.hpp \
	src/sound.cpp src/sound.hpp \
	src/terrain.cpp src/terrain.hpp \
	src/texture.cpp src/texture.hpp \
	src/time.cpp src/time.hpp \
	src/timetable.cpp src/timetable.hpp \
	src/utils.cpp src/utils.hpp \
	src/quad.cpp src/quad.hpp \
	src/scene/nox.cpp \
	src/scene/particles.cpp \
	src/scene/test.cpp \
	src/scene/tunnel.cpp \
	src/scene/tv.cpp \
	src/scene/water.cpp \
	src/scene/winter.cpp \
	vendor/src/nanosvg.cpp vendor/include/nanosvg.h \
	autoload.cpp autoload.h

if INPUT_ENABLED
libfrob_a_SOURCES += src/input.cpp src/input.hpp
endif

demo_CXXFLAGS = ${AM_CXXFLAGS} -DNAME="\"demo\"" -DTITLE="\"Experimental demo\""
demo_LDADD = libfrob.a ${engine_LIBS}
demo_SOURCES = src/main.cpp src/testing.cpp
demo_LDFLAGS = -pthread

nox2_CXXFLAGS = ${AM_CXXFLAGS} -DNAME="\"nox2\"" -DTITLE="\"NördtroXy II Invitation\""
nox2_LDADD = libfrob.a ${engine_LIBS}
nox2_LDFLAGS = -pthread
nox2_SOURCES = src/main.cpp src/nox2.cpp

examples_mrt_CXXFLAGS = ${AM_CXXFLAGS} -DNAME="\"example_mrt\"" -DTITLE="\"Example: MRT\""
examples_mrt_SOURCES = src/main.cpp examples/mrt/mrt.cpp
examples_mrt_LDADD = libfrob.a ${engine_LIBS}

examples_blur_CXXFLAGS = ${AM_CXXFLAGS} -DNAME="\"example_blur\"" -DTITLE="\"Example: Blur\""
examples_blur_SOURCES = src/main.cpp examples/blur/blur.cpp
examples_blur_LDADD = libfrob.a ${engine_LIBS}

editor_CXXFLAGS = ${AM_CXXFLAGS} $(gtk3_CFLAGS)
editor_LDFLAGS = -Wl,--export-dynamic -pthread
editor_LDADD = libfrob.a ${engine_LIBS} $(gtk3_LIBS)
editor_SOURCES = \
	src/editor.cpp \
	src/editor/classify.cpp \
	src/editor/render.cpp \
	src/editor/scenelist.cpp \
	src/editor/timecontrol.cpp

test_editor_classify_CXXFLAGS = ${AM_CXXFLAGS} $(CPPUNIT_CFLAGS) $(gtk3_CFLAGS)
test_editor_classify_LDFLAGS = $(CPPUNIT_LIBS)
test_editor_classify_SOURCES = test/editor_classify.cpp src/editor/classify.cpp

test_data_CXXFLAGS = ${AM_CXXFLAGS} $(CPPUNIT_CFLAGS)
test_data_LDADD = libfrob.a ${engine_LIBS} $(CPPUNIT_LIBS)

test_utils_CXXFLAGS = ${AM_CXXFLAGS} $(CPPUNIT_CFLAGS)
test_utils_LDADD = libfrob.a ${engine_LIBS} $(CPPUNIT_LIBS)

.PHONY: .autoload.tmp
BUILT_SOURCES = autoload.h autoload.cpp
CLEANFILES = autoload.cpp autoload.h

if AUTOLOAD
BUILT_SOURCES += .autoload.tmp
CLEANFILES += .autoload.tmp autoload.lst stamp-autoload

.autoload.tmp: Makefile
	@grep "^[[:blank:]]*REGISTER_SCENE_TYPE" -r ${top_srcdir}/src/scene/*.cpp | sed 's/.*(\(.*\),[ \t]*\"\(.*\)\"[ \t]*,.*).*/_register_\1:\2/g' > .autoload.tmp
	@cmp -s autoload.lst .autoload.tmp || touch stamp-autoload

stamp-autoload: .autoload.tmp

autoload.lst: stamp-autoload
	$(AM_V_GEN)cp .autoload.tmp autoload.lst
endif

autoload.h: autoload.lst Makefile
	$(AM_V_GEN)sed 's/\(.*\):\(.*\)/void \1();/' autoload.lst > autoload.h

autoload.cpp: autoload.lst Makefile
	@echo '#ifdef HAVE_CONFIG_H' > autoload.cpp
	@echo '#include "config.h"' >> autoload.cpp
	@echo '#endif' >> autoload.cpp
	@echo '#include "autoload.h"' >> autoload.cpp
	@echo '#include "globals.hpp"' >> autoload.cpp
	@echo '#include "logging.hpp"' >> autoload.cpp
	@echo '#include <cstdio>' >> autoload.cpp
	@echo 'namespace Engine { void autoload_scenes(){' >> autoload.cpp
	@echo ' 	Logging::verbose("Autoloading scenes.\n");' >> autoload.cpp
	$(AM_V_GEN)sed 's/\(.*\):\(.*\)/	Logging::verbose("  %s -> %s\\n", \"\2\", \"\1\"); \1();/' autoload.lst >> autoload.cpp
	@echo "}}" >> autoload.cpp

release: all
	@test "x${prefix}" = "x/" || (echo "Error: --prefix must be / when creating release (currently ${prefix})"; exit 1)
	mkdir -p release-dist
	$(MAKE) install DESTDIR=`pwd`/release-dist
