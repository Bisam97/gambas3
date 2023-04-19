/***************************************************************************

  gbc_read.c

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

#define __GBC_READ_C

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>

#include "gb_common.h"
#include "gb_common_buffer.h"
#include "gb_common_case.h"
#include "gb_error.h"
#include "gb_table.h"
#include "gb_file.h"

#include "gbc_compile.h"
#include "gbc_class.h"
#include "gbc_preprocess.h"
#include "gbc_help.h"
#include "gbc_read.h"

#define DEBUG

static bool is_init = FALSE;
static COMPILE *comp;
static const char *source_ptr;
static const char *_line_start;
static int source_length;
static bool _begin_line = FALSE;
static bool _no_quote = FALSE;

static bool _prep = FALSE;
static int _prep_index;

static PATTERN _last_pattern;

static char ident_car[256];
static char first_car[256];
static char noop_car[256];
static char canres_car[256];

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
	
	JOB->line = 1;
	JOB->max_line = FORM_FIRST_LINE - 1;
	
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


static void READ_exit(void)
{
	char *name = NULL;
	int len;
	int index;
	bool local;
	bool has_static;
	char c;

	local = FALSE;
	
	COMPILE_enum_class_start(name, len);
	
	while (len)
	{
		if (len == 1 && *name == '-')
		{
			local = TRUE;
		}
		else if (*name != '.')
		{
			has_static = FALSE;

			for(;;)
			{
				c = name[len - 1];
				if (c == '!')
				{
					has_static = TRUE;
					len--;
				}
				else if (c == '?')
					len--;
				else
					break;
			}

			if (TABLE_find_symbol(JOB->class->table, name, len, &index))
			{
				if (local)
					index = CLASS_add_class_unused(JOB->class, index);
				else
					index = CLASS_add_class_exported_unused(JOB->class, index);

				JOB->class->class[index].has_static = has_static;
			}
		}
		
		COMPILE_enum_class_next(name, len);
	}

	if (COMP_verbose)
		printf("\n");
}

static int get_utf8_length(const char *str, int len)
{
  int ulen = 0;
	int i;

  for (i = 0; i < len; i++)
  {
    if ((str[i] & 0xC0) != 0x80)
      ulen++;
  }

  return ulen;
}


int READ_get_column()
{
	const char *p = source_ptr;
	int col = 0;
	
	while (p > comp->source)
	{
		if (*p == '\n')
		{
			p++;
			break;
		}
		p--;
		col++;
	}
	
	return get_utf8_length(p, col + 1);
}


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
			str = COMP_res_info[index].name; //TABLE_get_symbol_name(COMP_res_table, index);
			if (ispunct(*str))
				snprintf(COMMON_buffer, COMMON_BUF_MAX, "%s%s%s", before, str, after);
			else
				strcpy(COMMON_buffer, str);
			break;
			
		case RT_INTEGER:
			snprintf(COMMON_buffer, COMMON_BUF_MAX, "%s%d%s", before, PATTERN_signed_index(*pattern), after);
			break;

		case RT_NUMBER:
		case RT_IDENTIFIER:
		case RT_CLASS:
			snprintf(COMMON_buffer, COMMON_BUF_MAX, "%s%s%s", before, TABLE_get_symbol_name(JOB->class->table, index), after);
			break;

		case RT_STRING:
		case RT_TSTRING:
			if (_no_quote)
				snprintf(COMMON_buffer, COMMON_BUF_MAX, "\"%s\"", TABLE_get_symbol_name(JOB->class->string, index));
			else
				strcpy(COMMON_buffer, "string");
			break;

		/*case RT_COMMAND:
			snprintf(COMMON_buffer, COMMON_BUF_MAX, "#%d", index);
			break;*/

		case RT_NEWLINE:
			strcpy(COMMON_buffer, "end of line");
			break;

		case RT_END:
			strcpy(COMMON_buffer, "end of file");
			break;

		case RT_SUBR:
			//snprintf(COMMON_buffer, COMMON_BUF_MAX, "%s%s%s", bafore, COMP_subr_info[index].name, after);
			strcpy(COMMON_buffer, COMP_subr_info[index].name);
			break;

		default:
			sprintf(COMMON_buffer, "%s?%02X.%02X.%d?%s", before, PATTERN_type(*pattern), PATTERN_flag(*pattern), (int)PATTERN_index(*pattern), after);
	}

	return COMMON_buffer;
}

