dnl ---- configure.ac for gb.image.imlib

m4_include([../version.m4])
AC_INIT([gambas3-gb-image-imlib],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.image.imlib)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  image_imlib, IMAGE_IMLIB, gb.image.imlib, [src],
  'imlib2 >= 1.4.0'
  )

AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT

GB_PRINT_MESSAGES
