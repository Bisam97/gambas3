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

#include <libpq-fe.h>

#ifdef fprintf
	#undef fprintf
	#undef snprintf
	#undef sprintf
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef Max
	#undef Max
#endif

#ifdef Min
	#undef Min
#endif

#ifdef snprintf
	#undef snprintf
#endif

#ifdef pg_snprintf
	#undef pg_snprintf
#endif

#include "main.h"

//-------------------------------------------------------------------------

GB_INTERFACE GB EXPORT;

static int _last_error;
static const char **_options_keys;
static const char **_options_values;

// PostgreSQL datatypes

enum { OID_BOOL, OID_INT2, OID_INT4, OID_INT8, OID_NUMERIC, OID_FLOAT4, OID_FLOAT8, OID_ABSTIME,
	OID_RELTIME, OID_DATE, OID_TIME, OID_TIMESTAMP, OID_DATETIME, OID_TIMESTAMPTZ, OID_BYTEA, OID_CHAR,
	OID_BPCHAR, OID_VARCHAR, OID_TEXT, OID_NAME, OID_CASH,
	OID_COUNT };

static const char *_oid_names[] = {
	"bool", "int2", "int4", "int8", "numeric", "float4", "float8", "abstime", 
	"reltime", "date", "time", "timestamp", "datetime", "timestamptz", "bytea", "char",
	"bpchar", "varchar", "text", "name", "cash", NULL
};

static int _oid[OID_COUNT] = { 0 };

static int _type;
static int _length;

//-------------------------------------------------------------------------

static void add_option(const char *key, const char *value)
{
	*(const char **)GB.Add(&_options_keys) = key;
	*(const char **)GB.Add(&_options_values) = value;
}

static void add_option_value(const char *key, int len, GB_VALUE *value)
{
	if (GB.Conv(value, GB_T_STRING))
		return;
	
	add_option(GB.TempString(key, len), value->_string.value.addr);
}

static bool check_result(PGresult *res)
{
	_last_error = 0;

	if (!res)
	{
		GB.Error("Out of memory");
		return TRUE;
	}

	_last_error = PQresultStatus(res);
	switch (_last_error)
	{
		case PGRES_COMMAND_OK:
		case PGRES_TUPLES_OK:
			return FALSE;

		default:
			GB.Error("&1", PQresultErrorMessage(res));
			PQclear(res);
			return TRUE;
	}
}

static bool do_query(PGconn *conn, const char *query, PGresult **pres)
{
	PGresult *res;

	res = PQexec(conn, query);
	
	if (check_result(res))
		return TRUE;
	
	if (pres)
		*pres = res;
	else
		PQclear(res);
	
	return FALSE;
}

static bool init_datatypes(PGconn *conn)
{
	char query[128];
	const char *oid;
	PGresult *res;
	int i;
	
	for(i = 0;; i++)
	{
		oid = _oid_names[i];
		if (!oid)
			break;
		sprintf(query, "select oid from pg_type where typname = '%s'", oid);
		if (do_query(conn, query, &res))
			return TRUE;
		if (PQntuples(res) == 1)
			_oid[i] = atoi(PQgetvalue(res, 0, 0));

		PQclear(res);
	}
	
	return FALSE;
}

// Internal function to convert a database type into a Gambas type

static GB_TYPE conv_type(Oid type)
{
	if (type == _oid[OID_BOOL])
		return GB_T_BOOLEAN;
	
	if (type == _oid[OID_INT2] || type == _oid[OID_INT4])
		return GB_T_INTEGER;

	if (type == _oid[OID_INT8])
		return GB_T_LONG;

	if (type == _oid[OID_NUMERIC] || type == _oid[OID_FLOAT4] || type == _oid[OID_FLOAT8])
			return GB_T_FLOAT;

	if (type == _oid[OID_ABSTIME] || type == _oid[OID_RELTIME] || type == _oid[OID_DATE]
	    || type == _oid[OID_TIME] || type == _oid[OID_TIMESTAMP] || type == _oid[OID_DATETIME]
	    || type == _oid[OID_TIMESTAMPTZ])
		return GB_T_DATE;

	if (type == _oid[OID_BYTEA])
		return DB_T_BLOB;

	return GB_T_STRING;
}

