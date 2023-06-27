dnl ---- configure.ac for gb.sdl

m4_include([../version.m4])
AC_INIT([gambas3-gb-sdl-sound],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.sdl.sound)
LT_INIT

dnl TODO: add a version test in GB_COMPONENT!
GB_PATH_SDL(1.2.8, [] , [touch DISABLED])

dnl check for SDL libs
GB_COMPONENT_PKG_CONFIG(
  sdlsound, SDLSOUND, gb.sdl.sound, [src],
  SDL_mixer)

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
