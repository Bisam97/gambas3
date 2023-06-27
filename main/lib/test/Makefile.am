COMPONENT = gb.test
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.test.la

gb_test_la_LIBADD = @C_LIB@
gb_test_la_LDFLAGS = -module @LD_FLAGS@
gb_test_la_CFLAGS = -I$(top_srcdir)/share -I$(top_srcdir)/gbx $(AM_CFLAGS)

gb_test_la_SOURCES = \
 main.h main.c
 


