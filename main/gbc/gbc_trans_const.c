/***************************************************************************

  gbc_trans_const.c

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __GBC_TRANS_CONST_C

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gbc_compile.h"

#include "gbc_trans.h"
#include "gb_code.h"

#define NO_GAMBAS_CASE_REPLACEMENT
#include "gambas.h"


//#define DEBUG 1

#define MAX_STACK 32
	
TRANS_CONST_VALUE _stack[MAX_STACK];
int _stack_ptr = 0;

typedef
	struct {
		const char *name;
		int value;
	}
	TRANS_CONST_DEF;
	
TRANS_CONST_DEF _const_gb[] = {
	#undef GB_CONSTANT
	#define GB_CONSTANT(_name, _type, _value) { _name, _value }
	#include "gb_constant_temp.h"
	{ NULL, 0}
};

static void throw_error(void)
{
	THROW("Constant expression expected");
}

static short get_nparam(PATTERN *tree, int count, int *pindex)
{
	PATTERN pattern;
	int nparam = 0;
	int index = *pindex;

	if (index < count)
	{
		pattern = tree[index + 1];
		if (PATTERN_is_param(pattern))
		{
			index++;
			nparam = PATTERN_index(pattern);
		}

		while (index < count)
		{
			pattern = tree[index + 1];
			if (!PATTERN_is_param(pattern))
				break;
			index++;
		}

		*pindex = index;
	}

	return (short)nparam;
}

static TRANS_CONST_VALUE *push_stack()
{
	if (_stack_ptr >= MAX_STACK)
		THROW("Constant expression too complex");
	return &_stack[_stack_ptr++];
}


static TRANS_CONST_VALUE *pop_stack(int nparam)
{
	_stack_ptr -= nparam;
	if (_stack_ptr < 0)
		ERROR_panic("Bad stack computation");
	return &_stack[_stack_ptr];
}


static void push_integer(int val)
{
	TRANS_CONST_VALUE *value = push_stack();
	value->type = T_INTEGER;
	value->value._integer = val;
}


static void push_long(int64_t val)
{
	TRANS_CONST_VALUE *value = push_stack();
	
	if (val < INT_MIN || val > INT_MAX)
	{
		value->type = T_LONG;
		value->value._long = val;
	}
	else
	{
		value->type = T_INTEGER;
		value->value._integer = (int)val;
	}
}


static void push_number(int index)
{
	TRANS_CONST_VALUE *value;
	TRANS_NUMBER number;

	if (TRANS_get_number(index, &number))
		THROW(E_SYNTAX);

	value = push_stack();
	
	if (number.type == T_INTEGER)
	{
		value->type = T_INTEGER;
		value->value._integer = number.ival;
	}
	else if (number.type == T_LONG)
	{
		value->type = T_LONG;
		value->value._long = number.lval;
	}
	else if (number.type == T_FLOAT)
		throw_error();

	if (number.complex)
		throw_error();
}


#if 0
static void push_string(int index, bool trans)
{
	TRANS_DECL decl;
	SYMBOL *sym;
	int len;

	if (index == VOID_STRING_INDEX)
		len = 0;
	else
	{
		sym = TABLE_get_symbol(JOB->class->string, index);
		len = sym->len;
	}

	if (len == 0)
	{
		CODE_push_void_string();
	}
	else if (len == 1 && !trans)
	{
		CODE_push_char(*(sym->name));
	}
	else
	{
		//CLEAR(&decl);

		if (trans)
			decl.type = TYPE_make_simple(T_CSTRING);
		else
			decl.type = TYPE_make_simple(T_STRING);
		decl.index = NO_SYMBOL;
		decl.value = index;

		CODE_push_const(CLASS_add_constant(JOB->class, &decl));
	}
	
	push_type_id(T_STRING);
}
#endif


static void push_identifier(int index, bool point)
{
	CLASS_SYMBOL *sym = CLASS_get_symbol(JOB->class, index);
	int type;
	CONSTANT *constant;
	TRANS_CONST_VALUE *value;

#if DEBUG
	fprintf(stderr, "trans_identifier: %.*s\n", sym->symbol.len, sym->symbol.name);
#endif
	
	if (point)
	{
		push_integer(index);
		return;
	}
	
	if (!TYPE_is_null(sym->global.type) && TYPE_get_kind(sym->global.type) == TK_CONST)
	{
		if (!TYPE_is_public(sym->global.type))
			sym->global_used = TRUE;
			//fprintf(stderr, "_last_symbol_used = %.*s / global\n", sym->symbol.len, sym->symbol.name);
		
		constant = &JOB->class->constant[sym->global.value];
		type = TYPE_get_id(constant->type);
		if (type >= T_BOOLEAN && type <= T_LONG)
		{
			push_integer(constant->value);
			return;
		}
		
		throw_error();
	}
	
	if (!TABLE_compare_ignore_case_len(sym->symbol.name, sym->symbol.len, "gb", 2))
	{
		value = push_stack();
		value->type = T_OBJECT;
		return;
	}
	
	throw_error();
}

static void trans_subr(int subr, short nparam)
{
	TRANS_CONST_VALUE *sp;
	SUBR_INFO *info = &COMP_subr_info[subr];

	if (nparam < info->min_param)
		THROW("Not enough arguments to &1()", info->name);
	else if (nparam > info->max_param)
		THROW("Too many arguments to &1()", info->name);

	sp = pop_stack(nparam);
	
	if (subr == SUBR_SizeOf)
	{
		static char _sizeof[9] = { 0, 1, 1, 2, 4, 8, 4, 8, 8 };
		int type;
		
		if (sp->type != T_INTEGER)
			THROW(E_BADARG);
		
		type = sp[0].value._integer;
		if (type < T_BOOLEAN)
			THROW(E_BADARG);
		if (type > T_DATE)
			throw_error();
		
		push_integer(_sizeof[type]);
		return;
	}
	
	throw_error();
}


static void trans_operation(short op, short nparam)
{
	COMP_INFO *info = &COMP_res_info[op];
	int64_t v1 = 0, v2 = 0;
	TRANS_CONST_VALUE *sp = pop_stack(nparam);
	const char *name;
	TRANS_CONST_DEF *p;

	if (nparam >= 1)
	{
		v1 = sp[0].type == T_LONG ? sp[0].value._long : (int64_t)sp[0].value._integer;
		if (nparam >= 2)
			v2 = sp[1].type == T_LONG ? sp[1].value._long : (int64_t)sp[1].value._integer;
	}
	
	switch (info->value)
	{
		case OP_PLUS:
			
			push_long(v1 + v2);
			break;
			
		case OP_MINUS:
			
			if (nparam == 1)
				push_long((- v1));
			else
				push_long(v1 - v2);
			break;
			
		case OP_STAR:
			
			push_long(v1 * v2);
			break;
			
		case OP_SLASH: case OP_DIV:
		
			push_long(v1 / v2);
			break;
			
		case OP_MOD:
			
			push_long(v1 % v2);
			break;
			
		case OP_NOT:
			
			push_long(~v1);
			break;
			
		case OP_AND:
			
			push_long(v1 & v2);
			break;
			
		case OP_OR:
			
			push_long(v1 | v2);
			break;
			
		case OP_XOR:
			
			push_long(v1 ^ v2);
			break;
			
		case OP_SHL: case OP_ASL:
			
			if (v2 < 0 || v2 > 64)
				THROW(E_BADARG);
			push_long(((v1 << v2) & 0x7FFFFFFFFFFFFFFFLL) | (v1 & 0x8000000000000000LL));
			break;
			
		case OP_SHR: case OP_ASR:
			
			if (v2 < 0 || v2 > 64)
				THROW(E_BADARG);
			push_long(((v1 >> v2) & 0x7FFFFFFFFFFFFFFFLL) | (v1 & 0x8000000000000000LL));
			break;
			
		case OP_LSL:
			
			if (v2 < 0 || v2 > 64)
				THROW(E_BADARG);
			push_long(v1 << v2);
			break;
		
		case OP_LSR:
			
			if (v2 < 0 || v2 > 64)
				THROW(E_BADARG);
			push_long(v1 >> v2);
			break;
			
		case OP_PT:
			
			if (nparam != 2)
				throw_error();

			if (sp[0].type != T_OBJECT)
				throw_error();
			
			name = TABLE_get_symbol_name(JOB->class->table, v2);
			
			for (p = _const_gb; p->name; p++)
			{
				if (!strcasecmp(name, p->name))
				{
					push_integer(p->value);
					break;
				}
			}
			
			if (!p->name)
				THROW("Unknown constant: gb.&1", name);
			
			break;
			
		default:
			throw_error();
	}
}


static void trans_const_from_tree(TRANS_TREE *tree, int count)
{
	static void *jump[] = {
		&&__CONTINUE, &&__CONTINUE, &&__RESERVED, &&__IDENTIFIER, &&__INTEGER, &&__NUMBER, &&__STRING, &&__TSTRING, &&__CONTINUE, &&__SUBR, &&__CLASS, &&__CONTINUE, &&__CONTINUE
	};
	
	int i, op;
	short nparam;
	PATTERN pattern;

	pattern = NULL_PATTERN;

	#if DEBUG
	fprintf(stderr, "-------------------------------------------- %d\n", _type_level);
	#endif

	i = 0;
	
	for (i = 0; i < count; i++)
	{
		TRANS_tree_index = i;
		pattern = tree[i];

		goto *jump[PATTERN_type(pattern)];
		
	__INTEGER:
		
		push_integer(PATTERN_signed_index(pattern));
		continue;

	__NUMBER:
		
		push_number(PATTERN_index(pattern));
		continue;

	__IDENTIFIER:
	
		push_identifier(PATTERN_index(pattern), PATTERN_is_point(pattern));
		continue;

	__SUBR:
	
		nparam = get_nparam(tree, count, &i);
		trans_subr(PATTERN_index(pattern), nparam);
		continue;

	__CLASS:
	__STRING:
	__TSTRING:
	
		throw_error();
		continue;
		
	__RESERVED:

		if (PATTERN_is(pattern, RS_TRUE))
		{
			push_integer(-1);
		}
		else if (PATTERN_is(pattern, RS_FALSE))
		{
			push_integer(0);
		}
		else
		{
			op = PATTERN_index(pattern);
			nparam = get_nparam(tree, count, &i);
			trans_operation((short)op, nparam);
		}
		
	__CONTINUE:
		;
	}
	
	TRANS_tree_index = -1;
}

TRANS_CONST_VALUE *TRANS_const(void)
{
	TRANS_TREE *tree;
	int tree_length;
	
	_stack_ptr = 0;

	TRANS_tree(FALSE, &tree, &tree_length);

	JOB->step = JOB_STEP_TREE;
	trans_const_from_tree(tree, tree_length);
	JOB->step = JOB_STEP_CODE;

	//FREE(&tree);
	
	if (_stack_ptr != 1)
		THROW(E_SYNTAX);
	
	return &_stack[0];
}
