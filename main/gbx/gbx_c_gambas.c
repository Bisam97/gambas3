/***************************************************************************

  gbx_c_gambas.c

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

#define __GBX_C_GAMBAS_C

#include <syslog.h>

#include "gbx_info.h"
#include "gbx_local.h"
#include "gbx_compare.h"
#include "gb_type_common.h"
#include "gb_file.h"
#include "gbx_date.h"
#include "gbx_exec.h"

#ifndef GBX_INFO

#include "gb_error.h"
#include "gbx_api.h"
#include "gbx_class.h"
#include "gbx_event.h"
#include "gbx_c_array.h"
#include "gbx_c_gambas.h"


static int get_arg_count(void)
{
	if (DEBUG_inside_eval && DEBUG_info)
	{
		if (DEBUG_info->fp && DEBUG_info->fp->vararg)
			return DEBUG_info->bp - DEBUG_info->pp;
	}
	else
	{
		if (FP && FP->vararg)
			return BP - PP;
	}
	return 0;
}

static VALUE *get_arg(int i)
{
	if (DEBUG_inside_eval && DEBUG_info)
		return &DEBUG_info->pp[i];
	else
		return &PP[i];
}

BEGIN_PROPERTY(Param_Count)

	GB_ReturnInteger(get_arg_count());

END_PROPERTY


BEGIN_PROPERTY(Param_Max)

	GB_ReturnInteger(get_arg_count() - 1);

END_PROPERTY


BEGIN_METHOD(Param_get, GB_INTEGER index)

	int index = VARG(index);
	VALUE *arg;

	if (index < 0 || index >= get_arg_count())
		THROW(E_BOUND);

	arg = get_arg(index);
	VALUE_conv(arg, T_VARIANT);
	TEMP = *arg;
	//VALUE_conv(&TEMP, T_VARIANT);

END_METHOD


BEGIN_PROPERTY(Param_All)

	GB_ARRAY all;
	int nparam = get_arg_count();
	int i;
	VALUE *arg;
	
	GB_ArrayNew(POINTER(&all), T_VARIANT, nparam);
	
	for (i = 0; i < nparam; i++)
	{
		arg = get_arg(i);
		VALUE_conv(arg, T_VARIANT);
		GB_StoreVariant((GB_VARIANT *)arg, GB_ArrayGet(all, i));
	}
	
	GB_ReturnObject(all);

END_PROPERTY


BEGIN_METHOD_VOID(Param_next)

	int *index = (int *)GB_GetEnum();

	if (*index >= get_arg_count())
		GB_StopEnum();
	else
	{
		VALUE *arg = get_arg(*index);
		VALUE_conv(arg, T_VARIANT);
		TEMP = *arg;
		(*index)++;
	}

END_METHOD


BEGIN_PROPERTY(Param_Name)

	GB_ReturnConstZeroString(EXEC_unknown_name);

END_PROPERTY


/*BEGIN_PROPERTY(Param_Property)

	GB_ReturnBoolean(EXEC_unknown_property);

END_PROPERTY*/

BEGIN_PROPERTY(Param_EventName)

	GB_ReturnConstZeroString(EVENT_Name);

END_PROPERTY

#endif

GB_DESC NATIVE_Param[] =
{
	GB_DECLARE_STATIC("Param"),

	GB_STATIC_PROPERTY_READ("Count", "i", Param_Count),
	GB_STATIC_PROPERTY_READ("Max", "i", Param_Max),
	GB_STATIC_PROPERTY_READ("All", "Variant[]", Param_All),

	GB_STATIC_PROPERTY_READ("Name", "s", Param_Name),
	GB_STATIC_PROPERTY_READ("EventName", "s", Param_EventName),
	//GB_STATIC_PROPERTY_READ("Property", "b", Param_Property),

	GB_STATIC_METHOD("_get", "v", Param_get, "(Index)i"),
	GB_STATIC_METHOD("_next", "v", Param_next, NULL),

	//GB_STATIC_METHOD("Copy", "Variant[]", CPARAM_copy, "[(Start)i(Length)i]"),

	GB_END_DECLARE
};

GB_DESC NATIVE_Gambas[] =
{
	GB_DECLARE_STATIC("gb"),

	#include "gb_constant_temp.h"

	GB_CONSTANT("NewLine", "s", "\n"),
	GB_CONSTANT("Tab", "s", "\t"),
	GB_CONSTANT("Cr", "s", "\r"),
	GB_CONSTANT("Lf", "s", "\n"),
	GB_CONSTANT("CrLf", "s", "\r\n"),

	GB_END_DECLARE
};

