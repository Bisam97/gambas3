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

#if MONGOC_CHECK_VERSION(1,24,0)
#else

static bool is_key(const char *key, int len, const char *check)
{
	int len_check = strlen(check);
	if (len_check != len)
		return FALSE;
	return strncmp(key, check, len) == 0;
}

static bool fill_index_opts_from_collection(mongoc_index_opt_t *opts, GB_COLLECTION col)
{
	GB_COLLECTION_ITER iter;
	GB_VALUE val;
	char *key;
	int len;
	
	mongoc_index_opt_init(opts);
	if (!col)
		return FALSE;
	
	GB.Collection.Enum(col, &iter, NULL, NULL, NULL);

	for(;;)
	{
		if (GB.Collection.Enum(col, &iter, (GB_VARIANT *)&val, &key, &len))
			break;
		
		if (is_key(key, len, "unique"))
		{
			GB.Conv(&val, GB_T_BOOLEAN);
			opts->unique = val._boolean.value;
			fprintf(stderr, "unique = %d\n", opts->unique);
		}
		else if (is_key(key, len, "name"))
		{
			GB.Conv(&val, GB_T_STRING);
			opts->name = GB.TempString(val._string.value.addr + val._string.value.start, val._string.value.len);
		}
		else if (is_key(key, len, "default_language"))
		{
			GB.Conv(&val, GB_T_STRING);
			opts->default_language = GB.TempString(val._string.value.addr + val._string.value.start, val._string.value.len);
		}
		else if (is_key(key, len, "language_override"))
		{
			GB.Conv(&val, GB_T_STRING);
			opts->language_override = GB.TempString(val._string.value.addr + val._string.value.start, val._string.value.len);
		}
		else
		{
			GB.Error("Unsupported index option: &1", GB.TempString(key, len));
			return TRUE;
		}
	}
	
	return FALSE;
}

#endif

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

