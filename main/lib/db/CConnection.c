/***************************************************************************

	CConnection.c

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

#define __CCONNECTION_C

#include "main.h"

#include "CTable.h"
//#include "CView.h"
#include "CDatabase.h"
#include "CUser.h"
#include "CConnection.h"


/***************************************************************************

	Connection

***************************************************************************/

static CCONNECTION *_current = NULL;

static SUBCOLLECTION_DESC _databases_desc =
{
	".Connection.Databases",
	(void *)CDATABASE_get,
	(void *)CDATABASE_exist,
	(void *)CDATABASE_list,
	(void *)CDATABASE_release
};

static SUBCOLLECTION_DESC _users_desc =
{
	".Connection.Users",
	(void *)CUSER_get,
	(void *)CUSER_exist,
	(void *)CUSER_list,
	(void *)CUSER_release
};

static SUBCOLLECTION_DESC _tables_desc =
{
	".Connection.Tables",
	(void *)CTABLE_get,
	(void *)CTABLE_exist,
	(void *)CTABLE_list,
	(void *)CTABLE_release
};

/*static GB_SUBCOLLECTION_DESC _views_desc =
{
	".ConnectionViews",
	(void *)CVIEW_get,
	(void *)CVIEW_exist,
	(void *)CVIEW_list
};*/



static void open_connection(CCONNECTION *_object)
{
	GB_COLLECTION options;
	
	options = GB.GetProperty(THIS, "Options")->_object.value;
	
	if (DB_Open(&THIS->desc, &THIS->driver, &THIS->db, options))
		return;

	THIS->limit = 0;
	THIS->trans = 0;

	THIS->db.flags.system = !THIS->desc.name || THIS->driver->Database.IsSystem(&THIS->db, THIS->desc.name);
}

static bool check_opened(CCONNECTION *_object)
{
	DB_CurrentDatabase = &THIS->db;

	/*if (!THIS->db.handle)
		open_connection(THIS);*/

	if (!THIS->db.handle)
	{
		GB.Error("Connection is not opened");
		return TRUE;
	}
	else
		return FALSE;
}

#define CHECK_OPEN() \
	if (check_opened(THIS)) \
		return;

static int get_current(CCONNECTION **current)
{
	if (*current == NULL)
	{
		if (_current == NULL)
		{
			GB.Error("No current connection");
			return TRUE;
		}
		*current = _current;
	}

	return FALSE;
}

#define CHECK_DB() \
	if (get_current((CCONNECTION **)(void *)&_object)) \
		return;

static void close_connection(CCONNECTION *_object)
{
	if (!THIS->db.handle)
		return;

	GB.Unref(POINTER(&THIS->databases));
	THIS->databases = NULL;
	GB.Unref(POINTER(&THIS->users));
	THIS->users = NULL;
	GB.Unref(POINTER(&THIS->tables));
	THIS->tables = NULL;

	THIS->driver->Close(&THIS->db);
	GB.FreeString(&THIS->db.charset);
	GB.FreeString(&THIS->db.full_version);

	THIS->db.handle = NULL;
	THIS->driver = NULL;
}


BEGIN_METHOD_VOID(Connection_new)

	THIS->db.handle = NULL;
	THIS->db.ignore_case = FALSE; // Now case is sensitive by default!
	THIS->db.timeout = 20; // Connection timeout is 20 seconds by default
	THIS->db.timezone = GB.System.TimeZone();

	if (_current == NULL)
		_current = THIS;

END_METHOD


BEGIN_METHOD_VOID(Connection_free)

	close_connection(THIS);

	if (_current == THIS)
		_current = NULL;

	GB.StoreString(NULL, &THIS->desc.type);
	GB.StoreString(NULL, &THIS->desc.host);
	GB.StoreString(NULL, &THIS->desc.user);
	GB.StoreString(NULL, &THIS->desc.password);
	GB.StoreString(NULL, &THIS->desc.name);
	GB.StoreString(NULL, &THIS->desc.port);
	GB.StoreString(NULL, &THIS->db.charset);

END_METHOD


