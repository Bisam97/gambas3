COMPONENT = gb.compress.zstd
include $(top_srcdir)/component.am

gblib_LTLIBRARIES = gb.compress.zstd.la

gb_compress_zstd_la_LIBADD = @ZSTD_LIB@
gb_compress_zstd_la_LDFLAGS = -module @LD_FLAGS@ @ZSTD_LDFLAGS@
gb_compress_zstd_la_CFLAGS = $(AM_CFLAGS) @ZSTD_INC@

gb_compress_zstd_la_SOURCES = main.h main.c
