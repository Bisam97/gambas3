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

#include "helper.h"
#include "main.h"

GB_INTERFACE GB EXPORT;

//-------------------------------------------------------------------------

static void conv_data(const char *data, int len, GB_VARIANT_VALUE *val, int type)
{
	GB_VALUE conv;
	GB_DATE_SERIAL date;
	double sec;

	switch (type)
	{
		case GB_T_BOOLEAN:

			val->type = GB_T_BOOLEAN;
			if (data[0] == 't' || data[0] == 'T')
				val->value._boolean = -1;
			else
				val->value._boolean = atoi(data) ? -1 : 0;
			break;

		case GB_T_INTEGER:

			GB.NumberFromString(GB_NB_READ_INTEGER, data, len, &conv);

			val->type = GB_T_INTEGER;
			val->value._integer = conv._integer.value;

			break;

		case GB_T_FLOAT:

			GB.NumberFromString(GB_NB_READ_FLOAT, data, len, &conv);

			val->type = GB_T_FLOAT;
			val->value._float = conv._float.value;

			break;

		case GB_T_LONG:

			GB.NumberFromString(GB_NB_READ_LONG, data, len, &conv);

			val->type = GB_T_LONG;
			val->value._long = conv._long.value;

			break;

		case GB_T_DATE:

			// TODO: Handle ISO6801 dates
			// TODO: Handle timezone
			
			memset(&date, 0, sizeof(date));

			switch (len)
			{
				case 14:
					sscanf(data, "%4d%2d%2d%2d%2d%lf", &date.year, &date.month,
								&date.day, &date.hour, &date.min, &sec);
					date.sec = (short) sec;
					date.msec = (short) ((sec - date.sec) * 1000 + 0.5);
					break;
				case 12:
					sscanf(data, "%2d%2d%2d%2d%2d%lf", &date.year, &date.month,
								&date.day, &date.hour, &date.min, &sec);
					date.sec = (short) sec;
					date.msec = (short) ((sec - date.sec) * 1000 + 0.5);
					break;
				case 10:
					if (sscanf(data, "%4d-%2d-%2d", &date.year, &date.month,
										&date.day) != 3)
					{
						if (sscanf(data, "%4d/%2d/%2d", &date.year, &date.month,
											&date.day) != 3)
						{
							if (sscanf(data, "%4d:%2d:%lf", &date.hour, &date.min,
												&sec) == 3)
							{
								date.sec = (short) sec;
								date.msec = (short) ((sec - date.sec) * 1000 + 0.5);
							}
							else
							{
								sscanf(data, "%2d%2d%2d%2d%2d", &date.year,
											&date.month, &date.day, &date.hour, &date.min);
							}
						}
					}

					break;
				case 8:
					if (sscanf(data, "%4d%2d%2d", &date.year, &date.month,
										&date.day) != 3)
					{
						sscanf(data, "%2d/%2d/%2d", &date.year, &date.month,
									&date.day);
					}
					break;
				case 6:
					sscanf(data, "%2d%2d%2d", &date.year, &date.month, &date.day);
					break;
				case 4:
					sscanf(data, "%2d%2d", &date.year, &date.month);
					break;
				case 2:
					sscanf(data, "%2d", &date.year);
					break;
				default:
					sscanf(data, "%4d-%2d-%2d %2d:%2d:%lf", &date.year,
								&date.month, &date.day, &date.hour, &date.min, &sec);
					date.sec = (short)sec;
					date.msec = (short)((sec - date.sec) * 1000 + 0.5);
			}
			if (date.year < 100)
				date.year += 1900;

			GB.MakeDate(&date, (GB_DATE *)&conv);

			val->type = GB_T_DATE;
			val->value._date.date = conv._date.value.date;
			val->value._date.time = conv._date.value.time;

			break;

		case (int)DB_T_BLOB:
			// Blob fields are read by blob_read()
			val->type = GB_T_NULL;
			break;

		default:

			val->type = GB_T_CSTRING;
			val->value._string = (char *)data;
	}
}

//-------------------------------------------------------------------------

BEGIN_METHOD(Sqlite3Helper_Open, GB_STRING path; GB_STRING host)

	GB.ReturnPointer(sqlite_open_database(GB.ToZeroString(ARG(path)), GB.ToZeroString(ARG(host))));

END_METHOD

BEGIN_METHOD_VOID(Sqlite3Helper_GetVersion)

	GB.ReturnNewZeroString(sqlite3_libversion());

END_METHOD

BEGIN_METHOD(Sqlite3Helper_Close, GB_POINTER database)

	sqlite_close_database((SQLITE_DATABASE *)VARG(database));

END_METHOD

