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
		SQLHANDLE conn;
		unsigned can_fetch_scroll : 1;
	}
ODBC_CONN;

//-------------------------------------------------------------------------

GB_INTERFACE GB EXPORT;

//-------------------------------------------------------------------------

static bool is_connection_string(const char *str)
{
	return strchr(str, '=') || strchr(str, ';');
}

static bool handle_error(int retcode, SQLHANDLE handle, SQLSMALLINT type)
{
	char *error = NULL;
	SQLINTEGER i;
	SQLINTEGER native;
	SQLTCHAR state[7];
	SQLTCHAR text[512];
	SQLSMALLINT len;

	if (SQL_SUCCEEDED(retcode))
		return FALSE;
	
	for (i = 0;; i++)
	{
		if (!SQL_SUCCEEDED(SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len)))
			break;
		
		if (error)
			error = GB.AddString(error, "; ", 2);
		error = GB.AddString(error, (char *)state, -1);
		error = GB.AddString(error, (char *)text, len);
	}

	GB.Error(error);
	GB.FreeString(&error);
	
	return TRUE;
}

//-------------------------------------------------------------------------

BEGIN_METHOD(OdbcHelper_Open, GB_STRING host; GB_STRING port; GB_STRING name; GB_STRING user; GB_STRING password; GB_INTEGER timeout; GB_OBJECT options)

	ODBC_CONN *conn;
	SQLHANDLE env_handle;
	SQLHANDLE conn_handle;
	SQLUSMALLINT can_fetch_scroll;
	SQLRETURN 	retcode;
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
	
	GB.Alloc(POINTER(&conn), sizeof(ODBC_CONN));
	conn->env = env_handle;
	conn->conn = conn_handle;
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

	SQLDisconnect(conn->conn);
	SQLFreeHandle(SQL_HANDLE_DBC, conn->conn);
	SQLFreeHandle(SQL_HANDLE_ENV, conn->env);
	GB.Free(POINTER(&conn));
	
END_METHOD

BEGIN_METHOD(OdbcHelper_CanFetchScroll, GB_POINTER database)

	GB.ReturnBoolean(((ODBC_CONN *)VARG(database))->can_fetch_scroll);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC OdbcHelperDesc[] =
{
	GB_DECLARE_STATIC("_OdbcHelper"),
	
	//GB_STATIC_METHOD("_exit", NULL, OdbcHelper_exit, NULL),
	
	GB_STATIC_METHOD("Open", "p", OdbcHelper_Open, "(Host)s(Port)s(Name)s(User)s(Password)s(Timeout)i(Options)Collection;"),
	GB_STATIC_METHOD("Close", NULL, OdbcHelper_Close, "(Database)p"),
	GB_STATIC_METHOD("CanFetchScroll", "b", OdbcHelper_CanFetchScroll, "(Database)p"),
	/*GB_STATIC_METHOD("Query", "p", OdbcHelper_Query, "(Database)p(Query)s"),
	GB_STATIC_METHOD("FreeResult", NULL, OdbcHelper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetResultCount", "i", OdbcHelper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("GetResultField", "s", OdbcHelper_GetResultField, "(Result)p(Field)i"),
	GB_STATIC_METHOD("GetResultData", "Variant[]", OdbcHelper_GetResultData,"(Result)p(Index)l(Next)b"),
	GB_STATIC_METHOD("QuoteString", "s", OdbcHelper_QuoteString, "(Value)s(AddE)b"),
	GB_STATIC_METHOD("GetResultBlob", "s", OdbcHelper_GetResultBlob,"(Result)p(Index)i(Field)i"),
	GB_STATIC_METHOD("QuoteBlob", "s", OdbcHelper_QuoteBlob, "(Value)s(AddE)b"),
	GB_STATIC_METHOD("GetFieldInfo", NULL, OdbcHelper_GetFieldInfo, "(Result)p(NoCollation)b"),
	GB_STATIC_METHOD("GetLastError", "i", OdbcHelper_GetLastError, NULL),
	
	GB_STATIC_PROPERTY_READ("Type", "i", OdbcHelper_Type),
	GB_STATIC_PROPERTY_READ("Length", "i", OdbcHelper_Length),
	GB_STATIC_PROPERTY_READ("Default", "v", OdbcHelper_Default),
	GB_STATIC_PROPERTY_READ("Collation", "s", OdbcHelper_Collation),*/
	
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
