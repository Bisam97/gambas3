dnl ---- configure.ac for gb.db.sqlite

m4_include([../version.m4])
AC_INIT([gambas3-gb-db2-sqlite3],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.db2.sqlite3)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  sqlite3, SQLITE3, gb.db2.sqlite3, [src],
  'sqlite3 >= 3.0')
 
AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
