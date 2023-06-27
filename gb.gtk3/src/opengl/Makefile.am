COMPONENT = gb.gtk3.opengl
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.gtk3.opengl.la

gb_gtk3_opengl_la_LIBADD = @GTK3OPENGL_LIB@
gb_gtk3_opengl_la_LDFLAGS = -module @LD_FLAGS@ @GTK3OPENGL_LDFLAGS@
gb_gtk3_opengl_la_CPPFLAGS = @GTK3OPENGL_INC@

gb_gtk3_opengl_la_SOURCES = \
 main.h main.c \
 c_glarea.h c_glarea.c

