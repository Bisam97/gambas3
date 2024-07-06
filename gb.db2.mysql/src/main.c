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

#include <mysql.h>
#include "main.h"

typedef
	struct {
		const char *pattern;
		int type;
		}
	CONV_STRING_TYPE;

typedef
	struct {
		const char *key;
		int cst;
		int type;
		}
	DB_MYSQL_OPTION;

//-------------------------------------------------------------------------

GB_INTERFACE GB EXPORT;

static int _last_error = 0;
static GB_TYPE _type = 0;
static int _length = 0;
static GB_VARIANT_VALUE _default = { GB_T_NULL };
static char *_collation = NULL;

// mySQL datatypes

static CONV_STRING_TYPE _types[] =
	{
		{ "tinyint", FIELD_TYPE_TINY },
		{ "smallint", FIELD_TYPE_SHORT },
		{ "mediumint", FIELD_TYPE_INT24 },
		{ "int", FIELD_TYPE_LONG },
		{ "bigint", FIELD_TYPE_LONGLONG },
		{ "decimal", FIELD_TYPE_DECIMAL },
		{ "numeric", FIELD_TYPE_DECIMAL },
		{ "float", FIELD_TYPE_FLOAT },
		{ "double", FIELD_TYPE_DOUBLE },
		{ "real", FIELD_TYPE_DOUBLE },
		{ "timestamp", FIELD_TYPE_TIMESTAMP },
		{ "date", FIELD_TYPE_DATE },
		{ "time", FIELD_TYPE_TIME },
		{ "datetime", FIELD_TYPE_DATETIME },
		{ "year", FIELD_TYPE_YEAR },
		{ "char", FIELD_TYPE_STRING },
		{ "varchar", FIELD_TYPE_VAR_STRING },
		{ "blob", FIELD_TYPE_BLOB },
		{ "tinyblob", FIELD_TYPE_TINY_BLOB },
		{ "mediumblob", FIELD_TYPE_MEDIUM_BLOB },
		{ "longblob", FIELD_TYPE_LONG_BLOB },
		{ "text", FIELD_TYPE_BLOB },
		{ "tinytext", FIELD_TYPE_TINY_BLOB },
		{ "mediumtext", FIELD_TYPE_MEDIUM_BLOB },
		{ "longtext", FIELD_TYPE_LONG_BLOB },
		{ "set", FIELD_TYPE_SET },
		{ "enum", FIELD_TYPE_ENUM },
		{ "bit", FIELD_TYPE_BIT },
		{ "null", FIELD_TYPE_NULL },
		{ NULL, 0 },
	};

//-------------------------------------------------------------------------

