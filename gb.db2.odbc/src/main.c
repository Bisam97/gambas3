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

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "main.h"

typedef struct
	{
		SQLHANDLE env;
		SQLHANDLE handle;
		SQLUSMALLINT max_column_name_len;
		unsigned can_fetch_scroll : 1;
	}
	ODBC_CONN;

typedef struct
	{
		SQLSMALLINT type;
		int len;
	}
	ODBC_FIELD;

typedef struct
	{
		ODBC_CONN *conn;
		SQLHSTMT handle;
		SQLLEN count;
		int n_columns;
		ODBC_FIELD *fields;
		unsigned can_fetch_scroll : 1;   //Does the Driver support SQLFetchScroll ?
		unsigned scrollable_cursor : 1;  //Is it possible to set a Scrollable cursor ?
	}
	ODBC_RESULT;

//-------------------------------------------------------------------------

GB_INTERFACE GB EXPORT;

static int _last_error = 0;
static GB_TYPE _type;
static int _length;

//-------------------------------------------------------------------------

static bool is_connection_string(const char *str)
{
	return strchr(str, '=') || strchr(str, ';');
}

static char *get_error(SQLHANDLE handle, SQLSMALLINT type)
{
	char *error = NULL;
	int i;
	SQLTCHAR state[7];
	SQLTCHAR text[512];
	SQLSMALLINT len;
	SQLINTEGER native;
	
	for (i = 0;; i++)
	{
		if (!SQL_SUCCEEDED(SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len)))
			break;
		
		if (error)
			error = GB.AddChar(error, ' ');
		error = GB.AddString(error, (char *)state, -1);
		error = GB.AddChar(error, ' ');
		error = GB.AddString(error, (char *)text, len);
	}

	return error;
}

static bool handle_error(SQLRETURN retcode, SQLHANDLE handle, SQLSMALLINT type)
{
	char *error;

	if (SQL_SUCCEEDED(retcode))
		return FALSE;
	
	_last_error = retcode;
	
	error = get_error(handle, type);
	GB.Error(error);
	GB.FreeString(&error);
	return TRUE;
}

static bool report_error(SQLRETURN retcode, SQLHANDLE handle, SQLSMALLINT type, const char *func, const char *func2)
{
	char *error;

	if (SQL_SUCCEEDED(retcode))
		return FALSE;
	
	error = get_error(handle, type);
	fprintf(stderr, "gb.db.odbc: warning: %s: ", func);
	if (func2) fprintf(stderr, "%s: ", func2);
	fprintf(stderr, "%s\n", error);
	GB.FreeString(&error);
	return TRUE;
}

// Internal function to convert a database type into a Gambas type

static GB_TYPE conv_type(int type)
{
	switch (type)
	{
		case SQL_TINYINT:
			return GB_T_BOOLEAN;

		case SQL_DECIMAL:
		case SQL_INTEGER:
		case SQL_SMALLINT:
			return GB_T_INTEGER;

		case SQL_BIGINT:
			// New datatype bigint 64 bits
			return GB_T_LONG;

		case SQL_NUMERIC:
		case SQL_FLOAT:
		case SQL_REAL:
		case SQL_DOUBLE:
			return GB_T_FLOAT;

		case SQL_DATETIME:
		case SQL_TYPE_DATE:
		case SQL_TYPE_TIME:
		case SQL_TYPE_TIMESTAMP:
			return GB_T_DATE;

		case SQL_LONGVARCHAR:
		case SQL_VARBINARY:
		case SQL_LONGVARBINARY:
			// Data type for BLOB
			return DB_T_BLOB;

		case SQL_CHAR:
		default:
			return GB_T_STRING;
	}
}

static bool conv_date(char *data, int len, int *pdate, int *ptime)
{
	GB_DATE_SERIAL date = { 0 };
	double sec;
	int n;
	GB_VALUE conv;
	bool ret = FALSE;
	
	conv._date.value.date = conv._date.value.time = 0;
	
	if (len > 0)
	{
		n = sscanf(data, "%4d-%2d-%2d %2d:%2d:%lf", &date.year, &date.month, &date.day, &date.hour, &date.min, &sec);
		if (n >= 3)
		{
			bool bc = (len > 3) && (strcmp(&data[len - 2], "BC") == 0);
		
			if (n == 6)
			{
				date.sec = (short)sec;
				date.msec = (short)((sec - date.sec) * 1000 + 0.5);
			}
			if (bc)
				date.year = (-date.year);
			
			GB.MakeDate(&date, (GB_DATE *)&conv, TRUE);
		}
		else if (GB.DateFromString(data, len, &conv, TRUE))
		{
			ret = TRUE;
		}
	}
	
	*pdate = conv._date.value.date;
	*ptime = conv._date.value.time;
	return ret;
}