// Internal function to convert a database boolean value

static int conv_boolean(const char *data)
{
	return strcasecmp(data, "t") == 0 || strcasecmp(data, "'t'") == 0;
}

// Internal function to convert a database value into a Gambas variant value

static void conv_data(const char *data, int len, GB_VARIANT_VALUE *val, Oid type)
{
	GB_VALUE conv;
	GB_DATE_SERIAL date;
	double sec;
	bool bc;

	if (type == _oid[OID_BOOL])
	{
		val->type = GB_T_BOOLEAN;
		val->value._boolean = conv_boolean(data) ? -1 : 0;
	}
	else if (type == _oid[OID_INT2] || type == _oid[OID_INT4])
	{
		GB.NumberFromString(GB_NB_READ_INTEGER, data, strlen(data), &conv);

		val->type = GB_T_INTEGER;
		val->value._integer = conv._integer.value;
	}
	else if (type == _oid[OID_INT8])
	{
		GB.NumberFromString(GB_NB_READ_LONG, data, strlen(data), &conv);

		val->type = GB_T_LONG;
		val->value._long = conv._long.value;
	}
	else if (type == _oid[OID_NUMERIC] || type == _oid[OID_FLOAT4] || type == _oid[OID_FLOAT8])
	{
		GB.NumberFromString(GB_NB_READ_FLOAT, data, strlen(data), &conv);

		val->type = GB_T_FLOAT;
		val->value._float = conv._float.value;
	}
	else if (type == _oid[OID_ABSTIME] || type == _oid[OID_RELTIME] || type == _oid[OID_DATE]
	         || type == _oid[OID_TIME] || type == _oid[OID_TIMESTAMP] || type == _oid[OID_DATETIME]
	         || type == _oid[OID_TIMESTAMPTZ])
	{
		memset(&date, 0, sizeof(date));

		if (len > 3 && strcmp(&data[len - 2], "BC") == 0)
			bc = TRUE;
		else
			bc = FALSE;

		if (type == _oid[OID_ABSTIME] || type == _oid[OID_RELTIME] || type == _oid[OID_DATE])
		{
			sscanf(data, "%4d-%2d-%2d", &date.year, &date.month, &date.day);
		}
		else if (type == _oid[OID_TIME])
		{
			sscanf(data, "%2d:%2d:%lf", &date.hour, &date.min, &sec);
			date.sec = (short)sec;
			date.msec = (short)((sec - date.sec) * 1000 + 0.5);
		}
		else
		{
			sscanf(data, "%4d-%2d-%2d %2d:%2d:%lf", &date.year, &date.month, &date.day, &date.hour, &date.min, &sec);
			date.sec = (short)sec;
			date.msec = (short)((sec - date.sec) * 1000 + 0.5);
		}

		if (bc)
			date.year = (-date.year);

		// 4713-01-01 BC is used for null dates

		if (date.year == -4713 && date.month == 1 && date.day == 1)
			date.year = date.month = date.day = 0;

		GB.MakeDate(&date, (GB_DATE *)&conv);

		val->type = GB_T_DATE;
		val->value._date.date = conv._date.value.date;
		val->value._date.time = conv._date.value.time;
	}
	else if (type == _oid[OID_BYTEA])
	{
		// The BLOB are read by the blob_read() driver function
		// You must set NULL there.
		val->type = GB_T_NULL;
	}
	else
	{
		val->type = GB_T_CSTRING;
		val->value._string = (char *)data;
	}
}

