/***************************************************************************

	main.c

	(c) 2004-2007 Andrea Bortolan <andrea_bortolan@yahoo.it>
	(c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>
	(c) 2015-2021 zxMarce <d4t4full@gmail.com>

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

//#define ODBC_TYPE
//#define ODBC_DEBUG
//#define DEBUG_ODBC

//#ifndef ODBC_DEBUG_HEADER
//#define ODBC_DEBUG_HEADER
//#endif

//#define ODBC_DEBUG_MEM

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifdef HAVE_SYS_TYPES_H
#undef HAVE_SYS_TYPES_H
#endif

#ifdef HAVE_UNISTD_H
#undef HAVE_UNISTD_H
#endif

#include "gb.db.proto.h"
#include "main.h"

// define unixODBC ONLY constants if they are not available
#ifndef SQLTables_TABLE_NAME
#define SQLTables_TABLE_NAME 3
#define SQLTables_TABLE_TYPE 4
#define SQLTables_REMARKS 5
#define SQLColumns_COLUMN_NAME 4
#define SQLColumns_COLUMN_SIZE 7
#define SQLColumns_SQL_DATA_TYPE 14
#endif

GB_INTERFACE GB EXPORT;
DB_INTERFACE DB EXPORT;

static char _buffer[32];
//static char _nullbuffer[10];
static DB_DRIVER _driver;

typedef struct
	{
		//SQLHENV odbcEnvHandle;
		//SQLHDBC odbcHandle;
		SQLHANDLE odbcEnvHandle;        	//ODBC environment handle
		SQLHANDLE odbcHandle;           	//ODBC connection handle
		SQLUSMALLINT drvrCanFetchScroll;	//Flag
		char *dsn_name;                 	//DSN name
		char *user_name;                	//Logged-in user name
	}
ODBC_CONN;


typedef struct
	{
		char *name;
		int id;
		SQLSMALLINT type;
		char *data;
		int len;
	}
ODBC_FIELD;


typedef struct
	{
		SQLHSTMT odbcStatHandle;
		SQLUSMALLINT Function_exist;	//Does the Driver support SQLFetchScroll ?
		SQLUSMALLINT Cursor_Scrollable;  //Is it possible to set a Scrollable cursor ?
		ODBC_FIELD *fields;
		SQLLEN count;
	}
ODBC_RESULT;


typedef struct
	{
		char *tablename;
		struct ODBC_TABLES *next;
	}
ODBC_TABLES;

/*
* zxMarce: Routine to print to console the errors from the ODBC subsystem.
* Adapted to obey Gambas' DB.IsDebug().
* Mostly from http://www.easysoft.com/developer/interfaces/odbc/diagnostics_error_status_codes.html
*/
static void reportODBCError(const char *fn, SQLHANDLE handle, SQLSMALLINT type)
{
	SQLINTEGER      i = 0;
	SQLINTEGER      native;
	SQLTCHAR        state[7];
	SQLTCHAR        text[256];
	SQLSMALLINT     len;
	SQLRETURN       ret;

	if (DB.IsDebug())
	{
		DB.Debug("gb.db.odbc","ERROR: %s", fn);
		do
		{
			ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);

			if (SQL_SUCCEEDED(ret))
				DB.Debug("gb.db.odbc", "%d:%s:%d:%s", (int)i, (char *)state, (int)native, (char *)text);
		}
		while (ret == SQL_SUCCESS);
	}
}


/*
* 20170623 zxMarce: Routine to publish underlying ODBC error(s) as a normal 
* Gambas error. Based on reportODBCError above, but does NOT obey DB.IsDebug(),
* as this is intended as a component-centric "official" error report routine.
* It is the intention to replace most GB.Error() calls with this routine.
* Thanks to Tobias Boege for native GB string handling tips!
*/
void throwODBCError(const char *failedODBCFunctionName,
                    SQLHANDLE handle,
                    SQLSMALLINT handleType
                   )
{
	SQLINTEGER	i = 0;
	SQLINTEGER	native;
	SQLTCHAR	state[7];
	SQLTCHAR	text[512];
	char	*errorText = NULL; //GB.NewString("gb.db.odbc: ", 12);
	SQLSMALLINT	len;
	SQLRETURN	ret;

	errorText = GB.AddString(errorText, (char *)failedODBCFunctionName, 0),
	errorText = GB.AddString(errorText, " failed:", 0);

	do
	{
		ret = SQLGetDiagRec(handleType, handle, ++i, state, &native, text, sizeof(text), &len);
		if (SQL_SUCCEEDED(ret))
		{
				errorText = GB.AddString(errorText, "\n", 1);
				errorText = GB.AddString(errorText, (char *)state, 0);
				errorText = GB.AddString(errorText, (char *)text, len);
		}
	}
	while (ret == SQL_SUCCESS);

	GB.Error(errorText);
	GB.FreeString(&errorText);
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
int GetRecordCount(SQLHANDLE stmtHandle, SQLINTEGER cursorScrollable)
{
	SQLRETURN retcode;              //ODBC call return values
	int formerRecIdx = -1;           //Where we were when this all started.
	SQLINTEGER myRecCnt = -1;       //Default for when there's no cursor.
	SQLINTEGER firstRecNo = 0;		//20161111 holder for 1st recno.
	SQLINTEGER lastRecNo = 0;		//20161111 holder for last recno.
	char mssg[128];					//Error reporting text.

	//Make sure the statement has a cursor
	if (!(stmtHandle && (cursorScrollable == SQL_TRUE)))
	{
		DB.Debug("gb.db.odbc", "GetRecordCount(): Cannot count records!");
		return ((int) myRecCnt);
	}

	//Tell ODBC we won't be actually reading data (speeds process up).
	//SQL_ATTR_RETRIEVE_DATA = [SQL_RD_ON] | SQL_RD_OFF
	retcode = SQLSetStmtAttr(stmtHandle, SQL_ATTR_RETRIEVE_DATA, (SQLPOINTER) SQL_RD_OFF, 0);
	if (!SQL_SUCCEEDED(retcode))
	{
		reportODBCError("SQLSetStmtAttr SQL_ATTR_RETRIEVE_DATA",
						stmtHandle,
						SQL_HANDLE_STMT);
	}

	//Fetch current row's index so we can return to it when done.
	retcode = SQLGetStmtAttr(stmtHandle, SQL_ATTR_ROW_NUMBER, &formerRecIdx, 0, 0);
	if (!SQL_SUCCEEDED(retcode))
	{
		reportODBCError("SQLGetStmtAttr SQL_ATTR_ROW_NUMBER",
						stmtHandle,
						SQL_HANDLE_STMT);
	}

	//Make sure the statement has a cursor
	if (formerRecIdx < 0)
	{
		DB.Debug("gb.db.odbc", "GetRecordCount: Current record returned %d, returning -1 as count", formerRecIdx);
		goto __RETURN_COUNT;
	}

	//Try to get (back?) to the first record, abort if not possible.
	retcode = SQLFetchScroll(stmtHandle, SQL_FETCH_FIRST, (SQLINTEGER) 0);
	if (!SQL_SUCCEEDED(retcode))
	{
		reportODBCError("SQLFetchScroll SQL_FETCH_FIRST", stmtHandle, SQL_HANDLE_STMT);
		retcode = SQLSetStmtAttr(stmtHandle, SQL_ATTR_RETRIEVE_DATA, (SQLPOINTER) SQL_RD_ON, 0);
		return ((int) myRecCnt);	
	} else {
		//Fetch the first record's index
		retcode = SQLGetStmtAttr(stmtHandle, SQL_ATTR_ROW_NUMBER, &firstRecNo, 0, 0);
		if (SQL_SUCCEEDED(retcode))
		{
		    //Inform first recno if in Debug mode and carry on
			DB.Debug("gb.db.odbc", "GetRecordCount: First recno=%d", (int) firstRecNo);
		} else {
			//Could not fetch the first recno: Abort!
			reportODBCError("SQLFetchScroll SQL_ATTR_ROW_NUMBER (first recno)", stmtHandle, SQL_HANDLE_STMT);
			retcode = SQLSetStmtAttr(stmtHandle, SQL_ATTR_RETRIEVE_DATA, (SQLPOINTER) SQL_RD_ON, 0);
			return ((int) myRecCnt);
		}
	}

	//Advance the cursor to the last record.
	retcode = SQLFetchScroll(stmtHandle, SQL_FETCH_LAST, (SQLINTEGER) 0);
	if (SQL_SUCCEEDED(retcode))
	{

		//Fetch the last record's index
		retcode = SQLGetStmtAttr(stmtHandle, SQL_ATTR_ROW_NUMBER, &lastRecNo, 0, 0);
		if (SQL_SUCCEEDED(retcode))
		{
	        //Set ret value
			DB.Debug("gb.db.odbc", "GetRecordCount: Last recno=%d", (int) lastRecNo);
		} else {
			reportODBCError("SQLGetStmtAttr SQL_ATTR_ROW_NUMBER (last recno)", stmtHandle, SQL_HANDLE_STMT);
		}

		//Return cursor to original row.
		retcode = SQLFetchScroll(stmtHandle, SQL_FETCH_ABSOLUTE, (SQLINTEGER) formerRecIdx);
		//Since we have set the "do not read data" statement attribute, this call (may) return
		//code 100 (SQL_NO_DATA) but that's OK for our purposes of just counting rows.
		if (!SQL_SUCCEEDED(retcode) && (retcode != SQL_NO_DATA))
		{
			snprintf(mssg, sizeof(mssg), "SQLFetchScroll SQL_FETCH_ABSOLUTE (code %d) (rec %d)",
				 	 (int)retcode, formerRecIdx);
			reportODBCError(mssg, stmtHandle, SQL_HANDLE_STMT);
		}

	} else {
		reportODBCError("SQLFetchScroll SQL_FETCH_LAST", stmtHandle, SQL_HANDLE_STMT);
	}

	myRecCnt = (lastRecNo - firstRecNo + 1);
	DB.Debug("gb.db.odbc", "GetRecordCount: Record count=%d", (int) myRecCnt);

__RETURN_COUNT:
	
	//Tell ODBC we will be reading data now.
	//SQL_ATTR_RETRIEVE_DATA = [SQL_RD_ON] | SQL_RD_OFF
	retcode = SQLSetStmtAttr(stmtHandle, SQL_ATTR_RETRIEVE_DATA, (SQLPOINTER) SQL_RD_ON, 0);
	if (!SQL_SUCCEEDED(retcode))
	{
		reportODBCError("SQLSetStmtAttr SQL_ATTR_RETRIEVE_DATA", stmtHandle, SQL_HANDLE_STMT);
	}

	return ((int) myRecCnt);

}


/* BM: Replaces malloc() and free() by GB.Alloc() and GB.Free() */
static void *my_malloc(size_t size)
{
	void *ptr;
	GB.Alloc(&ptr, (int)size);
	return ptr;
}

static void my_free(void *_ptr)
{
	GB.Free(POINTER(&_ptr));
}

#define malloc(_size) my_malloc(_size)
#define free(_ptr) my_free(_ptr)


#if 0
static void quote(char *data, int len, DB_FORMAT_CALLBACK add)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquote\n");
fflush(stderr);
#endif
	int i;
	unsigned char c;
	//char buffer[8];

	(*add)("'", 1);
	for (i = 0; i < len; i++)
	{
		c = (unsigned char)data[i];
		if (c == '\\')
			(*add)("\\\\", 2);
		else if (c == '\'')
			(*add)("''", 2);

		else
			(*add)((char *)&c, 1);
	}
	(*add)("'", 1);
}
#endif

