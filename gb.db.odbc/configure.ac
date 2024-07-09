dnl ---- configure.ac for gb.db.odbc driver

dnl ---- Initialization

m4_include([../version.m4])
AC_INIT([gambas3-gb-db-odbc],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.db.odbc)
LT_INIT

dnl ---- ODBC driver

GB_COMPONENT_SEARCH(
  odbc, ODBC, gb.db.odbc, [src],
  odbc, 
  [GB_FIND(sql.h sqlext.h sqltypes.h, $prefix /usr/local/lib /usr/local /opt /usr/lib /usr, include odbc*/include)],
  [GB_FIND(libodbc.$SHLIBEXT, $prefix /usr/local /opt /usr /opt/local, lib odbc*/lib )],
  [$C_LIB -lodbc])
 
dnl ---- Create makefiles

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