static char *quote_string(const char *data, int len, bool add_e)
{
	char *result = NULL;
	int i;
	unsigned char c;
	char buffer[8];

	if (add_e)
		result = GB.AddChar(result, 'E');

	result = GB.AddChar(result, '\'');
	for (i = 0; i < len; i++)
	{
		c = (unsigned char)data[i];
		if (c == '\\')
			result = GB.AddString(result, "\\\\", 2);
		else if (c == '\'')
			result = GB.AddString(result, "''", 2);
		else if (c < 32 || c > 127)
		{
			buffer[0] = '\\';
			buffer[1] = '0' + ((c >> 6) & 0x7);
			buffer[2] = '0' + ((c >> 3) & 0x7);
			buffer[3] = '0' + (c & 0x7);
			result = GB.AddString(result, buffer, 4);
		}
		else
			result = GB.AddChar(result, c);
	}
	result = GB.AddChar(result, '\'');
	
	return GB.FreeStringLater(result);
}

//-------------------------------------------------------------------------

BEGIN_METHOD(PostgresqlHelper_Open, GB_STRING host; GB_STRING port; GB_STRING name; GB_STRING user; GB_STRING password; GB_INTEGER timeout; GB_OBJECT options)

	PGconn *conn;
	char *name = GB.ToZeroString(ARG(name));
	GB_COLLECTION options = VARG(options);
	char buffer[16];

	if (!name || !*name)
		name = "template1";

	//fprintf(stderr, "gb.db.postgresql: host = `%s` port = `%s` dbnname = `%s` user = `%s` password = `%s`\n", desc->host, desc->port, dbname, desc->user, desc->password);

	GB.NewArray(&_options_keys, sizeof(char *), 0);
	GB.NewArray(&_options_values, sizeof(char *), 0);
	
	add_option("host", GB.ToZeroString(ARG(host)));
	add_option("port", GB.ToZeroString(ARG(port)));
	add_option("dbname", name);
	add_option("user", GB.ToZeroString(ARG(user)));
	add_option("password", GB.ToZeroString(ARG(password)));
	sprintf(buffer, "%d", VARG(timeout));
	add_option("connect_timeout", buffer);
	
	if (options)
		GB.Collection.Browse(options, add_option_value);
	
	add_option(NULL, NULL);
	
	conn = PQconnectdbParams((const char *const *)_options_keys, (const char *const *)_options_values, FALSE);
	
	GB.FreeArray(&_options_keys);
	GB.FreeArray(&_options_values);

	if (!conn)
	{
		GB.Error("Out of memory");
		return;
	}

	if (PQstatus(conn) == CONNECTION_BAD)
	{
		GB.Error("Cannot open database: &1", PQerrorMessage(conn));
		PQfinish(conn);
		return;
	}
	
	if (init_datatypes(conn))
	{
		PQfinish(conn);
		return;
	}
	
	/* encoding */

	if (PQsetClientEncoding(conn, GB.System.Charset()))
		fprintf(stderr, "gb.db.postgresql: warning: cannot set encoding to %s\n", GB.System.Charset());

	GB.ReturnPointer(conn);
	
END_METHOD

BEGIN_METHOD(PostgresqlHelper_Close, GB_POINTER database)

	PQfinish((PGconn *)VARG(database));

END_METHOD

BEGIN_METHOD(PostgresqlHelper_GetVersion, GB_POINTER database)

	PGconn *conn = (PGconn *)VARG(database);
	PGresult *res;

	if (!do_query(conn, "select version()", &res))
	{
		GB.ReturnNewZeroString(PQgetvalue(res, 0, 0));
		PQclear(res);
	}

END_METHOD

BEGIN_METHOD(PostgresqlHelper_Query, GB_POINTER database; GB_STRING query)

	PGconn *conn = (PGconn *)VARG(database);
	char *query = GB.ToZeroString(ARG(query));
	PGresult *res;
	
	if (do_query(conn, query, &res))
		return;
	
	GB.ReturnPointer(res);

END_METHOD

BEGIN_METHOD(PostgresqlHelper_FreeResult, GB_POINTER result)

	PQclear((PGresult *)VARG(result));