/* internal function to quote a value stored as a blob */

static void quote_blob(char *data, int len, DB_FORMAT_CALLBACK add)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquote_blob\n");
fflush(stderr);
#endif
	int i;
	unsigned char c;
	//char buffer[8];

	(*add)("'", 1);
	for (i = 0; i < len; i++)
	{
		c = (unsigned char)data[i];
		if (c == '\\')
			(*add)("\\\\\\\\", 4);
		else if (c == '\'')
			(*add)("''", 2);
		else if (c == 0)
			(*add)("\\\\000", 5);
		/*else if (c < 32 || c == 127)
		{
			int n = sprintf(buffer, "\\\\%03o", c);
			(*add)(buffer, n);
		}*/
		else
			(*add)((char *)&c, 1);
	}
	(*add)("'", 1);
}


/* internal function to unquote a value stored as a string */

#if 0
static int unquote(char *data, int len, DB_FORMAT_CALLBACK add)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tunquote\n");
fflush(stderr);
#endif

	int i;
	char c;

	if (!data || *data != '\'')
		return TRUE;

	for (i = 1;; i++)
	{
		c = data[i];
		if (c == '\'')
			break;
		if (c == '\\')
			i++;
		(*add)(&data[i], 1);
	}

	return FALSE;
}
#endif

/* internal function to unquote a value stored as a blob */

#if 0
static int unquote_blob(char *data, int len, DB_FORMAT_CALLBACK add)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tunquote_blob\n");
fflush(stderr);
#endif
	int i;
	char c;


	for (i = 0; i < len; i++)
	{
		c = data[i];

		if (c == '\\')
		{
			i++;
			c = data[i];
			if (c >= '0' && c <= '9' && i < (len - 2))
			{
				c = ((data[i] - '0') << 6) + ((data[i + 1] - '0') << 3) + (data[i + 2] - '0');
				i += 2;
				(*add)(&c, 1);
				continue;
			}
		}
		(*add)(&data[i], 1);
	}

	return FALSE;
}
#endif

/*
Internal function to check if the .Host property is actually an ODBC connection string.
ODBC ConnStrings have one or more "ParamName=ParamValue" pairs, delimited by semicolons.
The function helps the component know whether to call SQLConnect (when a host/DSN),
or SQLDriverConnect (when a ConnString).
I know there are C functions to locate CHARs in a CHAR[], but I'm not well versed
in C and less in what's available in a Gambas component, so I stuck to whatever other
routines use in this module.
zxMarce, 20150826
*/
static bool is_host_a_connstring(const char *host_or_cs)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tis_host_a_connstring: '%s'\n", host_or_cs);
fflush(stderr);
#endif

	int counter;
	char curChar;

	if (!host_or_cs)
		return FALSE;

	for (counter = 0; counter < strlen(host_or_cs); counter++)
	{
		curChar = host_or_cs[counter];
		if ((curChar == '=') || (curChar == ';'))
			return TRUE;
	}

	return FALSE;
}

/* Internal function to convert a database type into a Gambas type */
static GB_TYPE conv_type(int type)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tconv_type: Field type: %d\n",type);
fflush(stderr);
#endif
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


/* Internal function to convert a database value into a Gambas variant value */
static void conv_data(char *data, int len, GB_VARIANT_VALUE * val, int type)
{
	GB_VALUE conv;

	switch (type)
	{
		//case SQL_NUMERIC:
		case SQL_INTEGER:
		case SQL_SMALLINT:
			val->type = GB_T_INTEGER;
			if (GB.NumberFromString(GB_NB_READ_INTEGER, data, len, &conv))
				val->value._integer = 0;
			else
				val->value._integer = conv._integer.value;
			break;

		case SQL_DECIMAL:
		case SQL_NUMERIC:
		case SQL_FLOAT:
		case SQL_REAL:
		case SQL_DOUBLE:
			val->type = GB_T_FLOAT;
			if (GB.NumberFromString(GB_NB_READ_FLOAT | GB_NB_LOCAL, data, len, &conv) && GB.NumberFromString(GB_NB_READ_FLOAT, data, len, &conv))
			{
				fprintf(stderr, "gb.db.odbc: unable to convert float: %.*s\n", len, data);
				val->value._float = 0;
			}
			else
				val->value._float = conv._float.value;
			break;

		case SQL_BIGINT: // Data type bigint 64 bits
			val->type = GB_T_LONG;
			if (GB.NumberFromString(GB_NB_READ_LONG, data, len, &conv))
				val->value._long = 0;
			else
				val->value._long = conv._long.value;
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
				GB_DATE_SERIAL date = { 0 };
				double sec;
				int n;
				
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
						
						GB.MakeDate(&date, (GB_DATE *)&conv);
					}
					else if (GB.DateFromString(data, len, &conv, TRUE))
					{
						fprintf(stderr, "gb.db.odbc: unable to convert date: %.*s\n", len, data);
					}
				}
				
				val->type = GB_T_DATE;
				val->value._date.date = conv._date.value.date;
				val->value._date.time = conv._date.value.time;
				break;
			}
			
		case SQL_TINYINT:
			val->type = GB_T_BOOLEAN;
			val->value._boolean = atoi(data) ? -1 : 0;
			break;

		case SQL_CHAR:
		default:
			val->type = GB_T_CSTRING;
			val->value._string = data;
			break;
	}

}



/*****************************************************************************

	get_quote()

	Returns the character used for quoting object names.

*****************************************************************************/

static const char *get_quote(void)
{
	return QUOTE_STRING;
}

/*****************************************************************************
   zxMarce: Added 20160928 to retrieve the connected Database (or 'Catalog')
   name directly from unixODBC/driver.
   Actually, this routine *should* be called once after every operation,
   because SPs or even plaintext queries could change the Catalog (for example
   by using the "USE <newcatalog>" command), but we don't have access to a
   DB_DESC structure from the query-running code.
*****************************************************************************/
void GetConnectedDBName(DB_DESC *desc, ODBC_CONN *odbc)
{

	SQLRETURN	retcode;
	SQLINTEGER	charsNeeded = 0;
	char *dbName;

	/*zxMarce: Attribute to fetch is SQL_ATTR_CURRENT_CATALOG
	  We call the function first with a NULL buffer pointer so as to
	  retrieve the necessary buffer size.
	*/
	retcode = SQLGetConnectAttrA(odbc->odbcHandle, SQL_ATTR_CURRENT_CATALOG,
				    NULL, (SQLINTEGER) 0,
				    (SQLINTEGER *) &charsNeeded
				   );

	if (SQL_SUCCEEDED(retcode))
	{
		dbName = GB.NewString(NULL, charsNeeded);
		
		/*zxMarce: We call the function again, this time specifying a
		  hopefully big enough buffer for storing the catalog name.
		*/
		retcode = SQLGetConnectAttr(odbc->odbcHandle, SQL_ATTR_CURRENT_CATALOG,
					    dbName, charsNeeded,
					    &charsNeeded
					   );
		
		if (SQL_SUCCEEDED(retcode))
		{
			GB.FreeString(&desc->name);
			desc->name = dbName;
		}
	}

	if (desc->name)
		DB.Debug("gb.db.odbc", "GetConnectedDBName: desc->name (%d chars): '%s'", (int)charsNeeded, desc->name);
	else
		DB.Debug("gb.db.odbc", "GetConnectedDBName: desc->name: NULL");

}

/*****************************************************************************

	open_database()

	Connect to a database.

	<desc> points at a structure describing each connection parameter.
	<db> points at the DB_DATABASE structure that must be initialized.

	This function must return TRUE if the connection has failed.

	The name of the database can be NULL, meaning a default database.

*****************************************************************************/

