COMPONENT = gb.qt6.x11
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt6.x11.la

gb_qt6_x11_la_LIBADD = @QT6X11_LIB@
gb_qt6_x11_la_LDFLAGS = -module @LD_FLAGS@ @QT6X11_LDFLAGS@
gb_qt6_x11_la_CXXFLAGS = @THREAD_INC@ $(AM_CXXFLAGS) $(GB_CXXFLAGS_STD_CPP17)
gb_qt6_x11_la_CPPFLAGS = @QT6X11_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt6_x11_la_SOURCES = \
 main.h main.cpp \
 x11.h x11.c
