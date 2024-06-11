/***************************************************************************

  c_mongoclient.c

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

#define __C_MONGOCLIENT_C

#include "helper.h"
#include "c_mongoclient.h"

//--------------------------------------------------------------------------

BEGIN_METHOD(MongoClient_new, GB_STRING url)

	bson_error_t error;

	THIS->uri = mongoc_uri_new_with_error(GB.ToZeroString(ARG(url)), &error);
	if (!THIS->uri)
	{
		GB.Error("Incorrect URL: &1", error.message);
		return;
	}

	THIS->client = mongoc_client_new_from_uri_with_error(THIS->uri, &error);
	if (!THIS->client)
		GB.Error("Unable to connect to MongoDB server: &1", error.message);

	mongoc_client_set_appname(THIS->client, GB.Application.Name());
	
	THIS->database = mongoc_client_get_default_database(THIS->client);
	if (!THIS->database)
		THIS->database = mongoc_client_get_database(THIS->client, "admin");

END_METHOD

BEGIN_METHOD_VOID(MongoClient_free)

	mongoc_database_destroy(THIS->database);
	mongoc_client_destroy(THIS->client);
	mongoc_uri_destroy(THIS->uri);

END_METHOD

BEGIN_PROPERTY(MongoClient_Database)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(mongoc_database_get_name(THIS->database));
	else
	{
		mongoc_database_destroy(THIS->database),
		THIS->database = mongoc_client_get_database(THIS->client, GB.ToZeroString(PROP(GB_STRING)));
	}

END_PROPERTY

BEGIN_METHOD(MongoClient_Exec, GB_OBJECT command)

	GB_COLLECTION command = VARG(command);
	bson_t *bson_command;
	bson_t reply;
	bson_error_t error;
	bool ret;
	
	bson_command = HELPER_to_bson(command, FALSE);
	if (!bson_command)
		return;
	
	ret = mongoc_client_command_simple(THIS->client, mongoc_database_get_name(THIS->database), bson_command, NULL, &reply, &error);
	bson_destroy(bson_command);
	
	if (!ret)
	{
		GB.Error("&1", error.message);
		return;
	}
	
	GB.ReturnObject(HELPER_from_bson(&reply));
	bson_destroy(&reply);

END_METHOD

BEGIN_PROPERTY(MongoClient_Databases)

	char **dbs;
	bson_error_t error;
	GB_ARRAY result;
	int i;

	dbs = mongoc_client_get_database_names_with_opts(THIS->client, NULL, &error);
	if (!dbs)
	{
		GB.Error("&1", error.message);
		return;
	}
	
	GB.Array.New(&result, GB_T_STRING, 0);
	
	for (i = 0; dbs[i]; i++)
		*(char **)GB.Array.Add(result) = GB.NewZeroString(dbs[i]);
	
	bson_strfreev(dbs);
	
	GB.ReturnObject(result);

END_PROPERTY

BEGIN_PROPERTY(MongoClient_Collections)

	char **cols;
	bson_error_t error;
	GB_ARRAY result;
	int i;

	cols = mongoc_database_get_collection_names_with_opts(THIS->database, NULL, &error);
	if (!cols)
	{
		GB.Error("&1", error.message);
		return;
	}
	
	GB.Array.New(&result, GB_T_STRING, 0);
	
	for (i = 0; cols[i]; i++)
		*(char **)GB.Array.Add(result) = GB.NewZeroString(cols[i]);
	
	bson_strfreev(cols);
	
	GB.ReturnObject(result);

END_PROPERTY

BEGIN_METHOD(MongoClient_get, GB_STRING collection)

	GB.ReturnObject(HELPER_create_collection(THIS, GB.ToZeroString(ARG(collection))));

END_METHOD

//--------------------------------------------------------------------------

GB_DESC MongoClientDesc[] = {

	GB_DECLARE("MongoClient", sizeof(CMONGOCLIENT)),

	GB_METHOD("_new", NULL, MongoClient_new, "(URL)s"),
	GB_METHOD("_free", NULL, MongoClient_free, NULL),

	GB_PROPERTY_READ("Databases", "String[]", MongoClient_Databases),
	GB_PROPERTY_READ("Collections", "String[]", MongoClient_Collections),

	GB_PROPERTY("Database", "s", MongoClient_Database),
	
	GB_METHOD("Exec", "Collection", MongoClient_Exec, "(Command)Collection;"),
	
	GB_METHOD("_get", "MongoCollection", MongoClient_get, "(Collection)s"),

	GB_END_DECLARE
};
