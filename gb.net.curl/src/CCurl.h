/***************************************************************************

  CCurl.h

  Advanced Network component

  (c) 2003-2008 Daniel Campos Fernández <dcamposf@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/
#ifndef __CCURL_H
#define __CCURL_H

#include "gambas.h"
#include "gbcurl.h"
#include "CProxy.h"
#include <curl/curl.h>
#include <curl/easy.h>

#ifndef __CCURL_C


extern GB_DESC CCurlDesc[];
extern GB_STREAM_DESC CurlStream;

#else

#define THIS            ((CCURL *)_object)
#define THIS_STATUS     ((curlData*)THIS->stream._free[0])->status
#define THIS_CURL       ((curlData*)THIS->stream._free[0])->curl
#define THIS_URL        ((curlData*)THIS->stream._free[0])->url
#define THIS_FILE       ((curlData*)THIS->stream._free[0])->file
#define THIS_PROTOCOL   ((curlData*)THIS->stream._free[0])->protocol

#endif

typedef  struct
{
	int *parent_status;
	Adv_proxy proxy;
}  
CPROXY;

typedef  struct
{
	GB_BASE    ob;
	GB_STREAM  stream;
	CPROXY     proxy;
	Adv_user   user;
	int        len_data;
	char       *buf_data;
	GB_VARIANT_VALUE tag;
	int mode; // 0 -> Async, sync
	long TimeOut;

	int iMethod; // 0->Get, 1->Put

	int   ReturnCode;
	char *ReturnString;
}  CCURL;

void CCURL_stop(void *_object);

void CCURL_stream_init  (GB_STREAM *stream,int fd);
int  CCURL_stream_read  (GB_STREAM *stream, char *buffer, int len);
int  CCURL_stream_write (GB_STREAM *stream, char *buffer, int len);
int  CCURL_stream_eof   (GB_STREAM *stream);
int  CCURL_stream_lof   (GB_STREAM *stream, int64_t *len);
int  CCURL_stream_open  (GB_STREAM *stream, const char *path, int mode, void *data);
int  CCURL_stream_seek  (GB_STREAM *stream, int64_t pos, int whence);
int  CCURL_stream_tell  (GB_STREAM *stream, int64_t *pos);
int  CCURL_stream_flush (GB_STREAM *stream);
int  CCURL_stream_close (GB_STREAM *stream);
int  CCURL_stream_handle(GB_STREAM *stream);

void CCURL_raise_finished (long lParam);
void CCURL_raise_error    (long lParam);
void CCURL_raise_connect  (long lParam);
void CCURL_raise_read     (long lParam);

void CCURL_init_post(void);
void CCURL_post_curl(long data);

void CCURL_Manage_ErrCode(void *_object,long ErrCode);


#endif