#define IMPLEMENT(_name, _prop) \
BEGIN_PROPERTY(Connection_##_name) \
\
	if (READ_PROPERTY) \
		GB.ReturnString(THIS->desc._prop); \
	else \
		GB.StoreString(PROP(GB_STRING), &THIS->desc._prop); \
\
END_PROPERTY

IMPLEMENT(Type, type)
IMPLEMENT(Host, host)
IMPLEMENT(User, user)
IMPLEMENT(Password, password)
IMPLEMENT(Name, name)
IMPLEMENT(Port, port)


BEGIN_PROPERTY(Connection_Version)

	CHECK_DB();
	CHECK_OPEN();

	GB.ReturnInteger(THIS->db.version);

END_PROPERTY


BEGIN_PROPERTY(Connection_FullVersion)

	CHECK_DB();
	CHECK_OPEN();

	GB.ReturnString(THIS->db.full_version);

END_PROPERTY


BEGIN_PROPERTY(Connection_Timeout)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->db.timeout);
	else
		THIS->db.timeout = VPROP(GB_INTEGER);

END_PROPERTY


#if 0
BEGIN_PROPERTY(Connection_Timezone)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->db.timezone);
	else
		THIS->db.timezone = VPROP(GB_INTEGER);

END_PROPERTY
#endif


BEGIN_PROPERTY(Connection_Opened)

	CHECK_DB();

	GB.ReturnBoolean(THIS->db.handle != NULL);

END_PROPERTY


BEGIN_PROPERTY(Connection_Error)

	CHECK_DB();

	GB.ReturnInteger(THIS->db.error);

END_PROPERTY


/*BEGIN_PROPERTY(Connection_Transaction)

	CHECK_DB();

	GB.ReturnInteger(THIS->trans);

END_PROPERTY*/


BEGIN_METHOD_VOID(Connection_Open)

	CHECK_DB();

	if (THIS->db.handle)
	{
		GB.Error("Connection already opened");
		return;
	}

	open_connection(THIS);

END_METHOD


BEGIN_METHOD_VOID(Connection_Close)

	CHECK_DB();

	close_connection(THIS);

END_METHOD

#if 0
BEGIN_PROPERTY(CCONNECTION_ignore_case)

	CHECK_DB();
	CHECK_OPEN();

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->db.ignore_case);
	else
	{
		if (THIS->db.flags.no_case)
		{
			if (THIS->db.ignore_case)
				GB.Error("This database driver cannot be case sensitive");
			else
				GB.Error("This database driver is always case sensitive");
			return;
		}
		THIS->db.ignore_case = VPROP(GB_BOOLEAN);
	}

END_PROPERTY
#endif

BEGIN_PROPERTY(Connection_IgnoreCharset)

	CHECK_DB();

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->ignore_charset);
	else
		THIS->ignore_charset = VPROP(GB_BOOLEAN);

END_PROPERTY

BEGIN_PROPERTY(Connection_Collations)

	GB_ARRAY array;

	CHECK_DB();
	CHECK_OPEN();

	if (!THIS->db.flags.no_collation)
	{
		array = THIS->driver->GetCollations(&THIS->db);
		if (array)
		{
			GB.ReturnObject(array);
			return;
		}
	}
	
	GB.Error("Collations are not supported");

END_PROPERTY

BEGIN_METHOD_VOID(Connection_Begin)

	CHECK_DB();
	CHECK_OPEN();

	if (!THIS->db.flags.no_nest || THIS->trans == 0)
		THIS->driver->Begin(&THIS->db);
	THIS->trans++;

END_METHOD


BEGIN_METHOD_VOID(Connection_Commit)

	CHECK_DB();
	CHECK_OPEN();

	if (THIS->trans == 0)
	{
		//GB.Error("Not in a transaction");
		return;
	}

	THIS->trans--;
	if (!THIS->db.flags.no_nest || THIS->trans == 0)
		THIS->driver->Commit(&THIS->db);

END_METHOD


BEGIN_METHOD_VOID(Connection_Rollback)

	CHECK_DB();
	CHECK_OPEN();

	if (THIS->trans == 0)
	{
		//GB.Error("Not in a transaction");
		return;
	}

	THIS->trans--;
	if (!THIS->db.flags.no_nest || THIS->trans == 0)
		THIS->driver->Rollback(&THIS->db);

