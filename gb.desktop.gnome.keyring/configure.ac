dnl ---- configure.ac for gb.desktop.gnome.keyring

m4_include([../version.m4])
AC_INIT([gambas3-gb-desktop-gnome-keyring],[GB_VERSION],[GB_MAIL],[],[GB_URL])
AC_CONFIG_MACRO_DIR([m4])

GB_INIT(gb.desktop.gnome.keyring)
LT_INIT

GB_COMPONENT_PKG_CONFIG(
  gb_desktop_gnome_keyring,
  DESKTOP_GNOME_KEYRING,
  gb.desktop.gnome.keyring,
  [src],
  gnome-keyring-1)

AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT
GB_PRINT_MESSAGES