static DB_MYSQL_OPTION _options[] = {
#if LIBMYSQL_VERSION_ID >= 50000
	{ "INIT_COMMAND", MYSQL_INIT_COMMAND, GB_T_STRING },
	{ "COMPRESS", MYSQL_OPT_COMPRESS, GB_T_BOOLEAN },
	{ "CONNECT_TIMEOUT ", MYSQL_OPT_CONNECT_TIMEOUT , GB_T_INTEGER},
	{ "LOCAL_INFILE", MYSQL_OPT_LOCAL_INFILE, GB_T_BOOLEAN},
	{ "PROTOCOL", MYSQL_OPT_PROTOCOL, GB_T_STRING},
	{ "READ_TIMEOUT", MYSQL_OPT_READ_TIMEOUT, GB_T_INTEGER},
#if LIBMYSQL_VERSION_ID < 80034
	{ "RECONNECT", MYSQL_OPT_RECONNECT, GB_T_BOOLEAN},
#endif
#if LIBMYSQL_VERSION_ID < 80000
	{ "SSL_VERIFY_SERVER_CERT", MYSQL_OPT_SSL_VERIFY_SERVER_CERT, GB_T_BOOLEAN},
#endif
	{ "WRITE_TIMEOUT", MYSQL_OPT_WRITE_TIMEOUT, GB_T_INTEGER},
	{ "READ_DEFAULT_FILE", MYSQL_READ_DEFAULT_FILE, GB_T_STRING},
	{ "READ_DEFAULT_GROUP", MYSQL_READ_DEFAULT_GROUP, GB_T_STRING},
	{ "REPORT_DATA_TRUNCATION", MYSQL_REPORT_DATA_TRUNCATION, GB_T_BOOLEAN},
#if LIBMYSQL_VERSION_ID < 80000
	{ "SECURE_AUTH", MYSQL_SECURE_AUTH, GB_T_BOOLEAN},
#endif
	{ "SET_CHARSET_DIR", MYSQL_SET_CHARSET_DIR, GB_T_STRING},
	{ "SET_CHARSET_NAME", MYSQL_SET_CHARSET_NAME, GB_T_STRING},
#endif
	
#if LIBMYSQL_VERSION_ID >= 50200
	{ "DEFAULT_AUTH", MYSQL_DEFAULT_AUTH, GB_T_STRING },
	{ "ENABLE_CLEARTEXT_PLUGIN", MYSQL_ENABLE_CLEARTEXT_PLUGIN, GB_T_BOOLEAN },
	{ "SSL_MODE", MYSQL_OPT_SSL_MODE, GB_T_STRING},
	{ "PLUGIN_DIR", MYSQL_PLUGIN_DIR, GB_T_STRING},
#endif
	
#if LIBMYSQL_VERSION_ID >= 50626
	{ "BIND", MYSQL_OPT_BIND, GB_T_STRING },
	{ "CAN_HANDLE_EXPIRED_PASSWORDS", MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS, GB_T_BOOLEAN },
#if HAVE_MYSQL_RETRY_COUNT
	{ "RETRY_COUNT ", MYSQL_OPT_RETRY_COUNT , GB_T_INTEGER},
#endif
	{ "SSL_CA", MYSQL_OPT_SSL_CA, GB_T_STRING},
	{ "SSL_CAPATH", MYSQL_OPT_SSL_CAPATH, GB_T_STRING},
	{ "SSL_CERT", MYSQL_OPT_SSL_CERT, GB_T_STRING},
	{ "SSL_CIPHER", MYSQL_OPT_SSL_CIPHER, GB_T_STRING},
	{ "SSL_CRL", MYSQL_OPT_SSL_CRL, GB_T_STRING},
	{ "SSL_CRLPATH", MYSQL_OPT_SSL_CRLPATH, GB_T_STRING},
	{ "SSL_KEY", MYSQL_OPT_SSL_KEY, GB_T_STRING},
	{ "SERVER_PUBLIC_KEY", MYSQL_SERVER_PUBLIC_KEY, GB_T_STRING},
#endif	

#if LIBMYSQL_VERSION_ID >= 50700
	{ "GET_SERVER_PUBLIC_KEY", MYSQL_OPT_GET_SERVER_PUBLIC_KEY, GB_T_BOOLEAN},
	{ "MAX_ALLOWED_PACKET", MYSQL_OPT_MAX_ALLOWED_PACKET, GB_T_INTEGER},
	{ "NET_BUFFER_LENGTH", MYSQL_OPT_NET_BUFFER_LENGTH, GB_T_INTEGER},
	{ "TLS_VERSION", MYSQL_OPT_TLS_VERSION, GB_T_STRING},
#endif
	
#if LIBMYSQL_VERSION_ID >= 80000
	{ "COMPRESSION_ALGORITHMS", MYSQL_OPT_COMPRESSION_ALGORITHMS, GB_T_STRING},
	{ "LOAD_DATA_LOCAL_DIR", MYSQL_OPT_LOAD_DATA_LOCAL_DIR, GB_T_STRING},
	{ "TLS_CIPHERSUITES", MYSQL_OPT_TLS_CIPHERSUITES, GB_T_STRING},
	{ "SSL_FIPS_MODE", MYSQL_OPT_SSL_FIPS_MODE, GB_T_INTEGER},
	{ "ZSTD_COMPRESSION_LEVEL", MYSQL_OPT_ZSTD_COMPRESSION_LEVEL, GB_T_INTEGER},
#endif
	
	{ NULL }
};

static MYSQL *_options_conn;

