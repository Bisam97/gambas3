dnl ---- configure.ac for gb.mongodb

m4_include([../version.m4])
AC_INIT(gambas3-gb-mongodb,[GB_VERSION], [GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.mongodb)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  mongodb,
  MONGODB,
  gb.mongodb,
  [src],
  libmongoc-1.0)
AC_CONFIG_FILES( Makefile src/Makefile )
AC_OUTPUT

GB_PRINT_MESSAGES
