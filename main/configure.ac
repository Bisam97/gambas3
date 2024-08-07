dnl ---- configure.ac for main programs

m4_include([../version.m4])
AC_INIT([gambas3-main],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT([main])
GB_TRUNK_VERSION
GB_MATH_FUNC
LT_INIT([dlopen win32-dll])

AM_PROG_CC_C_O

CFLAGS=$gb_save_cflags
CXXFLAGS=$gb_save_cxxflags

if test "x${GAMBAS_CONFIG_FAILURE}" != "x"; then
  SAVE_GAMBAS_CONFIG_FAILURE=${GAMBAS_CONFIG_FAILURE}
  GAMBAS_CONFIG_FAILURE=''
fi

dnl ---- Check for internationalization library

if test "x$GB_INTL_LIB" != x; then

GB_COMPONENT(
  intl, INTL, libintl, [],
  [GB_FIND(libintl.h, /opt/local /usr/local /usr, include)],
  [GB_FIND(libintl.$SHLIBEXT, /opt/local /usr/local /usr /, lib)],
  [-lintl],
  [],
  [Cannot find 'libintl' library. It may be located inside the system C library, so you can ignore that warning...])

fi

dnl ---- Remove DISABLED file that could have been generated before

rm -f FAILED DISABLED DISABLED.*

dnl ---- Check for charset conversion library

if test "x$GB_ICONV_LIB" != x; then

GB_COMPONENT(
  conv, CONV, libiconv, [],
  [GB_FIND(iconv.h, /opt/local /usr/local /usr, include)],
  [GB_FIND(libiconv.$SHLIBEXT, /opt/local /usr/local /usr, lib)],
  [-liconv],
  [],
  [Cannot find 'libiconv' library. It may be merged inside the system C library, so you can ignore that warning...])

fi

dnl ---- Remove DISABLED file that could have been generated before

rm -f FAILED DISABLED DISABLED.*

dnl ---- Check for gettext library

if test "x$GETTEXT_LIB" != x; then

GB_COMPONENT(
  gettext, GETTEXT, libgettextlib, [],
  [],
  [GB_FIND(libgettextlib.$SHLIBEXT, /opt/local /usr/local /usr, lib)],
  [$GETTEXT_LIB],
  [],
  [Cannot find 'libgettextlib' library. It may be merged inside the system C library, so you can ignore that warning...])
  
fi

dnl ---- Remove DISABLED file that could have been generated before

rm -f FAILED DISABLED DISABLED.*

dnl ---- Check for ffi library

GB_COMPONENT_SEARCH(
  ffi, FFI, libffi, [],
  libffi,
  [GB_FIND(ffi.h, /usr/local /usr/local/lib/64 /usr/local/lib /usr /usr/lib64 /usr/lib /usr/lib/gcc/*/*, include ffi/include)],
  [GB_FIND(libffi.$SHLIBEXT, /usr/local /usr /usr/lib/gcc/*/*, lib .)],
  [-lffi])

if test -z "$FFI_LIB"; then
  GB_COMPONENT(
    ffi, FFI, libffi, [],
    [GB_FIND(ffi.h, /opt/local /usr/local /usr/local/lib /usr /usr/lib /usr/lib/gcc/*/*, include ffi/include)],
    [GB_FIND(libffi.$SHLIBEXT, /usr/local /usr, lib)],
    [-lffi])
fi

dnl ---- Remove DISABLED file that could have been generated before

rm -f FAILED DISABLED DISABLED.*

if test "x${SAVE_GAMBAS_CONFIG_FAILURE}" != "x"; then
  GAMBAS_CONFIG_FAILURE=${SAVE_GAMBAS_CONFIG_FAILURE}
  SAVE_GAMBAS_CONFIG_FAILURE=''
fi

dnl ---- Check for the Linux inotify headers

GB_COMPONENT(
  inotify, INOTIFY, gb.inotify, [inotify],
  [GB_FIND(sys/inotify.h, /usr/local/lib /usr/local /usr/lib /usr, include)],
  [GB_FIND(, /usr/local /usr, lib)],
  [$GB_INOTIFY_LIB])

dnl ---- We do not use libtool to load shared libraries anymore!

AC_DEFINE(DONT_USE_LTDL, 1, [Do not use libtool to load shared libraries])
if test "$SYSTEM" != "OPENBSD" && test "$SYSTEM" != "FREEBSD" && test "$SYSTEM" != "NETBSD"; then
  DL_LIB="-ldl"
else
  DL_LIB=""
fi

AC_SUBST(DL_LIB)

dnl ---- Check for Portland scripts

AC_CHECK_PROGS(XDG_UTILS, [xdg-mime xdg-icon-resource], [])

dnl ---- Create makefiles

AC_CONFIG_FILES([\
Makefile \
share/Makefile \
man/Makefile \
gbc/Makefile \
gbx/Makefile \
lib/Makefile \
lib/debug/Makefile \
lib/eval/Makefile \
lib/db/Makefile \
lib/vb/Makefile \
lib/compress/Makefile \
lib/option/Makefile \
lib/geom/Makefile \
lib/draw/Makefile \
lib/gui/Makefile \
lib/gui.opengl/Makefile \
lib/gui.webview/Makefile \
lib/gui.qt/Makefile \
lib/gui.qt.webkit/Makefile \
lib/gui.qt.opengl/Makefile \
lib/gui.qt.ext/Makefile \
lib/gui.trayicon/Makefile \
lib/image/Makefile \
lib/image.effect/Makefile \
lib/signal/Makefile \
lib/term/Makefile \
lib/complex/Makefile \
lib/data/Makefile \
lib/clipper/Makefile \
lib/inotify/Makefile \
lib/jit/Makefile \
lib/test/Makefile \
lib/hash/Makefile \
comp/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