static int open_database(DB_DESC *desc, DB_DATABASE *db)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\topen_database\n");
fflush(stderr);
#endif

	//int V_OD_erg;
	SQLRETURN 	retcode;
	ODBC_CONN 	*odbc;
	bool 		hostIsAConnString;
	char 		*host;
	char 		*user;

	host = desc->host;
	if (!host)
		host = "";

	user = desc->user;
	if (!user)
		user = "";

	hostIsAConnString = is_host_a_connstring(host);

	/* Allocate the ODBC handle */
	odbc = (ODBC_CONN *)malloc(sizeof(ODBC_CONN));
	odbc->odbcHandle = NULL;
	odbc->odbcEnvHandle = NULL;
	odbc->dsn_name = NULL;

	/* Allocate the Environment handle */
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &odbc->odbcEnvHandle);

        if (!SQL_SUCCEEDED(retcode))
	{
		free(odbc);
		GB.Error("Unable to allocate ODBC environment handle");
		return TRUE;
	}

	/* Set the Envoronment attributes */
	retcode = SQLSetEnvAttr(odbc->odbcEnvHandle, SQL_ATTR_ODBC_VERSION,(void *) SQL_OV_ODBC3, 0);

        if (!SQL_SUCCEEDED(retcode))
	{
		SQLFreeHandle(SQL_HANDLE_ENV, odbc->odbcEnvHandle);
		free(odbc);
		GB.Error("Unable to set ODBC environment attributes");
		return TRUE;
	}

	/* Allocate the Database Connection handle */
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, odbc->odbcEnvHandle, &odbc->odbcHandle);

        if (!SQL_SUCCEEDED(retcode))
	{
		SQLFreeHandle(SQL_HANDLE_ENV, odbc->odbcEnvHandle);
		free(odbc);
		GB.Error("Unable to allocate ODBC database handle");
		return TRUE;

	}

	/* 20170818 zxMarce: The following timeout was incorrectly set. The right timeout is the CONNECT
	 * timeout. The LOGIN timeout is actually used once the connection is established. Also took the
	 * opportunity to make sure ODBC uses cursors either from the Driver or the Driver Manager, thus
	 * almost ensuring cursors are available.
	*/
	//SQLSetConnectAttr(odbc->odbcHandle, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)(intptr_t)db->timeout, 0);
	SQLSetConnectAttr(odbc->odbcHandle, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)(intptr_t)db->timeout, 0);
	
	/* 
	 * 20210404: Watch out, by using SQL_CUR_USE_IF_NEEDED, if the driver does not provide cursors, 
	 * unixODBC seems to need bound columns. This has the nasty side effect to raise SQL errors. Do 
	 * not yet know how to workaround this.
	 * The SQL_CUR_USE_IF_NEEDED constant tells ODBC to provide its own cursors if the driver doesn't.
	 */
	//SQLSetConnectAttr(odbc->odbcHandle, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER)SQL_CUR_USE_IF_NEEDED, 0);
	SQLSetConnectAttr(odbc->odbcHandle, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER)SQL_CUR_USE_DRIVER, 0);
	
	if (hostIsAConnString)
	{
		/* zxMarce: Connect to Database (desc->host is an ODBC Connection String) */
		retcode = SQLDriverConnect(odbc->odbcHandle, 0, (SQLCHAR *)host, SQL_NTS, 0, 0, 0, SQL_DRIVER_NOPROMPT);
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

	} else {
		/* Connect to Database (desc->host is an ODBC Data Source Name) */
		retcode = SQLConnect(odbc->odbcHandle, (SQLCHAR *)host, SQL_NTS, (SQLCHAR *)user, SQL_NTS, (SQLCHAR *) desc->password, SQL_NTS);
	}

    //zxMarce: Must bail out NOW if failed to connect, or nonsense errors will appear.
	if (!SQL_SUCCEEDED(retcode))
	{
		throwODBCError((hostIsAConnString ? "SQLDriverConnect" : "SQLConnect"), odbc->odbcHandle, SQL_HANDLE_DBC);
		free(odbc);
		//GB.Error("Error connecting to database");
		return TRUE;
	}

	retcode = SQLSetConnectAttr(odbc->odbcHandle, SQL_ATTR_AUTOCOMMIT, (void *) SQL_AUTOCOMMIT_ON, SQL_NTS);

	/* desc->name is a pointer intended to point to the database name ('catalog', in
	 * MSSQL parlance) to which we are connected. But that is NOT the actual database
	 * name when we use a connstring. When we use a connstring, the '.Database' connection
	 * property would then have the whole connection string, so we tweak it here to make it
	 * point to the actual database (catalog?) name.
	*/
	//odbc->dsn_name = malloc(sizeof(char) * strlen(host));
	//strcpy(odbc->dsn_name, host);
	GetConnectedDBName(desc, odbc);

	odbc->user_name = malloc(sizeof(char) * strlen(user));
	strcpy(odbc->user_name, user);

	db->version = 3;

	retcode = SQLGetFunctions(odbc->odbcHandle, SQL_API_SQLFETCHSCROLL, &odbc->drvrCanFetchScroll);
    if (!SQL_SUCCEEDED(retcode))
	{
		throwODBCError("SQLGetFunctions SQL_API_SQLFETCHSCROLL", odbc->odbcHandle, SQL_HANDLE_DBC);
		free(odbc);
		//GB.Error("Error calling the ODBC SQLGetFunctions API");
		return TRUE;
	}

	/* flags */
	db->flags.no_table_type = TRUE;
	db->flags.no_seek = (odbc->drvrCanFetchScroll == SQL_FALSE);
	db->flags.no_serial = TRUE;		// Need to be done!
	db->flags.no_blob = FALSE;		// Need to be done!
	db->flags.no_collation = TRUE;

	db->handle = odbc;
	return FALSE;
}


/*****************************************************************************

	close_database()

	Terminates the database connection.

	<handle> contains the database handle.

*****************************************************************************/

static void close_database(DB_DATABASE *db)
{
	//SQLRETURN retcode;
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tclose_database\n");
fflush(stderr);
#endif

	ODBC_CONN *conn = (ODBC_CONN *)db->handle;
	//SQLRETURN retcode = 0;

	if (conn->odbcHandle)
		SQLDisconnect(conn->odbcHandle);
	else
		GB.Error("ODBC module internal error disconnecting hDBC");

	if (conn->odbcHandle)
	{
		SQLFreeHandle(SQL_HANDLE_DBC, conn->odbcHandle);
		conn->odbcHandle = NULL;
	}
	else
		GB.Error("ODBC module internal error freeing hDBC");

	if (conn->odbcEnvHandle)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, conn->odbcEnvHandle);
		conn->odbcEnvHandle = NULL;
	}
	else
		GB.Error("ODBC module internal error freeing hENV");

	if (conn->dsn_name)
	{
		free(conn->dsn_name);
		conn->dsn_name = NULL;
	}

	if (conn->user_name)
	{
		free(conn->user_name);
		conn->user_name = NULL;
	}

	if (conn)
	{
		free(conn);
		db->handle = NULL;
	}

}


/*****************************************************************************

	get_collations()

	Return the available collations as a Gambas string array.

*****************************************************************************/

static GB_ARRAY get_collations(DB_DATABASE *db)
{
	return NULL;
}


/*****************************************************************************

	format_value()

	This function transforms a gambas value into a string value that can
	be inserted into a SQL query.

	<arg> points to the value.
	<add> is a callback called to insert the string into the query.

	This function must return TRUE if it translates the value, and FALSE if
	it does not.

	If the value is not translated, then a default translation is used.

*****************************************************************************/

static int format_value(GB_VALUE * arg, DB_FORMAT_CALLBACK add)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tformat_value\n");
fflush(stderr);
#endif

	int l;
	GB_DATE_SERIAL *date;

	switch (arg->type)
	{
		case GB_T_BOOLEAN:
/*Note this is likely to go to a tinyint  */

			if (VALUE((GB_BOOLEAN *) arg))
				add("'1'", 3);
			else
				add("'0'", 3);
			return TRUE;

		case GB_T_STRING:
		case GB_T_CSTRING:

			return FALSE;							// default

		case GB_T_DATE:

			date = GB.SplitDate((GB_DATE *) arg);

			l = sprintf(_buffer, "'%04d-%02d-%02d-%02d.%02d.%02d",
									date->year, date->month, date->day,
									date->hour, date->min, date->sec);

			add(_buffer, l);

			if (date->msec)
			{
				l = sprintf(_buffer, ".%03d", date->msec);
				add(_buffer, l);
			}

			add("'", 1);

			return TRUE;

		default:
			return FALSE;
	}
}

/*****************************************************************************

	format_blob()

	This function transforms a blob value into a string value that can
	be inserted into a SQL query.

	<blob> points to the DB_BLOB structure.
	<add> is a callback called to insert the string into the query.

*****************************************************************************/

static void format_blob(DB_BLOB *blob, DB_FORMAT_CALLBACK add)
{
	// BM: must be done!
	#ifdef ODBC_DEBUG_HEADER
	fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
	fprintf(stderr,"\tformat_blob\n");
	fflush(stderr);
	#endif
	quote_blob(blob->data, blob->length, add);

}

static char *query_param[3];

static void query_get_param(int index, char **str, int *len, char quote)
{
	//DB.Debug("gb.db.odbc", "query_get_param() invoked.");
	if (index > 3)
		return;

	index--;
	*str = query_param[index];
	*len = strlen(*str);

	if (quote == '\'' || quote == '`')
	{
		*str = DB.QuoteString(*str, *len, quote);
		*len = GB.StringLength(*str);
	}
}

/*
	Reverse the function call order to make C happy:
		2021-04-03 21:03:10.117 gb.db.odbc: query_fill.SQLFetch(): retcode2=-1
		2021-04-03 21:03:10.117 gb.db.odbc: query_fill() invoked.
		2021-04-03 21:03:10.117 gb.db.odbc: query_make_result() invoked.
		2021-04-03 21:03:10.117 gb.db.odbc: query_init() invoked.
		2021-04-03 21:03:10.117 gb.db.odbc: cannot do GetRecordCount()!
		2021-04-03 21:03:10.111 gb.db.odbc: 0x55dfb1cfbca8: SELECT * FROM Employees
		2021-04-03 21:03:10.111 gb.db.odbc: do_query() invoked.
		2021-04-03 21:03:10.111 gb.db.odbc: exec_query() invoked.
*/

static int get_num_columns(ODBC_RESULT *result)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tget_num_columns\n");
fflush(stderr);
#endif

	SQLSMALLINT colsNum = 0;
	SQLRETURN retcode;

	retcode = SQLNumResultCols(result->odbcStatHandle, &colsNum);

	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		GB.Error("ODBC error: Unable to get the number of columns");

	return colsNum;
}

/*****************************************************************************

	query_fill()

	Fill a result buffer with the value of each field of a record.

	<db> is the database handle, as returned by open_database()
	<result> is the handle of the result.
	<pos> is the index of the record in the result.
	<buffer> points to an array having one element for each field in the
	result.
	<next> is a boolean telling if we want the next row.

	This function must return DB_OK, DB_ERROR or DB_NO_DATA

	This function must use GB.StoreVariant() to store the value in the
	buffer.

*****************************************************************************/