static void add_option_value(const char *key, int len, GB_VALUE *value)
{
	DB_MYSQL_OPTION *p;
	union {
		unsigned int _uint;
		unsigned long _ulong;
	} tmp;
	char *sval;
	
	for (p = _options;; p++)
	{
		if (!p->key)
			return;
		
		if (strlen(p->key) == len && !strncasecmp(p->key, key, len))
			break;
	}
	
	if (GB.Conv(value, p->type))
		return;
	
	switch(p->cst)
	{
		case MYSQL_OPT_COMPRESS:
			if (value->_boolean.value)
				mysql_options(_options_conn, p->cst, NULL);
			break;
		
		case MYSQL_OPT_PROTOCOL:

			sval = value->_string.value.addr;
			if (!strcasecmp(sval, "DEFAULT"))
				tmp._uint = MYSQL_PROTOCOL_DEFAULT;
			else if (!strcasecmp(sval, "TCP"))
				tmp._uint = MYSQL_PROTOCOL_TCP;
			else if (!strcasecmp(sval, "SOCKET"))
				tmp._uint = MYSQL_PROTOCOL_SOCKET;
			else if (!strcasecmp(sval, "PIPE"))
				tmp._uint = MYSQL_PROTOCOL_PIPE;
			else if (!strcasecmp(sval, "MEMORY"))
				tmp._uint = MYSQL_PROTOCOL_MEMORY;
			else
				return;
			
			mysql_options(_options_conn, p->cst, &tmp._uint);
			break;
		
		case MYSQL_OPT_LOCAL_INFILE:
			
			tmp._uint = value->_boolean.value;
			mysql_options(_options_conn, p->cst, &tmp._uint);
			break;
			
#if LIBMYSQL_VERSION_ID >= 50200
		case MYSQL_OPT_SSL_MODE:
			
			sval = value->_string.value.addr;
			if (!strcasecmp(sval, "DISABLED"))
				tmp._uint = SSL_MODE_DISABLED;
			else if (!strcasecmp(sval, "PREFERRED"))
				tmp._uint = SSL_MODE_PREFERRED;
			else if (!strcasecmp(sval, "REQUIRED"))
				tmp._uint = SSL_MODE_REQUIRED;
			else if (!strcasecmp(sval, "VERIFY_CA"))
				tmp._uint = SSL_MODE_VERIFY_CA;
			else if (!strcasecmp(sval, "VERIFY_IDENTITY"))
				tmp._uint = SSL_MODE_VERIFY_IDENTITY;
			else
				return;
			
			mysql_options(_options_conn, p->cst, &tmp._uint);
			break;
#endif
		
#if LIBMYSQL_VERSION_ID >= 50700
		case MYSQL_OPT_MAX_ALLOWED_PACKET:
		case MYSQL_OPT_NET_BUFFER_LENGTH:
			
			tmp._ulong = value->_integer.value;
			mysql_options(_options_conn, p->cst, &tmp._ulong);
			break;
#endif

#if LIBMYSQL_VERSION_ID >= 80000
		case MYSQL_OPT_SSL_FIPS_MODE:
			
			sval = value->_string.value.addr;
			if (!strcasecmp(sval, "OFF"))
				tmp._uint = SSL_FIPS_MODE_OFF;
			else if (!strcasecmp(sval, "ON"))
				tmp._uint = SSL_FIPS_MODE_ON;
			else if (!strcasecmp(sval, "STRICT"))
				tmp._uint = SSL_FIPS_MODE_STRICT;
			else
				return;
			
			mysql_options(_options_conn, p->cst, &tmp._uint);
			break;
#endif

		default:
			
			if (p->type == GB_T_BOOLEAN)
				mysql_options(_options_conn, p->cst, &value->_boolean.value);
			else if (p->type == GB_T_INTEGER)
				mysql_options(_options_conn, p->cst, &value->_integer.value);
			else if (p->type == GB_T_STRING)
				mysql_options(_options_conn, p->cst, value->_string.value.addr);
	}
	
}


