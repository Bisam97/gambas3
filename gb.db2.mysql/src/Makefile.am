COMPONENT = gb.db2.mysql
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.db2.mysql.la

gb_db2_mysql_la_LIBADD = @MYSQL_LIB@
gb_db2_mysql_la_LDFLAGS = -module @LD_FLAGS@ @MYSQL_LDFLAGS@
gb_db2_mysql_la_CPPFLAGS = @MYSQL_INC@

gb_db2_mysql_la_SOURCES = \
 main.h main.c