// Internal function to convert a database value into a Gambas variant value

static void get_data(SQLHSTMT handle, int column, ODBC_FIELD *field, GB_VARIANT_VALUE *val)
{
	char *data;
	SQLLEN len = 0;

	//report_error(SQLGetData(result->handle, i + 1, SQL_C_CHAR, result->data, field->len, &len_read), result->handle, SQL_HANDLE_STMT, "SQLGetData", NULL);
			
	switch (field->type)
	{
		//case SQL_NUMERIC:
		case SQL_INTEGER:
		case SQL_SMALLINT:
			
			val->type = GB_T_INTEGER;
			SQLGetData(handle, column, SQL_C_SLONG, &val->value._integer, 0, &len);
			break;

		case SQL_DECIMAL:
		case SQL_NUMERIC:
		case SQL_FLOAT:
		case SQL_REAL:
		case SQL_DOUBLE:
			
			val->type = GB_T_FLOAT;
			SQLGetData(handle, column, SQL_C_DOUBLE, &val->value._float, 0, &len);
			break;

		case SQL_TINYINT:
			
			val->type = GB_T_BOOLEAN;
			SQLGetData(handle, column, SQL_C_SLONG, &val->value._boolean, 0, &len);
			if (val->value._boolean)
				val->value._boolean = -1;
			break;

		case SQL_BIGINT: // Data type bigint 64 bits
			
			val->type = GB_T_LONG;
			SQLGetData(handle, column, SQL_C_SBIGINT, &val->value._long, 0, &len);
			break;

		case SQL_LONGVARCHAR:
		case SQL_VARBINARY:
		case SQL_LONGVARBINARY: // Data type BLOB
			
			// The BLOB are read by the blob_read() driver function
			// You must set NULL there.
			val->type = GB_T_NULL;
			break;

		case SQL_TYPE_DATE:
		case SQL_TYPE_TIME:
		case SQL_TYPE_TIMESTAMP:
		case SQL_DATETIME: // Data type for Time
		{
			int date, time;
			
			data = alloca(field->len + 1);
			SQLGetData(handle, column, SQL_C_CHAR, data, field->len + 1, &len);
			
			if (conv_date(data, len, &date, &time))
				fprintf(stderr, "gb.db.odbc: warning: unable to convert date: %.*s\n", (int)len, data);
			
			val->type = GB_T_DATE;
			val->value._date.date = date;
			val->value._date.time = time;
			break;
		}
		
		case SQL_CHAR:
		default:

			data = GB.TempString(NULL, field->len);
			SQLGetData(handle, column, SQL_C_CHAR, data, field->len + 1, &len);
			
			val->type = GB_T_CSTRING;
			val->value._string = data;
			
			break;
	}
	
	if (len <= 0)
		val->type = GB_T_NULL;
}


/* zxMarce: This is one way -hope there's an easier one- to retrieve a rowset
*  count for SELECT statements. Four steps (must have an scrollable cursor!):
*    1- Remember the current row.
*    2- Seek down to the last row in the rowset
*    3- Get the last row's index (recno)
*    4- Seek back to wherever we were at in step 1
*  20161110 zxMarce: Ok, it did not work that OK for Firebird; it looks like
*  the FB driver returns one-less than the record count (record count seems to
*  be zero-based), so we will instead do as follows, if we have a scrollable
*  recordset:
*    1- Remember the current row.
*    2- Seek up to the first row in the rowset
*    3- Get the first row's index (firstRecNo)
*    4- Seek down to the last row in the rowset
*    5- Get the last row's index (lastRecNo)
*    6- Seek back to wherever we were at in step 1
*    7- Return (lastRecNo - firstRecNo + 1).
*/