static char *quote_string(const char *data, int len)
{
	char *result = NULL;
	int i;
	unsigned char c;
	//char buffer[8];

	result = GB.AddChar(result, '\'');
	for (i = 0; i < len; i++)
	{
		c = (unsigned char)data[i];
		if (c == '\\')
			result = GB.AddString(result, "\\\\", 2);
		else if (c == '\'')
			result = GB.AddString(result, "''", 2);
		else if (c == 0)
			result = GB.AddString(result, "\\0", 2);
		else
			result = GB.AddChar(result, c);
	}
	result = GB.AddChar(result, '\'');

	return GB.FreeStringLater(result);
}


static void check_connection(MYSQL *conn)
{
	unsigned long thread_id;

	thread_id = mysql_thread_id(conn);

	mysql_ping(conn);

	if (mysql_thread_id(conn) != thread_id)
	{
		//DB.Debug("gb.db.mysql", "connection lost\n");
		// Connection has been reestablished, set utf8 again
		mysql_query(conn, "set names 'utf8'");
	}
}


static bool do_query(MYSQL *conn, const char *query, MYSQL_RES **pres)
{
	MYSQL_RES *res;
	int ret;

	check_connection(conn);

	if (mysql_query(conn, query))
	{
		GB.Error("&1", mysql_error(conn));
		ret = TRUE;
	}
	else 
	{
		res = mysql_store_result(conn);
		ret = FALSE;
		if (pres)
			*pres = res;
		else
			mysql_free_result(res);
	}

	_last_error = mysql_errno(conn);
	return ret;
}


// Internal function to convert a database type into a Gambas type
//
// Look at https://dev.mysql.com/doc/refman/5.0/en/c-api-data-structures.html
// for how to make the difference between a text field and a blob field.

#define IS_BINARY_FIELD(_f) ((_f)->charsetnr == 63)
#define SET_BINARY_FIELD(_f, _v) ((_f)->charsetnr = (_v) ? 63 : 0)

static GB_TYPE conv_type(const MYSQL_FIELD *f)
{
	switch(f->type)
	{
		case FIELD_TYPE_TINY:
			return (f->max_length == 1 && f->length == 1) ? GB_T_BOOLEAN : GB_T_INTEGER;

		case FIELD_TYPE_INT24:
		case FIELD_TYPE_SHORT:
		case FIELD_TYPE_LONG:
		case FIELD_TYPE_YEAR:
			return GB_T_INTEGER;

		case FIELD_TYPE_LONGLONG:
			return GB_T_LONG;

		case FIELD_TYPE_FLOAT:
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:
			return GB_T_FLOAT;

		case FIELD_TYPE_DATE:
		case FIELD_TYPE_DATETIME:
		case FIELD_TYPE_TIME:
		case FIELD_TYPE_TIMESTAMP:
			return GB_T_DATE;

		case FIELD_TYPE_LONG_BLOB:
		case FIELD_TYPE_TINY_BLOB:
		case FIELD_TYPE_MEDIUM_BLOB:
		case FIELD_TYPE_BLOB:
			if (IS_BINARY_FIELD(f))
				return DB_T_BLOB;
			else
				return GB_T_STRING;
			
		case FIELD_TYPE_BIT:
			if (f->max_length == 1)
				return GB_T_BOOLEAN;
			else if (f->max_length <= 32)
				return GB_T_INTEGER;
			else if (f->max_length <= 64)
				return GB_T_LONG;

		case FIELD_TYPE_STRING:
		case FIELD_TYPE_VAR_STRING:
		case FIELD_TYPE_SET:
		case FIELD_TYPE_ENUM:
		default:
			//fprintf(stderr, "FIELD_TYPE_*: %d\n", len);
			return GB_T_STRING;

	}
}


// Internal function to convert a string database type
// into a fake MYSQL_FIELD structure

