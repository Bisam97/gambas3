COMPONENT = gb.qt6.opengl
include $(top_srcdir)/component.am
include $(top_srcdir)/gb.qt.am

gblib_LTLIBRARIES = gb.qt6.opengl.la

gb_qt6_opengl_la_LIBADD = @QT6OPENGL_LIB@ @QT6OPENGL_LIB@
gb_qt6_opengl_la_LDFLAGS = -module @LD_FLAGS@ @QT6OPENGL_LDFLAGS@
gb_qt6_opengl_la_CXXFLAGS = @THREAD_INC@ $(AM_CXXFLAGS) $(GB_CXXFLAGS_STD_CPP17)
gb_qt6_opengl_la_CPPFLAGS = @QT6OPENGL_INC@

gb_qt6_opengl_la_SOURCES = main.h main.cpp CGLarea_moc.cpp CGLarea.h CGLarea.cpp
 
 

 

