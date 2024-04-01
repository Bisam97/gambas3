dnl ---- configure.ac for gb.qt6

m4_include([../version.m4])
AC_INIT([gambas3-gb-qt6],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.qt6)
LT_INIT
PKG_PROG_PKG_CONFIG

GB_CHECK_XWINDOW
AM_CONDITIONAL(XWINDOW, test x"$have_x" = xyes)

GB_COMPONENT_PKG_CONFIG(
  qt6, QT6, gb.qt6, [src],
  Qt6Core Qt6Gui Qt6Widgets Qt6Svg Qt6PrintSupport
)

GB_COMPONENT_PKG_CONFIG(
  qt6x11, QT6X11, gb.qt6.x11, [x11],
  Qt6Core Qt6Gui Qt6Widgets x11-xcb
)

GB_COMPONENT_PKG_CONFIG(
  qt6wayland, QT6WAYLAND, gb.qt6.wayland, [wayland],
  Qt6Core Qt6Gui Qt6Widgets
)

GB_COMPONENT_PKG_CONFIG(
  qt6webview, QT6WEBVIEW, gb.qt6.webview, [webview],
  Qt6Core Qt6WebEngineCore Qt6WebEngineWidgets
)

GB_COMPONENT_PKG_CONFIG(
  qt6opengl, QT6OPENGL, gb.qt6.opengl, [opengl],
  Qt6Core Qt6Gui Qt6Widgets Qt6OpenGL Qt6OpenGLWidgets gl
)

GB_COMPONENT_PKG_CONFIG(
  qt6ext, QT6EXT, gb.qt6.ext, [ext],
  Qt6Core Qt6Gui Qt6Widgets
)

MOC=`$PKG_CONFIG --variable=libexecdir Qt6Core`/moc
AC_SUBST(MOC)

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
src/x11/Makefile \
src/wayland/Makefile \
src/webview/Makefile \
src/opengl/Makefile \
src/ext/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
