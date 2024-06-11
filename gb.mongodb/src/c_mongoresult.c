/***************************************************************************

  c_mongoresult.c

  gb.mongodb component

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#define __C_MONGORESULT_C

#include "helper.h"
#include "c_mongoresult.h"



//--------------------------------------------------------------------------

BEGIN_METHOD_VOID(MongoResult_free)

	mongoc_cursor_destroy(THIS->cursor);
	GB.Unref(POINTER(&THIS->client));

END_METHOD

BEGIN_METHOD_VOID(MongoResult_next)

	const bson_t *doc;
	bson_error_t error;

	if (!mongoc_cursor_next(THIS->cursor, &doc))
	{
		GB.StopEnum();
		if (mongoc_cursor_error(THIS->cursor, &error))
			GB.Error("&1", error.message);
	}
	else
	{
		GB.ReturnObject(HELPER_from_bson(doc));
	}

END_METHOD

BEGIN_METHOD_VOID(MongoResult_MoveNext)

	const bson_t *doc;
	bson_error_t error;
	bool ok;
	
	ok = mongoc_cursor_next(THIS->cursor, &doc);
	
	if (mongoc_cursor_error(THIS->cursor, &error))
		GB.Error("&1", error.message);
	else
		GB.ReturnBoolean(!ok);

END_METHOD

BEGIN_PROPERTY(MongoResult_Current)

	const bson_t *doc;
	bson_error_t error;

	doc = mongoc_cursor_current(THIS->cursor);
	if (!doc)
	{
		if (mongoc_cursor_error(THIS->cursor, &error))
			GB.Error("&1", error.message);
		else
			GB.ReturnNull();
	}
	else
		GB.ReturnObject(HELPER_from_bson(doc));

END_PROPERTY


//--------------------------------------------------------------------------

GB_DESC MongoResultDesc[] = {

	GB_DECLARE("MongoResult", sizeof(CMONGORESULT)),
	GB_NOT_CREATABLE(),

	GB_METHOD("_free", NULL, MongoResult_free, NULL),
	
	GB_METHOD("_next", "Collection", MongoResult_next, NULL),
	
	GB_METHOD("MoveNext", "b", MongoResult_MoveNext, NULL),
	
	GB_PROPERTY_READ("Current", "Collection", MongoResult_Current),
	
	GB_END_DECLARE
};
