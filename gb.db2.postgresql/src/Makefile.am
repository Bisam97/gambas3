COMPONENT = gb.db2.postgresql
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.db2.postgresql.la

gb_db2_postgresql_la_LIBADD = @POSTGRESQL_LIB@
gb_db2_postgresql_la_LDFLAGS = -module @LD_FLAGS@ @POSTGRESQL_LDFLAGS@
gb_db2_postgresql_la_CFLAGS = $(AM_CFLAGS)
gb_db2_postgresql_la_CPPFLAGS = @POSTGRESQL_INC@

gb_db2_postgresql_la_SOURCES = \
 main.h main.c
