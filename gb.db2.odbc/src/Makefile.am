COMPONENT = gb.db2.odbc
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.db2.odbc.la

gb_db2_odbc_la_LIBADD = @ODBC_LIB@
gb_db2_odbc_la_LDFLAGS = -module @LD_FLAGS@ @ODBC_LDFLAGS@
gb_db2_odbc_la_CFLAGS = $(AM_CFLAGS)
gb_db2_odbc_la_CPPFLAGS = @ODBC_INC@

gb_db2_odbc_la_SOURCES = \
 main.h main.c
