/***************************************************************************

  helper.h

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

#ifndef __HELPER_H
#define __HELPER_H

#include "main.h"
#include "c_mongoclient.h"
#include "c_mongocollection.h"
#include "c_mongoresult.h"

bson_t *HELPER_to_bson(GB_COLLECTION col, bool null_is_void);
bson_t *HELPER_to_bson_with_id(GB_COLLECTION col, char *id, int len);
GB_COLLECTION HELPER_from_bson(const bson_t *bson);

CMONGOCOLLECTION *HELPER_create_collection(CMONGOCLIENT *client, const char *name);
CMONGORESULT *HELPER_create_result(CMONGOCLIENT *client, mongoc_cursor_t *cursor);

#endif /* __MAIN_H */
