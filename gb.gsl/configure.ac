dnl ---- configure.ac for gb.gsl

m4_include([../version.m4])
AC_INIT([gambas3-gb-gsl],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.gsl)
LT_INIT

GB_COMPONENT_SEARCH(
  gsl, GSL, gb.gsl, [src],
  gsl,
  [GB_FIND(gsl/gsl_cblas.h gsl/gsl_math.h, /usr /usr/local `gsl-config --prefix`, include include/gsl)],
  [GB_FIND(libgslcblas.$SHLIBEXT libgsl.$SHLIBEXT, /usr/lib /usr/local /usr/local/lib `gsl-config --prefix`, lib)],
  [$X_LIBS -llibgsl -llibgslcblas])

AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT
GB_PRINT_MESSAGES
