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

//-------------------------------------------------------------------------

GB_INTERFACE GB EXPORT;

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------

/*
GB_DESC PostgresqlHelperDesc[] =
{
	GB_DECLARE_STATIC("_PostgresqlHelper"),
	
	GB_STATIC_METHOD("_exit", NULL, PostgresqlHelper_exit, NULL),
	
	GB_STATIC_METHOD("Open", "p", PostgresqlHelper_Open, "(Host)s(Port)s(Name)s(User)s(Password)s(Timeout)i(Options)Collection;"),
	GB_STATIC_METHOD("Close", NULL, PostgresqlHelper_Close, "(Database)p"),
	GB_STATIC_METHOD("Query", "p", PostgresqlHelper_Query, "(Database)p(Query)s"),
	GB_STATIC_METHOD("FreeResult", NULL, PostgresqlHelper_FreeResult, "(Result)p"),
	GB_STATIC_METHOD("GetResultCount", "i", PostgresqlHelper_GetResultCount, "(Result)p"),
	GB_STATIC_METHOD("GetResultField", "s", PostgresqlHelper_GetResultField, "(Result)p(Field)i"),
	GB_STATIC_METHOD("GetResultData", "Variant[]", PostgresqlHelper_GetResultData,"(Result)p(Index)l(Next)b"),
	GB_STATIC_METHOD("QuoteString", "s", PostgresqlHelper_QuoteString, "(Value)s(AddE)b"),
	GB_STATIC_METHOD("GetResultBlob", "s", PostgresqlHelper_GetResultBlob,"(Result)p(Index)i(Field)i"),
	GB_STATIC_METHOD("QuoteBlob", "s", PostgresqlHelper_QuoteBlob, "(Value)s(AddE)b"),
	GB_STATIC_METHOD("GetFieldInfo", NULL, PostgresqlHelper_GetFieldInfo, "(Result)p(NoCollation)b"),
	GB_STATIC_METHOD("GetLastError", "i", PostgresqlHelper_GetLastError, NULL),
	
	GB_STATIC_PROPERTY_READ("Type", "i", PostgresqlHelper_Type),
	GB_STATIC_PROPERTY_READ("Length", "i", PostgresqlHelper_Length),
	GB_STATIC_PROPERTY_READ("Default", "v", PostgresqlHelper_Default),
	GB_STATIC_PROPERTY_READ("Collation", "s", PostgresqlHelper_Collation),
	
	GB_END_DECLARE
};

//-------------------------------------------------------------------------

GB_DESC *GB_CLASSES [] EXPORT =
{
	PostgresqlHelperDesc,
	NULL
};
*/

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}