BEGIN_METHOD(MongoCollection_Delete, GB_OBJECT filter; GB_OBJECT options)

	bson_error_t error;
	
	if (MISSING(filter))
	{
		if (!mongoc_collection_drop(THIS->collection, &error))
			GB.Error("&1", error.message);
	}
	else
	{
		bson_t *filter = HELPER_to_bson(VARGOPT(filter, NULL), TRUE);
		bson_t *options = HELPER_to_bson(VARGOPT(options, NULL), TRUE);

		if (!mongoc_collection_delete_many(THIS->collection, filter, options, NULL, &error))
			GB.Error("&1", error.message);
		
		bson_destroy(filter);
		bson_destroy(options);
	}

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
	else
		GB.ReturnNull();
	
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
	
	filter = bson_new();
	bson_append_utf8(filter, "_id", 3, STRING(id), LENGTH(id));
	
	if (!VARG(doc))
	{
		ok = mongoc_collection_delete_one(THIS->collection, filter, NULL, NULL, &error);
	}
	else
	{
		options = BCON_NEW("upsert", BCON_BOOL(TRUE));
		
		doc = HELPER_to_bson_except(VARG(doc), "_id");
		if (!HELPER_bson_add_string(doc, "_id", STRING(id), LENGTH(id)))
			ok = mongoc_collection_replace_one(THIS->collection, filter, doc, options, NULL, &error);
		else
			ok = TRUE;
		
		bson_destroy(options);
		bson_destroy(doc);
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

BEGIN_METHOD(MongoCollection_Remove, GB_STRING id)

	bool ok;
	bson_t *filter;
	bson_error_t error;
	
	filter = bson_new();
	bson_append_utf8(filter, "_id", 3, STRING(id), LENGTH(id));
	
	ok = mongoc_collection_delete_one(THIS->collection, filter, NULL, NULL, &error);
	
	if (!ok)
		GB.Error("&1", error.message);

	bson_destroy(filter);

END_METHOD

//--------------------------------------------------------------------------

BEGIN_METHOD(MongoCollection_Indexes_Add, GB_OBJECT keys; GB_OBJECT options)

	bson_t *keys;
	bson_error_t error;
	bool err = FALSE;

	keys = HELPER_to_bson(VARG(keys), FALSE);
	if (!keys)
		return;

#if MONGOC_CHECK_VERSION(1,24,0)
	
	bson_t *options = HELPER_to_bson(VARGOPT(options, NULL), TRUE);
	mongoc_index_model_t *model = mongoc_index_model_new(keys, options);
	
	if (!mongoc_collection_create_indexes_with_opts(THIS->collection, &model, 1, NULL, NULL, &error))
	{
		err = TRUE;
		GB.Error("&1", error.message);
	}
	
	mongoc_index_model_destroy(model);
	bson_destroy(options);

#else
	
	mongoc_index_opt_t index_opts;
	GB_COLLECTION options = VARGOPT(options, NULL);
	
	if (fill_index_opts_from_collection(&index_opts, options))
		return;
	
	if (!mongoc_collection_create_index_with_opts(THIS->collection, keys, &index_opts, NULL, NULL, &error))
	{
		err = TRUE;
		GB.Error("&1", error.message);
	}
	
#endif
	
	if (!err)
		GB.ReturnNewZeroString(mongoc_collection_keys_to_index_string(keys));
	
	bson_destroy(keys);
	
END_METHOD

BEGIN_METHOD(MongoCollection_Indexes_Remove, GB_STRING name)

	bson_error_t error;
	
	if (!mongoc_collection_drop_index_with_opts(THIS->collection, GB.ToZeroString(ARG(name)), NULL, &error)) 
		GB.Error("&1", error.message);

END_METHOD

BEGIN_METHOD_VOID(MongoCollection_Indexes_Query)

	mongoc_cursor_t *cursor;

	cursor = mongoc_collection_find_indexes_with_opts(THIS->collection, NULL);
	
	GB.ReturnObject(HELPER_create_result(THIS->client, cursor));

END_METHOD

//--------------------------------------------------------------------------

GB_DESC MongoCollectionIndexesDesc[] = {
	
	GB_DECLARE_VIRTUAL(".MongoCollection.Indexes"),
	
	GB_METHOD("Add", "s", MongoCollection_Indexes_Add, "(Keys)Collection;[(Options)Collection;]"),
	GB_METHOD("Remove", NULL, MongoCollection_Indexes_Remove, "(Name)s"),
	GB_METHOD("Query", "MongoResult", MongoCollection_Indexes_Query, NULL),
	
	GB_END_DECLARE
};

GB_DESC MongoCollectionDesc[] = {

	GB_DECLARE("MongoCollection", sizeof(CMONGOCOLLECTION)),
	GB_NOT_CREATABLE(),

	GB_METHOD("_free", NULL, MongoCollection_free, NULL),
	
	GB_PROPERTY_READ("Name", "s", MongoCollection_Name),
	GB_PROPERTY_READ("Count", "l", MongoCollection_Count),
	
	GB_METHOD("Query", "MongoResult", MongoCollection_Query, "[(Filter)Collection;(Options)Collection;]"),
	GB_METHOD("Delete", NULL, MongoCollection_Delete, "[(Filter)Collection;(Options)Collection;]"),
	GB_METHOD("Add", NULL, MongoCollection_Add, "(Document)Collection;[(Options)Collection;]"),
	GB_METHOD("Remove", NULL, MongoCollection_Remove, "(Id)s"),
	GB_METHOD("Replace", NULL, MongoCollection_Replace, "(Filter)Collection;(Document)Collection;[(Options)Collection;]"),
	//GB_METHOD("Rename", NULL, MongoCollection_Rename, "(NewName)s[]")
	GB_METHOD("Find", "String[]", MongoCollection_Find, "[(Filter)Collection;]"),

	GB_METHOD("_get", "Collection", MongoCollection_get, "(Id)s"),
	GB_METHOD("_put", NULL, MongoCollection_put, "(Document)Collection;(Id)s"),
	
	GB_PROPERTY_SELF("Indexes", ".MongoCollection.Indexes"),
	
	GB_END_DECLARE
};
