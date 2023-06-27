COMPONENT = gb.qt5.wayland
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt5.wayland.la

gb_qt5_wayland_la_LIBADD = @QT5WAYLAND_LIB@
gb_qt5_wayland_la_LDFLAGS = -module @LD_FLAGS@ @QT5WAYLAND_LDFLAGS@
gb_qt5_wayland_la_CXXFLAGS = @THREAD_INC@ $(AM_CXXFLAGS) -std=c++11
gb_qt5_wayland_la_CPPFLAGS = @QT5WAYLAND_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt5_wayland_la_SOURCES = \
 main.h main.cpp
