ACLOCAL_AMFLAGS = -I m4
AM_DEFAULT_SOURCE_EXT = .cpp

engine_CXXFLAGS = ${GL_CFLAGS} ${GLU_CFLAGS} ${GLEW_CFLAGS} ${GLM_CFLAGS} ${SDL_CFLAGS} ${CL_CFLAGS} ${BULLET_CFLAGS}
engine_LIBS = ${VENDOR_LIBS} ${GL_LIBS} ${GLU_LIBS} ${GLEW_LIBS} ${SDL_LIBS} ${CL_LIBS} ${BULLET_LIBS} -lSDL_image -lfmodex -lassimp
AM_CXXFLAGS = -Wall -Woverloaded-virtual -Wvla -Wconversion -I${top_srcdir}/src ${engine_CXXFLAGS} ${VENDOR_CFLAGS}

noinst_LIBRARIES = libfrob.a
bin_PROGRAMS = basejump
noinst_PROGRAMS = examples_mrt examples_blur examples_shadowmaps examples_particles examples_terrain examples_hdr
TESTS = test/utils test/data test/aabb test/quadtree

if BUILD_EDITOR
bin_PROGRAMS += editor
TESTS += test/editor_classify
endif

if BUILD_TESTS
check_PROGRAMS = ${TESTS}
endif

libfrob_a_SOURCES = \
	src/aabb.cpp src/aabb.hpp \
	src/aabb2d.cpp src/aabb2d.hpp \
	src/bindable.hpp \
	src/camera.cpp src/camera.hpp \
	src/Controller.cpp src/Controller.hpp \
	src/cl.cpp src/cl.hpp \
	src/config.cpp src/config.hpp \
	src/color.cpp src/color.hpp \
	src/data.cpp src/data.hpp \
	src/engine.cpp src/engine.hpp \
	src/globals.cpp src/globals.hpp \
	src/intersect2d.cpp src/intersect2d.hpp \
	src/light.cpp src/light.hpp \
	src/lights_data.cpp src/lights_data.hpp \
	src/line2d.cpp src/line2d.hpp \
	src/loading.cpp src/loading.hpp \
	src/logging.cpp src/logging.hpp \
	src/material.cpp src/material.hpp \
	src/mesh.cpp src/mesh.hpp \
	src/meta.cpp src/meta.hpp \
	src/movable_light.cpp src/movable_light.hpp \
	src/movable_object.cpp src/movable_object.hpp \
	src/particle_system.cpp src/particle_system.hpp \
	src/path.cpp src/path.hpp \
	src/rendertarget.cpp src/rendertarget.hpp \
	src/render_object.cpp src/render_object.hpp \
	src/shader.cpp src/shader.hpp \
	src/sky.cpp src/sky.hpp \
	src/skybox.cpp src/skybox.hpp \
	src/cube_vertices.hpp \
	src/sound.cpp src/sound.hpp \
	src/terrain.cpp src/terrain.hpp \
	src/texture.cpp src/texture.hpp \
	src/time.cpp src/time.hpp \
	src/timetable.cpp src/timetable.hpp \
	src/triangle2d.cpp src/triangle2d.hpp \
	src/utils.cpp src/utils.hpp \
	src/Protagonist.cpp src/Protagonist.hpp \
	src/quad.cpp src/quad.hpp \
	src/quadtree.cpp src/quadtree.hpp \
	src/XboxController.cpp src/XboxController.hpp \
	src/techniques/blur.cpp src/techniques/blur.hpp \
	src/techniques/dof.cpp src/techniques/dof.hpp \
	src/techniques/hdr.cpp src/techniques/hdr.hpp \
	src/techniques/temporalblur.cpp src/techniques/temporalblur.hpp \
	vendor/src/nanosvg.cpp vendor/include/nanosvg.h

libfrob_a_SOURCES += src/input.cpp src/input.hpp

basejump_CXXFLAGS = ${AM_CXXFLAGS}
basejump_LDADD = libfrob.a ${engine_LIBS}
basejump_SOURCES = src/main.cpp src/basejump.cpp \
									 src/game.cpp src/game.hpp
basejump_LDFLAGS = -pthread

examples_mrt_CXXFLAGS = ${AM_CXXFLAGS}
examples_mrt_SOURCES = src/main.cpp examples/mrt/mrt.cpp
examples_mrt_LDADD = libfrob.a ${engine_LIBS}

examples_shadowmaps_CXXFLAGS = ${AM_CXXFLAGS}
examples_shadowmaps_SOURCES = src/main.cpp examples/shadowmaps/shadowmaps.cpp
examples_shadowmaps_LDADD = libfrob.a ${engine_LIBS}

examples_particles_CXXFLAGS = ${AM_CXXFLAGS}
examples_particles_SOURCES = src/main.cpp examples/particles/particles.cpp
examples_particles_LDADD = libfrob.a ${engine_LIBS}

examples_blur_CXXFLAGS = ${AM_CXXFLAGS}
examples_blur_SOURCES = src/main.cpp examples/blur/blur.cpp
examples_blur_LDADD = libfrob.a ${engine_LIBS}

examples_terrain_CXXFLAGS = ${AM_CXXFLAGS}
examples_terrain_SOURCES = src/main.cpp examples/terrain/terrain.cpp
examples_terrain_LDADD = libfrob.a ${engine_LIBS}

examples_hdr_CXXFLAGS = ${AM_CXXFLAGS}
examples_hdr_SOURCES = src/main.cpp examples/hdr/hdr.cpp
examples_hdr_LDADD = libfrob.a ${engine_LIBS}

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

test_aabb_CXXFLAGS = ${AM_CXXFLAGS} $(CPPUNIT_CFLAGS)
test_aabb_LDADD = libfrob.a ${engine_LIBS} $(CPPUNIT_LIBS)

test_quadtree_CXXFLAGS = ${AM_CXXFLAGS} $(CPPUNIT_CFLAGS)
test_quadtree_LDADD = libfrob.a ${engine_LIBS} $(CPPUNIT_LIBS)

release: all
	@test "x${prefix}" = "x/" || (echo "Error: --prefix must be / when creating release (currently ${prefix})"; exit 1)
	mkdir -p release-dist
	$(MAKE) install DESTDIR=`pwd`/release-dist
