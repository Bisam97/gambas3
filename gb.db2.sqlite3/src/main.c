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

static int _type = 0;
static int _length = 0;

//-------------------------------------------------------------------------

static void conv_data(const char *data, int len, GB_VARIANT_VALUE *val, int type)
{
	GB_VALUE conv;
	GB_DATE_SERIAL date;
	double sec;
	bool gmt;

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

			// TODO: Handle timezone
			
			//fprintf(stderr, "conv_data: date = %.*s\n", len, data);
			
			memset(&date, 0, sizeof(date));
			
			if (len >= 18 && data[len - 1] == 'Z')
			{
				gmt = FALSE;
				
				CLEAR(&date);
				
				if (len == 18)
				{
					sscanf(data, "%4d-%2d-%2dT%2d%2d%2dZ", &date.year, &date.month, &date.day, &date.hour, &date.min, &date.sec);
				}
				else if (len == 22)
					sscanf(data, "%4d-%2d-%2dT%2d%2d%2d.%3dZ", &date.year, &date.month, &date.day, &date.hour, &date.min, &date.sec, &date.msec);
			}
			else
			{
				gmt = TRUE;

				switch (len)
				{
					case 14:
						sscanf(data, "%4d%2d%2d%2d%2d%lf", &date.year, &date.month, &date.day, &date.hour, &date.min, &sec);
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
			}
			
			GB.MakeDate(&date, (GB_DATE *)&conv, !gmt);

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

static char *quote_blob(const char *data, int len)
{
	static const char *hexa_digit = "0123456789ABCDEF";

	char *result = NULL;
	int i;
	unsigned char c;

	if (len == 0)
	{
		result = GB.AddString(result, "NULL", 4);
	}
	else
	{
		result = GB.AddChar(result, 'X');
		result = GB.AddChar(result, '\'');
		for (i = 0; i < len; i++)
		{
			c = (unsigned char) data[i];
			result = GB.AddChar(result, hexa_digit[c >> 4]);
			result = GB.AddChar(result, hexa_digit[c & 15]);
		}
		result = GB.AddChar(result, '\'');
	}
	
	return GB.FreeStringLater(result);
}

//-------------------------------------------------------------------------

BEGIN_METHOD(Sqlite3Helper_Open, GB_STRING path)

	GB.ReturnPointer(sqlite_open_database(GB.ToZeroString(ARG(path))));

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

	GB.ReturnInteger(((SQLITE_RESULT *)VARG(result))->nrow);

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetResultField, GB_POINTER result; GB_INTEGER field)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int i = VARG(field);
	
	if (i >= res->ncol)
		GB.ReturnNull();
	else
	{
		_type = res->types[i];
		_length = res->lengths[i];
		GB.ReturnConstZeroString(res->names[i]);
	}

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetResultData, GB_POINTER result; GB_INTEGER pos; GB_BOOLEAN next)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int pos = VARG(pos);
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
			sqlite_query_get(res, pos, i, &data, &len);
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

BEGIN_METHOD(Sqlite3Helper_GetResultBlob, GB_POINTER result; GB_INTEGER pos; GB_INTEGER field)

	SQLITE_RESULT *res = (SQLITE_RESULT *)VARG(result);
	int pos = VARG(pos);
	int field = VARG(field);
	char *data;
	int len;

	sqlite_query_get(res, pos, field, &data, &len);
	GB.ReturnConstString(data, len);
	
END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetType, GB_STRING name)

	GB.ReturnInteger(sqlite_get_type(GB.ToZeroString(ARG(name)), &_length));

END_METHOD

BEGIN_METHOD(Sqlite3Helper_GetValue, GB_STRING value; GB_INTEGER type)

	GB_VARIANT_VALUE value;
	
	conv_data(STRING(value), LENGTH(value), &value, VARG(type));
	GB.ReturnVariant(&value);
	
END_METHOD

BEGIN_PROPERTY(Sqlite3Helper_Type)

	GB.ReturnInteger(_type);

END_PROPERTY

BEGIN_PROPERTY(Sqlite3Helper_Length)

	GB.ReturnInteger(_length);

END_PROPERTY

BEGIN_METHOD(Sqlite3Helper_QuoteBlob, GB_STRING value)

	GB.ReturnString(quote_blob(STRING(value), LENGTH(value)));

END_METHOD

//-------------------------------------------------------------------------

GB_DESC Sqlite3HelperDesc[] =
{
	GB_DECLARE_STATIC("_Sqlite3Helper"),
	
	GB_STATIC_METHOD("Open", "p", Sqlite3Helper_Open, "(Path)s"),
	GB_STATIC_METHOD("GetVersion", "s", Sqlite3Helper_GetVersion, NULL),
	GB_STATIC_METHOD("Close", NULL, Sqlite3Helper_Close, "(Database)p"),
	GB_STATIC_METHOD("Query", "p", Sqlite3Helper_Query, "(Database)p(Query)s(Timeout)i[(WithTypes)b]"),
	GB_STATIC_METHOD("FreeResult", NULL, Sqlite3Helper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetError", "i", Sqlite3Helper_GetError, "(Database)p"),
	GB_STATIC_METHOD("GetErrorMessage", "s", Sqlite3Helper_GetErrorMessage, "(Database)p"),
	GB_STATIC_METHOD("GetResultCount", "i", Sqlite3Helper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("GetResultField", "s", Sqlite3Helper_GetResultField, "(Result)p(Field)i"),
	GB_STATIC_METHOD("GetResultData", "Variant[]", Sqlite3Helper_GetResultData,"(Result)p(Index)i(Next)b"),
	GB_STATIC_METHOD("GetResultBlob", "s", Sqlite3Helper_GetResultBlob,"(Result)p(Index)i(Field)i"),
	GB_STATIC_METHOD("GetType", "i", Sqlite3Helper_GetType, "(Type)s"),
	GB_STATIC_METHOD("GetValue", "v", Sqlite3Helper_GetValue, "(Value)s(Type)i"),
	GB_STATIC_METHOD("QuoteBlob", "s", Sqlite3Helper_QuoteBlob, "(Value)s"),
	
	GB_STATIC_PROPERTY_READ("Type", "i", Sqlite3Helper_Type),
	GB_STATIC_PROPERTY_READ("Length", "i", Sqlite3Helper_Length),
	
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
