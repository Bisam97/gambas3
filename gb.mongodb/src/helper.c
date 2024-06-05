/***************************************************************************

  helper.c

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

#define __HELPER_C

#include "helper.h"

bson_t *HELPER_to_bson(void *col)
{
	bool is_collection;
	bson_t *bson;
	GB_COLLECTION_ITER iter;
	GB_VALUE value;
	char *key;
	int len;
	char buffer[16];
	int i, count = 0;

	is_collection = GB.Is(col, GB.FindClass("Collection"));
	if (!is_collection && !GB.Is(col, GB.FindClass("Array")))
		return NULL;
	
	bson = bson_new();

	if (is_collection)
	{
		GB.Collection.Enum((GB_COLLECTION)col, &iter, NULL, NULL, NULL);
	}
	else
	{
		i = 0;
		count = GB.Array.Count((GB_ARRAY)col);
	}

	for(;;)
	{
		if (is_collection)
		{
			if (GB.Collection.Enum(col, &iter, (GB_VARIANT *)&value, &key, &len))
				break;
		}
		else
		{
			if (i >= count)
				break;
			len = sprintf(buffer, "%d", i);
			key = buffer;
			i++;
		}
	
		if (value.type == GB_T_VARIANT)
			GB.Conv(&value, value._variant.value.type);
		
		switch(value.type)
		{
			case GB_T_BOOLEAN:
				bson_append_bool(bson, key, len, value._boolean.value);
				break;
				
			case GB_T_BYTE:
			case GB_T_SHORT:
			case GB_T_INTEGER:
				bson_append_int32(bson, key, len, value._integer.value);
				break;
				
			case GB_T_LONG:
			case GB_T_POINTER:
				bson_append_int64(bson, key, len, value._long.value);
				break;
				
			case GB_T_SINGLE:
				bson_append_double(bson, key, len, (double)value._single.value);
				break;
				
			case GB_T_FLOAT:
				bson_append_double(bson, key, len, value._float.value);
				break;
				
			case GB_T_CSTRING:
			case GB_T_STRING:
				bson_append_utf8(bson, key, len, value._string.value.addr + value._string.value.start, value._string.value.len);
				break;
			
			default:
				
				if (value.type >= GB_T_OBJECT)
				{
					void *ob = value._object.value;
					if (GB.Is(ob, GB.FindClass("Collection")))
					{
						bson_t *doc = HELPER_to_bson(ob);
						if (!doc)
							return NULL;
						
						bson_append_document(bson, key, len, doc);
						break;
					}
					else if (GB.Is(ob, GB.FindClass("Array")))
					{
						bson_t *doc = HELPER_to_bson(ob);
						if (!doc)
							return NULL;
						
						bson_append_array(bson, key, len, doc);
						break;
					}
				}
				
				GB.Error("Unsupported datatype");
				return NULL;
		}
	}

  return bson;
}

/*
typedef enum {
   BSON_TYPE_EOD = 0x00,
   BSON_TYPE_BINARY = 0x05,
   BSON_TYPE_UNDEFINED = 0x06,
   BSON_TYPE_OID = 0x07,
   BSON_TYPE_DATE_TIME = 0x09,
   BSON_TYPE_REGEX = 0x0B,
   BSON_TYPE_DBPOINTER = 0x0C,
   BSON_TYPE_CODE = 0x0D,
   BSON_TYPE_SYMBOL = 0x0E,
   BSON_TYPE_CODEWSCOPE = 0x0F,
   BSON_TYPE_TIMESTAMP = 0x11,
   BSON_TYPE_MAXKEY = 0x7F,
   BSON_TYPE_MINKEY = 0xFF,
} bson_type_t;*/

static GB_COLLECTION from_bson(bson_iter_t *iter, bool collection)
{
	void *result;
	GB_VALUE val;
	bson_type_t type;

	if (collection)
		GB.Collection.New(&result, FALSE);
	else
		GB.Array.New(&result, GB_T_VARIANT, 0);
	
	while (bson_iter_next(iter))
	{
		type = bson_iter_type(iter);
		switch (type)
		{
			case BSON_TYPE_DOUBLE:
				val.type = GB_T_FLOAT;
				val._float.value = bson_iter_double(iter);
				break;
				
			case BSON_TYPE_UTF8:
			{
				uint len;
				
				val.type = GB_T_CSTRING;
				val._string.value.addr = (char *)bson_iter_utf8 (iter, &len);
				val._string.value.start = 0;
				val._string.value.len = len;
				break;
			}
			
			case BSON_TYPE_BOOL:
				val.type = GB_T_BOOLEAN;
				val._boolean.value = bson_iter_bool(iter);
				break;
				
			case BSON_TYPE_NULL:
				continue;
				
			case BSON_TYPE_INT32:
				val.type = GB_T_INTEGER;
				val._integer.value = bson_iter_int32(iter);
				break;
				
			case BSON_TYPE_INT64:
				val.type = GB_T_LONG;
				val._integer.value = bson_iter_int64(iter);
				break;
				
			case BSON_TYPE_DOCUMENT:
			{
				bson_iter_t child;
				if (!bson_iter_recurse(iter, &child))
					continue;
				
				val.type = GB.FindClass("Collection");
				val._object.value = from_bson(&child, TRUE);
				break;
			}
				
			case BSON_TYPE_ARRAY:
			{
				bson_iter_t child;
				if (!bson_iter_recurse(iter, &child))
					continue;
				
				val.type = GB.FindClass("Variant[]");
				val._object.value = from_bson(&child, FALSE);
				break;
			}
				
			default:
				fprintf(stderr, "gb.mongodb: warning: unsupported datatype ignored: 0x%02X\n", type);
				continue;
		}

		GB.BorrowValue(&val);
		GB.Conv(&val, GB_T_VARIANT);

		if (collection)
			GB.Collection.Set(result, bson_iter_key(iter), bson_iter_key_len(iter), (GB_VARIANT *)&val);
		else
			GB.Store(GB.Array.Type(result), &val, GB.Array.Add(result));
		GB.ReleaseValue(&val);
	}
	
	return result;
}


GB_COLLECTION HELPER_from_bson(bson_t *bson)
{
	bson_iter_t iter;
	
	if (!bson_iter_init(&iter, bson))
		return NULL;
	
	return from_bson(&iter, TRUE);
}


CMONGOCOLLECTION *HELPER_create_collection(CMONGOCLIENT *client, const char *name)
{
	CMONGOCOLLECTION *ob = GB.New(GB.FindClass("MongoCollection"), NULL, NULL);
	
	ob->client = client;
	GB.Ref(client);
	
	ob->collection = mongoc_database_get_collection(client->database, name);
	
	return ob;
}
