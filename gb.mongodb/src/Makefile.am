COMPONENT = gb.mongodb
include $(top_srcdir)/component.am
gblib_LTLIBRARIES = gb.mongodb.la
gb_mongodb_la_LIBADD = @MONGODB_LIB@
gb_mongodb_la_LDFLAGS = -module @LD_FLAGS@ @MONGODB_LDFLAGS@
gb_mongodb_la_CPPFLAGS = @MONGODB_INC@

gb_mongodb_la_SOURCES = \
	main.c main.h \
	helper.c helper.h \
	c_mongoresult.h c_mongoresult.c \
	c_mongocollection.c c_mongocollection.h \
	c_mongoclient.c c_mongoclient.h

