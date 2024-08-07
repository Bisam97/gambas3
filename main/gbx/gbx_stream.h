/***************************************************************************

	gbx_stream.h

	(c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#ifndef __GBX_STREAM_H
#define __GBX_STREAM_H

#include "gb_hash.h"
#include "gbx_value.h"
#include "gbx_archive.h"

union STREAM;

typedef
	struct STREAM_CLASS {
		int (*open)(union STREAM *stream, const char *path, int mode, void *data);
		int (*close)(union STREAM *stream);
		int (*read)(union STREAM *stream, char *buffer, int len);
		int (*write)(union STREAM *stream, char *buffer, int len);
		int (*seek)(union STREAM *stream, int64_t pos, int whence);
		int (*tell)(union STREAM *stream, int64_t *pos);
		int (*flush)(union STREAM *stream);
		int (*eof)(union STREAM *stream);
		int (*lof)(union STREAM *stream, int64_t *len);
		int (*handle)(union STREAM *stream);
		}
	STREAM_CLASS;

typedef
	struct {
		union STREAM *redirect;
		char *buffer;
		short buffer_pos;
		short buffer_len;
		char *unread;
		int unread_pos;
		int unread_len;
		void **read_objects;
		HASH_TABLE *write_objects;
	}
	STREAM_EXTRA;
	
typedef
	struct {
		STREAM_CLASS *type;
		short mode;
		unsigned swap : 1;
		unsigned eol : 2;
		unsigned eof : 1;
		unsigned no_fionread : 1;
		unsigned no_lseek : 1;
		unsigned available_now : 1;
		unsigned standard : 1;
		unsigned blocking : 1;
		unsigned redirected : 1;
		unsigned no_read_ahead : 1;
		unsigned null_terminated : 1;
		unsigned check_read : 1;
		unsigned no_share : 1;
		unsigned _reserved : 1;
		#if __WORDSIZE == 64
		unsigned _reserved2 : 32;
		#endif
		STREAM_EXTRA *extra;
		#if DEBUG_STREAM
		int tag;
		#endif
		}
	STREAM_COMMON;

typedef
	struct {
		STREAM_COMMON common;
		int _reserved[6];
		}
	STREAM_RESERVED;

typedef
	struct {
		STREAM_COMMON common;
		int64_t size;
		int fd;
		unsigned watch : 1;
		unsigned has_size : 1;
		unsigned use_size : 1;
		}
	STREAM_DIRECT;

typedef
	struct {
		STREAM_COMMON common;
		int fd;
		unsigned can_write : 1;
		}
	STREAM_PIPE;

typedef
	struct {
		STREAM_COMMON common;
		FILE *file;
		}
	STREAM_BUFFER;

typedef
	struct {
		STREAM_COMMON common;
		char *addr;
		intptr_t pos;
		}
	STREAM_MEMORY;

typedef
	struct {
		STREAM_COMMON common;
		ARCHIVE *arch;
		int size;
		int start;
		int pos;
		}
	STREAM_ARCH;

typedef
	struct {
		STREAM_COMMON common;
		void *process;
		}
	STREAM_PROCESS;

typedef
	struct {
		STREAM_COMMON common;
		char *buffer;
		int pos;
		int size;
		}
	STREAM_STRING;

typedef
	struct {
		STREAM_COMMON common;
		intptr_t pos;
		}
	STREAM_NULL;

typedef
	union STREAM {
		STREAM_CLASS *type;
		STREAM_COMMON common;
		STREAM_RESERVED _reserved;
		STREAM_DIRECT direct;
		STREAM_BUFFER buffer;
		STREAM_PIPE pipe;
		STREAM_MEMORY memory;
		STREAM_ARCH arch;
		STREAM_PROCESS process;
		STREAM_STRING string;
		STREAM_NULL null;
		}
	STREAM;

enum {
	ST_EOL_UNIX = 0,
	ST_EOL_WINDOWS = 1,
	ST_EOL_MAC = 2
	};

#ifndef __STREAM_IMPL_C

EXTERN STREAM_CLASS STREAM_direct;
EXTERN STREAM_CLASS STREAM_lock;
EXTERN STREAM_CLASS STREAM_buffer;
EXTERN STREAM_CLASS STREAM_pipe;
EXTERN STREAM_CLASS STREAM_memory;
EXTERN STREAM_CLASS STREAM_arch;
EXTERN STREAM_CLASS STREAM_process;
EXTERN STREAM_CLASS STREAM_string;
EXTERN STREAM_CLASS STREAM_null;

#else

#define DECLARE_STREAM(stream) \
STREAM_CLASS stream = \
{ \
	(void *)stream_open, \
	(void *)stream_close, \
	(void *)stream_read, \
	(void *)stream_write, \
	(void *)stream_seek, \
	(void *)stream_tell, \
	(void *)stream_flush, \
	(void *)stream_eof, \
	(void *)stream_lof, \
	(void *)stream_handle \
}

#endif

#define STREAM_BUFFER_SIZE 1024

bool STREAM_in_archive(const char *path);

void STREAM_open(STREAM *stream, const char *path, int mode);

void STREAM_release(STREAM *stream);
void STREAM_close(STREAM *stream);
void STREAM_write(STREAM *stream, void *addr, int len);
char *STREAM_line_input(STREAM *stream, char *escape);
char *STREAM_input(STREAM *stream);
int64_t STREAM_tell(STREAM *stream);
void STREAM_seek(STREAM *stream, int64_t pos, int whence);
int STREAM_read(STREAM *stream, void *addr, int len);
void STREAM_peek(STREAM *stream, void *addr, int len);
int STREAM_read_max(STREAM *stream, void *addr, int len);
bool STREAM_read_ahead(STREAM *stream);
//char STREAM_getchar(STREAM *stream);
void STREAM_read_type(STREAM *stream, TYPE type, VALUE *value);
void STREAM_write(STREAM *stream, void *addr, int len);
void STREAM_write_zeros(STREAM *stream, int len);
void STREAM_write_type(STREAM *stream, TYPE type, VALUE *value);
void STREAM_write_eol(STREAM *stream);
void STREAM_flush(STREAM *stream);
int STREAM_handle(STREAM *stream);
bool STREAM_lof_safe(STREAM *stream, int64_t *len);
void STREAM_lof(STREAM *stream, int64_t *len);
bool STREAM_eof(STREAM *stream);
bool STREAM_default_eof(STREAM *stream);

void STREAM_load(const char *path, char **buffer, int *len);

bool STREAM_map(const char *path, char **paddr, int *plen);
void STREAM_unmap(char *addr, int len);

int STREAM_lock_all_fd(int fd);
bool STREAM_lock_all(STREAM *stream);

#define STREAM_is_closed(_stream) ((_stream)->type == NULL)
#define STREAM_is_closed_for_writing(_stream) (STREAM_is_closed(_stream) && !(_stream)->common.redirected)

void STREAM_blocking(STREAM *stream, bool block);
#define STREAM_is_blocking(_stream) ((_stream)->common.blocking)
void STREAM_check_blocking(STREAM *stream);

bool STREAM_get_readable(STREAM *stream, int *len);

void STREAM_exit(void);

void STREAM_begin(STREAM *stream);
void STREAM_cancel(STREAM *stream);
void STREAM_end(STREAM *stream);

#endif
