/***************************************************************************

	gbx_subr_conv.c

	(c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA 02110-1301, USA.

***************************************************************************/

#include "gb_common.h"

#include "gbx_value.h"
#include "gbx_subr.h"
#include "gbx_local.h"

#include "gbx_string.h"
#include "gbx_date.h"
#include "gbx_number.h"


void SUBR_is_type(ushort code)
{
	static void *jump[] = {
		&&__BAD, &&__BOOLEAN, &&__BAD, &&__BAD, &&__INTEGER, &&__LONG, &&__BAD, &&__FLOAT,
		&&__DATE, &&__BAD, &&__BAD, &&__BAD, &&__BAD, &&__BAD, &&__NUMBER, &&__NULL,
		&&__BAD, &&__BAD
		};

	bool test;
	char *addr;
	int len;
	VALUE temp;

	SUBR_ENTER_PARAM(1);

	code &= 0x3F;

	if (code == T_NULL)
		goto __NULL;

	//VALUE_conv_string(PARAM);
	SUBR_get_string_len(PARAM, &addr, &len);
	VALUE_from_local_string(&temp, addr, len);

	goto *jump[code];

__BOOLEAN:

	test = temp.type == T_BOOLEAN;
	goto __END;

__INTEGER:

	test = temp.type == T_INTEGER;
	goto __END;

__LONG:

	test = temp.type == T_LONG || temp.type == T_INTEGER;
	goto __END;

__FLOAT:
__NUMBER:

	test = temp.type == T_FLOAT || temp.type == T_LONG || temp.type == T_INTEGER;
	goto __END;

__DATE:

	test = temp.type == T_DATE;
	goto __END;

__NULL:

	test = VALUE_is_null(PARAM);
	goto __END;

__BAD:

	THROW_ILLEGAL();

__END:

	RELEASE(PARAM);
	SP--;
	SP->type = T_BOOLEAN;
	SP->_integer.value = (-test);
	SP++;
}


void SUBR_type(ushort code)
{
	TYPE type;
	int val;
	CLASS *class;

	SUBR_ENTER_PARAM(1);

	if (code & 0x3F)
	{
		if (PARAM->type == T_CLASS)
		{
			class = PARAM->_class.class;
			val = CLASS_sizeof(class) - (class->size - class->off_event);
		}
		else
		{
			val = SUBR_get_integer(PARAM);
			if (val < T_BOOLEAN || val > T_OBJECT)
				THROW_ARG();
			val = TYPE_sizeof_memory(val);
		}
	}
	else
	{
		type = PARAM->type;
		if (type == T_VARIANT)
			type = PARAM->_variant.vtype;

		if (type == T_CSTRING)
			val = T_STRING;
		else if (TYPE_is_object(type) && type != T_NULL)
			val = T_OBJECT;
		else
			val = type;
	}

	RETURN->_integer.value = val;
	RETURN->type = T_INTEGER;

	SUBR_LEAVE();
}


void SUBR_str(void)
{
	char *addr;
	int len;

	SUBR_ENTER_PARAM(1);

	VALUE_to_local_string(PARAM, &addr, &len);
	STRING_new_temp_value(RETURN, addr, len);

	SUBR_LEAVE();
}


void SUBR_val(void)
{
	char *addr;
	int len;

	SUBR_ENTER_PARAM(1);

	if (SUBR_check_string(PARAM))
		VALUE_null(RETURN);
	else
	{
		VALUE_get_string(PARAM, &addr, &len);
		VALUE_from_local_string(RETURN, addr, len);
		VALUE_conv_variant(RETURN);
	}

	SUBR_LEAVE();
}