END_METHOD


BEGIN_METHOD(Connection_Limit, GB_INTEGER limit)

	CHECK_DB();
	CHECK_OPEN();

	THIS->limit = VARG(limit);
	GB.ReturnObject(THIS);

END_PROPERTY

static char *_make_query_buffer;
static char *_make_query_original;

static void make_query_get_param(int index, char **str, int *len)
{
	if (index == 1)
		*str = _make_query_buffer;
	else if (index == 2)
		*str = _make_query_original;

	*len = -1;
}

static char *make_query(CCONNECTION *_object, char *pattern, int len, int narg, GB_VALUE *arg)
{
	char *query;
	const char *keyword;
	char buffer[32];

	query = DB_MakeQuery(THIS->driver, &THIS->db, pattern, len, narg, arg);

	if (query && THIS->limit > 0 && strncasecmp(query, "SELECT ", 7) == 0)
	{
		keyword = THIS->db.limit.keyword;
		if (!keyword)
			keyword = "LIMIT";

		snprintf(buffer, sizeof(buffer), "%s %d", keyword, THIS->limit);

		_make_query_buffer = buffer;
		_make_query_original = &query[7];

		switch (THIS->db.limit.pos)
		{
			case DB_LIMIT_AT_BEGIN:
				query = GB.SubstString("SELECT &1 &2", 0, make_query_get_param);
				break;

			case DB_LIMIT_AT_END:
			default:
				query = GB.SubstString("SELECT &2 &1", 0, make_query_get_param);
				break;
		}

		THIS->limit = 0;
	}

	return query;
}

BEGIN_METHOD(Connection_Exec, GB_STRING query; GB_VALUE param[0])

	char *query;
	CRESULT *result;

	CHECK_DB();
	CHECK_OPEN();

	query = make_query(THIS, STRING(query), LENGTH(query), GB.NParam(), ARG(param[0]));
	if (!query)
		return;

	result = DB_MakeResult(THIS, RESULT_FIND, NULL, query);

	if (result)
		GB.ReturnObject(result);

END_METHOD


BEGIN_METHOD(Connection_Create, GB_STRING table; GB_BOOLEAN ret)

	CRESULT *result;
	char *table = GB.ToZeroString(ARG(table));

	CHECK_DB();
	CHECK_OPEN();

	if (!table || !*table)
	{
		GB.Error("Void table name");
		return;
	}

	result = DB_MakeResult(THIS, RESULT_CREATE, table, NULL);

	if (result)
	{
		if (THIS->db.flags.no_returning)
			DB_Debug("gb.db", "'RETURNING' keyword is not supported by this '%s' connection", THIS->driver->name);
		else
			result->returning = VARGOPT(ret, FALSE);

		GB.ReturnObject(result);
	}
	else
		GB.ReturnNull();

END_METHOD


static char *get_query(char *prefix, CCONNECTION *_object, char *table, int len_table, char *query, int len_query, GB_VALUE *arg)
{
	if (!len_table)
	{
		GB.Error("Void table name");
		return NULL;
	}

	q_init();

	q_add(prefix);
	q_add(" ");
	q_add(DB_GetQuotedTable(THIS->driver, &THIS->db, table, len_table));

	if (query && len_query > 0)
	{
		q_add(" ");
		if (strncasecmp(query, "WHERE ", 6) && strncasecmp(query, "ORDER BY ", 9))
			q_add("WHERE ");
		q_add_length(query, len_query);
	}

	query = make_query(THIS, q_get(), q_length(), GB.NParam(), arg);

	return query;
}


BEGIN_METHOD(Connection_Find, GB_STRING table; GB_STRING query; GB_VALUE param[0])

	char *query;
	CRESULT *result;

	CHECK_DB();
	CHECK_OPEN();

	query = get_query("SELECT * FROM", THIS, STRING(table), LENGTH(table),
		MISSING(query) ? NULL : STRING(query),
		MISSING(query) ? 0 : LENGTH(query),
		ARG(param[0]));

	if (!query)
		return;

	result = DB_MakeResult(THIS, RESULT_FIND, NULL, query);

	if (result)
		GB.ReturnObject(result);

