/***************************************************************************

  gbx_stream_null.c

  (c) 2000-2019 Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __STREAM_IMPL_C

#include "gb_common.h"
#include "gbx_stream.h"


static int stream_open(STREAM *stream, const char *path, int mode)
{
  stream->null.pos = 0;
	stream->common.available_now = TRUE;

  return FALSE;
}


static int stream_close(STREAM *stream)
{
  return FALSE;
}


static int stream_read(STREAM *stream, char *buffer, int len)
{
	return 0;
}

static int stream_write(STREAM *stream, char *buffer, int len)
{
	stream->null.pos += len;
	return len;
}

static int stream_seek(STREAM *stream, int64_t pos, int whence)
{
  int64_t new_pos;

  switch(whence)
  {
    case SEEK_SET:
      new_pos = pos;
      break;

    case SEEK_CUR:
      new_pos = stream->null.pos + pos;
      break;

    case SEEK_END:
      return TRUE;

    default:
      return TRUE;
  }

  if (new_pos < 0)
    return TRUE;

  stream->null.pos = new_pos;
  return FALSE;
}


static int stream_tell(STREAM *stream, int64_t *pos)
{
  *pos = (int64_t)stream->null.pos;
  return FALSE;
}


static int stream_flush(STREAM *stream)
{
  return FALSE;
}


static int stream_eof(STREAM *stream)
{
  return TRUE;
}


static int stream_lof(STREAM *stream, int64_t *len)
{
  *len = 0;
  return TRUE;
}


static int stream_handle(STREAM *stream)
{
  return -1;
}


DECLARE_STREAM(STREAM_null);