void SUBR_format(ushort code)
{
	int fmt_type;
	char *format = NULL;
	int len = 0;
	DATE_SERIAL *date;
	char *str;
	int len_str;

	SUBR_ENTER();

	if (NPARAM == 1)
		fmt_type = LF_STANDARD;
	else
	{
		if (PARAM[1].type == T_VARIANT)
			VARIANT_undo(&PARAM[1]);

		if (TYPE_is_string(PARAM[1].type))
		{
			fmt_type = LF_USER;
			VALUE_get_string(&PARAM[1], &format, &len);
			if (!len)
				fmt_type = LF_STANDARD;
		}
		else if (TYPE_is_integer(PARAM[1].type))
		{
			fmt_type = PARAM[1]._integer.value;
			if (fmt_type <= LF_USER || fmt_type >= LF_MAX)
				THROW_ARG();
		}
		else
			THROW_TYPE(T_INTEGER, PARAM[1].type);
	}

	if (PARAM->type == T_VARIANT)
		VARIANT_undo(PARAM);

	if (PARAM->type == T_DATE)
	{
		date = DATE_split(PARAM);
		if (LOCAL_format_date(date, fmt_type, format, len, &str, &len_str))
			THROW(E_FORMAT);
	}
	else
	{
		VALUE_conv_float(PARAM);
		if (LOCAL_format_number(PARAM->_float.value, fmt_type, format, len, &str, &len_str, TRUE))
			THROW(E_FORMAT);
	}

	/*if (NPARAM >= 2)
		RELEASE_STRING(&PARAM[1]);*/

	STRING_new_temp_value(RETURN, str, len_str);

	SUBR_LEAVE();
}


void SUBR_hex_bin(ushort code)
{
	int prec = 0;
	int base;
	int max_prec;

	SUBR_ENTER();

	VALUE_conv(PARAM, T_LONG);

	switch(code >> 8)
	{
		case CODE_BIN:
			base = 2;
			max_prec = 64;
			break;

		case CODE_HEX:
			base = 16;
			max_prec = 16;
			break;

		default:
			if (NPARAM == 0) // Compatibility with 2010 bytecode
			{
				SUBR_upper(1);
				return;
			}
			base = 8;
			max_prec = 22;
			break;
	}

	if (NPARAM == 2)
	{
		VALUE_conv_integer(&PARAM[1]);

		prec = PARAM[1]._integer.value;

		if (prec < 1 || prec > max_prec)
			THROW_ARG();
	}

	NUMBER_int_to_string(PARAM->_long.value, prec, base, RETURN);

	SUBR_LEAVE();
}


void SUBR_pi(ushort code)
{
	SUBR_ENTER();

	if (NPARAM == 0)
	{
		SP->type = T_FLOAT;
		SP->_float.value = M_PI;
		SP++;
	}
	else
	{
		VALUE_conv_float(PARAM);
		PARAM->_float.value = M_PI * PARAM->_float.value;
	}
}


void SUBR_base(ushort code)
{
	int base;
	const char *str;
	int len;
	int64_t result;

	switch (code & 0x3F)
	{
		case 0: // old Pi()

			SP->type = T_FLOAT;
			SP->_float.value = M_PI;
			SP++;
			break;

		case 1: // old Pi(n)

			SP--;
			VALUE_conv_float(SP);
			SP->_float.value *= M_PI;
			SP++;
			break;

		case 2: // Base$()

			{
				SUBR_ENTER_PARAM(2);

				base = SUBR_get_integer(&PARAM[1]);
				if (base < 2 || base > 36)
					THROW_ARG();

				VALUE_conv(PARAM, T_LONG);
				NUMBER_int_to_string(PARAM->_long.value, 0, base, RETURN);

				SUBR_LEAVE();
			}
			break;

		case 3: // Dec()

			{
				SUBR_ENTER_PARAM(2);

				base = SUBR_get_integer(&PARAM[1]);
				if (base < 2 || base > 36)
					THROW_ARG();

				VALUE_get_string(PARAM, &str, &len);

				switch (NUMBER_read_integer(str, len, base, &result))
				{
					case NB_READ_OVERFLOW: THROW_OVERFLOW();
					case NB_READ_SYNTAX: THROW_ARG();
					case NB_READ_LONG:
						RETURN->type = T_LONG;
						RETURN->_long.value = result;
					default:
						RETURN->type = T_INTEGER;
						RETURN->_integer.value = (int)result;
				}

				SUBR_LEAVE();
			}
			break;
	}
}
