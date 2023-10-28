/***************************************************************************

  eval_read.c

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

#define __EVAL_READ_C

#include <ctype.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gb_table.h"

#include "eval.h"
#include "eval_read.h"

//#define DEBUG 1

const char *READ_source_ptr;
#define source_ptr READ_source_ptr

static bool is_init = FALSE;
static bool _begin_line = FALSE;

static char ident_car[256];
static char first_car[256];
static char noop_car[256];
static char canres_car[256];

#undef isspace
#define isspace(_c) (((uchar)_c) <= ' ')
#undef isdigit
#define isdigit(_c) ((_c) >= '0' && (_c) <= '9')

enum
{
	GOTO_BREAK,
	GOTO_SPACE,
	GOTO_NEWLINE,
	GOTO_COMMENT,
	GOTO_STRING,
	GOTO_IDENT,
	GOTO_QUOTED_IDENT,
	GOTO_ERROR,
	GOTO_SHARP,
	GOTO_NUMBER,
	GOTO_NUMBER_OR_OPERATOR,
	GOTO_OPERATOR
};


static void READ_init(void)
{
	unsigned char i;

	if (!is_init)
	{
		for (i = 0; i < 255; i++)
		{
			ident_car[i] = (i != 0) && ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || (i >= '0' && i <= '9') || strchr("$_?@", i));
			noop_car[i] = ident_car[i] || (i >= '0' && i <= '9') || i <= ' ';
			canres_car[i] = (i != ':') && (i != '.') && (i != '!') && (i != '(');

			if (i == 0)
				first_car[i] = GOTO_BREAK;
			else if (i == '\n')
				first_car[i] = GOTO_NEWLINE;
			else if (i <= ' ')
				first_car[i] = GOTO_SPACE;
			else if (i == '\'')
				first_car[i] = GOTO_COMMENT;
			else if (i == '"')
				first_car[i] = GOTO_STRING;
			else if (i == '#')
				first_car[i] = GOTO_SHARP;
			else if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || i == '$' || i == '_')
				first_car[i] = GOTO_IDENT;
			else if (i == '{')
				first_car[i] = GOTO_QUOTED_IDENT;
			else if (i >= '0' && i <= '9')
				first_car[i] = GOTO_NUMBER;
			else if (i >= 127)
				first_car[i] = GOTO_ERROR;
			else if (i == '+' || i == '-' || i == '&')
				first_car[i] = GOTO_NUMBER_OR_OPERATOR;
			else
				first_car[i] = GOTO_OPERATOR;
		}

		is_init = TRUE;
	}
}

static bool _no_quote = FALSE;
#define BUF_MAX 255
static char _buffer[BUF_MAX + 1];

char *READ_get_pattern(PATTERN *pattern)
{
	int type = PATTERN_type(*pattern);
	int index = PATTERN_index(*pattern);
	const char *str;
	const char *before = _no_quote ? "" : "'";
	const char *after = _no_quote ? "" : "'";

	switch(type)
	{
		case RT_RESERVED:
			//snprintf(COMMON_buffer, COMMON_BUF_MAX, "%s%s%s", before, TABLE_get_symbol_name(COMP_res_table, index), after);
			str = COMP_res_info[index].name;
			if (ispunct((unsigned char)*str))
				snprintf(_buffer, BUF_MAX, "%s%s%s", before, str, after);
			else
				strcpy(_buffer, str);
			break;

		case RT_INTEGER:
			snprintf(_buffer, BUF_MAX, "%s%d%s", before, PATTERN_signed_index(*pattern), after);
			break;

		case RT_NUMBER:
		case RT_IDENTIFIER:
		case RT_CLASS:
			snprintf(_buffer, BUF_MAX, "%s%s%s", before, TABLE_get_symbol_name(EVAL->table, index), after);
			break;

		case RT_STRING:
		case RT_TSTRING:
			if (_no_quote)
				snprintf(_buffer, BUF_MAX, "\"%s\"", TABLE_get_symbol_name(EVAL->string, index));
			else
				strcpy(_buffer, "string");
			break;

		case RT_NEWLINE:
		case RT_END:
			strcpy(_buffer, "end of expression");
			break;

		case RT_SUBR:
			//snprintf(COMMON_buffer, COMMON_BUF_MAX, "%s%s%s", bafore, COMP_subr_info[index].name, after);
			strcpy(_buffer, COMP_subr_info[index].name);
			break;

		case RT_COMMENT:
			strncpy(_buffer, TABLE_get_symbol_name(EVAL->string, index), BUF_MAX);
			_buffer[BUF_MAX] = 0;
			break;
			
		case RT_SPACE:
			snprintf(_buffer, BUF_MAX, "[%d]", index);
			break;

		default:
			sprintf(_buffer, "%s?%08X?%s", before, *pattern, after);
	}

	return _buffer;
}

#if DEBUG
void READ_dump_pattern(PATTERN *pattern)
{
	int type = PATTERN_type(*pattern);
	int index = PATTERN_index(*pattern);
	int pos;

	pos = (int)(pattern - EVAL->pattern);
	if (pos < 0 || pos >= EVAL->pattern_count)
		return;

	printf("%d ", pos);

	if (PATTERN_flag(*pattern) & RT_FIRST)
		printf("!");
	else
		printf(" ");

	if (PATTERN_flag(*pattern) & RT_POINT)
		printf(".");
	else
		printf(" ");

	if (PATTERN_flag(*pattern) & RT_CLASS)
		printf("C");
	else
		printf(" ");

	printf(" ");

	_no_quote = TRUE;

	if (type == RT_RESERVED)
		printf("RESERVED     %s\n", READ_get_pattern(pattern));
	else if (type == RT_INTEGER)
		printf("INTEGER      %s\n", READ_get_pattern(pattern));
	else if (type == RT_NUMBER)
		printf("NUMBER       %s\n", READ_get_pattern(pattern));
	else if (type == RT_IDENTIFIER)
		printf("IDENTIFIER   %s\n", READ_get_pattern(pattern));
	else if (type == RT_CLASS)
		printf("CLASS        %s\n", READ_get_pattern(pattern));
	else if (type == RT_STRING)
		printf("STRING       %s\n", READ_get_pattern(pattern));
	else if (type == RT_TSTRING)
		printf("TSTRING      %s\n", READ_get_pattern(pattern));
	else if (type == RT_NEWLINE)
		printf("NEWLINE      (%d)\n", index);
	else if (type == RT_END)
		printf("END\n");
	else if (type == RT_PARAM)
		printf("PARAM        %d\n", index);
	else if (type == RT_SUBR)
		printf("SUBR         %s\n", READ_get_pattern(pattern));
	else if (type == RT_COMMENT)
		printf("COMMENT      %s\n", READ_get_pattern(pattern));
	else if (type == RT_SPACE)
		printf("SPACE        %s\n", READ_get_pattern(pattern));
	else
		printf("?            %d\n", index);

	_no_quote = FALSE;
}
#endif

#define get_char_offset(_offset) ((unsigned char)source_ptr[(_offset)])
#define get_char() ((unsigned char)(*source_ptr))

static unsigned char next_char(void)
{
	source_ptr++;
	return get_char();
}

#ifdef DEBUG

static void add_pattern(int type, int index)
{
	EVAL->pattern[EVAL->pattern_count++] = PATTERN_make(type, index);
	READ_dump_pattern(&EVAL->pattern[EVAL->pattern_count - 1]);
}

#else

#define add_pattern(_type, _index) EVAL->pattern[EVAL->pattern_count++] = PATTERN_make((_type), (_index));

#endif

static PATTERN get_previous_pattern(int n)
{
	int i;
	PATTERN pattern;
	
	for (i = EVAL->pattern_count - 1; i >= 0; i--)
	{
		pattern = EVAL->pattern[i];
		if (!PATTERN_is_space(pattern))
		{
			n--;
			if (n == 0)
				return pattern;
		}
	}

	return NULL_PATTERN;
}

#define get_last_pattern() get_previous_pattern(1)
#define get_last_last_pattern() get_previous_pattern(2)

static void add_newline()
{
	add_pattern(RT_NEWLINE, 0);
}


static void add_end()
{
	add_pattern(RT_END, 0);
}


#include "gbc_read_temp.h"


static void add_quoted_identifier(void)
{
	unsigned char car;
	const char *start;
	int len;
	int index;
	int type;
	PATTERN last_pattern;

	last_pattern = get_last_pattern();

	type = RT_IDENTIFIER;

	start = source_ptr;
	len = 1;

	for(;;)
	{
		source_ptr++;
		car = get_char();
		len++;
		if (!ident_car[car])
			break;
	}

	source_ptr++;
	
	if (!EVAL->analyze)
	{
		if (car != '}')
			THROW("Missing '}'");
	
		if (len == 2)
			THROW("Void identifier");
	}
	else
	{
		if (!car)
			len--;
		
		if (car != '}' || len <= 2)
			type = RT_ERROR;
	}
	
	if (!EVAL->analyze && PATTERN_is(last_pattern, RS_EXCL))
	{
		index = TABLE_add_symbol(EVAL->string, start + 1, len - 2);
		type = RT_STRING;
	}
	else
	{
		if (!EVAL->rewrite && type != RT_ERROR)
		{
			start++;
			len -= 2;
		}
		index = TABLE_add_symbol(type == RT_ERROR ? EVAL->string : EVAL->table, start, len);
	}

	add_pattern(type, index);
}


static void add_operator()
{
	unsigned char car;
	const char *start;
	const char *end;
	int len;
	int op = NO_SYMBOL;
	int index;

	start = source_ptr;
	end = start;
	len = 1;

	for(;;)
	{
		source_ptr++;

		index = RESERVED_find_word(start, len);
		if (index >= 0)
		{
			op = index;
			end = source_ptr;
		}

		car = get_char();
		//if (!isascii(car) || !ispunct(car))
		if (noop_car[car])
			break;
		len++;
	}

	source_ptr = end;

	/*if (EVAL->analyze && op == RS_QUES)
		op = RS_PRINT;*/

	if (op < 0)
		THROW("Unknown operator");

	add_pattern(RT_RESERVED, op);
}