static int get_record_count(SQLHANDLE handle, bool scrollable_cursor)
{
	SQLRETURN ret;                // ODBC call return values
	int formerRecIdx = -1;        // Where we were when this all started.
	SQLINTEGER myRecCnt = -1;     // Default for when there's no cursor.
	SQLINTEGER firstRecNo = 0;    // 20161111 holder for 1st recno.
	SQLINTEGER lastRecNo = 0;     // 20161111 holder for last recno.
	const char *msg = "Unable to get record count";

	//Make sure the statement has a cursor
	if (!(handle && scrollable_cursor))
	{
		fprintf(stderr, "gb.db.odbc: warning: cannot count records\n");
		return -1;
	}

	//Tell ODBC we won't be actually reading data (speeds process up).
	//SQL_ATTR_RETRIEVE_DATA = [SQL_RD_ON] | SQL_RD_OFF
	report_error(SQLSetStmtAttr(handle, SQL_ATTR_RETRIEVE_DATA, (SQLPOINTER) SQL_RD_OFF, 0), handle, SQL_HANDLE_STMT, msg, "SQLSetStmtAttr: do not retrieve data");

	//Fetch current row's index so we can return to it when done.
	SQLGetStmtAttr(handle, SQL_ATTR_ROW_NUMBER, &formerRecIdx, 0, 0); //, handle, SQL_HANDLE_STMT, msg, "SQLGetStmtAttr: get current record");

	//Make sure the statement has a cursor
	if (formerRecIdx < 0)
	{
		//fprintf(stderr, "gb.db.odbc: warning: %s: current record index is %d, returning -1 as count\n", msg, formerRecIdx);
		goto __RETURN_COUNT;
	}

	//Try to get (back?) to the first record, abort if not possible.
	if (report_error(SQLFetchScroll(handle, SQL_FETCH_FIRST, (SQLINTEGER) 0), handle, SQL_HANDLE_STMT, msg, "SQLFetchScroll: first record"))
		goto __RETURN_COUNT;
	
	//Fetch the first record's index
	if (report_error(SQLGetStmtAttr(handle, SQL_ATTR_ROW_NUMBER, &firstRecNo, 0, 0), handle, SQL_HANDLE_STMT, msg, "SQLGetStmtAttr: first record"))
		goto __RETURN_COUNT;

	//Advance the cursor to the last record.
	if (report_error(SQLFetchScroll(handle, SQL_FETCH_LAST, (SQLINTEGER) 0), handle, SQL_HANDLE_STMT, msg, "SQLFetchScroll: last record"))
		goto __RETURN_COUNT;

	//Fetch the last record's index
	if (report_error(SQLGetStmtAttr(handle, SQL_ATTR_ROW_NUMBER, &lastRecNo, 0, 0), handle, SQL_HANDLE_STMT, msg, "SQLGetStmtAttr: last record"))
		goto __RETURN_COUNT;

	//Return cursor to original row.
	ret = SQLFetchScroll(handle, SQL_FETCH_ABSOLUTE, (SQLINTEGER) formerRecIdx);
	//Since we have set the "do not read data" statement attribute, this call (may) return
	//code 100 (SQL_NO_DATA) but that's OK for our purposes of just counting rows.
	if (!SQL_SUCCEEDED(ret) && (ret != SQL_NO_DATA))
	{
		report_error(ret, handle, SQL_HANDLE_STMT, msg, "SQLFetchScroll: set current record back");
		goto __RETURN_COUNT;
	}

	myRecCnt = (lastRecNo - firstRecNo + 1);
	//DB.Debug("gb.db.odbc", "GetRecordCount: Record count=%d", (int) myRecCnt);

__RETURN_COUNT:
	
	//Tell ODBC we will be reading data now.
	//SQL_ATTR_RETRIEVE_DATA = [SQL_RD_ON] | SQL_RD_OFF
	report_error(SQLSetStmtAttr(handle, SQL_ATTR_RETRIEVE_DATA, (SQLPOINTER) SQL_RD_ON, 0), handle, SQL_HANDLE_STMT, msg, "SQLSetStmtAttr: retrieve data");
	return ((int) myRecCnt);

}


static SQLHSTMT _result_handle;
static bool _result_scrollable_cursor;

static bool start_query(ODBC_CONN *conn)
{
	SQLRETURN ret;
	// Allocate the space for the result structure
	if (handle_error(SQLAllocHandle(SQL_HANDLE_STMT, conn->handle, &_result_handle), conn->handle, SQL_HANDLE_DBC))
		return TRUE;

	ret = SQLSetStmtAttr(_result_handle, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE, 0);
	_result_scrollable_cursor = ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	
	return FALSE;
}