BEGIN_METHOD(Sqlite3Helper_Query, GB_POINTER conn; GB_STRING query; GB_INTEGER timeout; GB_BOOLEAN with_types)

	SQLITE_RESULT *res;
	int timeout = VARG(timeout);
	int max_retry;
	SQLITE_DATABASE *conn = (SQLITE_DATABASE *)VARG(conn);
	char *query = GB.ToZeroString(ARG(query));
	bool with_types = VARGOPT(with_types, FALSE);
	int err;
	int retry = 0;
	
	if (timeout > 0)
		max_retry = timeout * 5;
	else if (timeout == 0)
		max_retry = 600; // 120 s max
	else
		max_retry = 0;

	for(;;)
	{
		err = 0;

		res = sqlite_query_exec(conn, query, with_types);

		if (res)
		{
			GB.ReturnPointer(res);
			return;
		}

		err = conn->error;

		if (err != SQLITE_BUSY || retry >= max_retry)
		{
			GB.ReturnNull();
			return;
		}

		retry++;
		usleep(200000);
	}

END_METHOD

BEGIN_METHOD(Sqlite3Helper_FreeResult, GB_POINTER result)

	sqlite_query_free((SQLITE_RESULT *)VARG(result));

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetError, GB_POINTER conn)

	GB.ReturnInteger(((SQLITE_DATABASE *)VARG(conn))->error);

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetErrorMessage, GB_POINTER conn)

	GB.ReturnConstZeroString(sqlite_get_error_message((SQLITE_DATABASE *)VARG(conn)));

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetResultCount, GB_POINTER result)

	GB.ReturnLong(((SQLITE_RESULT *)VARG(result))->nrow);

END_METHOD

BEGIN_METHOD(Sqlite3Helper_FillResult, GB_POINTER result; GB_LONG pos; GB_BOOLEAN next)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int64_t pos = VARG(pos);
	int i;
	char *data;
	int len;
	GB_VARIANT value;
	int type;
	GB_ARRAY buffer;
	
	GB.Array.New(&buffer, GB_T_VARIANT, res->ncol);
	
	for (i = 0; i < res->ncol; i++)
	{
		type = res->types[i];

		if (type == DB_T_BLOB)
			data = NULL;
		else
		{
			// TODO: make result position an int64_t
			sqlite_query_get(res, (int)pos, i, &data, &len);
			if (len == 0)
				data = NULL;
		}

		value.type = GB_T_VARIANT;
		value.value.type = GB_T_NULL;

		if (data)
			conv_data(data, len, &value.value, type);
		
		GB.StoreVariant(&value, GB.Array.Get(buffer, i));
	}
	
	GB.ReturnObject(buffer);

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetResultFields, GB_POINTER result)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int i;
	GB_ARRAY fields;

	GB.Array.New(&fields, GB_T_STRING, res->ncol);
	for (i = 0; i < res->ncol; i++)
		*(char **)GB.Array.Get(fields, i) = GB.NewZeroString(res->names[i]);
	
	GB.ReturnObject(fields);

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetResultTypes, GB_POINTER result)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int i;
	GB_ARRAY types;

	GB.Array.New(&types, GB_T_INTEGER, res->ncol);
	for (i = 0; i < res->ncol; i++)
		*(int *)GB.Array.Get(types, i) = res->types[i];
	
	GB.ReturnObject(types);

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetResultLengths, GB_POINTER result)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int i;
	GB_ARRAY lengths;

	GB.Array.New(&lengths, GB_T_INTEGER, res->ncol);
	for (i = 0; i < res->ncol; i++)
		*(int *)GB.Array.Get(lengths, i) = res->lengths[i];
	
	GB.ReturnObject(lengths);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC Sqlite3HelperDesc[] =
{
	GB_DECLARE_STATIC("_Sqlite3Helper"),
	
	GB_STATIC_METHOD("Open", "p", Sqlite3Helper_Open, "(Path)s(Host)s"),
	GB_STATIC_METHOD("GetVersion", "s", Sqlite3Helper_GetVersion, NULL),
	GB_STATIC_METHOD("Close", NULL, Sqlite3Helper_Close, "(Database)p"),
	GB_STATIC_METHOD("Query", "p", Sqlite3Helper_Query, "(Database)p(Query)s(Timeout)i[(WithTypes)b]"),
	GB_STATIC_METHOD("FreeResult", NULL, Sqlite3Helper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetError", "i", Sqlite3Helper_GetError, "(Database)p"),
	GB_STATIC_METHOD("GetErrorMessage", "s", Sqlite3Helper_GetErrorMessage, "(Database)p"),
	GB_STATIC_METHOD("GetResultCount", "l", Sqlite3Helper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("FillResult","Variant[]",Sqlite3Helper_FillResult,"(Result)p(Index)l(Next)b"),
	GB_STATIC_METHOD("GetResultFields", "String[]", Sqlite3Helper_GetResultFields, "(Result)p"),
	GB_STATIC_METHOD("GetResultTypes", "Integer[]", Sqlite3Helper_GetResultTypes, "(Result)p"),
	GB_STATIC_METHOD("GetResultLengths", "Integer[]", Sqlite3Helper_GetResultLengths, "(Result)p"),
	
	GB_END_DECLARE
};

//-------------------------------------------------------------------------

GB_DESC *GB_CLASSES [] EXPORT =
{
	Sqlite3HelperDesc,
	NULL
};

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}
