/***************************************************************************

  c_mongocollection.c

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

#define __C_MONGOCOLLECTION_C

#include "helper.h"
#include "c_mongocollection.h"



//--------------------------------------------------------------------------

BEGIN_METHOD_VOID(MongoCollection_free)

	mongoc_collection_destroy(THIS->collection);
	GB.Unref(POINTER(&THIS->client));

END_METHOD

BEGIN_PROPERTY(MongoCollection_Name)

	GB.ReturnNewZeroString(mongoc_collection_get_name(THIS->collection));

END_PROPERTY

//--------------------------------------------------------------------------

GB_DESC MongoCollectionDesc[] = {

	GB_DECLARE("MongoCollection", sizeof(CMONGOCOLLECTION)),
	GB_NOT_CREATABLE(),

	GB_METHOD("_free", NULL, MongoCollection_free, NULL),
	
	GB_PROPERTY_READ("Name", "s", MongoCollection_Name),

	GB_END_DECLARE
};
