dnl ---- configure.ac for gb.media

m4_include([../version.m4])
AC_INIT([gambas3-gb-media],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.media)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  media, MEDIA, gb.media, [src],
  gstreamer-1.0 gstreamer-video-1.0
)
AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT

GB_PRINT_MESSAGES