static int query_fill(DB_DATABASE *db, DB_RESULT result, int pos, GB_VARIANT_VALUE *buffer, int next)
{
	ODBC_RESULT *res = (ODBC_RESULT *) result;
	GB_VARIANT value;
	SQLRETURN retcode2;
	SQLINTEGER i;
	ODBC_FIELD *field;
	int nResultCols;
	SQLLEN len_read;


#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquery_fill result %p,result->odbcStatHandle %p, pos %d\n",res,res->odbcStatHandle,pos);
fflush(stderr);
#endif

	/*nResultCols = get_num_columns(res);
	if (nResultCols == 0)
		return DB_ERROR;*/
	
	nResultCols = GB.Count(res->fields);
	
	DB.Debug("gb.odbc","query_fill: %p: %d (%d)", result, pos, next);

	/*current = res->fields;

	for (i = 0; i < nResultCols; i++)
	{
		if(current->next)
			current = (ODBC_FIELDS *) current->next;
	}*/

	if (res->Function_exist == SQL_TRUE)		//Does driver support SQLFetchScroll?
	{
		if (res->Cursor_Scrollable == SQL_TRUE && !next)	//Does the query support scrolling?
		{
			retcode2 = SQLFetchScroll(
				res->odbcStatHandle, 
				SQL_FETCH_ABSOLUTE, 
				pos + 1
			);
		}
		else
		{
			retcode2 = SQLFetchScroll(
				res->odbcStatHandle, 
				SQL_FETCH_NEXT, 
				pos + 1
			);
		}
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
			return DB_ERROR;
		}
		retcode2 = SQLFetch(res->odbcStatHandle);
	}

	if (
		(retcode2 != SQL_SUCCESS) && 
		(retcode2 != SQL_SUCCESS_WITH_INFO) && 
		(retcode2 != SQL_NO_DATA_FOUND) && 
		(retcode2 != SQL_NO_DATA)
	)
	{
		DB.Debug("gb.db.odbc","SQLFetchScroll()/SQLFetch() returned code %d, cannot fetch a row.", (int)retcode2);
		GB.Error("Unable to fetch row");
		return DB_ERROR;
	}

	if (retcode2 == SQL_NO_DATA_FOUND || retcode2 == SQL_NO_DATA)
		return DB_NO_DATA;

	for (i = 0; i < nResultCols; i++)
	{
		field = &res->fields[i];

#if 0
		char * 		fieldata;
		SQLULEN 	precision = 0;
		SQLSMALLINT colnamelen = 0, scale = 0, type;
		SQLLEN 		read = 0;
		SQLCHAR 	namebuff[25];

		displaySize = 0;

		SQLDescribeCol(
			res->odbcStatHandle, 
			i + 1, 
			namebuff, 
			sizeof(namebuff), 
			&colnamelen,
			&type,
			&precision, 
			&scale,
			NULL
		);
		
		fprintf(stderr, "[%d] %s\n", i, namebuff);

		/*
		 * 20210405 zxMarce: The line below asked for the attribute
		 * SQL_DESC_LENGTH, but it should have asked instead for 
		 * SQL_DESC_DISPLAY_SIZE. It messed up drivers like MDBTools,
		 * where SQL_DESC_LENGTH seems to return the field name length
		 * instead of the max field data length. Sources:
		 * https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlcolattribute-function
		 * https://www.ibm.com/support/producthub/db2/docs/content/SSEPGG_11.5.0/com.ibm.db2.luw.apdv.cli.doc/doc/r0000569.html
		 */
		SQLColAttribute(
			res->odbcStatHandle, 
			i + 1, 
			SQL_DESC_DISPLAY_SIZE, 
			"",
			0, 
			NULL, 
			(SQLPOINTER) &displaySize
		);

		read = 0;

		if (displaySize >= strlen((char *) namebuff))
		{
			displaySize = displaySize + 1;
		}
		else
		{
			displaySize = strlen((char *) namebuff) + 1;
		}

		if (displaySize > 0)
		{
			
			if (displaySize < 2)
			{
				displaySize = 2;
			}

			if (
				(type != SQL_LONGVARCHAR) && 
				(type != SQL_VARBINARY) && 
				(type != SQL_LONGVARBINARY)
			)
			{
				fieldata = malloc(sizeof(char) * (displaySize));
				SQLGetData(
					res->odbcStatHandle,
					i + 1,
					SQL_C_CHAR, 
					fieldata,
					displaySize,
					&read
				);
			}
			else
			{
				//BLOB field, not retrieved here
				//the BLOB field hasn't the string terminator
				//displaySize = displaySize-1;
				displaySize--;
			}
			current->outlen = displaySize;
		}
#endif

		value.type = GB_T_VARIANT;
		value.value.type = GB_T_NULL;

		if (
			(field->type != SQL_LONGVARCHAR) && 
			(field->type != SQL_VARBINARY) && 
			(field->type != SQL_LONGVARBINARY)
		)
		{
			*field->data = 0; // If SQLGetData returns nothing
			len_read = 0;
			
			if (!SQL_SUCCEEDED(SQLGetData(res->odbcStatHandle, i + 1, SQL_C_CHAR, field->data, field->len, &len_read)))
				reportODBCError("SQLGetData", res->odbcStatHandle, SQL_HANDLE_STMT);
			
			DB.Debug("gb.db.odbc", "query_fill: %s (%d) = %.*s", field->name, field->type, (int)len_read, field->data);
			
			if (len_read > 0)
				conv_data((char *)field->data, len_read, &value.value, (int)field->type);
			
			GB.StoreVariant(&value, &buffer[i]);
		}

		
		/*if (current == NULL)
		{
			GB.Error("ODBC internal error 4");
			return DB_ERROR;
		}

		//fprintf(stderr, "Lunghezza letta = %d\n", read);
		if (current)
		{
			if(current->fieldata == NULL)
			{
				GB.Error("ODBC internal error 5");
				return DB_ERROR;
			}

			if (read == -1)
			{
				fieldata[0] = ' ';
				fieldata[1] = '\0';
				current->type = SQL_CHAR;
			}
			conv_data((char *) fieldata, &value.value, (int) current->type);
		}

		GB.StoreVariant(&value, &buffer[i]);

		if(
			(displaySize > 0) && 
			(fieldata != NULL)
		)
		{
			free(fieldata);
		}

		current = (ODBC_FIELDS *) current->next;
		fieldata = NULL;*/

	}/* for all columns in this row  */

	return DB_OK;
}


/* Internal function - create the space for the result and bind the column to each field-space allocated */
static void query_make_result(ODBC_RESULT * result)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquery_make_result result %p, result->odbcStatHandle %p\n", result, result->odbcStatHandle);
fflush(stderr);
#endif

	//SQLCHAR colname[32];
	SQLSMALLINT colnamelen;
	SQLULEN precision;
	SQLSMALLINT scale;
	SQLINTEGER i;
	SQLLEN displaySize;
	ODBC_FIELD *field;
	SQLINTEGER collen;
	int nResultCols;
	SQLSMALLINT type;

	nResultCols = get_num_columns(result);
	
	DB.Debug("gb.db.odbc", "query_make_result: %p (%d columns)", result, nResultCols);
	
	GB.NewArray(POINTER(&result->fields), sizeof(ODBC_FIELD), nResultCols);
	
	for (i = 0; i < nResultCols; i++)
	{
		field = &result->fields[i];
		
		SQLDescribeCol(
			result->odbcStatHandle, 
			i + 1, 
			NULL, 
			0, 
			&colnamelen, 
			&type, 
			&precision, 
			&scale, 
			NULL
		);
		
		field->name = GB.NewString(NULL, colnamelen);
		
		SQLDescribeCol(
			result->odbcStatHandle, 
			i + 1, 
			(SQLCHAR *)field->name, 
			colnamelen + 1, 
			&colnamelen, 
			&type, 
			&precision, 
			&scale, 
			NULL
		);
		
		if (type < 0)
		{
			DB.Debug("gb.db.odbc", "field '%s' has datatype: %d, assuming SQLCHAR instead", field->name, type);
			type = SQL_CHAR;
		}
		
		field->type = type;
		
		DB.Debug("gb.db.odbc", "query_make_result: '%s' -> type = %d", field->name, field->type);

		collen = precision;

		/* Get display length for column */
		SQLColAttribute(
			result->odbcStatHandle, 
			i + 1, 
			SQL_COLUMN_DISPLAY_SIZE, 
			NULL,
			0, 
			NULL, 
			&displaySize
		);

		/*
		* Set column length to max of display length, and column name
		* length. Plus one byte for null terminator
		*/
		//fprintf(stderr, "%s: collen : %ld, display len %ld\n", field->name, strlen(field->name), displaySize);

		/*if (displaySize >= colnamelen)
		{
			collen = displaySize + 1;
		}
		else
		{
			collen = colnamelen + 1;
		}*/

		collen = Max(displaySize, colnamelen) + 1;
		if (collen <= 0)
			collen = 1;
		
		field->data = malloc(collen);
		field->len = collen;
	}
}


/*****************************************************************************

	query_init()

	Initialize an info structure from a query result.

	<result> is the handle of the query result.
	<info> points to the info structure.
	<count> will receive the number of records returned by the query.

	This function must initialize the info->nfield field with the number of
	fields in the query result.

	If <count> receives -1, that will mean that the result is "move forward"
	only

*****************************************************************************/

static void query_init(DB_RESULT result, DB_INFO * info, int *count)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquery_init\n");
fflush(stderr);
#endif

	ODBC_RESULT *res = (ODBC_RESULT *) result;
	SQLSMALLINT colsNum = 0;
	colsNum = get_num_columns(res);

	DB.Debug("gb.db.odbc", "query_init: %p -> %d columns", result, colsNum);
	
	if (colsNum == 0)
		return;

	*count = res->count;
	info->nfield = colsNum;
	query_make_result(res);
}


