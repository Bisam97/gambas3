/***************************************************************************

  c_hash.c

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

#define __C_HASH_C

#include "main.h"
#include "c_hash.h"

static void do_hash(int algo, const char *data, int len)
{
	HASH_begin(algo);
	HASH_process(data, len);
	GB.ReturnString(HASH_end());
}

//-------------------------------------------------------------------------

BEGIN_METHOD(Hash_Md5, GB_STRING data)

	do_hash(HASH_MD5, STRING(data), LENGTH(data));

END_METHOD

BEGIN_METHOD(Hash_Sha1, GB_STRING data)

	do_hash(HASH_SHA1, STRING(data), LENGTH(data));

END_METHOD

BEGIN_METHOD(Hash_Sha256, GB_STRING data)

	do_hash(HASH_SHA256, STRING(data), LENGTH(data));

END_METHOD

BEGIN_METHOD(Hash_Sha512, GB_STRING data)

	do_hash(HASH_SHA512, STRING(data), LENGTH(data));

END_METHOD

//-------------------------------------------------------------------------

GB_DESC HashDesc[] =
{
	GB_DECLARE_STATIC("Hash"),

	GB_STATIC_METHOD("Md5", "s", Hash_Md5, "(Data)s"),
	GB_STATIC_METHOD("Sha1", "s", Hash_Sha1, "(Data)s"),
	GB_STATIC_METHOD("Sha256", "s", Hash_Sha256, "(Data)s"),
	GB_STATIC_METHOD("Sha512", "s", Hash_Sha512, "(Data)s"),

	GB_END_DECLARE
};

