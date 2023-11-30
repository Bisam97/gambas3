/***************************************************************************

	gbx_c_file.h

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

#ifndef __GBX_C_FILE_H
#define __GBX_C_FILE_H

#include "gambas.h"

#include "gbx_value.h"
#include "gbx_stream.h"
#include "gbx_object.h"
#include "gb_file.h"

typedef
	struct {
		OBJECT ob;
		STREAM stream;
		GB_VARIANT_VALUE tag;
		}
	CSTREAM;

typedef
	struct {
		CSTREAM ob;
		}
	CFILE;

typedef
	struct {
		OBJECT ob;
		FILE_STAT info;
		char *path;
		}
	CSTAT;

#ifndef __GBX_C_FILE_C

extern GB_DESC StreamLinesDesc[];
extern GB_DESC StreamTermDesc[];
extern GB_DESC StreamDesc[];
extern GB_DESC FileDesc[];
extern GB_DESC StatDesc[];
extern GB_DESC StatPermDesc[];

extern mode_t CFILE_default_dir_auth;

#else

#define THIS ((CFILE *)_object)
#define THIS_STREAM ((CSTREAM *)_object)
#define THIS_STAT ((CSTAT *)_object)
#define THE_STREAM CSTREAM_TO_STREAM(THIS_STREAM)

#endif

#define CSTREAM_TO_STREAM(_cstream) (&((CSTREAM *)(void *)(_cstream))->stream)
#define CSTREAM_FROM_STREAM(_stream) ((CSTREAM *)((char *)(_stream) - sizeof(OBJECT)))

enum { CFILE_IN = 0, CFILE_OUT = 1, CFILE_ERR = 2 };

CFILE *CFILE_create(const STREAM *stream, int mode);
void CFILE_exit(void);
void CFILE_init_watch(void);
CFILE *CFILE_get_standard_stream(int num);

#endif