END_METHOD

BEGIN_METHOD(PostgresqlHelper_GetResultCount, GB_POINTER result)

	GB.ReturnInteger(PQntuples((PGresult *)VARG(result)));

END_METHOD

BEGIN_METHOD(PostgresqlHelper_GetResultData, GB_POINTER result; GB_INTEGER pos; GB_BOOLEAN next)

	PGresult *res = (PGresult *)VARG(result);
	int pos = VARG(pos);
	int i;
	char *data;
	GB_VARIANT value;
	GB_ARRAY buffer;
	
	GB.Array.New(&buffer, GB_T_VARIANT, PQnfields(res));

	for (i = 0; i < PQnfields(res); i++)
	{
		data = PQgetvalue(res, pos, i);

		value.type = GB_T_VARIANT;
		value.value.type = GB_T_NULL;

		if (!PQgetisnull(res, pos, i))
			conv_data(data, PQgetlength(res, pos, i), &value.value, PQftype(res, i));

		GB.StoreVariant(&value, GB.Array.Get(buffer, i));
	}
	
	GB.ReturnObject(buffer);

END_METHOD

BEGIN_METHOD(PostgresqlHelper_GetResultField, GB_POINTER result; GB_INTEGER field)

	PGresult *res = (PGresult *)VARG(result);
	int i = VARG(field);
	int len = 0;

	if (i >= PQnfields(res))
		GB.ReturnNull();
	else
	{
		_type = conv_type(PQftype(res, i));

		if (_type == GB_T_STRING)
		{
			len = PQfmod(res, i);
			if (len < 0)
				len = 0;
			else
				len -= 4;
		}

		_length = len;
		
		GB.ReturnConstZeroString(PQfname(res, i));
	}
	
END_METHOD

BEGIN_PROPERTY(PostgresqlHelper_Type)

	GB.ReturnInteger(_type);

END_PROPERTY

BEGIN_PROPERTY(PostgresqlHelper_Length)

	GB.ReturnInteger(_length);

END_PROPERTY

BEGIN_METHOD(PostgresqlHelper_QuoteString, GB_STRING value; GB_BOOLEAN add_e)

	GB.ReturnString(quote_string(STRING(value), LENGTH(value), VARG(add_e)));

END_METHOD

//-------------------------------------------------------------------------

GB_DESC PostgresqlHelperDesc[] =
{
	GB_DECLARE_STATIC("_PostgresqlHelper"),
	
	GB_STATIC_METHOD("Open", "p", PostgresqlHelper_Open, "(Host)s(Port)s(Name)s(User)s(Password)s(Timeout)i(Options)Collection;"),
	GB_STATIC_METHOD("Close", NULL, PostgresqlHelper_Close, "(Database)p"),
	GB_STATIC_METHOD("GetVersion", "s", PostgresqlHelper_GetVersion, "(Database)p"),
	GB_STATIC_METHOD("Query", "p", PostgresqlHelper_Query, "(Database)p(Query)s"),
	GB_STATIC_METHOD("FreeResult", NULL, PostgresqlHelper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetResultCount", "i", PostgresqlHelper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("GetResultField", "s", PostgresqlHelper_GetResultField, "(Result)p(Field)i"),
	GB_STATIC_METHOD("GetResultData", "Variant[]", PostgresqlHelper_GetResultData,"(Result)p(Index)l(Next)b"),
	GB_STATIC_METHOD("QuoteString", "s", PostgresqlHelper_QuoteString, "(Value)s(AddE)b"),
	
	GB_STATIC_PROPERTY_READ("Type", "i", PostgresqlHelper_Type),
	GB_STATIC_PROPERTY_READ("Length", "i", PostgresqlHelper_Length),
	
	GB_END_DECLARE
};

//-------------------------------------------------------------------------

GB_DESC *GB_CLASSES [] EXPORT =
{
	PostgresqlHelperDesc,
	NULL
};

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}
