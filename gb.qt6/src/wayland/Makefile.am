COMPONENT = gb.qt6.wayland
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt6.wayland.la

gb_qt6_wayland_la_LIBADD = @QT6WAYLAND_LIB@
gb_qt6_wayland_la_LDFLAGS = -module @LD_FLAGS@ @QT6WAYLAND_LDFLAGS@
gb_qt6_wayland_la_CXXFLAGS = @THREAD_INC@ $(AM_CXXFLAGS) $(GB_CXXFLAGS_STD_CPP17)
gb_qt6_wayland_la_CPPFLAGS = @QT6WAYLAND_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt6_wayland_la_SOURCES = \
 main.h main.cpp
