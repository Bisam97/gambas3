/***************************************************************************

  gbc_type.c

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

#define __GBC_TYPE_C

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gbc_type.h"

#include "gbc_class.h"
#include "gbc_compile.h"

typedef
	struct {
		char prefix1;
		char prefix2;
	}
	TYPE_CHECK_PREFIX;


const char *TYPE_name[] = 
{
  "Void", "Boolean", "Byte", "Short", "Integer", "Long", "Single", "Float", "Date",
  "String", "CString", "Variant", "Array", "Pointer", "Class", "Null",
  "Object"
};


#if 0
size_t TYPE_sizeof(TYPE type)
{
  TYPE_ID id = TYPE_get_id(type);

  switch(id)
  {
    case T_BOOLEAN:
      return 1;

    case T_BYTE:
      return 1;

    case T_SHORT:
      return 2;

    case T_INTEGER:
      return 4;

    case T_LONG:
      return 8;

    case T_SINGLE:
      return 4;

    case T_FLOAT:
      return 8;

    case T_DATE:
      return 8;

    case T_STRING:
      return 4;

    case T_VARIANT:
      return 12;

    case T_OBJECT:
      return 4;
      
    case T_POINTER:
    	return 4;

    case T_ARRAY:
      {
        int i;
        size_t size;
        CLASS_ARRAY *array = &JOB->class->array[TYPE_get_value(type)];

        size = 1;
        for (i = 0; i < array->ndim; i++)
          size *= array->dim[i];

        size *= TYPE_sizeof(array->type);

        return (size + 3) & ~3;
      }

    default:
      ERROR_panic("TYPE_sizeof: bad type id");
  }
}
#endif


const char *TYPE_get_short_desc(TYPE type)
{
  static const char *name[] = {
    "",  "b", "i", "i", "i", "l", "g", "f", 
    "d", "s", "s", "p", "v", "?", "?", "?", 
    "o"
    };

  TYPE_ID id;

  id = TYPE_get_id(type);

  if (id == T_ARRAY)
    return "?";
  else
    return name[id];
}


char *TYPE_get_desc(TYPE type)
{
  static char buf[256];

  TYPE_ID id;
  int value;
	CLASS_SYMBOL *sym;

  id = TYPE_get_id(type);
  value = TYPE_get_value(type);

  if (id == T_ARRAY)
  {
    strcpy(buf, TYPE_name[JOB->class->array[value].type.t.id]);
    strcat(buf, "[]");
  }
  else if (id == T_OBJECT)
	{
		if (value == -1)
			strcpy(buf, "Object");
		else
		{
			sym = CLASS_get_symbol(JOB->class, JOB->class->class[value].index);
			sprintf(buf, "%.*s", sym->symbol.len, sym->symbol.name);
		}
	}
  else if (id < T_OBJECT)
  {
    strcpy(buf, TYPE_name[id]);
  }
  else
    *buf = 0;
  
  return buf;
}


bool TYPE_check_prefix(TYPE type, const char *prefix, int len)
{
	static TYPE_CHECK_PREFIX _check[] = {
		{ 0 },         // T_VOID
		{ 'b' },       // T_BOOLEAN
		{ 'i', 'n' },  // T_BYTE
		{ 'i', 'n' },  // T_SHORT
		{ 'i', 'n' },  // T_INTEGER
		{ 'i', 'n' },  // T_LONG
		{ 'f' },       // T_SINGLE
		{ 'f' },       // T_FLOAT
		{ 'd' },       // T_DATE
		{ 's' },       // T_STRING
		{ 's' },       // T_CSTRING
		{ 'p' },       // T_POINTER
		{ 'v' },       // T_VARIANT
		{ 'a' },       // T_ARRAY,
		{ 0 },         // T_STRUCT,
		{ 0 }
	};
	
	if (len == 0) // no prefix
		return FALSE;
	
	if (type.t.id < T_OBJECT)
	{
		if (len != 1)
			return TRUE;
		
		if (*prefix != _check[type.t.id].prefix1 && *prefix != _check[type.t.id].prefix2)
			return TRUE;
		
		return FALSE;
	}
	
	if (len == 1 && *prefix == 'h')
		return FALSE;
	
	char *class_name = TYPE_get_desc(type);
	
	if (strcasecmp(class_name, "collection") == 0)
		return len != 1 || *prefix != 'c';
	
	if (strcasecmp(class_name, "result") == 0)
		return len != 1 || *prefix != 'r';
	
	return FALSE;
}
