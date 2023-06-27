COMPONENT = gb.gtk3.x11
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.gtk3.x11.la

gb_gtk3_x11_la_LIBADD = @GTK3X11_LIB@
gb_gtk3_x11_la_LDFLAGS = -module @LD_FLAGS@ @GTK3X11_LDFLAGS@
gb_gtk3_x11_la_CFLAGS = @GTK3X11_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_gtk3_x11_la_SOURCES = \
 main.h main.c \
 x11.h x11.c