END_METHOD


BEGIN_METHOD(Connection_Delete, GB_STRING table; GB_STRING query; GB_VALUE param[0])

	char *query;

	CHECK_DB();
	CHECK_OPEN();

	query = get_query("DELETE FROM", THIS, STRING(table), LENGTH(table),
		MISSING(query) ? NULL : STRING(query),
		MISSING(query) ? 0 : LENGTH(query),
		ARG(param[0]));

	if (!query)
		return;

	DB_MakeResult(THIS, RESULT_DELETE, NULL, query);

END_METHOD


BEGIN_METHOD(Connection_Edit, GB_STRING table; GB_STRING query; GB_VALUE param[0])

	char *query;
	CRESULT *result;
	/*char *table = GB.ToZeroString(ARG(table));*/

	CHECK_DB();
	CHECK_OPEN();

	/*if (check_table(THIS, table, TRUE))
		return;*/

	query = get_query("SELECT * FROM", THIS, STRING(table), LENGTH(table),
		MISSING(query) ? NULL : STRING(query),
		MISSING(query) ? 0 : LENGTH(query),
		ARG(param[0]));

	if (!query)
		return;

	result = DB_MakeResult(THIS, RESULT_EDIT, GB.ToZeroString(ARG(table)), query);

	if (result)
		GB.ReturnObject(result);

END_METHOD


BEGIN_METHOD(Connection_Quote, GB_STRING name; GB_BOOLEAN is_table)

	char *name = STRING(name);
	int len = LENGTH(name);

	CHECK_DB();
	CHECK_OPEN();

	if (VARGOPT(is_table, FALSE)) // && THIS->db.flags.schema)
		GB.ReturnNewZeroString(DB_GetQuotedTable(THIS->driver, &THIS->db, STRING(name), LENGTH(name)));
	else
	{
		q_init();
		q_add(THIS->driver->GetQuote());
		q_add_length(name, len);
		q_add(THIS->driver->GetQuote());
		GB.ReturnString(q_get());
	}

END_METHOD


BEGIN_METHOD(Connection_FormatBlob, GB_STRING data)

	DB_BLOB blob;

	CHECK_DB();
	CHECK_OPEN();

	blob.data = STRING(data);
	blob.length = LENGTH(data);

	q_init();
	DB_CurrentDatabase = &THIS->db;
	(*THIS->driver->FormatBlob)(&blob, q_add_length);
	GB.ReturnString(q_get());

END_METHOD


BEGIN_METHOD(Connection_Subst, GB_STRING query; GB_VALUE param[0])

	char *query;

	CHECK_DB();
	CHECK_OPEN();

	query = make_query(THIS, STRING(query), LENGTH(query), GB.NParam(), ARG(param[0]));

	if (!query)
		return;

	GB.ReturnString(query);

END_METHOD


BEGIN_PROPERTY(Connection_Current)

	if (READ_PROPERTY)
		GB.ReturnObject(_current);
	else
		_current = (CCONNECTION *)VPROP(GB_OBJECT);

END_PROPERTY


BEGIN_PROPERTY(Connection_Charset)

	CHECK_DB();
	CHECK_OPEN();

	if (THIS->db.charset)
		GB.ReturnString(THIS->db.charset);
	else
		GB.ReturnConstZeroString("ASCII");

END_PROPERTY


BEGIN_PROPERTY(Connection_Databases)

	CHECK_DB();
	CHECK_OPEN();

	GB_SubCollectionNew(&THIS->databases, &_databases_desc, THIS);
	GB.ReturnObject(THIS->databases);

END_PROPERTY


BEGIN_PROPERTY(Connection_Users)

	CHECK_DB();
	CHECK_OPEN();

	GB_SubCollectionNew(&THIS->users, &_users_desc, THIS);
	GB.ReturnObject(THIS->users);

END_PROPERTY


