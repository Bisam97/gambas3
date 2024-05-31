dnl ---- configure.ac for gb.pdf

m4_include([../version.m4])
AC_INIT([gambas3-gb-pdf],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.pdf)
LT_INIT
PKG_PROG_PKG_CONFIG

GB_COMPONENT_PKG_CONFIG(
  poppler, POPPLER, gb.pdf, [src],
  'poppler >= 0.20'
)

if test "$have_poppler" = "yes"; then
  $PKG_CONFIG --atleast-version=0.58.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_58, $((1-$?)), Poppler version >= 0.58)
  $PKG_CONFIG --atleast-version=0.64.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_64, $((1-$?)), Poppler version >= 0.64)
  $PKG_CONFIG --atleast-version=0.71.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_71, $((1-$?)), Poppler version >= 0.71)
  $PKG_CONFIG --atleast-version=0.72.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_72, $((1-$?)), Poppler version >= 0.72)
  $PKG_CONFIG --atleast-version=0.76.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_76, $((1-$?)), Poppler version >= 0.76)
  $PKG_CONFIG --atleast-version=0.83.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_83, $((1-$?)), Poppler version >= 0.83)
  $PKG_CONFIG --atleast-version=0.85.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_85, $((1-$?)), Poppler version >= 0.85)
  $PKG_CONFIG --atleast-version=0.86.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_0_86, $((1-$?)), Poppler version >= 0.86)
  $PKG_CONFIG --atleast-version=21.06.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_21_06_0, $((1-$?)), Poppler version >= 21.06.0)
  $PKG_CONFIG --atleast-version=22.06.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_22_06_0, $((1-$?)), Poppler version >= 22.06.0)
  $PKG_CONFIG --atleast-version=24.02.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_24_02_0, $((1-$?)), Poppler version >= 24.02.0)
  $PKG_CONFIG --atleast-version=24.05.0 poppler
  AC_DEFINE_UNQUOTED(POPPLER_VERSION_24_05_0, $((1-$?)), Poppler version >= 24.05.0)
fi

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
