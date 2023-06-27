COMPONENT = gb.gtk3.wayland
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.gtk3.wayland.la

gb_gtk3_wayland_la_LIBADD = @GTK3WAYLAND_LIB@
gb_gtk3_wayland_la_LDFLAGS = -module @LD_FLAGS@ @GTK3WAYLAND_LDFLAGS@
gb_gtk3_wayland_la_CFLAGS = @GTK3WAYLAND_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_gtk3_wayland_la_SOURCES = \
 main.h main.c