static int xdigit_val(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	else
		return (-1);
}


static void add_string()
{
	unsigned char car;
	const char *start;
	int len;
	int index;
	ushort newline;
	bool jump;
	char *p;
	int i;

	start = source_ptr;
	len = 0;
	newline = 0;
	jump = FALSE;
	p = (char *)source_ptr;

	//fprintf(stderr, "EVAL_read: add_string: [%d] %s\n", source_ptr - EVAL->source, source_ptr);

	for(;;)
	{
		source_ptr++;
		car = get_char();

		//fprintf(stderr, "EVAL_read: car = %d jump = %d\n", car, jump);

		if (jump)
		{
			if (car == '\n')
				newline++;
			else if (car == '"')
				jump = FALSE;
			else if (!car || !isspace(car))
				break;
		}
		else
		{
			p++;
			len++;

			if (!car || car == '\n')
				THROW("Non terminated string");

			if (car == '\\')
			{
				source_ptr++;
				car = get_char();

				if (car == 'n')
					*p = '\n';
				else if (car == 't')
					*p = '\t';
				else if (car == 'r')
					*p = '\r';
				else if (car == 'b')
					*p = '\b';
				else if (car == 'v')
					*p = '\v';
				else if (car == 'f')
					*p = '\f';
				else if (car == 'e')
					*p = '\x1B';
				else if (car == '0')
					*p = 0;
				else if (car == '\"' || car == '\'' || car == '\\')
					*p = car;
				else
				{
					if (car == 'x')
					{
						i = xdigit_val(get_char_offset(1));
						if (i >= 0)
						{
							car = i;
							i = xdigit_val(get_char_offset(2));
							if (i >= 0)
							{
								car = (car << 4) | (uchar)i;
								*p = car;
								source_ptr += 2;
								continue;
							}
						}
					}

					THROW("Bad character constant in string");
				}
			}
			else if (car == '"')
			{
				p--;
				len--;
				jump = TRUE;
			}
			else
				*p = car;
		}
	}

	p[1] = 0;

	//fprintf(stderr, "EVAL_read: add_string (end): [%d] %s\n", source_ptr - EVAL->source, source_ptr);
	if (len > 0)
	{
		index = TABLE_add_symbol(EVAL->string, start + 1, len);
		add_pattern(RT_STRING, index);
	}
	else
		add_pattern(RT_STRING, VOID_STRING_INDEX);

	for (i = 0; i < newline; i++)
		add_newline();

	source_ptr -= newline;
}