static bool finish_query(ODBC_CONN *conn, SQLRETURN ret, ODBC_RESULT **presult)
{
	ODBC_RESULT *result;
	
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
	{
		//DB.Debug("gb.db.odbc", "do_query: SQLExecDirect() returned code %d", (int)retcode);
		handle_error(ret, _result_handle, SQL_HANDLE_STMT);
		SQLFreeHandle(SQL_HANDLE_STMT, _result_handle);
		return TRUE;
	}

	if (presult)
	{
		GB.AllocZero(POINTER(&result), sizeof(ODBC_RESULT));
		result->conn = conn;
		result->handle = _result_handle;
		result->scrollable_cursor = _result_scrollable_cursor;
		result->can_fetch_scroll = conn->can_fetch_scroll;

		if (ret == SQL_NO_DATA)
			result->count = 0;
		else
			result->count = get_record_count(_result_handle, _result_scrollable_cursor);
		
		*presult = result;
		//DB.Debug("gb.db.odbc", "do_query: create handle %p", odbcres->odbcStatHandle);
	}
	else
	{
		SQLFreeHandle(SQL_HANDLE_STMT, _result_handle);
	}

	return FALSE;
}

// Internal function to implement the query execution

static bool do_query(ODBC_CONN *conn, const char *query, ODBC_RESULT **presult)
{
	SQLRETURN ret;
	
	if (start_query(conn))
		return TRUE;
	
	ret = SQLExecDirect(_result_handle, (SQLCHAR *)query, SQL_NTS);
	
	return finish_query(conn, ret, presult);
}

static void init_result(ODBC_RESULT *result)
{
	SQLRETURN ret;
	SQLSMALLINT n_columns = 0;
	SQLSMALLINT name_len;
	SQLSMALLINT type;
	SQLULEN precision;
	SQLSMALLINT scale;
	int i;
	
	ret = SQLNumResultCols(result->handle, &n_columns);

	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO))
		report_error(ret, result->handle, SQL_HANDLE_STMT, "Unable to get columns count", "SQLNumResultCols");
	else
	{
		result->n_columns = n_columns;
		GB.Alloc(POINTER(&result->fields), sizeof(ODBC_FIELD) * n_columns);
		
		for (i = 0; i < n_columns; i++)
		{
			SQLDescribeCol(result->handle, i + 1, NULL, 0, &name_len, &type, &precision, &scale, NULL);
	
			if (type == SQL_UNKNOWN_TYPE)
				//DB.Debug("gb.db.odbc", "field '%s' has datatype: %d, assuming SQLCHAR instead", field->name, type);
				type = SQL_CHAR;
	
			result->fields[i].type = type;
			result->fields[i].len = precision;
		}
	}
}

static void free_result(ODBC_RESULT *result)
{
	SQLFreeHandle(SQL_HANDLE_STMT, result->handle);
	GB.Free(POINTER(&result->fields));
	GB.Free(POINTER(&result));
}

static void return_info(ODBC_CONN *conn, SQLUSMALLINT info)
{
	char *data;
	SQLSMALLINT len;

	if (handle_error(SQLGetInfo(conn->handle, info, NULL, 0, &len), conn->handle, SQL_HANDLE_DBC))
		return;
	
	data = alloca(len + 1);
	if (handle_error(SQLGetInfo(conn->handle, info, data, len + 1, &len), conn->handle, SQL_HANDLE_DBC))
		return;
	
	GB.ReturnNewZeroString(data);
}

//-------------------------------------------------------------------------

