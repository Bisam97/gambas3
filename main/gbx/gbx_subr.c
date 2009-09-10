/***************************************************************************

  gbx_subr.c

  (c) 2000-2009 Benoît Minisini <gambas@users.sourceforge.net>

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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/

#define __GBX_SUBR_C

#include "gb_common.h"
#include "gbx_subr.h"
#include "gambas.h"
#include "gbx_api.h"

/*int NPARAM;*/

void SUBR_leave_void(int nparam)
{
  RELEASE_MANY(SP, nparam);
  
  SP->type = T_VOID;
  SP++;
}

void SUBR_leave(int nparam)
{
  BORROW(RP);

  RELEASE_MANY(SP, nparam);

  //*SP++ = *RP;
  COPY_VALUE(SP, RP);
  SP++;
  RP->type = T_VOID;
}


bool SUBR_check_string(VALUE *param)
{
__RETRY:

  if (TYPE_is_string(param->type))
  	return (param->_string.len == 0);

  if (TYPE_is_null(param->type))
    return TRUE;

  if (param->type == T_VARIANT)
  {
    VARIANT_undo(param);
    goto __RETRY;
  }

  THROW(E_TYPE, TYPE_get_name(T_STRING), TYPE_get_name((param)->type));
}


void SUBR_check_integer(VALUE *param)
{
  if (param->type == T_VARIANT)
    VARIANT_undo(param);

  if (TYPE_is_integer(param->type))
    return;

  THROW(E_TYPE, TYPE_get_name(T_INTEGER), TYPE_get_name((param)->type));
}


void SUBR_check_float(VALUE *param)
{
  if (param->type == T_VARIANT)
    VARIANT_undo(param);

  if (TYPE_is_number(param->type))
  {
    VALUE_conv(param, T_FLOAT);
    return;
  }

  THROW(E_TYPE, TYPE_get_name(T_INTEGER), TYPE_get_name((param)->type));
}


int SUBR_get_integer(VALUE *param)
{
  SUBR_check_integer(param);
  return param->_integer.value;
}


void *SUBR_get_pointer(VALUE *param)
{
  if (param->type == T_VARIANT)
    VARIANT_undo(param);

  if (param->type != T_POINTER)
	  THROW(E_TYPE, "Pointer", TYPE_get_name((param)->type));
	
	return (void *)param->_pointer.value;
}


double SUBR_get_float(VALUE *param)
{
  SUBR_check_float(param);
  return param->_float.value;
}


char *SUBR_get_string(VALUE *param)
{
  char *str;

  if (SUBR_check_string(param))
    return "";

  STRING_copy_from_value_temp(&str, param);
  return str;
}


char *SUBR_copy_string(VALUE *param)
{
  char *ret;

  SUBR_check_string(param);
  STRING_copy_from_value_temp(&ret, param);
  /*RELEASE_STRING(param);*/
  return ret;
}


void SUBR_get_string_len(VALUE *param, char **str, int *len)
{
  if (SUBR_check_string(param))
  {
    *str = NULL;
    *len = 0;
  }
  else
	{
		*str = param->_string.addr + param->_string.start;
		*len = param->_string.len;
	}
  //VALUE_get_string(param, str, len);
}

bool SUBR_get_boolean(VALUE *param)
{
	VALUE_conv(param, T_BOOLEAN);
	return param->_boolean.value;
}