static void add_comment()
{
	unsigned char car;
	const char *start;
	int len;
	int index;
	int type;

	start = source_ptr;
	len = 1;

	for(;;)
	{
		source_ptr++;
		car = get_char();
		if (car == 0 || car == '\n')
			break;
		len++;
	}

	index = TABLE_add_symbol(EVAL->string, start, len);
	type = RT_COMMENT;

	add_pattern(type, index);
}


static void add_string_for_analyze()
{
	unsigned char car;
	const char *start;
	int len;
	int index;
	int type;

	start = source_ptr;
	len = 0;

	for(;;)
	{
		source_ptr++;
		car = get_char();
		if (car == '\\')
		{
			source_ptr++;
			car = get_char();
			len++;
			if (car == 0)
				break;
		}
		else if (car == 0 || car == '\n' || car == '"')
			break;
		len++;
	}

	if (car == '"')
		source_ptr++;

	index = TABLE_add_symbol(EVAL->string, start + 1, len);
	type = RT_STRING;

	add_pattern(type, index);
	//fprintf(stderr, "add_string_for_analyze: %s\n", TABLE_get_symbol_name(EVAL->string, index));
}


static void add_spaces()
{
	unsigned char car;
	int len;

	len = 1;

	for(;;)
	{
		source_ptr++;
		car = get_char();
		if (car > ' ' || car == '\n' || car == 0)
			break;
		len++;
	}

	add_pattern(RT_SPACE, len);
}


