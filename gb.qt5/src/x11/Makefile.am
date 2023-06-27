COMPONENT = gb.qt5.x11
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt5.x11.la

gb_qt5_x11_la_LIBADD = @QT5X11_LIB@
gb_qt5_x11_la_LDFLAGS = -module @LD_FLAGS@ @QT5X11_LDFLAGS@
gb_qt5_x11_la_CXXFLAGS = @THREAD_INC@ $(AM_CXXFLAGS) -std=c++11
gb_qt5_x11_la_CPPFLAGS = @QT5X11_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt5_x11_la_SOURCES = \
 main.h main.cpp \
 x11.h x11.c
