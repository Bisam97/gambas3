/***************************************************************************

  main.c

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __MAIN_C

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "gb_common.h"
#include "hash.h"
#include "c_hash.h"
#include "main.h"

const GB_INTERFACE *GB_PTR EXPORT;

static union {
  //sha3_ctx_t sha3;
  sha512_ctx_t sha512;
  sha256_ctx_t sha256;
  sha1_ctx_t sha1;
  md5_ctx_t md5;
} _context;

static int _algo = -1;
static void (*_update)(void*, const void*, size_t);
static unsigned (*_final)(void*, void*);
static int _hash_len;

bool HASH_begin(int algo)
{
	switch(algo)
	{
		case HASH_MD5:
			md5_begin(&_context.md5);
			_update = (void*)md5_hash;
			_final = (void*)md5_end;
			_hash_len = 16;
			break;

		case HASH_SHA1:
			sha1_begin(&_context.sha1);
			_update = (void*)sha1_hash;
			_final = (void*)sha1_end;
			_hash_len = 20;
			break;

		case HASH_SHA256:
			sha256_begin(&_context.sha256);
			_update = (void*)sha256_hash;
			_final = (void*)sha256_end;
			_hash_len = 32;
			break;

		case HASH_SHA512:
			sha512_begin(&_context.sha512);
			_update = (void*)sha512_hash;
			_final = (void*)sha512_end;
			_hash_len = 64;
			break;

		default:
			return TRUE;
	}

	_algo = algo;
	return FALSE;
}

void HASH_process(const void *data, size_t len)
{
	if (_algo < 0)
		return;

	(*_update)(&_context, data, len);
}

char *HASH_end(void)
{
	static const char hex_digit[16] = "0123456789ABCDEF";

	char *result = NULL;
	unsigned char hash[64];
	char hex[2];
	int size;
	int i;

	if (_algo < 0)
		return NULL;

	size = (*_final)(&_context, (void *)&hash[0]);

	for (i = 0; i < size; i++)
	{
		hex[0] = hex_digit[hash[i] >> 4];
		hex[1] = hex_digit[hash[i] & 15];
		result = GB.AddString(result, hex, 2);
	}

	GB.FreeStringLater(result);
	return result;
}

/*void *GB_HASH_1[] EXPORT = {

  (void *)1,
  HASH_begin,
  HASH_process,
  HASH_end,
  NULL
  };*/

GB_DESC *GB_CLASSES[] EXPORT =
{
	HashDesc,
  NULL
};

int EXPORT GB_INIT(void)
{
  return 0;
}


void EXPORT GB_EXIT()
{
}

