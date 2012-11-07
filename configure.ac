AC_INIT([frobnicator-demo-engine],[1],[ext@sidvind.com])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AH_BOTTOM([
#include "forward.hpp"
#include "platform.hpp"
])

AC_GNU_SOURCE
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CXX
AC_PROG_RANLIB

AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-std=c++0x], [CXXFLAGS="$CXXFLAGS -std=c++0x"], [AC_MSG_ERROR([Support for C++11 is required])])
AC_LANG_POP

AX_CHECK_CL
AX_CHECK_GL
AX_CHECK_GLU
AX_GLEW
AX_GLM
AM_PATH_SDL

AS_IF([test "x$no_gl" == "xyes"], [AC_MSG_ERROR([OpenGL libraries required])])
AS_IF([test "x$no_cl" == "xyes"], [AC_MSG_ERROR([OpenCL libraries required])])

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([GL/glx.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_FUNCS([access gettimeofday usleep setitimer])

AC_ARG_ENABLE([editor], [AS_HELP_STRING([--enable-editor], [Build editor. @<:@default=disabled@:>@])])
AS_IF([test "x$enable_editor" = "xyes"],
   [PKG_CHECK_MODULES([gtk3], [gtk+-3.0 gtkglext-3.0])],
   [enable_editor=no])
AM_CONDITIONAL([BUILD_EDITOR], [test "x$enable_editor" = "xyes"])

loadingscene="yes"
AC_ARG_ENABLE([loading], [AS_HELP_STRING([--enable-loading], [Enable loadingscene. @<:@default=disabled@:>@])])
AS_IF([test "x$enable_loading" != "xyes"], [
  AC_DEFINE([NOLOAD],  [1], [Define to 1 to disable loadingscene])
  loadingscene="no"
])

AC_ARG_ENABLE([autoload], [AS_HELP_STRING([--enable-autoload], [Automatically loads all scenes. If disabled you need to make autoload.lst yourself. @<:@default=enabled@:>@])], [], [enable_autoload="yes"])
AM_CONDITIONAL([AUTOLOAD], [test "x$enable_autoload" == "xyes"])

dnl Setup paths
AC_DEFINE_UNQUOTED([srcdir], ["${srcdir}"], [srcdir])
AC_DEFINE_UNQUOTED([datadir], ["${datadir}"], [datadir])

dnl Defaults
AC_ARG_VAR([FULLSCREEN], [If non-empty fullscreen is used by default])
AS_IF([test "x${FULLSCREEN}" == "x" -o "x${FULLSCREEN}" == "x0"], [FULLSCREEN=0], [FULLSCREEN=1])
AC_DEFINE_UNQUOTED([FULLSCREEN], [$FULLSCREEN], [Define to 1 if fullscreen is used by default])

AC_DEFINE_UNQUOTED([ENABLE_INPUT], [1], [Input always on])

AM_PATH_CPPUNIT(1.9.6,,[AC_MSG_NOTICE([cppunit not found, tests disabled])])
AM_CONDITIONAL([BUILD_TESTS], [test "x$no_cppunit" != "xyes"])

platform=$(uname -s)
arch=$(uname -m)
AS_IF([test "x${platform}" == "xDarwin"], [arch="osx"])

dnl Use vendor libs
VENDOR_CFLAGS="-isystem \${top_srcdir}/vendor/src"
VENDOR_LIBS=""
AC_ARG_ENABLE([vendor-libs], [AS_HELP_STRING([--enable-vendor-libs], [Use libraries from vendor folder instead of system. @<:@default=enabled@:>@])], [], [enable_vendor_libs="yes"])
AS_IF([test "x$enable_vendor_libs" == "xyes"], [
	VENDOR_CFLAGS+=" -isystem \${top_srcdir}/vendor/include"
	VENDOR_LIBS+=" -L \${top_srcdir}/vendor/libs/$arch -Wl,--rpath \${top_srcdir}/vendor/libs/$arch"
])
AC_SUBST(VENDOR_CFLAGS)
AC_SUBST(VENDOR_LIBS)

dnl Show config
echo
echo "     Platform/arch: $platform $arch (vendor libs: $enable_vendor_libs)"
echo "         Data path: $DATA_PATH"
echo "    Editor enabled: $enable_editor"
echo "     Input enabled: $enable_input"
echo "     Loading scene: $loadingscene"
echo " Scene autoloading: $enable_autoload"
echo "          CXXFLAGS: $CXXFLAGS"
echo "           LDFLAGS: $LDFLAGS"
echo

AC_OUTPUT