static void conv_string_type(const char *type, MYSQL_FIELD *f)
{
	CONV_STRING_TYPE *cst;
	long l;

	if (strncmp(type, "national ", 9) == 0)
		type += 9;

	for (cst = _types; cst->pattern; cst++)
	{
		if (strncmp(type, cst->pattern, strlen(cst->pattern)) == 0)
			break;
	}

	if (cst->type)
	{
		SET_BINARY_FIELD(f, FALSE);
		f->max_length = 0;
		
		if (cst->type == FIELD_TYPE_BLOB || cst->type == FIELD_TYPE_TINY_BLOB || cst->type == FIELD_TYPE_MEDIUM_BLOB || cst->type == FIELD_TYPE_LONG_BLOB)
		{
			SET_BINARY_FIELD(f, strcmp(&type[strlen(type) - 4], "blob") == 0);
		}
		else
		{
			type += strlen(cst->pattern);
			if (sscanf(type, "(%ld)", &l) == 1)
			{
				f->max_length = l;
				if (cst->type == FIELD_TYPE_TINY)
					f->length = l;
			}
		}
	}

	f->type = cst->type;
}


static void conv_data(int version, const char *data, long data_length, GB_VARIANT_VALUE *val, MYSQL_FIELD *f)
{
	GB_VALUE conv;
	GB_DATE_SERIAL date;
	double sec;
	int type = f->type;

	switch (type)
	{
		case FIELD_TYPE_TINY:

			if (f->max_length == 1 && f->length == 1)
			{
				val->type = GB_T_BOOLEAN;
				/*GB.NumberFromString(GB_NB_READ_INTEGER, data, strlen(data), &conv);*/
				val->value._boolean = atoi(data) != 0 ? -1 : 0;
			}
			else
			{
				GB.NumberFromString(GB_NB_READ_INTEGER, data, strlen(data), &conv);

				val->type = GB_T_INTEGER;
				val->value._integer = conv._integer.value;
			}

			break;

		case FIELD_TYPE_INT24:
		case FIELD_TYPE_SHORT:
		case FIELD_TYPE_LONG:
		/*case FIELD_TYPE_TINY:*/
		case FIELD_TYPE_YEAR:

			GB.NumberFromString(GB_NB_READ_INTEGER, data, strlen(data), &conv);

			val->type = GB_T_INTEGER;
			val->value._integer = conv._integer.value;

			break;

		case FIELD_TYPE_LONGLONG:

			GB.NumberFromString(GB_NB_READ_LONG, data, strlen(data), &conv);

			val->type = GB_T_LONG;
			val->value._long = conv._long.value;

			break;

		case FIELD_TYPE_FLOAT:
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:

			GB.NumberFromString(GB_NB_READ_FLOAT, data, strlen(data), &conv);

			val->type = GB_T_FLOAT;
			val->value._float = conv._float.value;

			break;

		case FIELD_TYPE_DATE:
		case FIELD_TYPE_DATETIME:
		case FIELD_TYPE_TIME:
		case FIELD_TYPE_TIMESTAMP:

			// TIMESTAMP display format changed since MySQL 4.1!
			if (type == FIELD_TYPE_TIMESTAMP && version >= 40100)
				type = FIELD_TYPE_DATETIME;

			memset(&date, 0, sizeof(date));

			switch(type)
			{
				case FIELD_TYPE_DATE:

					sscanf(data, "%4d-%2d-%2d", &date.year, &date.month, &date.day);
					break;

				case FIELD_TYPE_TIME:

					sscanf(data, "%4d:%2d:%lf", &date.hour, &date.min, &sec);
					date.sec = (short)sec;
					date.msec = (short)((sec - date.sec) * 1000 + 0.5);
					break;

				case FIELD_TYPE_DATETIME:

					sscanf(data, "%4d-%2d-%2d %2d:%2d:%lf", &date.year, &date.month, &date.day, &date.hour, &date.min, &sec);
					date.sec = (short)sec;
					date.msec = (short)((sec - date.sec) * 1000 + 0.5);
					break;

				case FIELD_TYPE_TIMESTAMP:
					switch(strlen(data))
					{
						case 14:
							sscanf(data, "%4d%2d%2d%2d%2d%lf", &date.year, &date.month, &date.day, &date.hour, &date.min, &sec);
							date.sec = (short)sec;
							date.msec = (short)((sec - date.sec) * 1000 + 0.5);
							break;
						case 12:
							sscanf(data, "%2d%2d%2d%2d%2d%lf", &date.year, &date.month, &date.day, &date.hour, &date.min, &sec);
							date.sec = (short)sec;
							date.msec = (short)((sec - date.sec) * 1000 + 0.5);
							break;
						case 10:
							sscanf(data, "%2d%2d%2d%2d%2d", &date.year, &date.month, &date.day, &date.hour, &date.min );
							break;
						case 8:
							sscanf(data, "%4d%2d%2d", &date.year, &date.month, &date.day);
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
					}
					if (date.year < 100)
							date.year += 1900;
				break;
			}

			GB.MakeDate(&date, (GB_DATE *)&conv);

			val->type = GB_T_DATE;
			val->value._date.date = conv._date.value.date;
			val->value._date.time = conv._date.value.time;

			break;

		case FIELD_TYPE_BLOB:
		case FIELD_TYPE_LONG_BLOB:
		case FIELD_TYPE_TINY_BLOB:
		case FIELD_TYPE_MEDIUM_BLOB:
			if (IS_BINARY_FIELD(f))
			{
				// The BLOB are read by the blob_read() driver function
				// You must set NULL there.
				val->type = GB_T_NULL;
				break;
			}
			// else continue!

		case FIELD_TYPE_STRING:
		case FIELD_TYPE_VAR_STRING:
		case FIELD_TYPE_SET:
		case FIELD_TYPE_ENUM:
		default:
			val->type = GB_T_CSTRING;
			val->value._string = (char *)data;
			//val->_string.start = 0;
			//if (data && data_length == 0)
			//	data_length = strlen(data);
			//val->_string.len = data_length;
			//fprintf(stderr, "conv_data: len = %d\n", len);
			/*GB.NewString(&val->_string.value, data, strlen(data));*/

			break;
	}
}