BEGIN_METHOD(OdbcHelper_Open, GB_STRING host; GB_STRING port; GB_STRING name; GB_STRING user; GB_STRING password; GB_INTEGER timeout; GB_OBJECT options)

	ODBC_CONN *conn;
	SQLHANDLE env_handle;
	SQLHANDLE conn_handle;
	SQLUSMALLINT can_fetch_scroll;
	SQLRETURN retcode;
	SQLUSMALLINT max_column_name_len;
	SQLSMALLINT len;
	char *host = GB.ToZeroString(ARG(host));

	//GB.Alloc(&conn, sizeof(ODBC_CONN));
	
	// Allocate the Environment handle
	
	if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_handle)))
	{
		GB.Error("Unable to allocate ODBC environment handle");
		return;
	}

	// Set the Environment attributes
	if (!SQL_SUCCEEDED(SQLSetEnvAttr(env_handle, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0)))
	{
		SQLFreeHandle(SQL_HANDLE_ENV, env_handle);
		GB.Error("Unable to set ODBC environment attributes");
		return;
	}

	// Allocate the Database Connection handle
	if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, env_handle, &conn_handle)))
	{
		SQLFreeHandle(SQL_HANDLE_ENV, env_handle);
		GB.Error("Unable to allocate ODBC database handle");
		return;
	}

	/* 20170818 zxMarce: The following timeout was incorrectly set. The right timeout is the CONNECT
	 * timeout. The LOGIN timeout is actually used once the connection is established. Also took the
	 * opportunity to make sure ODBC uses cursors either from the Driver or the Driver Manager, thus
	 * almost ensuring cursors are available.
	*/
	
	if (handle_error(SQLSetConnectAttr(conn_handle, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)(intptr_t)VARG(timeout), 0), conn_handle, SQL_HANDLE_DBC))
		goto __ERROR;
	
	/* 
	 * 20210404: Watch out, by using SQL_CUR_USE_IF_NEEDED, if the driver does not provide cursors, 
	 * unixODBC seems to need bound columns. This has the nasty side effect to raise SQL errors. Do 
	 * not yet know how to workaround this.
	 * The SQL_CUR_USE_IF_NEEDED constant tells ODBC to provide its own cursors if the driver doesn't.
	 */
	//SQLSetConnectAttr(odbc->odbcHandle, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER)SQL_CUR_USE_IF_NEEDED, 0);
	if (handle_error(SQLSetConnectAttr(conn_handle, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER)SQL_CUR_USE_DRIVER, 0), conn_handle, SQL_HANDLE_DBC))
		goto __ERROR;
	
	if (is_connection_string(host))
	{
		retcode = SQLDriverConnect(conn_handle, 0, (SQLCHAR *)host, SQL_NTS, 0, 0, 0, SQL_DRIVER_NOPROMPT);
		/* The last three zero params in the call above can be used to retrieve the actual connstring used,
		should unixODBC "complete" the passed ConnString with data from a matching defined DSN. Not
		doing it here, but maybe useful to fill in the other Gambas Connection object properties (user,
		pass, etc) after parsing it. Also note that the ConnString MAY refer to a DSN, and include
		user/pass, if desired.
		Example - ODBC-ConnString for FreeTDS, all one line (must assign this to the Connection.Host
		property in Gambas code and then call Connection.Open):
			"Driver=FreeTDS;
			TDS_Version=<useNormally'7.2'>;
			Server=<serverNameOrIP>;
			Port=<serverTcpPort>;
			Database=<defaultDatabase>"
			UId=<userName>;
			Pwd=<password>;
		*/

	}
	else
	{
		/* Connect to Database (desc->host is an ODBC Data Source Name) */
		retcode = SQLConnect(conn_handle, (SQLCHAR *)host, SQL_NTS, (SQLCHAR *)GB.ToZeroString(ARG(user)), SQL_NTS, (SQLCHAR *)GB.ToZeroString(ARG(password)), SQL_NTS);
	}

	if (handle_error(retcode, conn_handle, SQL_HANDLE_DBC))
		goto __ERROR;

	if (handle_error(SQLSetConnectAttr(conn_handle, SQL_ATTR_AUTOCOMMIT, (void *) SQL_AUTOCOMMIT_ON, SQL_NTS), conn_handle, SQL_HANDLE_DBC))
		goto __ERROR;

	/* desc->name is a pointer intended to point to the database name ('catalog', in
	 * MSSQL parlance) to which we are connected. But that is NOT the actual database
	 * name when we use a connstring. When we use a connstring, the '.Database' connection
	 * property would then have the whole connection string, so we tweak it here to make it
	 * point to the actual database (catalog?) name.
	*/
	
	// TODO: do it in another API
	//GetConnectedDBName(desc, odbc);

	// TODO: Use SQLGetInfo() to retrieve the DBMS version string
	//db->version = 3;
	//db->full_version = GB.NewZeroString("3");

	if (handle_error(SQLGetFunctions(conn_handle, SQL_API_SQLFETCHSCROLL, &can_fetch_scroll), conn_handle, SQL_HANDLE_DBC))
		goto __ERROR;
	
	if (handle_error(SQLGetInfo(conn_handle, SQL_MAX_COLUMN_NAME_LEN, &max_column_name_len, 0, &len), conn_handle, SQL_HANDLE_DBC))
		goto __ERROR;
	
	GB.Alloc(POINTER(&conn), sizeof(ODBC_CONN));
	conn->env = env_handle;
	conn->handle = conn_handle;
	conn->max_column_name_len = max_column_name_len;
	conn->can_fetch_scroll = can_fetch_scroll;
	
	GB.ReturnPointer(conn);
	return;
	
