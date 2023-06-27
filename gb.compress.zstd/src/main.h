/***************************************************************************

  main.h

  (c) 2019-present Laurent Carlier <lordheavym@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#ifndef __MAIN_H
#define __MAIN_H

#include "gambas.h"
#include "gb_common.h"
#include "../gb.compress.h"
#include <stdint.h>

#ifndef __MAIN_C
extern GB_INTERFACE GB;
extern COMPRESS_INTERFACE COMPRESSION;
extern GB_STREAM_DESC ZSTDStream;
#endif

static int ZSTD_stream_open(GB_STREAM *stream, const char *path, int mode, void *data);
static int ZSTD_stream_close(GB_STREAM *stream);
static int ZSTD_stream_read(GB_STREAM *stream, char *buffer, int len);
static int ZSTD_stream_write(GB_STREAM *stream, char *buffer, int len);
static int ZSTD_stream_seek(GB_STREAM *stream, int64_t offset, int whence);
static int ZSTD_stream_tell(GB_STREAM *stream, int64_t *npos);
static int ZSTD_stream_flush(GB_STREAM *stream);
static int ZSTD_stream_eof(GB_STREAM *stream);
static int ZSTD_stream_lof(GB_STREAM *stream, int64_t *len);

#endif /* __MAIN_H */