/* Internal function to implement the query execution */
static int do_query(DB_DATABASE *db, const char *error, ODBC_RESULT **res, const char *qtemp, int nsubst, ...)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tdo_query db %p, ODBC_result res %p, db->handle %p, query = '%s'\n", db, res, db->handle, qtemp);
fflush(stderr);
#endif

	va_list args;
	ODBC_CONN *handle = (ODBC_CONN *)db->handle;
	SQLRETURN retcode = SQL_SUCCESS;
	ODBC_RESULT *odbcres;
	const char *query;
	int i;

	if (nsubst)
	{
		va_start(args, nsubst);
		if (nsubst > 3)
		{
			nsubst = 3;
		}
		for (i = 0; i < nsubst; i++)
		{
			query_param[i] = va_arg(args, char *);
		}
		query = DB.SubstString(qtemp, 0, query_get_param);
	}
	else
	{
		query = qtemp;
	}

	//DB.Debug("gb.db.odbc", "do_query() db->handle=%p, query='%s'", handle, query);

	GB.AllocZero(POINTER(&odbcres), sizeof(ODBC_RESULT));

	/* Allocate the space for the result structure */
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, handle->odbcHandle, &odbcres->odbcStatHandle);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
	{
		//GB.Error("Cannot allocate statement handle");
		throwODBCError("SQLAllocHandle", handle->odbcHandle, SQL_HANDLE_DBC);
		return retcode;
	}

	retcode = SQLSetStmtAttr(odbcres->odbcStatHandle, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE, 0);
	odbcres->Cursor_Scrollable = ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) ? SQL_FALSE : SQL_TRUE;
	odbcres->Function_exist = handle->drvrCanFetchScroll;
	DB.Debug("gb.db.odbc", "do_query() Cursor_Scrollable = %d, drvrCanFetchScroll %d", odbcres->Cursor_Scrollable, handle->drvrCanFetchScroll);

	/* Execute the query */
	retcode = SQLExecDirect(odbcres->odbcStatHandle, (SQLCHAR *) query, SQL_NTS);
	//DB.Debug("gb.db.odbc", "do_query() SQLExecDirect()=%d [SQL_SUCCESS=%d]", (int)retcode, (int)SQL_SUCCESS);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) && (retcode != SQL_NO_DATA))
	{
		DB.Debug("gb.db.odbc", "do_query: SQLExecDirect() returned code %d", (int)retcode);
		throwODBCError("SQLExecDirect", odbcres->odbcStatHandle, SQL_HANDLE_STMT);
		SQLFreeHandle(SQL_HANDLE_STMT, odbcres->odbcStatHandle);
		//GB.Error("Error while executing the statement");
		return retcode;
	}

	if (res)
	{
		if (retcode == SQL_NO_DATA)
		{
			odbcres->count = 0;
			retcode = SQL_SUCCESS;
		}
		else
		{
			odbcres->count = GetRecordCount(odbcres->odbcStatHandle, odbcres->Cursor_Scrollable);
		}
		*res = odbcres;
		DB.Debug("gb.db.odbc", "do_query: create handle %p", odbcres->odbcStatHandle);
	}
	else
	{
		SQLFreeHandle(SQL_HANDLE_STMT, odbcres->odbcStatHandle);
		GB.Free(POINTER(&odbcres));
	}

	return retcode;
}

/*****************************************************************************

	exec_query()

	Send a query to the server and gets the result.

	<handle> is the database handle, as returned by open_database()
	<query> is the query string.
	<result> will receive the result handle of the query.
	<err> is an error message used when the query failed.

	<result> can be NULL, when we don't care getting the result.

*****************************************************************************/

static int exec_query(DB_DATABASE *db, const char *query, DB_RESULT * result, const char *err)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\texec_query\n");
fflush(stderr);
#endif
	return do_query(db, err, (ODBC_RESULT **) result, query, 0);
}


/* Internal function - free the result structure create to allocate the result row */
static void query_free_result(ODBC_RESULT * result)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquery_free_result %p\n",result);
fflush(stderr);
#endif

	int i;
	ODBC_FIELD *field;
	
	for (i = 0; i < GB.Count(result->fields); i++)
	{
		field = &result->fields[i];
		GB.FreeString(&field->name);
		free(field->data);
	}
	
	GB.FreeArray(POINTER(&result->fields));

	free(result);
}


/*****************************************************************************

	query_release()

	Free the info structure filled by query_init() and the result handle.

	<result> is the handle of the query result.
	<info> points to the info structure.
	<invalid> tells if the associated connection has been closed.

*****************************************************************************/

static void query_release(DB_RESULT result, DB_INFO *info, bool invalid)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tquery_release\n");
fflush(stderr);
#endif

	ODBC_RESULT *res = (ODBC_RESULT *) result;

	/*if (res != NULL)*/	//query_free_result(res);

	if (!invalid)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, res->odbcStatHandle);
		DB.Debug("gb.db.odbc", "query_release: %p: free handle %p", result, res->odbcStatHandle);
	}
	else
		DB.Debug("gb.db.odbc", "query_release: %p: database is closed, do not free the handle", result);
	//free(res->odbcStatHandle);

	query_free_result(res);
	//free(res->odbcStatHandle);
	//res->odbcStatHandle=NULL;

}


/*****************************************************************************

	get_last_insert_id()

	Return the value of the last serial field used in an INSERT statement

	<db> is the database handle, as returned by open_database()

*****************************************************************************/

static int64_t get_last_insert_id(DB_DATABASE *db)
{
	GB.Error("Unsupported feature");
	return -1;
}


/*****************************************************************************

	blob_read()

	Returns the value of a BLOB field.

	<result> is the handle of the result.
	<pos> is the index of the record in the result.
	<blob> points at a DB_BLOB structure that will receive a pointer to the
	data and its length.

*****************************************************************************/

static void blob_read(DB_RESULT result, int pos, int field, DB_BLOB *blob)
{
	ODBC_RESULT *res = (ODBC_RESULT *)result;
	SQLCHAR buffer[1024];
	SQLLEN len_read;
	int old_length;
	SQLRETURN ret;

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tblob_read DB_RESULT %p, dbresult->stathandle %p, pos %d , field %d\n",res,res->odbcStatHandle ,pos,field);
fflush(stderr);
#endif

	blob->data = NULL;
	blob->length = 0;
	
	for(;;)
	{
		len_read = 0;
		ret = SQLGetData(res->odbcStatHandle, field + 1, SQL_C_BINARY, buffer, sizeof(buffer), &len_read);
		if (ret == SQL_ERROR || ret == SQL_NO_DATA || len_read <= 0)
			break;
	
		fprintf(stderr, "blob_read: %d %ld\n", blob->length, len_read);
		if (len_read > sizeof(buffer) || len_read == SQL_NO_TOTAL)
			len_read = sizeof(buffer);
		
		old_length = blob->length;
		blob->length += len_read;
		GB.Realloc(POINTER(&blob->data), blob->length);
		
		memcpy(&blob->data[old_length], buffer, len_read);
	} 
	
	if (ret != SQL_NO_DATA)
		DB.Debug("gb.db.odbc", "unable to read blob from field '%s'", res->fields[field].name);
}


/*****************************************************************************

	field_name()

	Return the name of a field in a result from its index.

	<result> is the result handle.
	<field> is the field index.

*****************************************************************************/

static char *field_name(DB_RESULT result, int field)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_name\n");
fflush(stderr);
#endif
	ODBC_RESULT *res = (ODBC_RESULT *)result;
	return res->fields[field].name;
}


/*****************************************************************************

	field_index()

	Return the index of a field in a result from its name.

	<result> is the result handle.
	<name> is the field name.
	<handle> is needed by this driver to enable table.field syntax

*****************************************************************************/

static int field_index(DB_RESULT result, const char *name, DB_DATABASE *db)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_index\n");
fflush(stderr);
#endif
	ODBC_RESULT *res = (ODBC_RESULT *) result;
	int i;

	for (i = 0; i < GB.Count(res->fields); i++)
	{
		if (strcmp(res->fields[i].name, name) == 0)
			return i;
	}
	
	return (-1);
}


/*****************************************************************************

	field_type()

	Return the Gambas type of a field in a result from its index.

	<result> is the result handle.
	<field> is the field index.

*****************************************************************************/

static GB_TYPE field_type(DB_RESULT result, int field)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_type id %d\n",field);
fflush(stderr);
#endif
	ODBC_RESULT *res = (ODBC_RESULT *)result;

	return conv_type(res->fields[field].type);
}


/*****************************************************************************

	field_length()

	Return the length of a field in a result from its index.

	<result> is the result handle.
	<field> is the field index.

*****************************************************************************/

static int field_length(DB_RESULT result, int field)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_length\n");
fflush(stderr);
#endif
	SQLCHAR colname[32];
	SQLSMALLINT coltype;
	SQLSMALLINT colnamelen;
	SQLULEN precision;
	SQLSMALLINT scale;
	ODBC_RESULT *res = (ODBC_RESULT *) result;

	SQLDescribeCol(res->odbcStatHandle, field + 1, colname, sizeof(colname), &colnamelen, &coltype, &precision, &scale, NULL);

	return colnamelen;

}


