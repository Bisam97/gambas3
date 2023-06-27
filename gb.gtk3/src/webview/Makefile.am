COMPONENT = gb.gtk3.webview
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.gtk3.webview.la

gb_gtk3_webview_la_LIBADD = @GTK3_LIB@ @GTK3WEBVIEW_LIB@
gb_gtk3_webview_la_LDFLAGS = -module @LD_FLAGS@ @GTK3_LDFLAGS@ @GTK3WEBVIEW_LDFLAGS@
gb_gtk3_webview_la_CPPFLAGS = @GTK3_INC@ @GTK3WEBVIEW_INC@

gb_gtk3_webview_la_SOURCES = \
 main.h main.c \
 c_webview.h c_webview.c \
 c_websettings.h c_websettings.c