static void free_field_info()
{
	GB.StoreVariant(NULL, &_default);
	GB.FreeString(&_collation);
}

//-------------------------------------------------------------------------

BEGIN_METHOD_VOID(MySqlHelper_exit)

	free_field_info();

END_METHOD

BEGIN_METHOD(MySqlHelper_Open, GB_STRING host; GB_STRING port; GB_STRING name; GB_STRING user; GB_STRING password; GB_INTEGER timeout; GB_OBJECT options)

	MYSQL *conn;
	char *name;
	char *host;
	char *socket;
	unsigned int timeout;
	char *env;
	GB_COLLECTION options;
	#if HAVE_MYSQL_SSL_MODE_DISABLED
	unsigned int mode;
	#endif

	conn = mysql_init(NULL);

	// NULL is a possible database name
	name = GB.ToZeroString(ARG(name));

	//mysql_options(conn, MYSQL_READ_DEFAULT_GROUP,"Gambas");

	//fprintf(stderr, "mysql_real_connect: host = '%s'\n", desc->host);

	host = GB.ToZeroString(ARG(host));
	if (*host == '/')
	{
		socket = host;
		host = NULL;
	}
	else
		socket = NULL;
	
	#if LIBMYSQL_VERSION_ID < 80034
	char reconnect = TRUE;
	mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
	#endif
	
	timeout = VARG(timeout);
	mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	
	options = VARG(options);
	if (options)
	{
		_options_conn = conn;
		GB.Collection.Browse(options, add_option_value);
	}
	
	env = getenv("GB_DB_MYSQL_NOSSL");
	if (env && strcmp(env, "0"))
	{
	#if HAVE_MYSQL_SSL_MODE_DISABLED
		mode = SSL_MODE_DISABLED;
		mysql_options(conn, MYSQL_OPT_SSL_MODE, &mode);
	#else
		fprintf(stderr, "gb.db.mysql: warning: disabling SSL connection is not supported with your version of MySQL client library.\n");
	#endif
	}
	
	if (!mysql_real_connect(conn, host, GB.ToZeroString(ARG(user)), GB.ToZeroString(ARG(password)),
			name, atoi(GB.ToZeroString(ARG(port))), socket,
			CLIENT_MULTI_RESULTS | CLIENT_REMEMBER_OPTIONS /*client flag */)){
		mysql_close(conn);
		GB.Error("Cannot open database: &1", mysql_error(conn));
		return;
	}

	GB.ReturnPointer(conn);

END_METHOD

BEGIN_METHOD(MySqlHelper_Close, GB_POINTER database)

	mysql_close((MYSQL *)VARG(database));

