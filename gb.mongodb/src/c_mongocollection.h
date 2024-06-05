/***************************************************************************

  c_mongocollection.h

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

#ifndef __C_MONGOCOLLECTION_H
#define __C_MONGOCOLLECTION_H

#include "main.h"
#include "c_mongoclient.h"

#ifndef __C_MONGOCOLLECTION_C

extern GB_DESC MongoCollectionDesc[];

#else

#define THIS ((CMONGOCOLLECTION *)_object)

#endif

typedef
	struct {
		GB_BASE ob;
		CMONGOCLIENT *client;
		mongoc_collection_t *collection;
	}
	CMONGOCOLLECTION;

#endif /* __MAIN_H */
