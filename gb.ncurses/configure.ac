dnl ---- configure.ac for gb.ncurses

m4_include([../version.m4])
AC_INIT([gambas3-gb-ncurses],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.ncurses)
LT_INIT
PKG_PROG_PKG_CONFIG

# Most distributions I have seen apparently provide the ncurses and panel
# pkg-config files (which may be synonymous with ncursesw and panelw for
# wide-character support). Solus only provides the *w versions.
#
# We should prefer linking with the wide-character version but still
# allow to compile on systems that don't have ncursesw pkg-config files.
$PKG_CONFIG --silence-errors --exists ncursesw panelw
if test $? -eq "0"
  then gb_ncurses_pkgconfig_names="ncursesw panelw"
  else gb_ncurses_pkgconfig_names="ncurses panel"
fi

GB_COMPONENT_PKG_CONFIG(
  ncurses, NCURSES, gb.ncurses, [src],
  $gb_ncurses_pkgconfig_names)

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
