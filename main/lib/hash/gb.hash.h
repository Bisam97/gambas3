/***************************************************************************

  gb.hash.h

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

#ifndef __GB_HASH_H
#define __GB_HASH_H

#include "gambas.h"

enum
{
	HASH_MD5,
	HASH_SHA1,
	HASH_SHA256,
	HASH_SHA512,
	HASH_CRC32
};

#define HASH_INTERFACE_VERSION 1

typedef
	struct {
		int version;
			bool (*Begin)(int hash);
			void (*Process)(const void *buffer, size_t len);
			char *(*End)();
		}
	HASH_INTERFACE;

#endif