END_METHOD

BEGIN_METHOD(MySqlHelper_Query, GB_POINTER database; GB_STRING query)

	MYSQL *conn = (MYSQL *)VARG(database);
	char *query = GB.ToZeroString(ARG(query));
	MYSQL_RES *res;
	
	if (do_query(conn, query, &res))
		return;

	GB.ReturnPointer(res);

END_METHOD

BEGIN_METHOD(MySqlHelper_FreeResult, GB_POINTER result)

	mysql_free_result((MYSQL_RES *)VARG(result));

END_METHOD

BEGIN_PROPERTY(MySqlHelper_LastError)

	GB.ReturnInteger(_last_error);

END_PROPERTY

BEGIN_METHOD(MySqlHelper_GetResultCount, GB_POINTER result)

	GB.ReturnInteger(mysql_num_rows((MYSQL_RES *)VARG(result)));

END_METHOD

BEGIN_METHOD(MySqlHelper_GetResultField, GB_POINTER result; GB_INTEGER field)

	MYSQL_RES *result = (MYSQL_RES *)VARG(result);
	int field = VARG(field);
	int nfield = mysql_num_fields(result);
	bool add_table;
	MYSQL_FIELD *f;
	char *name;
	int i;
	
	if (field >= nfield)
	{
		GB.ReturnVoidString();
		return;
	}
	
	f = mysql_fetch_fields(result);
	add_table = FALSE;
	
	for (i = 1; i < nfield; i++)
	{
		if (strcmp(f[i].table, f[0].table))
		{
			add_table = TRUE;
			break;
		}
	}
	
	f = mysql_fetch_field_direct(result, field);
	
	_type = conv_type(f);
	_length = _type == GB_T_STRING ? f->max_length : 0;
	
	if (add_table && f->table)
	{
		name = GB.NewZeroString(f->table);
		name = GB.AddChar(name, '.');
		name = GB.AddString(name, f->name, -1);
		GB.FreeStringLater(name);
		GB.ReturnString(name);
	}
	else
		GB.ReturnNewZeroString(f->name);

END_METHOD

BEGIN_METHOD(MySqlHelper_GetResultData, GB_POINTER result; GB_INTEGER pos; GB_BOOLEAN next; GB_INTEGER version)

	MYSQL_RES *res = (MYSQL_RES *)VARG(result);
	int pos = VARG(pos);
	bool next = VARG(next);
	int version = VARG(version);
	int nfield = mysql_num_fields(res);
	MYSQL_FIELD *field;
	MYSQL_ROW row;
	int i;
	char *data;
	GB_VARIANT value;
	GB_ARRAY buffer;

	if (!next || pos == 0)
		mysql_data_seek(res, pos);/* move to record */

	GB.Array.New(&buffer, GB_T_VARIANT, nfield);
	
	row = mysql_fetch_row(res);
	mysql_field_seek(res, 0);
	
	for (i = 0; i < nfield; i++)
	{
		field = mysql_fetch_field(res);
		data = row[i];

		value.type = GB_T_VARIANT;
		value.value.type = GB_T_NULL;

		if (data)
			conv_data(version, data, mysql_fetch_lengths(res)[i], &value.value, field);

		GB.StoreVariant(&value, GB.Array.Get(buffer, i));
	
		//fprintf(stderr, "query_fill: %d: (%d, %d) : %s : %d\n", i, field->type, field->length, data, buffer[i].type);
	}
	
	GB.ReturnObject(buffer);
	
END_METHOD

BEGIN_PROPERTY(MySqlHelper_Type)

	GB.ReturnInteger((int)_type);

END_PROPERTY

BEGIN_PROPERTY(MySqlHelper_Length)

	GB.ReturnInteger(_length);

END_PROPERTY

BEGIN_METHOD(MySqlHelper_GetResultBlob, GB_POINTER result; GB_INTEGER pos; GB_INTEGER field)

	MYSQL_RES *res = (MYSQL_RES *)VARG(result);
	int field = VARG(field);
	MYSQL_ROW row;

	mysql_data_seek(res, VARG(pos));/* move to record */
	row = mysql_fetch_row(res);
	GB.ReturnConstString(row[field], mysql_fetch_lengths(res)[field]);
	
