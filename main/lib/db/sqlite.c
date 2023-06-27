/***************************************************************************

  sqlite.c

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

#define __SQLITE_C

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

#include "main.h"


/* Internal function to check whether a file is a sqlite database file */

static bool is_sqlite2_database(const char *filename)
{
  FILE* fp;
  bool res;
  char magic_text[48];

  fp = fopen(filename, "r");
  if (!fp)
    return FALSE;

  res = fread(magic_text, 1, 47, fp) == 47;
  fclose(fp);

  if (!res)
    return FALSE;

  magic_text[47] = '\0';

  if (strcmp(magic_text, "** This file contains an SQLite 2.1 database **"))
    return FALSE;

  return TRUE;
}

static bool is_sqlite3_database(const char *filename)
{
	FILE *fp;
	bool res;
	char magic_text[16];

	fp = fopen(filename, "r");
	if (!fp)
		return FALSE;

	res = fread(magic_text, 1, 15, fp) == 15;
	fclose(fp);

	if (!res)
		return FALSE;

	magic_text[15] = '\0';

	if (strcmp(magic_text, "SQLite format 3"))
		return FALSE;

	return TRUE;
}

static bool is_database_file(const char *filename)
{
	return is_sqlite3_database(filename) || is_sqlite2_database(filename);
}

static char *find_database(const char *name, const char *hostName)
{
	char *dbhome = NULL;
	char *fullpath = NULL;
	char *path;

	/* Does Name includes fullpath */
	if (*name == '/')
	{
		if (is_database_file(name))
			return (char *)name;
	}

	/* Hostname contains home area */
	fullpath = GB.NewZeroString(hostName);
	fullpath = GB.AddChar(fullpath, '/');
	fullpath = GB.AddString(fullpath, name, 0);
	
	path = GB.FileName(fullpath, GB.StringLength(fullpath));
	GB.FreeString(&fullpath);
	
	if (is_database_file(path))
		return path;

	/* Check the GAMBAS_SQLITE_DBHOME setting */
	dbhome = getenv("GAMBAS_SQLITE_DBHOME");

	if (dbhome != NULL)
	{
		fullpath = GB.NewZeroString(dbhome);
		fullpath = GB.AddChar(fullpath, '/');
		fullpath = GB.AddString(fullpath, name, 0);

		path = GB.FileName(fullpath, GB.StringLength(fullpath));
		GB.FreeString(&fullpath);

		if (is_database_file(path))
			return path;
	}

	fullpath = GB.NewZeroString(GB.TempDir());
	fullpath = GB.AddString(fullpath, "/sqlite/", 0);
	fullpath = GB.AddString(fullpath, name, 0);
	GB.FreeStringLater(fullpath);

	if (is_database_file(fullpath))
		return fullpath;

	return NULL;
}

//-------------------------------------------------------------------------

static int open_database(DB_DESC *desc, DB_DATABASE * db)
{
	char *host;
	const char *db_fullpath = NULL;
	bool ver2 = FALSE;

	if (!desc->name) // memory database
		goto __SQLITE;

	host = desc->host;
	if (!host)
		host = "";

	db_fullpath = find_database(desc->name, host);
	if (!db_fullpath)
	{
		GB.Error("Unable to locate database `&1` in `&2`", desc->name, host);
		return TRUE;
	}
					 	
	ver2 = is_sqlite2_database(db_fullpath);
	
	if (ver2)
		goto __SQLITE2;
	else
		goto __SQLITE3;
	
__SQLITE:
  
  GB.Component.Load("gb.db.sqlite3");
  GB.Error(NULL);
	
	if (GB.Component.Exist("gb.db.sqlite3"))
		goto __SQLITE3;
	else
		goto __SQLITE2;

__SQLITE2:
		DB_TryAnother("sqlite2");
		return TRUE;

__SQLITE3:
		DB_TryAnother("sqlite3");
		return TRUE;
}


//-------------------------------------------------------------------------

static int database_is_system(DB_DATABASE * db, const char *name)
{
	return FALSE;
}

/*****************************************************************************

  The driver interface

*****************************************************************************/

DB_DRIVER DB_sqlite_pseudo_driver =
{
	.name = "sqlite",
	.Open = open_database,
	.Database.IsSystem = database_is_system
};

