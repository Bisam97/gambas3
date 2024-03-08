COMPONENT = gb.qt6.ext
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt6.ext.la

gb_qt6_ext_la_LIBADD = @QT6EXT_LIB@
gb_qt6_ext_la_LDFLAGS = -module @LD_FLAGS@ @QT6EXT_LDFLAGS@
gb_qt6_ext_la_CXXFLAGS = @THREAD_INC@ $(AM_CXXFLAGS) $(GB_CXXFLAGS_STD_CPP17)
gb_qt6_ext_la_CPPFLAGS = @QT6EXT_INC@ -I$(top_srcdir)/share -I$(top_srcdir)/src/share

gb_qt6_ext_la_SOURCES = \
 main.h main.cpp \
 CLCDNumber.h CLCDNumber_moc.cpp CLCDNumber.cpp \
 CDial.h CDial_moc.cpp CDial.cpp \
 CTextEdit.h CTextEdit_moc.cpp CTextEdit.cpp
