dnl ---- configure.ac for gb.pcre

m4_include([../version.m4])
AC_INIT([gambas3-gb-pcre],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.pcre)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  pcre, PCRE, gb.pcre, [src],
  libpcre2-8)

if test "$have_pcre" != "yes"; then

  GB_COMPONENT_PKG_CONFIG_AGAIN(
    pcre, PCRE, gb.pcre, [src],
    libpcre, [trying with pcre])
  gb_use_pcre2=0
  AC_MSG_NOTICE([Using libpcre])

else

  AC_DEFINE_UNQUOTED(PCRE2, 1, libpcre2 is used)
  gb_use_pcre2=1
  AC_MSG_NOTICE([Using libpcre2])

fi

AM_CONDITIONAL(USE_PCRE2, test "$gb_use_pcre2" = 1)

AC_CONFIG_FILES([\
Makefile \
src/Makefile \
])
AC_OUTPUT

GB_PRINT_MESSAGES