__ERROR:

	SQLFreeHandle(SQL_HANDLE_DBC, conn_handle);
	SQLFreeHandle(SQL_HANDLE_ENV, env_handle);
	return;

END_METHOD

BEGIN_METHOD(OdbcHelper_Close, GB_POINTER database)

	ODBC_CONN *conn = (ODBC_CONN *)VARG(database);

	SQLDisconnect(conn->handle);
	SQLFreeHandle(SQL_HANDLE_DBC, conn->handle);
	SQLFreeHandle(SQL_HANDLE_ENV, conn->env);
	GB.Free(POINTER(&conn));
	
END_METHOD

BEGIN_METHOD(OdbcHelper_CanFetchScroll, GB_POINTER database)

	GB.ReturnBoolean(((ODBC_CONN *)VARG(database))->can_fetch_scroll);

END_METHOD

BEGIN_METHOD(OdbcHelper_GetQuoteCharacter, GB_POINTER database)

	return_info((ODBC_CONN *)VARG(database), SQL_IDENTIFIER_QUOTE_CHAR);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_GetDatabaseType, GB_POINTER database)

	return_info((ODBC_CONN *)VARG(database), SQL_DBMS_NAME);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_GetDatabaseName, GB_POINTER database)

	return_info((ODBC_CONN *)VARG(database), SQL_DATABASE_NAME);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_GetDatabaseUser, GB_POINTER database)

	return_info((ODBC_CONN *)VARG(database), SQL_USER_NAME);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_GetVersion, GB_POINTER database)

	return_info((ODBC_CONN *)VARG(database), SQL_DBMS_VER);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_Query, GB_POINTER database; GB_STRING query)

	ODBC_CONN *conn = (ODBC_CONN *)VARG(database);
	char *query = GB.ToZeroString(ARG(query));
	ODBC_RESULT *result;

	if (do_query(conn, query, &result))
		return;
	
	init_result(result);
	GB.ReturnPointer(result);

END_METHOD

BEGIN_METHOD(OdbcHelper_FreeResult, GB_POINTER result)

	free_result((ODBC_RESULT *)VARG(result));

END_METHOD

BEGIN_METHOD(OdbcHelper_GetResultCount, GB_POINTER result)

	GB.ReturnInteger(((ODBC_RESULT *)VARG(result))->count);

END_METHOD

BEGIN_METHOD(OdbcHelper_GetResultField, GB_POINTER result; GB_INTEGER field)

	ODBC_RESULT *result = (ODBC_RESULT *)VARG(result);
	int field = VARG(field);
	SQLUSMALLINT max_len;
	char *name;
	SQLSMALLINT name_len = -1;
	SQLULEN precision;
	SQLSMALLINT scale;
	SQLSMALLINT type;
	
	if (field >= result->n_columns)
	{
		GB.ReturnVoidString();
		return;
	}
	
	max_len = result->conn->max_column_name_len;
	
	name = alloca(max_len);
	
	SQLDescribeCol(result->handle, field + 1, (SQLCHAR *)name, max_len, &name_len, &type, &precision, &scale, NULL);
	
	if (name_len <= 0)
		sprintf(name, "#%d", field + 1);
	
	_type = conv_type(result->fields[field].type);
	if (_type == GB_T_STRING)
		_length = result->fields[field].len;
	else
		_length = 0;
	
	GB.ReturnNewZeroString(name);
	
END_METHOD

BEGIN_PROPERTY(OdbcHelper_Type)

	GB.ReturnInteger((int)_type);

END_PROPERTY

BEGIN_PROPERTY(OdbcHelper_Length)

	GB.ReturnInteger(_length);

END_PROPERTY

