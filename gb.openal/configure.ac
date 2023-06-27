dnl ---- configure.ac for gb.openal

m4_include([../version.m4])
AC_INIT([gambas3-gb-openal],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.openal)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  openal,
  OPENAL,
  gb.openal,
  [src],
  ['openal >= 1.13' alure])
AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT
GB_PRINT_MESSAGES
