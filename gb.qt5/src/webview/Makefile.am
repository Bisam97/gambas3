COMPONENT = gb.qt5.webview
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt5.webview.la

gb_qt5_webview_la_LIBADD = @QT5WEBVIEW_LIB@
gb_qt5_webview_la_LDFLAGS = -module @LD_FLAGS@ @QT5WEBVIEW_LDFLAGS@
gb_qt5_webview_la_CPPFLAGS = @QT5WEBVIEW_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt5_webview_la_SOURCES = \
 main.h main.cpp \
 c_webview.h c_webview.cpp c_webview_moc.cpp \
 c_websettings.h c_websettings.cpp
