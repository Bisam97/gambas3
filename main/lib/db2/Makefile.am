COMPONENT = gb.db2
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.db2.la

gb_db2_la_LIBADD = @C_LIB@
gb_db2_la_LDFLAGS = -module @LD_FLAGS@
gb_db2_la_CFLAGS = -I$(top_srcdir)/share $(AM_CFLAGS)

gb_db2_la_SOURCES = \
 gb.db.h \
 main.h main.c \
 deletemap.h deletemap.c

