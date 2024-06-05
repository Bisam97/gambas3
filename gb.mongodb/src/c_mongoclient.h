/***************************************************************************

  c_mongoclient.h

  gb.mongodb component

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#ifndef __C_MONGOCLIENT_H
#define __C_MONGOCLIENT_H

#include "main.h"

#ifndef __C_MONGOCLIENT_C

extern GB_DESC MongoClientDesc[];

#else

#define THIS ((CMONGOCLIENT *)_object)

#endif

typedef
	struct {
		GB_BASE ob;
		mongoc_uri_t *uri;
		mongoc_client_t *client;
		mongoc_database_t *database;
		char *database_name;
	}
	CMONGOCLIENT;

#endif /* __MAIN_H */