END_METHOD

BEGIN_METHOD(MySqlHelper_QuoteString, GB_STRING value)

	GB.ReturnString(quote_string(STRING(value), LENGTH(value)));

END_METHOD

BEGIN_METHOD(MySqlHelper_GetFieldInfo, GB_POINTER result; GB_INTEGER index; GB_INTEGER version)

	MYSQL_RES *res = (MYSQL_RES *)VARG(result);
	int version = VARG(version);
	MYSQL_ROW row;
	MYSQL_FIELD f;
	GB_VARIANT def;
	char *val;
	
	free_field_info();
	
	mysql_data_seek(res, VARG(index));
	row = mysql_fetch_row(res);
	
	conv_string_type(row[1], &f);
	_type = conv_type(&f);
	_length = _type == GB_T_STRING ? f.max_length : 0;

	if ((_type == GB_T_INTEGER || _type == GB_T_LONG) && strstr(row[6], "auto_increment"))
		_type = DB_T_SERIAL;
	else
	{
		if (!*row[3] || row[3][0] != 'Y')
		{
			def.type = GB_T_VARIANT;
			def.value.type = GB_T_NULL;

			val = row[5];

			// (BM) seems there is a bug in mysql
			if (_type == GB_T_DATE && val && strlen(val) >= 5 && strncmp(val, "00000", 5) == 0)
				val = NULL;

			if (val && *val)
				conv_data(version, val, 0, &def.value, &f);
			
			GB.StoreVariant(&def, &_default);
		}
	}

	if (row[2] && *row[2])
		_collation = GB.NewZeroString(row[2]);

END_METHOD

BEGIN_PROPERTY(MySqlHelper_Default)

	GB.ReturnVariant(&_default);

END_PROPERTY

BEGIN_PROPERTY(MySqlHelper_Collation)

	GB.ReturnString(_collation);

END_PROPERTY

//-------------------------------------------------------------------------

GB_DESC MySqlHelperDesc[] =
{
	GB_DECLARE_STATIC("_MySqlHelper"),
	
	GB_STATIC_METHOD("_exit", NULL, MySqlHelper_exit, NULL),

	GB_STATIC_METHOD("Open", "p", MySqlHelper_Open, "(Host)s(Port)s(Name)s(User)s(Password)s(Timeout)i(Options)Collection;"),
	GB_STATIC_METHOD("Close", NULL, MySqlHelper_Close, "(Database)p"),
	GB_STATIC_METHOD("Query", "p", MySqlHelper_Query, "(Database)p(Query)s"),
	GB_STATIC_METHOD("FreeResult", NULL, MySqlHelper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetResultCount", "i", MySqlHelper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("GetResultField", "s", MySqlHelper_GetResultField, "(Result)p(Field)i"),
	GB_STATIC_METHOD("GetResultData", "Variant[]", MySqlHelper_GetResultData,"(Result)p(Index)l(Next)b(Version)i"),
	GB_STATIC_METHOD("QuoteString", "s", MySqlHelper_QuoteString, "(Value)s"),
	GB_STATIC_METHOD("GetResultBlob", "s", MySqlHelper_GetResultBlob,"(Result)p(Index)i(Field)i"),
	GB_STATIC_METHOD("GetFieldInfo", NULL, MySqlHelper_GetFieldInfo, "(Result)p(Index)i(Version)i"),
	
	GB_STATIC_PROPERTY_READ("LastError", "i", MySqlHelper_LastError),
	GB_STATIC_PROPERTY_READ("Type", "i", MySqlHelper_Type),
	GB_STATIC_PROPERTY_READ("Length", "i", MySqlHelper_Length),
	GB_STATIC_PROPERTY_READ("Default", "v", MySqlHelper_Default),
	GB_STATIC_PROPERTY_READ("Collation", "s", MySqlHelper_Collation),
	
	GB_END_DECLARE
};

//-------------------------------------------------------------------------

GB_DESC *GB_CLASSES [] EXPORT =
{
	MySqlHelperDesc,
	NULL
};

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}
