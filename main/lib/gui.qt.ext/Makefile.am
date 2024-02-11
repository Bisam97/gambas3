COMPONENT = gb.gui.qt.ext
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.gui.qt.ext.la

gb_gui_qt_ext_la_LIBADD =
gb_gui_qt_ext_la_LDFLAGS = -module @LD_FLAGS@
gb_gui_qt_ext_la_CFLAGS = -I$(top_srcdir)/share -I$(top_srcdir)/gbx @INCLTDL@ $(AM_CFLAGS)

gb_gui_qt_ext_la_SOURCES = \
 main.h main.c