/*****************************************************************************

	begin_transaction()

	Begin a transaction.

	<handle> is the database handle.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int begin_transaction(DB_DATABASE *db)
{
	return (do_query(db, "Unable to begin transaction: &1", NULL, "BEGIN", 0));
}




/*****************************************************************************

	commi_transaction()

	Commit a transaction.

	<handle> is the database handle.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int commit_transaction(DB_DATABASE *db)
{
	return (do_query(db, "Unable to commit transaction: &1", NULL, "COMMIT", 0));
}


/*****************************************************************************

	rollback_transaction()

	Rollback a transaction.

	<handle> is the database handle.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int rollback_transaction(DB_DATABASE *db)
{
	return do_query(db, "Unable to rollback transaction: &1", NULL, "ROLLBACK", 0);
}



/*****************************************************************************

	table_init()

	Initialize an info structure from table fields.

	<handle> is the database handle.
	<table> is the table name.
	<info> points at the info structure.

	This function must initialize the following info fields:
	- info->nfield must contain the number of fields in the table.
	- info->field is an array of DB_FIELD, one element for each field.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int table_init(DB_DATABASE *db, const char *table, DB_INFO * info)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\ttable_init\n");
fflush(stderr);
#endif
	SQLCHAR field_name[256];
	
	SQLCHAR coltype[100];
	SQLCHAR precision[100];
	SQLSMALLINT colsNum;
	SQLHSTMT statHandle;
	//SQLRETURN V_OD_erg;
	SQLRETURN retcode;
	int i;
	DB_FIELD *f;
	ODBC_FIELD *fields, *field;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;


	info->table = GB.NewZeroString(table);

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, (ODBC_CONN *) han->odbcHandle, &statHandle);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return TRUE;

	if (!SQL_SUCCEEDED
			(colsNum =
			SQLColumns(statHandle, NULL, 0, NULL, 0, (SQLCHAR *) table, SQL_NTS, NULL,
									0)))
		goto __ERROR;

	GB.NewArray(POINTER(&fields), sizeof(ODBC_FIELD), 0);

	while (SQL_SUCCEEDED(SQLFetch(statHandle)))
	{
		field = GB.Add(POINTER(&fields));
		
		SQLGetData(statHandle, SQLColumns_COLUMN_NAME, SQL_C_CHAR, field_name, sizeof(field_name), 0);
		
		field->name = GB.NewZeroString((char *)field_name);

		if (!SQL_SUCCEEDED
				(SQLGetData
				(statHandle, SQLColumns_SQL_DATA_TYPE, SQL_C_CHAR, coltype,
					sizeof(coltype), 0)))
			goto __ERROR;

		fprintf(stderr, "table_init: %s -> %s\n", field->name, coltype);
		field->type = atol((char *)coltype);

		if (!SQL_SUCCEEDED
				(SQLGetData
				(statHandle, SQLColumns_COLUMN_SIZE, SQL_C_CHAR, precision,
					sizeof(precision), 0)))
			goto __ERROR;

		field->len = atol((char *)precision);
	}

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	
	info->nfield = GB.Count(fields);
	
	GB.Alloc(POINTER(&info->field), sizeof(DB_FIELD) * info->nfield);
	
	i = 0;

	for (i = 0; i < colsNum; i++)
	{
		field = &fields[i];
		f = &info->field[i];
		
		f->name = field->name;

		f->type = conv_type(field->type);

		f->length = 0;
		if (f->type == GB_T_STRING)
			f->length = field->len;
	}
	
	GB.FreeArray(POINTER(&fields));
	
	return FALSE;
	
__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	return TRUE;
}



/*****************************************************************************

	table_index()

	Initialize an info structure from table primary index.

	<handle> is the database handle.
	<table> is the table name.
	<info> points at the info structure.

	This function must initialize the following info fields:
	- info->nindex must contain the number of fields in the primary index.
	- info->index is a int[] giving the index of each index field in
		info->fields.

	This function must be called after table_init().

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int table_index(DB_DATABASE *db, const char *table, DB_INFO * info)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\ttable_index - info %p,info->nindex %d\n",info,info->nindex);
fflush(stderr);
#endif
  SQLCHAR field_name[256];
	int inx[256];
	SQLHSTMT statHandle, statHandle2;
	//SQLRETURN V_OD_erg;
	SQLRETURN nReturn = -1;
	SQLRETURN retcode;
	SQLCHAR szKeyName[256];
	SQLCHAR szColumnName[256];
	SQLCHAR query[101] = "SELECT * FROM ";
	SQLSMALLINT colsNum;
	int i, n;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;
	ODBC_FIELD *fields, *field;
	ODBC_RESULT *res;

	strcpy((char *)&query[14], table);


	colsNum = 0;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, (ODBC_CONN *) han->odbcHandle, &statHandle2);

	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return TRUE;

	retcode = SQLColumns(statHandle2, NULL, 0, NULL, 0, (SQLCHAR *) table, SQL_NTS, NULL, 0);
	if (!SQL_SUCCEEDED(retcode))
		goto __ERROR_2;

	GB.NewArray(POINTER(&fields), sizeof(ODBC_FIELD), 0);

	while (SQL_SUCCEEDED(SQLFetch(statHandle2)))
	{
		field = GB.Add(&fields);
		
		if (!SQL_SUCCEEDED(SQLGetData(statHandle2, SQLColumns_COLUMN_NAME, SQL_C_CHAR, field_name, sizeof(field_name), 0)))
			strcpy((char *)field_name, "?");

		field->name = GB.NewZeroString((char *)field_name);
	}

	retcode = SQLNumResultCols(statHandle2, &colsNum);

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle2);

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, han->odbcHandle, &statHandle);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return TRUE;

	res = malloc(sizeof(ODBC_RESULT));

	if (!SQL_SUCCEEDED(nReturn = SQLPrimaryKeys(statHandle, NULL, 0, NULL, 0, (SQLCHAR *)table, SQL_NTS)))
		goto __ERROR;

	retcode = SQLNumResultCols(statHandle, &colsNum);

	i = 0;

	while (SQL_SUCCEEDED(SQLFetch(statHandle)))
	{
		if (!SQL_SUCCEEDED(SQLGetData (statHandle, 4, SQL_C_CHAR, szColumnName, sizeof(szColumnName), 0)))
			strcpy((char *) szColumnName, "?");

		if (!SQL_SUCCEEDED(SQLGetData(statHandle, 6, SQL_C_CHAR, szKeyName, sizeof(szKeyName), 0)))
			strcpy((char *) szKeyName, "?");

		for (n = 0; n < colsNum; n++)
		{
			if (strcmp(fields[i].name, (char *)szColumnName) == 0)
			{
				inx[i] = n;
				break;
			}
		}

		i++;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);

	GB.Alloc(POINTER(&info->index), sizeof(int) * i);
	info->nindex = i;

	for (n = 0; n < i; n++)
		info->index[n] = inx[n];

	free(res);

	for (i = 0; i < GB.Count(fields); i++)
		GB.FreeString(&fields[i].name);
	
	GB.FreeArray(POINTER(&fields));
	
	return FALSE;
	
__ERROR_2:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle2);
	return TRUE;
	
__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	free(res);
	return TRUE;
}


/*****************************************************************************

	table_release()

	Free the info structure filled by table_init() and/or table_index()

	<handle> is the database handle.
	<info> points at the info structure.

*****************************************************************************/

static void table_release(DB_DATABASE *db, DB_INFO * info)
{
	/* All is done outside the driver */
}


/*****************************************************************************

	table_exist()

	Returns if a table exists

	<handle> is the database handle.
	<table> is the table name.

	This function returns TRUE if the table exists, and FALSE if not.

*****************************************************************************/

static int table_exist(DB_DATABASE *db, const char *table)
{

#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\ttable_exist\n");
fflush(stderr);
#endif
	SQLHSTMT statHandle;
	//SQLRETURN V_OD_erg;
	SQLRETURN nReturn = -1;
	SQLRETURN retcode;
	SQLCHAR szTableName[101] = "";
	SQLCHAR szTableType[101] = "";
	SQLCHAR szTableRemarks[301] = "";
	SQLLEN nIndicatorName;
	SQLLEN nIndicatorType;
	SQLLEN nIndicatorRemarks;
	int compare = -1;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;
	int len;

	len = strlen(table);
	if (len == 0)
		return FALSE;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, han->odbcHandle, &statHandle);

	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return FALSE; //V_OD_erg;

	// EXECUTE OUR SQL/CALL
	if (SQL_SUCCESS != (nReturn = SQLTables(statHandle, 0, 0, 0, 0, 0, 0, 0, 0)))
		return FALSE; //nReturn;

	SQLBindCol(statHandle, SQLTables_TABLE_NAME, SQL_C_CHAR, szTableName,
						sizeof(szTableName), &nIndicatorName);
	SQLBindCol(statHandle, SQLTables_TABLE_TYPE, SQL_C_CHAR, szTableType,
						sizeof(szTableType), &nIndicatorType);
	SQLBindCol(statHandle, SQLTables_REMARKS, SQL_C_CHAR, szTableRemarks,
						sizeof(szTableRemarks), &nIndicatorRemarks);
	// GET RESULTS
	nReturn = SQLFetch(statHandle);
	while ((nReturn == SQL_SUCCESS || nReturn == SQL_SUCCESS_WITH_INFO)
				&& compare != 0)
	{
		//printf("le tabelle in comparazione %s : %s\n",szTableName,table);
		compare = strncmp((char *)szTableName, table, len);
		szTableName[0] = '\0';
		szTableType[0] = '\0';
		szTableRemarks[0] = '\0';
		nReturn = SQLFetch(statHandle);
	}

	// FREE STATEMENT
	nReturn = SQLFreeHandle(SQL_HANDLE_STMT, statHandle);

	if (compare == 0)
		return TRUE;
	else
		return FALSE;
}


/*****************************************************************************

	table_list()

	Returns an array containing the name of each table in the database

	<handle> is the database handle.
	<tables> points to a variable that will receive the char* array.

	This function returns the number of tables, or -1 if the command has
	failed.

	Be careful: <tables> can be NULL, so that just the count is returned.

*****************************************************************************/

static int table_list(DB_DATABASE *db, char ***tables)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\ttable_list\n");
fflush(stderr);
#endif
	ODBC_TABLES tablelist, *curtable;
	SQLHSTMT statHandle;
	//SQLRETURN V_OD_erg;
	SQLRETURN nReturn = -1;
	SQLRETURN retcode;
	SQLCHAR szTableName[101] = "";
	SQLCHAR szTableType[101] = "";
	SQLCHAR szTableRemarks[301] = "";
	SQLLEN nIndicatorName;
	SQLLEN nIndicatorType;
	SQLLEN nIndicatorRemarks;
	int tablenum = 0;
	int i;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, han->odbcHandle, &statHandle);

	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return (-1);

	curtable = &tablelist;

	// EXECUTE OUR SQL/CALL
	if (SQL_SUCCESS != (nReturn = SQLTables(statHandle, 0, 0, 0, 0, 0, 0, 0, 0)))
		goto __ERROR;

	SQLBindCol(statHandle, SQLTables_TABLE_NAME, SQL_C_CHAR, szTableName,
						sizeof(szTableName), &nIndicatorName);
	SQLBindCol(statHandle, SQLTables_TABLE_TYPE, SQL_C_CHAR, szTableType,
						sizeof(szTableType), &nIndicatorType);
	SQLBindCol(statHandle, SQLTables_REMARKS, SQL_C_CHAR, szTableRemarks,
						sizeof(szTableRemarks), &nIndicatorRemarks);
	// GET RESULTS
	nReturn = SQLFetch(statHandle);

	if (nReturn != SQL_SUCCESS && nReturn != SQL_SUCCESS_WITH_INFO)
		goto __ERROR;

	while (nReturn == SQL_SUCCESS || nReturn == SQL_SUCCESS_WITH_INFO)
	{
		if (strcmp((char *)szTableType, "TABLE") == 0)
		{
			tablenum = tablenum + 1;
			curtable->tablename = malloc(sizeof(szTableName));
			curtable->next = malloc(sizeof(ODBC_TABLES));
			strcpy(curtable->tablename, (char *)szTableName);
			curtable = (ODBC_TABLES *) curtable->next;
		}
		szTableName[0] = '\0';
		szTableType[0] = '\0';
		szTableRemarks[0] = '\0';
		nReturn = SQLFetch(statHandle);
	}

	// FREE STATEMENT
	nReturn = SQLFreeHandle(SQL_HANDLE_STMT, statHandle);

	GB.NewArray(tables, sizeof(char *), tablenum);

	curtable = &tablelist;
	for (i = 0; i < tablenum; i++)
	{
		(*tables)[i] = GB.NewZeroString(curtable->tablename);
		free(curtable->tablename);
		curtable = (ODBC_TABLES *) curtable->next;
	}

	curtable = &tablelist;
	int g;

	for (i = tablenum; i > 0; i--)
	{
		for (g = 0; g < i; g++)
			curtable = (ODBC_TABLES *) curtable->next;

		free(curtable);
		curtable = &tablelist;
	}

	return (tablenum);
	
