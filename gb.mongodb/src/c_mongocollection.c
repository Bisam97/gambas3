/***************************************************************************

  c_mongocollection.c

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

#define __C_MONGOCOLLECTION_C

#include "helper.h"
#include "c_mongocollection.h"



//--------------------------------------------------------------------------

BEGIN_METHOD_VOID(MongoCollection_free)

	mongoc_collection_destroy(THIS->collection);
	GB.Unref(POINTER(&THIS->client));

END_METHOD

BEGIN_PROPERTY(MongoCollection_Name)

	GB.ReturnNewZeroString(mongoc_collection_get_name(THIS->collection));

END_PROPERTY

BEGIN_METHOD(MongoCollection_Query, GB_OBJECT filter; GB_OBJECT options)

	mongoc_cursor_t *cursor;
	bson_t *filter = HELPER_to_bson(VARGOPT(filter, NULL), TRUE);
	bson_t *options = HELPER_to_bson(VARGOPT(options, NULL), TRUE);

	cursor = mongoc_collection_find_with_opts(THIS->collection, filter, options, NULL);
	
	bson_destroy(filter);
	bson_destroy(options);

	GB.ReturnObject(HELPER_create_result(THIS->client, cursor));
	
END_METHOD

BEGIN_PROPERTY(MongoCollection_Count)

	int64_t count;
	bson_error_t error;
	bson_t filter = BSON_INITIALIZER;

	count = mongoc_collection_count_documents(THIS->collection, &filter, NULL, NULL, NULL, &error);

	if (count < 0)
		GB.Error("&1", error.message);
	else
		GB.ReturnLong(count);
	
END_PROPERTY

BEGIN_METHOD(MongoCollection_Add, GB_OBJECT doc; GB_OBJECT options)

	bson_error_t error;
	bson_t *doc = HELPER_to_bson(VARG(doc), FALSE);
	bson_t *options = HELPER_to_bson(VARGOPT(options, NULL), TRUE);
	
	if (doc)
	{
		if (!mongoc_collection_insert_one(THIS->collection, doc, options, NULL, &error))
			GB.Error("&1", error.message);
	}
	
	bson_destroy(doc);
	bson_destroy(options);

END_METHOD

BEGIN_METHOD(MongoCollection_Remove, GB_OBJECT filter; GB_OBJECT options)

	bson_error_t error;
	bson_t *filter = HELPER_to_bson(VARGOPT(filter, NULL), TRUE);
	bson_t *options = HELPER_to_bson(VARGOPT(options, NULL), TRUE);

	if (!mongoc_collection_delete_many(THIS->collection, filter, options, NULL, &error))
		GB.Error("&1", error.message);
	
	bson_destroy(filter);
	bson_destroy(options);

END_METHOD

BEGIN_METHOD(MongoCollection_Replace, GB_OBJECT filter; GB_OBJECT doc; GB_OBJECT options)

	bool ok;
	bson_error_t error;
	bson_t *filter = HELPER_to_bson(VARGOPT(filter, NULL), TRUE);
	bson_t *options = HELPER_to_bson(VARGOPT(options, NULL), TRUE);
	bson_t *doc = HELPER_to_bson(VARG(doc), TRUE);

	if (doc)
	{
		ok = mongoc_collection_replace_one(THIS->collection, filter, doc, options, NULL, &error);
		bson_destroy(doc);
	}
	else
	{
		ok = mongoc_collection_delete_one(THIS->collection, filter, options, NULL, &error);
	}

	if (!ok)
		GB.Error("&1", error.message);
	
	bson_destroy(filter);
	bson_destroy(options);

END_METHOD

BEGIN_METHOD_VOID(MongoCollection_Delete)

	bson_error_t error;
	
	if (!mongoc_collection_drop(THIS->collection, &error))
		GB.Error("&1", error.message);

END_METHOD

BEGIN_METHOD(MongoCollection_get, GB_STRING id)

	mongoc_cursor_t *cursor;
	bson_t *filter;
	const bson_t *doc;
	bson_error_t error;

	if (!LENGTH(id))
	{
		GB.Error("Void document id");
		return;
	}
	
	filter = bson_new();
	bson_append_utf8(filter, "_id", 3, STRING(id), LENGTH(id));
	
	cursor = mongoc_collection_find_with_opts(THIS->collection, filter, NULL, NULL);
	
	bson_destroy(filter);

	if (mongoc_cursor_next(cursor, &doc))
		GB.ReturnObject(HELPER_from_bson(doc));
	else if (mongoc_cursor_error(cursor, &error))
		GB.Error("&1", error.message);
	
	mongoc_cursor_destroy(cursor);

END_METHOD

BEGIN_METHOD(MongoCollection_put, GB_OBJECT doc; GB_STRING id)

	bson_t *doc;
	bson_t *filter;
	bson_t *options;
	bson_error_t error;
	bool ok;

	if (!LENGTH(id))
	{
		GB.Error("Void document id");
		return;
	}
	
	doc = HELPER_to_bson_with_id(VARG(doc), STRING(id), LENGTH(id));
	
	filter = bson_new();
	bson_append_utf8(filter, "_id", 3, STRING(id), LENGTH(id));
	
	if (doc)
	{
		options = BCON_NEW("upsert", BCON_BOOL(TRUE));
		ok = mongoc_collection_replace_one(THIS->collection, filter, doc, options, NULL, &error);
		bson_destroy(doc);
		bson_destroy(options);
	}
	else
	{
		ok = mongoc_collection_delete_one(THIS->collection, filter, NULL, NULL, &error);
	}

	if (!ok)
		GB.Error("&1", error.message);
	
	bson_destroy(filter);

END_METHOD

BEGIN_METHOD(MongoCollection_Find, GB_OBJECT filter)

	mongoc_cursor_t *cursor;
	bson_t *filter;
	bson_t *options;
	const bson_t *doc;
	bson_iter_t iter;
	GB_ARRAY result;
	const char *id;
	uint len;

	filter = HELPER_to_bson(VARGOPT(filter, NULL), TRUE);
	options = BCON_NEW("projection", "{", "_id", BCON_BOOL(TRUE), "}");
	
	cursor = mongoc_collection_find_with_opts(THIS->collection, filter, options, NULL);
	
	bson_destroy(filter);
	bson_destroy(options);

	GB.Array.New(&result, GB_T_STRING, 0);
	
	while (mongoc_cursor_next(cursor, &doc))
	{
		if (!bson_iter_init_find_w_len(&iter, doc, "_id", 3))
			continue;
		if (!BSON_ITER_HOLDS_UTF8(&iter))
			continue;
		
		id = bson_iter_utf8(&iter, &len);
		*(char **)GB.Array.Add(result) = GB.NewString(id, len);
	}
	
	GB.ReturnObject(result);

END_METHOD

//--------------------------------------------------------------------------

GB_DESC MongoCollectionDesc[] = {

	GB_DECLARE("MongoCollection", sizeof(CMONGOCOLLECTION)),
	GB_NOT_CREATABLE(),

	GB_METHOD("_free", NULL, MongoCollection_free, NULL),
	
	GB_PROPERTY_READ("Name", "s", MongoCollection_Name),
	GB_PROPERTY_READ("Count", "l", MongoCollection_Count),
	
	GB_METHOD("Delete", NULL, MongoCollection_Delete, NULL),
	
	GB_METHOD("Query", "MongoResult", MongoCollection_Query, "[(Filter)Collection;(Options)Collection;]"),
	GB_METHOD("Remove", NULL, MongoCollection_Remove, "[(Filter)Collection;(Options)Collection;]"),
	GB_METHOD("Add", NULL, MongoCollection_Add, "(Document)Collection;[(Options)Collection;]"),
	GB_METHOD("Replace", NULL, MongoCollection_Replace, "(Filter)Collection;(Document)Collection;[(Options)Collection;]"),
	//GB_METHOD("Rename", NULL, MongoCollection_Rename, "(NewName)s[]")
	GB_METHOD("Find", "String[]", MongoCollection_Find, "[(Filter)Collection;]"),

	GB_METHOD("_get", "Collection", MongoCollection_get, "(Id)s"),
	GB_METHOD("_put", NULL, MongoCollection_put, "(Document)Collection;(Id)s"),
	
	GB_END_DECLARE
};
