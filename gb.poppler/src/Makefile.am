COMPONENT = gb.poppler
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.poppler.la

gb_poppler_la_LIBADD = @POPPLER_LIB@
gb_poppler_la_LDFLAGS = -module @LD_FLAGS@ @POPPLER_LDFLAGS@
gb_poppler_la_CXXFLAGS = $(AM_CXXFLAGS) $(GB_CXXFLAGS_STD_CPP20)
gb_poppler_la_CPPFLAGS = @POPPLER_INC@

gb_poppler_la_SOURCES = main.h main.cpp \
	c_pdf_document.h c_pdf_document.cpp