__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	return (-1);
}


/*****************************************************************************

	table_primary_key()

	Returns a string representing the primary key of a table.

	<handle> is the database handle.
	<table> is the table name.
	<key> points to a string that will receive the primary key.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int table_primary_key(DB_DATABASE *db, const char *table, char ***primary)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\ttable_primary_key\n");
fflush(stderr);
#endif
	SQLHSTMT statHandle;
	SQLRETURN retcode;
	//SQLRETURN V_OD_erg;
	SQLRETURN nReturn = -1;
	SQLCHAR szKeyName[101] = "";
	SQLCHAR szColumnName[101] = "";
	SQLCHAR query[101] = "SELECT * FROM ";
	SQLSMALLINT colsNum;
	int i;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	// CREATE A STATEMENT
	ODBC_RESULT *res;

	strcpy((char *)&query[14], table);

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, han->odbcHandle, &statHandle);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return TRUE;

	res = malloc(sizeof(ODBC_RESULT));

	/*if (!SQL_SUCCEEDED
			(nReturn = SQLPrimaryKeys(statHandle, 0, 0, 0, SQL_NTS, (SQLCHAR *)table, SQL_NTS)))*/
	if (!SQL_SUCCEEDED(nReturn = SQLPrimaryKeys(statHandle, (SQLCHAR *)"", 0, (SQLCHAR *)"", 0, (SQLCHAR *)table, SQL_NTS)))
		goto __ERROR;
	// GET RESULTS

	SQLNumResultCols(statHandle, &colsNum);

	GB.NewArray(primary, sizeof(char *), 0);
	i = 0;

	while (SQL_SUCCEEDED(SQLFetch(statHandle)))
	{

		if (!SQL_SUCCEEDED
				(SQLGetData
				(statHandle, 4, SQL_C_CHAR, &szColumnName[0], sizeof(szColumnName), 0)))
			strcpy((char *) szColumnName, "?");

		if (!SQL_SUCCEEDED
				(SQLGetData
				(statHandle, 6, SQL_C_CHAR, &szKeyName[0], sizeof(szKeyName), 0)))
			strcpy((char *) szKeyName, "?");

		*(char **)GB.Add(primary) = GB.NewZeroString((char *)szColumnName);
		i++;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	free(res);
	return FALSE;

__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	free(res);
	return TRUE;
}


/*****************************************************************************

	table_is_system()

	Returns if a table is a system table.

	<handle> is the database handle.
	<table> is the table name.

	This function returns TRUE if the table is a system table, and FALSE if
	not.

*****************************************************************************/

static int table_is_system(DB_DATABASE *db, const char *table)
{
	return FALSE;
}


/*****************************************************************************

	table_delete()

	Deletes a table.

	<handle> is the database handle.
	<table> is the table name.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int table_delete(DB_DATABASE *db, const char *table)
{
	int exit;
	const char *query = "DROP TABLE &1";

	exit = do_query(db, "Cannot delete table: &1", NULL, query, 1, table);
	/* BM: What's that ???
	if (exit == 0)
	{
		exit = do_query(db, "Cannot delete table:&1", NULL, "COMMIT", 0);
	}*/

	return (exit);
}



/*****************************************************************************

	table_create()

	Creates a table.

	<handle> is the database handle.
	<table> is the table name.
	<fields> points to a linked list of field descriptions.
	<key> is the primary key.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int table_create(DB_DATABASE *db, const char *table, DB_FIELD *fields, char **primary, const char *type_not_used)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\ttable_create\n");
fflush(stderr);
#endif

	DB_FIELD *fp;
	int comma;
	char *type;
	int i, exit;

	DB.Query.Init();

	DB.Query.Add("CREATE TABLE ");
	DB.Query.Add(table);
	DB.Query.Add(" ( ");

	comma = FALSE;
	for (fp = fields; fp; fp = fp->next)
	{
		if (comma)
			DB.Query.Add(", ");
		else
			comma = TRUE;

		DB.Query.Add(fp->name);

//AB autoincrement field is mapped to Integer because this is Database dependent
		if (fp->type == DB_T_SERIAL)
			DB.Query.Add(" INTEGER ");
//AB Blob field
		else if (fp->type == DB_T_BLOB)
			DB.Query.Add(" LONG VARBINARY ");

		switch (fp->type)
		{
			case GB_T_BOOLEAN:
				type = "TINYINT"; //
				break;
			case GB_T_INTEGER:
				type = "INTEGER";
				break;
			case GB_T_FLOAT:
				type = "DOUBLE";
				break;
			case GB_T_DATE:
				type = "DATE"; // AB changed the field name used by default from Timestamp to Date
				break;
// New datatype BIGINT 64 bits
			case GB_T_LONG:
				type = "BIGINT";
				break;
			case GB_T_STRING:

				if (fp->length <= 0)
					type = "TEXT";
				else
				{
					sprintf(_buffer, "VARCHAR(%d)", fp->length);
					type = _buffer;
				}

				break;

			default:
				type = "TEXT";
				break;
		}

		DB.Query.Add(" ");
		DB.Query.Add(type);

		if (fp->def.type != GB_T_NULL)
		{
			DB.Query.Add(" NOT NULL DEFAULT ");
			DB.FormatVariant(&_driver, &fp->def, DB.Query.AddLength);
		}
		else if (DB.StringArray.Find(primary, fp->name) >= 0)
		{
			DB.Query.Add(" NOT NULL ");
		}
	}

	if (primary)
	{
		DB.Query.Add(", PRIMARY KEY (");

		for (i = 0; i < GB.Count(primary); i++)
		{
			if (i > 0)
				DB.Query.Add(",");

			DB.Query.Add(primary[i]);
		}

		DB.Query.Add(")");
	}

	DB.Query.Add(" )");

	exit = do_query(db, "Cannot create table: &1", NULL, DB.Query.Get(), 0);


	if (exit == 0)
	{
		exit = do_query(db, "Cannot create table:&1", NULL, "COMMIT", 0);
	}

	return (exit);

}


/*****************************************************************************

	field_exist()

	Returns if a field exists in a given table

	<handle> is the database handle.
	<table> is the table name.
	<field> is the field name.

	This function returns TRUE if the field exists, and FALSE if not.

*****************************************************************************/

static int field_exist(DB_DATABASE *db, const char *table, const char *field)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_exist\n");
fflush(stderr);
#endif
	SQLCHAR colname[256];
	SQLSMALLINT colsNum;
	SQLHSTMT statHandle;
	SQLRETURN retcode;
	//SQLRETURN V_OD_erg;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, (ODBC_CONN *) han->odbcHandle, &statHandle);

	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return FALSE; //V_OD_erg;

//printf("field exist dopo l'handler\n");

	if (!SQL_SUCCEEDED
			(colsNum =
			SQLColumns(statHandle, NULL, 0, NULL, 0, (SQLCHAR *) table, SQL_NTS, NULL,
									0)))
		goto __ERROR;

//printf("field exist dopo la SQLColumn : %u\n",colsNum);

	while (SQL_SUCCEEDED(SQLFetch(statHandle)))
	{
		SQLGetData(statHandle, SQLColumns_COLUMN_NAME, SQL_C_CHAR, colname, sizeof(colname), 0);
		if (strcmp((char *)colname, field) == 0)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
			return TRUE;
		}
	}

__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	return FALSE;
}


/*****************************************************************************

	field_list()

	Returns an array containing the name of each field in a given table

	<handle> is the database handle.
	<table> is the table name.
	<fields> points to a variable that will receive the char* array.

	This function returns the number of fields, or -1 if the command has
	failed.

	Be careful: <fields> can be NULL, so that just the count is returned.

*****************************************************************************/

static int field_list(DB_DATABASE *db, const char *table, char ***fields)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_list\n");
fflush(stderr);
#endif
	SQLCHAR field_name[256];
	SQLSMALLINT colsNum;
	SQLHSTMT statHandle;
	SQLRETURN retcode;
	//SQLRETURN V_OD_erg;
	ODBC_CONN *han = (ODBC_CONN *)db->handle;


	colsNum = 0;
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, (ODBC_CONN *) han->odbcHandle, &statHandle);

	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return (-1);

	retcode = SQLColumns(statHandle, NULL, 0, NULL, 0, (SQLCHAR *) table, SQL_NTS, NULL, 0);
	if (!SQL_SUCCEEDED(retcode))
		goto __ERROR;

	if (fields)
		GB.NewArray(fields, sizeof(char *), 0);

	while (SQL_SUCCEEDED(SQLFetch(statHandle)))
	{
		if (!SQL_SUCCEEDED
				(SQLGetData
				(statHandle, SQLColumns_COLUMN_NAME, SQL_C_CHAR, field_name,
					sizeof(field_name), 0)))
			strcpy((char *)field_name, "?");

		if (fields)
			*((char **)GB.Add(fields)) = GB.NewZeroString((char *)field_name);
		colsNum++;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);

	return colsNum;
	
