COMPONENT = gb.qt4.webview
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt4.webview.la

gb_qt4_webview_la_LIBADD = @QTWEBVIEW_LIB@
gb_qt4_webview_la_LDFLAGS = -module @LD_FLAGS@ @QTWEBVIEW_LDFLAGS@
gb_qt4_webview_la_CPPFLAGS = @QTWEBVIEW_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt4_webview_la_SOURCES = \
 main.h main.cpp \
 c_websettings.h c_websettings.cpp \
 c_webview.h c_webview.cpp c_webview_moc.cpp \
 jsonwriter.h jsonwriter.cpp