void EVAL_read(void)
{
	static const void *jump_char[12] =
	{
		&&__BREAK,
		&&__SPACE,
		&&__NEWLINE,
		&&__COMMENT,
		&&__STRING,
		&&__IDENT,
		&&__QUOTED_IDENT,
		&&__ERROR,
		&&__SHARP,
		&&__NUMBER,
		&&__NUMBER_OR_OPERATOR,
		&&__OPERATOR
	};

	unsigned char car;

	READ_init();

	source_ptr = EVAL->source;
	_begin_line = TRUE;

	for(;;)
	{
		car = get_char();
		goto *jump_char[(int)first_car[car]];

	__ERROR:

		THROW(E_SYNTAX);

	__SPACE:
	
		if (EVAL->analyze)
			add_spaces();
		else
			source_ptr++;
		
		continue;
		
	__NEWLINE:

		source_ptr++;
		add_newline();
		_begin_line = TRUE;
		continue;

	__COMMENT:

		if (EVAL->analyze)
		{
			add_comment();
		}
		else
		{
			do
			{
				source_ptr++;
				car = get_char();
			}
			while (car != '\n' && car != 0);
		}

		_begin_line = FALSE;
		continue;

	__STRING:

		if (EVAL->analyze)
			add_string_for_analyze();
		else
			add_string();
		_begin_line = FALSE;
		continue;

	__IDENT:

		add_identifier();
		_begin_line = FALSE;
		continue;

	__QUOTED_IDENT:

		add_quoted_identifier();
		_begin_line = FALSE;
		continue;

	__SHARP:

		if (_begin_line)
		{
			_begin_line = FALSE;
			if (get_char_offset(1) == '!' && EVAL->analyze)
				add_comment();
			else
				add_identifier();
			continue;
		}

	__NUMBER_OR_OPERATOR:

		if (add_number())
			goto __OPERATOR;

		_begin_line = FALSE;
		continue;

	__NUMBER:

		add_number();
		_begin_line = FALSE;
		continue;

	__OPERATOR:

		add_operator();
		_begin_line = FALSE;
		continue;
	}

__BREAK:

	// We add end markers to simplify the compiler job, when it needs to look
	// at many patterns in one shot.

	add_end();
	add_end();
	add_end();
	add_end();
}