__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	return -1;
}


/*****************************************************************************

	field_info()

	Get field description

	<handle> is the database handle.
	<table> is the table name.
	<field> is the field name.
	<info> points to a structure filled by the function.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int field_info(DB_DATABASE *db, const char *table, const char *field, DB_FIELD * info)
{
#ifdef ODBC_DEBUG_HEADER
fprintf(stderr,"[ODBC][%s][%d]\n",__FILE__,__LINE__);
fprintf(stderr,"\tfield_info\n");
fflush(stderr);
#endif
	SQLCHAR colname[128];
	SQLCHAR coltype[100];
	SQLCHAR precision[100];
	SQLHSTMT statHandle;
	SQLHSTMT statHandle1;
	SQLRETURN retcode;
	//SQLRETURN V_OD_erg;
	SQLLEN auton=SQL_FALSE;
	int i;
	int size_query = 32 + strlen(table) + strlen(field);
	char query[size_query];

	ODBC_CONN *han = (ODBC_CONN *)db->handle;
	ODBC_CONN *han1 = (ODBC_CONN *)db->handle;

	*precision = 0;

	/*strncpy((char *)&query[0], "SELECT ",7);
	strncpy((char *)&query[7], field,strlen(field));
	strncpy((char *)&query[strlen(field)+7], " FROM ",6);
	strncpy((char *)&query[strlen(field)+13], table,strlen(table));
	query[strlen(field)+14+strlen(table)]='\0';
	strncpy((char *)&query[strlen(field)+13+strlen(table)],"\0\n\0\n",4);*/

	snprintf(query, size_query, "SELECT %s FROM %s", field, table);

	for (i = 0; i < 100; i++)
		coltype[i] = '\0';

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, (ODBC_CONN *) han->odbcHandle, &statHandle);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		return TRUE;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, (ODBC_CONN *) han1->odbcHandle, &statHandle1);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
	{
		SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
		return TRUE;
	}

	retcode = SQLExecDirect(statHandle1, (SQLCHAR *) query , SQL_NTS);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		goto __ERROR;

	retcode = SQLColAttribute (statHandle1,1,SQL_DESC_AUTO_UNIQUE_VALUE,NULL,0,NULL,&auton);

	if (!SQL_SUCCEEDED(retcode = SQLColumns(statHandle, NULL, 0, NULL, 0, (SQLCHAR *) table, SQL_NTS, NULL,0)))
		goto __ERROR;

	while (SQL_SUCCEEDED(SQLFetch(statHandle)))
	{
		SQLGetData(statHandle, SQLColumns_COLUMN_NAME, SQL_C_CHAR, colname, sizeof(colname), 0);

		if (strcmp((char *)colname, field) == 0)
		{
			SQL_SUCCEEDED(SQLGetData(statHandle, SQLColumns_SQL_DATA_TYPE, SQL_C_CHAR, coltype, sizeof(coltype), 0));
			SQL_SUCCEEDED(SQLGetData(statHandle, SQLColumns_COLUMN_SIZE, SQL_C_CHAR, precision, sizeof(precision), 0));
			break;
		}
	}

	info->name = NULL;
	info->type = conv_type(atol((char *)coltype));
	info->length = 0;
	if (*precision)
		info->length = atol((char *) precision);

	if (info->type == GB_T_STRING)
	{
		if (info->length < 0)
			info->length = 0;
	}

	if (auton == SQL_TRUE)
		info->type = DB_T_SERIAL;

	info->def.type = GB_T_NULL;

	info->collation = NULL;

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle1);
	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	return FALSE;
	
__ERROR:

	SQLFreeHandle(SQL_HANDLE_STMT, statHandle1);
	SQLFreeHandle(SQL_HANDLE_STMT, statHandle);
	return TRUE;
}


/*****************************************************************************

	index_exist()

	Returns if an index exists in a given table

	<handle> is the database handle.
	<table> is the table name.
	<field> is the index name.

	This function returns TRUE if the index exists, and FALSE if not.

*****************************************************************************/

static int index_exist(DB_DATABASE *db, const char *table, const char *index)
{
	return FALSE;
}


/*****************************************************************************

	index_list()

	Returns an array containing the name of each index in a given table

	<handle> is the database handle.
	<table> is the table name.
	<indexes> points to a variable that will receive the char* array.

	This function returns the number of indexes, or -1 if the command has
	failed.

	Be careful: <indexes> can be NULL, so that just the count is returned.

*****************************************************************************/

static int index_list(DB_DATABASE *db, const char *table, char ***indexes)
{
	return (-1);
}


/*****************************************************************************

	index_info()

	Get index description

	<handle> is the database handle.
	<table> is the table name.
	<field> is the index name.
	<info> points to a structure filled by the function.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int index_info(DB_DATABASE *db, const char *table, const char *index,	DB_INDEX * info)
{
	return TRUE;
}


/*****************************************************************************

	index_delete()

	Deletes an index.

	<handle> is the database handle.
	<table> is the table name.
	<index> is the index name.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int index_delete(DB_DATABASE *db, const char *table, const char *index)
{
	return TRUE;
}


/*****************************************************************************

	index_create()

	Creates an index.

	<handle> is the database handle.
	<table> is the table name.
	<index> is the index name.
	<info> points to a structure describing the index.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int index_create(DB_DATABASE *db, const char *table, const char *index, DB_INDEX * info)
{
	return TRUE;
}


/*****************************************************************************

	database_exist()

	Returns if a database exists

	<handle> is any database handle.
	<name> is the database name.

	This function returns TRUE if the database exists, and FALSE if not.

*****************************************************************************/

static int database_exist(DB_DATABASE *db, const char *name)
{
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	return han->dsn_name && strcmp(han->dsn_name, name) == 0;
}



/*****************************************************************************

	database_list()

	Returns an array containing the name of each database

	<handle> is any database handle.
	<databases> points to a variable that will receive the char* array.

	This function returns the number of databases, or -1 if the command has
	failed.

	Be careful: <databases> can be NULL, so that just the count is returned.

*****************************************************************************/

static int database_list(DB_DATABASE *db, char ***databases)
{
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	if (!han->dsn_name)
		return 0;
	
	if (databases)
	{
		GB.NewArray(databases, sizeof(char *), 1);
		(*databases)[0] = GB.NewZeroString(han->dsn_name);
	}

	return (1);
}


/*****************************************************************************

	database_is_system()

	Returns if a database is a system database.

	<handle> is any database handle.
	<name> is the database name.

	This function returns TRUE if the database is a system database, and
	FALSE if not.

*****************************************************************************/

static int database_is_system(DB_DATABASE *db, const char *name)
{
	//GB.Error("ODBC does not implement this function");
	return FALSE;
}

/*****************************************************************************

	table_type()
	Not Valid in postgresql

	<handle> is the database handle.
	<table> is the table name.

*****************************************************************************/

static char *table_type(DB_DATABASE *db, const char *table, const char *type)
{
	if (type)
		GB.Error("ODBC does not have any table types");
	return NULL;
}

/*****************************************************************************

	database_delete()

	Deletes a database.

	<handle> is the database handle.
	<name> is the database name.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int database_delete(DB_DATABASE *db, const char *name)
{
	//GB.Error("ODBC does not implement this function");
	return TRUE;
}


/*****************************************************************************

	database_create()

	Creates a database.

	<handle> is the database handle.
	<name> is the database name.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int database_create(DB_DATABASE *db, const char *name)
{
	//GB.Error("ODBC does not implement this function");
	return TRUE;
}


/*****************************************************************************

	user_exist()

	Returns if a user exists.

	<handle> is any database handle.
	<name> is the user name.

	This function returns TRUE if the user exists, and FALSE if not.

*****************************************************************************/

static int user_exist(DB_DATABASE *db, const char *name)
{
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	return strcmp(han->user_name, name) == 0;
	//  GB.Error("ODBC does not implement this function");
}


/*****************************************************************************

	user_list()

	Returns an array containing the name of each user.

	<handle> is the database handle.
	<users> points to a variable that will receive the char* array.

	This function returns the number of users, or -1 if the command has
	failed.

	Be careful: <users> can be NULL, so that just the count is returned.

*****************************************************************************/

static int user_list(DB_DATABASE *db, char ***users)
{
	ODBC_CONN *han = (ODBC_CONN *)db->handle;

	if (users)
	{
		GB.NewArray(users, sizeof(char *), 1);
		(*users)[0] = GB.NewZeroString(han->user_name);
	}

	//GB.Error("ODBC does not implement this function");
	return (1);
}


/*****************************************************************************

	user_info()

	Get user description

	<handle> is the database handle.
	<name> is the user name.
	<info> points to a structure filled by the function.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int user_info(DB_DATABASE *db, const char *name, DB_USER *info)
{
	//GB.Error("ODBC does not implement this function");
	return TRUE;
}


/*****************************************************************************

	user_delete()

	Deletes a user.

	<handle> is any database handle.
	<name> is the user name.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int user_delete(DB_DATABASE *db, const char *name)
{
	//GB.Error("ODBC can't delete users");
	return TRUE;
}


/*****************************************************************************

	user_create()

	Creates a user.

	<handle> is the database handle.
	<name> is the user name.
	<info> points to a structure describing the user.

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int user_create(DB_DATABASE *db, const char *name, DB_USER * info)
{
	//GB.Error("ODBC can't create users");
	return TRUE;
}


/*****************************************************************************

	user_set_password()

	Change the user password.

	<handle> is the database handle.
	<name> is the user name.
	<password> is the new password

	This function returns TRUE if the command has failed, and FALSE if
	everything was OK.

*****************************************************************************/

static int user_set_password(DB_DATABASE *db, const char *name, const char *password)
{
	//GB.Error("ODBC can't set user's password");
	return TRUE;
}

/*****************************************************************************

	The driver interface

*****************************************************************************/

DECLARE_DRIVER(_driver, "odbc");

/*****************************************************************************

	The component entry and exit functions.

*****************************************************************************/

int EXPORT GB_INIT(void)
{
	GB.GetInterface("gb.db", DB_INTERFACE_VERSION, &DB);
	DB.Register(&_driver);

	return -1;
}

void EXPORT GB_EXIT()
{
}
