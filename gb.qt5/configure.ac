dnl ---- configure.ac for gb.qt5

m4_include([../version.m4])
AC_INIT([gambas3-gb-qt5],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.qt5)
LT_INIT
PKG_PROG_PKG_CONFIG

GB_CHECK_XWINDOW
AM_CONDITIONAL(XWINDOW, test x"$have_x" = xyes)

GB_COMPONENT_PKG_CONFIG(
  qt5, QT5, gb.qt5, [src],
  'Qt5Core >= 5.3.0' Qt5Gui Qt5Widgets Qt5Svg Qt5PrintSupport
)

GB_COMPONENT_PKG_CONFIG(
  qt5x11, QT5X11, gb.qt5.x11, [x11],
  'Qt5Core >= 5.3.0' Qt5Gui Qt5Widgets Qt5X11Extras x11-xcb
)

GB_COMPONENT_PKG_CONFIG(
  qt5wayland, QT5WAYLAND, gb.qt5.wayland, [wayland],
  'Qt5Core >= 5.3.0' Qt5Gui Qt5Widgets
)

GB_COMPONENT_PKG_CONFIG(
  qt5webkit, QT5WEBKIT, gb.qt5.webkit, [webkit],
  'Qt5Core >= 5.3.0' Qt5Gui Qt5Widgets Qt5Network Qt5Xml Qt5WebKit Qt5WebKitWidgets Qt5PrintSupport
)

GB_COMPONENT_PKG_CONFIG(
  qt5webview, QT5WEBVIEW, gb.qt5.webview, [webview],
  'Qt5Core >= 5.5.0' Qt5WebEngineWidgets
)

$PKG_CONFIG --atleast-version 5.4.0 Qt5Core
if test $? != 0; then
	AC_DEFINE_UNQUOTED(QT_OLD_OPENGL, 1, Use deprecated OpenGL widget)
	QT_OPENGL_SUPPORT=Qt5OpenGL
	GB_MESSAGE([Qt version is <= 5.3, using deprecated OpenGL widget])
fi

AM_CONDITIONAL(QT_OLD_OPENGL, [test x$QT_OPENGL_SUPPORT != x])

GB_COMPONENT_PKG_CONFIG(
  qt5opengl, QT5OPENGL, gb.qt5.opengl, [opengl],
  'Qt5Core >= 5.3.0' Qt5Gui Qt5Widgets $QT_OPENGL_SUPPORT gl x11
)

GB_COMPONENT_PKG_CONFIG(
  qt5ext, QT5EXT, gb.qt5.ext, [ext],
  'Qt5Core >= 5.3.0' Qt5Gui Qt5Widgets x11
)

MOC=`$PKG_CONFIG --variable=host_bins Qt5Core`/moc
AC_SUBST(MOC)

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
src/x11/Makefile \
src/wayland/Makefile \
src/webkit/Makefile \
src/webview/Makefile \
src/opengl/Makefile \
src/ext/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