void THROW_UNEXPECTED(PATTERN *pattern)
{
	switch (PATTERN_type(*pattern))
	{
		case RT_NEWLINE: case RT_END:
			THROW("Unexpected end of line");
		case RT_STRING: case RT_TSTRING:
			THROW("Unexpected string");
		default:
			THROW("Unexpected &1", READ_get_pattern(pattern));
	}
}

void READ_dump_pattern(PATTERN *pattern)
{
	int type = PATTERN_type(*pattern);
	int index = PATTERN_index(*pattern);

	/*pos = (int)(pattern - JOB->pattern);
	if (pos < 0 || pos >= JOB->pattern_count)
		return;
		
	printf("%d ", pos);*/

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
	/*else if (type == RT_COMMAND)
		printf("COMMAND      %d\n", index);*/
	else
		printf("?            %d\n", index);

	_no_quote = FALSE;
}


#if 0
static inline unsigned char get_char_offset(int offset)
{
	offset += source_ptr;

	if (offset >= source_length || offset < 0)
		return 0;
	else
		return (unsigned char)(comp->source[offset]);
}
#endif

#if 0
static unsigned char get_char(void)
{
	return get_char_offset(0);
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

static void add_pattern_no_dump(int type, int index)
{
	comp->pattern[comp->pattern_count] = _last_pattern = PATTERN_make(type, index);
	comp->pattern_pos[comp->pattern_count] = source_ptr - _line_start;
	comp->pattern_count++;
}

static void add_pattern(int type, int index)
{
	add_pattern_no_dump(type, index);
	printf("% 4d ", comp->pattern_pos[comp->pattern_count - 1]);
	READ_dump_pattern(&comp->pattern[comp->pattern_count - 1]);
}

#else

#define add_pattern_no_dump add_pattern

//#define add_pattern(_type, _index) comp->pattern[comp->pattern_count++] = PATTERN_make((_type), (_index));

static inline void add_pattern(int type, int index)
{
	comp->pattern[comp->pattern_count] = _last_pattern = PATTERN_make(type, index);
	comp->pattern_pos[comp->pattern_count] = source_ptr - _line_start;
	comp->pattern_count++;
}

#endif

static PATTERN get_last_last_pattern()
{
	if (comp->pattern_count > 1)
		return comp->pattern[comp->pattern_count - 2];
	else
		return NULL_PATTERN;
}

#define get_last_pattern() (_last_pattern)
//(comp->pattern[comp->pattern_count - 1])

static void jump_to_next_prep(void)
{
	unsigned char car;
	const char *line_start;
	
	for (;;)
	{
		line_start = source_ptr;
		
		for(;;)
		{
			car = get_char();
			if (!car)
				return;
			if (car == '\n' || !car || !isspace(car))
				break;
			source_ptr++;
		}
		
		if (car == '#')
		{
			source_ptr = line_start;
			return;
		}
		
		for(;;)
		{
			car = get_char();
			if (!car)
				return;
			source_ptr++;
			if (car == '\n')
				break;
		}
		
		add_pattern(RT_NEWLINE, comp->line);
		comp->line++;
	}
}

INLINE static void add_newline()
{
	int action = PREP_CONTINUE;
	
	if (_prep)
	{
		int line = comp->line;
		
		add_pattern_no_dump(RT_NEWLINE, comp->line);
		action = PREP_analyze(&comp->pattern[_prep_index]);
		_prep = FALSE;
		
		comp->pattern_count = _prep_index;
		comp->line = line;
	}
	
	if (action == PREP_LINE)
		comp->line = PREP_next_line;
	
	// Void lines must act as void help comments
	if (comp->line > 0 && PATTERN_is_newline(get_last_pattern()))
		HELP_add_at_current_line("\n");
	
	add_pattern(RT_NEWLINE, comp->line);
	comp->line++;

	if (action == PREP_IGNORE)
		jump_to_next_prep();
}

static void add_end()
{
	add_pattern(RT_END, 0);
	comp->line++;
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
	len = 0;

	for(;;)
	{
		car = next_char();
		if (!ident_car[car])
			break;
		len++;
	}

	if (car != '}')
		THROW("Missing '}'");
	
	if (len == 0)
		THROW("Void identifier");
	
	source_ptr++;
	start++;

	if (PATTERN_is(last_pattern, RS_EVENT) || PATTERN_is(last_pattern, RS_RAISE))
	{
		start--;
		len++;
		*((char *)start) = ':';
	}

	if (PATTERN_is(last_pattern, RS_EXCL))
	{
		index = TABLE_add_symbol(comp->class->string, start, len);
		type = RT_STRING;
	}
	else
		index = TABLE_add_symbol(comp->class->table, start, len);
	
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
	const char *end;
	int i;

	start = end = source_ptr;
	len = 0;
	newline = 0;
	jump = FALSE;
	p = (char *)source_ptr;

	for(;;)
	{
		source_ptr++;
		car = get_char();

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

			if (car == '\n')
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
				end = source_ptr;
				jump = TRUE;
				comp->line += newline;
				newline = 0;
			}
			else
				*p = car;
		}
	}

	if (len > 0)
	{
		index = TABLE_add_symbol(comp->class->string, start + 1, len);
		add_pattern(RT_STRING, index);
	}
	else
		add_pattern(RT_STRING, VOID_STRING_INDEX);

	source_ptr = end + 1;
	//for (i = 0; i < newline; i++)
	//	add_newline();
}