BEGIN_PROPERTY(Connection_Tables)

	CHECK_DB();
	CHECK_OPEN();

	GB_SubCollectionNew(&THIS->tables, &_tables_desc, THIS);
	GB.ReturnObject(THIS->tables);

END_PROPERTY


/*BEGIN_PROPERTY(CCONNECTION_views)

	CHECK_DB();
	CHECK_OPEN();

	GB.SubCollection.New(&THIS->views, &_views_desc, THIS);
	GB.ReturnObject(THIS->views);

END_PROPERTY*/


BEGIN_PROPERTY(Connection_Debug)

	if (READ_PROPERTY)
		GB.ReturnBoolean(DB_IsDebug());
	else
		DB_SetDebug(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Connection_Handle)

	CHECK_DB();
	GB.ReturnPointer(THIS->db.handle);

END_PROPERTY


BEGIN_PROPERTY(Connection_LastInsertId)

	CHECK_DB();
	CHECK_OPEN();
	
	GB.ReturnLong((*THIS->driver->GetLastInsertId)(&THIS->db));

END_PROPERTY

GB_DESC CConnectionDesc[] =
{
	GB_DECLARE("_Connection", sizeof(CCONNECTION)),

	GB_METHOD("_new", NULL, Connection_new, NULL),
	GB_METHOD("_free", NULL, Connection_free, NULL),

	GB_PROPERTY("Type", "s", Connection_Type),
	GB_PROPERTY("Host", "s", Connection_Host),
	GB_PROPERTY("Login", "s", Connection_User),
	GB_PROPERTY("User", "s", Connection_User),
	GB_PROPERTY("Password", "s", Connection_Password),
	GB_PROPERTY("Name", "s", Connection_Name),
	GB_PROPERTY("Port", "s", Connection_Port),
	GB_PROPERTY("Timeout", "i", Connection_Timeout),
	//GB_PROPERTY("Timezone", "i", Connection_Timezone),
	GB_PROPERTY_READ("Charset", "s", Connection_Charset),
	GB_PROPERTY_READ("Version", "i", Connection_Version),
	GB_PROPERTY_READ("FullVersion", "s", Connection_FullVersion),
	GB_PROPERTY_READ("Opened", "b", Connection_Opened),
	GB_PROPERTY_READ("Error", "i", Connection_Error),
	//GB_PROPERTY_READ("Transaction", "i", Connection_Transaction),
	GB_PROPERTY("IgnoreCharset", "b", Connection_IgnoreCharset),
	GB_PROPERTY_READ("Collations", "String[]", Connection_Collations),
	GB_PROPERTY_READ("Handle", "p", Connection_Handle),
	
	GB_PROPERTY_READ("LastInsertId", "l", Connection_LastInsertId),

	GB_METHOD("Open", NULL, Connection_Open, NULL),
	GB_METHOD("Close", NULL, Connection_Close, NULL),

	GB_METHOD("Limit", "Connection", Connection_Limit, "(Limit)i"),
	GB_METHOD("Exec", "Result", Connection_Exec, "(Request)s(Arguments)."),
	GB_METHOD("Create", "Result", Connection_Create, "(Table)s[(Return)b]"),
	GB_METHOD("Find", "Result", Connection_Find, "(Table)s[(Request)s(Arguments).]"),
	GB_METHOD("Edit", "Result", Connection_Edit, "(Table)s[(Request)s(Arguments).]"),
	GB_METHOD("Delete", NULL, Connection_Delete, "(Table)s[(Request)s(Arguments).]"),
	GB_METHOD("Subst", "s", Connection_Subst, "(Format)s(Arguments)."),

	GB_METHOD("Begin", NULL, Connection_Begin, NULL),
	GB_METHOD("Commit", NULL, Connection_Commit, NULL),
	GB_METHOD("Rollback", NULL, Connection_Rollback, NULL),

	GB_METHOD("Quote", "s", Connection_Quote, "(Name)s[(Table)b]"),
	GB_METHOD("FormatBlob", "s", Connection_FormatBlob, "(Data)s"),

	GB_PROPERTY("Tables", ".Connection.Tables", Connection_Tables),
	GB_PROPERTY("Databases", ".Connection.Databases", Connection_Databases),
	GB_PROPERTY("Users", ".Connection.Users", Connection_Users),
	//GB_PROPERTY("Views", ".ConnectionViews", CCONNECTION_views),

	GB_CONSTANT("_Properties", "s", "Type,Host,Login,Password,Name,Port"),

	GB_END_DECLARE
};