BEGIN_METHOD(OdbcHelper_GetTables, GB_POINTER database)

	ODBC_CONN *conn = (ODBC_CONN *)VARG(database);
	ODBC_RESULT *result;
	
	if (start_query(conn))
		return;
	
	if (finish_query(conn, SQLTables(_result_handle, NULL, 0, NULL, 0, NULL, 0, NULL, 0), &result))
		return;
	
	init_result(result);
	GB.ReturnPointer(result);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_GetResultData, GB_POINTER result; GB_INTEGER pos; GB_BOOLEAN next)

	ODBC_RESULT *result = (ODBC_RESULT *)VARG(result);
	int pos = VARG(pos);
	bool next = VARG(next);
	int i;
	ODBC_FIELD *field;
	SQLRETURN ret;
	GB_VARIANT value;
	GB_ARRAY buffer;
	
	if (result->can_fetch_scroll)
	{
		if (result->scrollable_cursor && !next)
			ret = SQLFetchScroll(result->handle, SQL_FETCH_ABSOLUTE, pos + 1);
		else
			ret = SQLFetchScroll(result->handle, SQL_FETCH_NEXT, pos + 1);
	}
	else
	{
		/**
		 * 20210409 zxMarce: The next IF makes sure the query is not
		 * forced to fetch-back, as the first fetch is issued with:
		 *   (next == false) && (pos == 0)
		 * Subsequent valid (forward) fetches are issued with:
		 *   (next == true) && (pos != 0)
		 * Any invalid fetch is detected by comparing against:
		 *   (!next) && (pos != 0)
		 * which will trigger a descriptive error message.
		 */
		if ((!next) && (pos != 0))
		{
			GB.Error("Forward-only result cannot fetch backwards");
			return;
		}
		
		ret = SQLFetch(result->handle);
	}

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA_FOUND && ret != SQL_NO_DATA)
	{
		//DB.Debug("gb.db.odbc","SQLFetchScroll()/SQLFetch() returned code %d, cannot fetch a row.", (int)retcode2);
		GB.Error("Unable to fetch row");
		return;
	}

	if (ret == SQL_NO_DATA_FOUND || ret == SQL_NO_DATA)
	{
		GB.ReturnNull();
		return;
	}

	GB.Array.New(&buffer, GB_T_VARIANT, result->n_columns);

	for (i = 0; i < result->n_columns; i++)
	{
		field = &result->fields[i];

		value.type = GB_T_VARIANT;
		value.value.type = GB_T_NULL;

		if (field->type != SQL_LONGVARCHAR && field->type != SQL_VARBINARY && field->type != SQL_LONGVARBINARY)
		{
			get_data(result->handle, i + 1, field, &value.value);
			GB.StoreVariant(&value, GB.Array.Get(buffer, i));
		}
	}

	GB.ReturnObject(buffer);

END_METHOD

BEGIN_METHOD(OdbcHelper_GetResultBlob, GB_POINTER result; GB_INTEGER pos; GB_INTEGER field)

	ODBC_RESULT *result = (ODBC_RESULT *)VARG(result);
	int column = VARG(field) + 1;
	SQLLEN len_read;
	SQLCHAR buffer[1024];
	SQLRETURN ret;
	char *data = NULL;

	for(;;)
	{
		len_read = 0;
		ret = SQLGetData(result->handle, column, SQL_C_BINARY, buffer, sizeof(buffer), &len_read);
		if (ret == SQL_ERROR || ret == SQL_NO_DATA || len_read <= 0)
			break;
	
		if (len_read > sizeof(buffer) || len_read == SQL_NO_TOTAL)
			len_read = sizeof(buffer);
		
		data = GB.AddString(data, (char *)buffer, len_read);
	} 
	
	if (ret != SQL_NO_DATA)
		handle_error(ret, result->handle, SQL_HANDLE_STMT);
	
	GB.ReturnString(data);
	
END_METHOD

BEGIN_METHOD_VOID(OdbcHelper_GetLastError)

	GB.ReturnInteger(_last_error);

END_METHOD

BEGIN_METHOD(OdbcHelper_GetFieldInfo, GB_POINTER database; GB_STRING table; GB_STRING field)

	ODBC_CONN *conn = (ODBC_CONN *)VARG(database);
	ODBC_RESULT *result;
	char *field = MISSING(field) ? NULL : STRING(field);
	int field_len = MISSING(field) ? 0 : LENGTH(field);
	
	if (start_query(conn))
		return;
	
	if (finish_query(conn, SQLColumns(_result_handle, NULL, 0, NULL, 0, (SQLCHAR *)STRING(table), LENGTH(table), (SQLCHAR *)field, field_len), &result))
		return;
	
	init_result(result);
	GB.ReturnPointer(result);
	
END_METHOD

BEGIN_METHOD(OdbcHelper_ConvType, GB_INTEGER odbc_type)

	GB.ReturnInteger(conv_type(VARG(odbc_type)));

END_METHOD

BEGIN_METHOD(OdbcHelper_ConvDate, GB_STRING str)

	GB_DATE val;
	int date, time;
	
	if (conv_date(STRING(str), LENGTH(str), &date, &time))
		GB.ReturnNull();
	else
	{
		val.value.date = date;
		val.value.time = time;
		GB.ReturnDate(&val);
	}
		