#if 0
static void add_command()
{
	unsigned char car;
	const char *start;
	int len;

	start = source_ptr;
	len = 0;

	for(;;)
	{
		source_ptr++;
		car = get_char();
		if (car == '\n' || !car)
			break;
		len++;
	}

	if (len)
	{
		//TABLE_add_symbol(comp->class->string, start + 1, len, NULL, &index);
		if (len == 7 && !strncasecmp(start + 1, "SECTION", 7))
			add_pattern(RT_COMMAND, RC_SECTION);
	}

	add_newline();
}
#endif

void READ_do(void)
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

	comp = JOB;

	READ_init();
	PREP_init();
	
	//add_pattern(RT_NEWLINE, 0);

	source_ptr = comp->source;
	source_length = BUFFER_length(comp->source);
	_line_start = source_ptr;
	_begin_line = TRUE;
	_prep = FALSE;
	_last_pattern = NULL_PATTERN;

	//while (source_ptr < source_length)
	for(;;)
	{
		car = get_char();
		goto *jump_char[(int)first_car[car]];
		
	__ERROR:
		
		THROW("Syntax error");
			
	__SPACE:

		source_ptr++;
		continue;

	__NEWLINE:

		source_ptr++;
		add_newline();
		_begin_line = TRUE;
		_line_start = source_ptr;
		continue;

	__COMMENT:

		source_ptr++;
		
		if (HELP_is_help_comment(source_ptr))
			HELP_add_at_current_line(source_ptr);
		
		car = get_char();
		while (car != '\n')
		{
			source_ptr++;
			car = get_char();
		}

		_begin_line = FALSE;
		continue;

	__STRING:
			
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
			_prep = TRUE;
			_prep_index = comp->pattern_count;
			
			add_identifier();
			_begin_line = FALSE;
			continue;
		}
		else
			goto __OPERATOR;
	
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
	
	add_newline();

	JOB->max_line = JOB->line;

	add_end();
	add_end();
	add_end();
	add_end();

	PREP_exit();
	READ_exit();
}

