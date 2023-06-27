dnl ---- configure.ac for gb.opengl

m4_include([../version.m4])
AC_INIT([gambas3-gb-opengl],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.opengl)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  opengl, OPENGL, gb.opengl, [src],
  [gl glew])

GB_COMPONENT_PKG_CONFIG(
  glsl, GLSL, gb.opengl.glsl, [glsl],
  [gl glew])

GB_COMPONENT_PKG_CONFIG(
  glu, GLU, gb.opengl.glu, [glu],
  [glu])

GB_COMPONENT_PKG_CONFIG(
  sge, SGE, gb.opengl.sge, [sge],
	[gl glew])

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
src/glu/Makefile \
src/glsl/Makefile \
src/sge/Makefile
])
AC_OUTPUT

GB_PRINT_MESSAGES