END_METHOD

BEGIN_METHOD(OdbcHelper_GetPrimaryKey, GB_POINTER database; GB_STRING table)

	ODBC_CONN *conn = (ODBC_CONN *)VARG(database);
	ODBC_RESULT *result;
	
	if (start_query(conn))
		return;
	
	if (finish_query(conn, SQLPrimaryKeys(_result_handle, NULL, 0, NULL, 0, (SQLCHAR *)STRING(table), LENGTH(table)), &result))
		return;
	
	init_result(result);
	GB.ReturnPointer(result);

END_METHOD

BEGIN_METHOD(OdbcHelper_GetIndexInfo, GB_POINTER database; GB_STRING table)

	ODBC_CONN *conn = (ODBC_CONN *)VARG(database);
	ODBC_RESULT *result;
	
	if (start_query(conn))
		return;
	
	if (finish_query(conn, SQLStatistics(_result_handle, NULL, 0, NULL, 0, (SQLCHAR *)STRING(table), LENGTH(table), SQL_INDEX_ALL, SQL_QUICK), &result))
		return;
	
	init_result(result);
	GB.ReturnPointer(result);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC OdbcHelperDesc[] =
{
	GB_DECLARE_STATIC("_OdbcHelper"),
	
	//GB_STATIC_METHOD("_exit", NULL, OdbcHelper_exit, NULL),
	
	GB_STATIC_METHOD("Open", "p", OdbcHelper_Open, "(Host)s(Port)s(Name)s(User)s(Password)s(Timeout)i(Options)Collection;"),
	GB_STATIC_METHOD("Close", NULL, OdbcHelper_Close, "(Database)p"),
	GB_STATIC_METHOD("CanFetchScroll", "b", OdbcHelper_CanFetchScroll, "(Database)p"),
	GB_STATIC_METHOD("GetQuoteCharacter", "s",OdbcHelper_GetQuoteCharacter, "(Database)p"),
	GB_STATIC_METHOD("GetDatabaseType", "s", OdbcHelper_GetDatabaseType, "(Database)p"),
	GB_STATIC_METHOD("GetDatabaseName", "s", OdbcHelper_GetDatabaseName, "(Database)p"),
	GB_STATIC_METHOD("GetDatabaseUser", "s", OdbcHelper_GetDatabaseUser, "(Database)p"),
	GB_STATIC_METHOD("GetVersion", "s", OdbcHelper_GetVersion, "(Database)p"),
	GB_STATIC_METHOD("Query", "p", OdbcHelper_Query, "(Database)p(Query)s"),
	GB_STATIC_METHOD("FreeResult", NULL, OdbcHelper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetResultCount", "i", OdbcHelper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("GetResultField", "s", OdbcHelper_GetResultField, "(Result)p(Field)i"),
	GB_STATIC_METHOD("GetResultData", "Variant[]", OdbcHelper_GetResultData,"(Result)p(Index)l(Next)b"),
	GB_STATIC_METHOD("GetResultBlob", "s", OdbcHelper_GetResultBlob,"(Result)p(Index)i(Field)i"),
	GB_STATIC_METHOD("GetLastError", "i", OdbcHelper_GetLastError, NULL),
	GB_STATIC_METHOD("GetTables", "p", OdbcHelper_GetTables, "(Database)p"),
	GB_STATIC_METHOD("GetFieldInfo", "p", OdbcHelper_GetFieldInfo, "(Database)p(Table)s[(Field)s]"),
	GB_STATIC_METHOD("ConvType", "i", OdbcHelper_ConvType, "(OdbcType)i"),
	GB_STATIC_METHOD("ConvDate", "d", OdbcHelper_ConvDate, "(Value)s"),
	GB_STATIC_METHOD("GetPrimaryKey", "p", OdbcHelper_GetPrimaryKey, "(Database)p(Table)s"),
	GB_STATIC_METHOD("GetIndexInfo", "p", OdbcHelper_GetIndexInfo, "(Database)p(Table)s"),
	
	GB_STATIC_PROPERTY_READ("Type", "i", OdbcHelper_Type),
	GB_STATIC_PROPERTY_READ("Length", "i", OdbcHelper_Length),
	
	GB_END_DECLARE
};

//-------------------------------------------------------------------------

GB_DESC *GB_CLASSES [] EXPORT =
{
	OdbcHelperDesc,
	NULL
};


int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}
