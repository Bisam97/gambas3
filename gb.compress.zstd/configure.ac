dnl ---- Initialization

m4_include([../version.m4])
AC_INIT([gambas3-gb-compress-zstd],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.compress.zstd)
LT_INIT

dnl ---- zstd compression driver

GB_COMPONENT_PKG_CONFIG(
  zstd, ZSTD, gb.compress.zstd, [src],
  'libzstd >= 1.3.3')

dnl ---- Create makefiles

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