GB_DESC CDBDesc[] =
{
	GB_DECLARE("DB", 0), GB_VIRTUAL_CLASS(),

	GB_CONSTANT("Boolean", "i", GB_T_BOOLEAN),
	GB_CONSTANT("Integer", "i", GB_T_INTEGER),
	GB_CONSTANT("Long", "i", GB_T_LONG),
	GB_CONSTANT("Float", "i", GB_T_FLOAT),
	GB_CONSTANT("Date", "i", GB_T_DATE),
	GB_CONSTANT("String", "i", GB_T_STRING),
	GB_CONSTANT("Serial", "i", DB_T_SERIAL),
	GB_CONSTANT("Blob", "i", DB_T_BLOB),

	GB_STATIC_PROPERTY("Current", "Connection", Connection_Current),

	GB_STATIC_METHOD("Open", NULL, Connection_Open, NULL),
	GB_STATIC_METHOD("Close", NULL, Connection_Close, NULL),

	GB_STATIC_PROPERTY_READ("Charset", "s", Connection_Charset),
	GB_STATIC_PROPERTY_READ("Version", "i", Connection_Version),
	GB_STATIC_PROPERTY_READ("FullVersion", "s", Connection_FullVersion),
	GB_STATIC_PROPERTY_READ("Opened", "b", Connection_Opened),
	GB_STATIC_PROPERTY_READ("Error", "i", Connection_Error),
	//GB_STATIC_PROPERTY_READ("Transaction", "i", Connection_Transaction),
	GB_STATIC_PROPERTY("IgnoreCharset", "b", Connection_IgnoreCharset),
	GB_STATIC_PROPERTY_READ("Collations", "String[]", Connection_Collations),
	GB_STATIC_PROPERTY_READ("Handle", "p", Connection_Handle),

	GB_STATIC_PROPERTY_READ("LastInsertId", "l", Connection_LastInsertId),

	GB_STATIC_PROPERTY("Debug", "b", Connection_Debug),

	GB_STATIC_METHOD("Limit", "Connection", Connection_Limit, "(Limit)i"),
	GB_STATIC_METHOD("Exec", "Result", Connection_Exec, "(Request)s(Arguments)."),
	GB_STATIC_METHOD("Create", "Result", Connection_Create, "(Table)s[(Return)b]"),
	GB_STATIC_METHOD("Find", "Result", Connection_Find, "(Table)s[(Request)s(Arguments).]"),
	GB_STATIC_METHOD("Edit", "Result", Connection_Edit, "(Table)s[(Request)s(Arguments).]"),
	GB_STATIC_METHOD("Delete", NULL, Connection_Delete, "(Table)s[(Request)s(Arguments).]"),
	GB_STATIC_METHOD("Subst", "s", Connection_Subst, "(Format)s(Arguments)."),

	GB_STATIC_METHOD("Begin", NULL, Connection_Begin, NULL),
	GB_STATIC_METHOD("Commit", NULL, Connection_Commit, NULL),
	GB_STATIC_METHOD("Rollback", NULL, Connection_Rollback, NULL),

	GB_STATIC_METHOD("Quote", "s", Connection_Quote, "(Name)s[(Table)b]"),
	GB_STATIC_METHOD("FormatBlob", "s", Connection_FormatBlob, "(Data)s"),

	GB_STATIC_PROPERTY("Tables", ".Connection.Tables", Connection_Tables),
	//GB_STATIC_PROPERTY("Views", ".ConnectionViews", CCONNECTION_views),
	GB_STATIC_PROPERTY("Databases", ".Connection.Databases", Connection_Databases),
	GB_STATIC_PROPERTY("Users", ".Connection.Users", Connection_Users),
	
	GB_END_DECLARE
};


