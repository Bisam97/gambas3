/***************************************************************************

  gbx_struct.c

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

#define __GBX_STRUCT_C

#include "gb_alloc.h"
#include "gbx_struct.h"

void *CSTRUCT_create_static(void *ref, CLASS *class, char *addr)
{
	CSTATICSTRUCT *object;
	
  ALLOC(&object, sizeof(CSTATICSTRUCT));

  object->ob.class = class;
  object->ob.ref = 0;
	object->ref = ref;
	object->addr = addr;

  class->count++;

	if (ref != STRUCT_CONST)
		OBJECT_REF_CHECK(ref);
  
	//fprintf(stderr, "CSTRUCT_create_static: %s %p ref = %p addr = %p\n", class->name, object, ref, addr);
	
	return object;
}

int CSTRUCT_get_size(CLASS *class)
{
	return class->size - sizeof(CSTRUCT);
}

void CSTRUCT_release(CSTRUCT *ob)
{
	if (ob->ref != STRUCT_CONST)
		OBJECT_UNREF(ob->ref);
}
