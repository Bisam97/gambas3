/***************************************************************************

  gbc_read.c

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
#include "gb_error.h"
#include "gb_table.h"
#include "gb_file.h"

#include "gbc_compile.h"
#include "gbc_class.h"
#include "gbc_read.h"


//#define DEBUG
//#define BIG_COMMENT 1

static bool is_init = FALSE;
static COMPILE *comp;
static const char *source_ptr;
static int source_length;
static bool begin_line = FALSE;

static char ident_car[256];
static char first_car[256];

enum
{
  GOTO_BREAK, 
  GOTO_SPACE, 
  GOTO_COMMENT, 
  GOTO_STRING, 
  GOTO_IDENT, 
  GOTO_QUOTED_IDENT, 
	GOTO_NUMBER,
  GOTO_ERROR,
  GOTO_OTHER
};

static void READ_init(void)
{
  unsigned char i;
  
  JOB->line = 1;
  
  if (!is_init)
  {
    for (i = 0; i < 255; i++)
    {
      ident_car[i] = (i != 0) && ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || (i >= '0' && i <= '9') || strchr("$_?@", i));
      
      if (i == 0)
        first_car[i] = GOTO_BREAK;
      else if (i <= ' ')
        first_car[i] = GOTO_SPACE;
      else if (i == '\'')
        first_car[i] = GOTO_COMMENT;
      else if (i == '"')
        first_car[i] = GOTO_STRING;
      else if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || i == '$' || i == '_')
        first_car[i] = GOTO_IDENT;
      else if (i == '{')
        first_car[i] = GOTO_QUOTED_IDENT;
			else if (i >= '0' && i <= '9')
				first_car[i] = GOTO_NUMBER;
      else if (i >= 127)
        first_car[i] = GOTO_ERROR;
      else
        first_car[i] = GOTO_OTHER;
    }
  
    is_init = TRUE;
  }
}


static void READ_exit(void)
{
  char *p, *p2;
  int index;
  int len;
  bool local;


	local = FALSE;
  p = COMP_classes;

  for(;;)
  {
    p2 = strchr(p, '\n');
    if (p2 == p)
      break;

		len = p2 - p;
		
		if (len == 1 && *p == '-')
		{
			local = TRUE;
		}
		else
		{
			if (TABLE_find_symbol(JOB->class->table, p, len, NULL, &index))
			{
				if (local)
					CLASS_add_class_unused(JOB->class, index);
				else
					CLASS_add_class_exported_unused(JOB->class, index);
			}
		}
		
    p = p2 + 1;
  }

  if (JOB->verbose)
    printf("\n");
}


/*static void char *quote_string(const char *src)
{
  char *p = COMMON_buffer;
  char c;

  for(;;)
  {
    c = *src++;
    if (c == 0)
      break;
    if (p >= &COMMON_buffer[COMMON_BUF_MAX - 2])
      break;
    if (c
  }
}*/

static bool _no_quote = FALSE;

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

    case RT_COMMAND:
    	snprintf(COMMON_buffer, COMMON_BUF_MAX, "#%d", index);
    	break;

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
      sprintf(COMMON_buffer, "%s?%08X?%s", before, *pattern, after);
  }

  return COMMON_buffer;
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
  else if (type == RT_COMMAND)
    printf("COMMAND      %d\n", index);
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

static void add_pattern(int type, int index)
{
  comp->pattern[comp->pattern_count++] = PATTERN_make(type, index);
  READ_dump_pattern(&comp->pattern[comp->pattern_count - 1]);
}

#else

#define add_pattern(_type, _index) comp->pattern[comp->pattern_count++] = PATTERN_make((_type), (_index));

#endif

static PATTERN get_last_last_pattern()
{
  if (comp->pattern_count > 1)
    return comp->pattern[comp->pattern_count - 2];
  else
    return NULL_PATTERN;
}

static PATTERN get_last_pattern()
{
  if (comp->pattern_count > 0)
    return comp->pattern[comp->pattern_count - 1];
  else
    return NULL_PATTERN;
}

static void add_newline()
{
  add_pattern(RT_NEWLINE, comp->line);
  comp->line++;
}


static void add_end()
{
  add_pattern(RT_END, 0);
  comp->line++;
}


static bool is_number()
{
  int pos = 0;
  unsigned char car;
  unsigned char car2;

  car = get_char_offset(pos);

  if (car == '-' || car == '+')
  {
    pos++;
    car = get_char_offset(pos);
  }

  if (isdigit(car))
    return TRUE;

  car2 = toupper(get_char_offset(pos + 1));

  if (car == '&')
  {
    if (car2 == 'H')
    {
      pos += 2;
      goto __HEX;
    }

    if (car2 == 'X')
    {
      pos += 2;
      goto __BIN;
    }

    pos++;
    goto __HEX;
  }
  else if (car == '%')
  {
    pos ++;
    goto __BIN;
  }
  else
    return FALSE;

__HEX:

  car = get_char_offset(pos);
  return (isdigit(car) || index("abcdefABCDEF", car) != NULL);

__BIN:

  car = get_char_offset(pos);
  return (car == '0' || car == '1');
}


static void add_number()
{
  unsigned char car;
  const char *start;
  int index;
  char sign;
  PATTERN last_pattern;

  start = source_ptr;
  car = get_char();

  if (car == '-' || car == '+')
  {
    sign = car;
    car = next_char();
  }
  else
    sign = 0;

  if (car == '&')
  {
    car = toupper(next_char());

    if (car == 'H')
      goto READ_HEXA;
    else if (car == 'X')
      goto READ_BINARY;
    else
    {
      source_ptr--;
      goto READ_HEXA;
    }
  }
  else if (car == '%')
    goto READ_BINARY;
  else
    goto READ_NUMBER;

READ_BINARY:

  for (;;)
  {
    car = next_char();
    if (car != '0' && car != '1')
      break;
  }

  if (car == '&')
    car = next_char();

  goto END;

READ_HEXA:

  for (;;)
  {
    car = next_char();
    if (!isxdigit(car))
      break;
  }

  if (car == '&')
    car = next_char();

  goto END;

READ_NUMBER:

  while (isdigit(car))
    car = next_char();

  if (car == '.')
  {
    do
    {
      car = next_char();
    }
    while (isdigit(car));
  }

  if (toupper(car) == 'E')
  {
    car = next_char();
    if (car == '+' || car == '-')
      car = next_char();

    while (isdigit(car))
      car = next_char();
  }

  goto END;

END:

  last_pattern = get_last_pattern();

  if (sign && (!PATTERN_is_reserved(last_pattern) || PATTERN_is(last_pattern, RS_RBRA) || PATTERN_is(last_pattern, RS_RSQR)))
  {
    add_pattern(RT_RESERVED, RESERVED_find_word(&sign, 1));
    TABLE_add_symbol(comp->class->table, start + 1, source_ptr - start - 1, NULL, &index);
    add_pattern(RT_NUMBER, index);
  }
  else
  {
    TABLE_add_symbol(comp->class->table, start, source_ptr - start, NULL, &index);
    add_pattern(RT_NUMBER, index);
  }
}



static void add_identifier(bool no_res)
{
  unsigned char car;
  const char *start;
  int len;
  int index;
  int type;
  int flag;
  PATTERN last_pattern;
  bool not_first;
  bool can_be_reserved;
  bool can_be_subr;
  bool is_type;
  bool last_func, last_declare, last_type;
  
  last_pattern = get_last_pattern();
  
  if (PATTERN_is_reserved(last_pattern))
  {
    flag = RES_get_ident_flag(PATTERN_index(last_pattern));
    not_first = (flag & RSF_INF) != 0;
    last_func = (flag & RSF_ILF) != 0;
    last_declare = (flag & RSF_ILD) != 0;
    last_type = (flag & RSF_ILT) != 0;
		if (flag & RSF_ILDD)
		{
			PATTERN last_last_pattern = get_last_last_pattern();
			
			if (PATTERN_is_reserved(last_last_pattern) && RES_get_ident_flag(PATTERN_index(last_last_pattern)) & RSF_ILD)
				last_declare = TRUE;
			flag &= ~RSF_ILDD; // flag == 0 means we can read a subroutine!
		}
    //last_event = flag & RSF_ILE;
  }
  else
  {
    flag = 0;
    not_first = last_func = last_declare = last_type = FALSE;
  }

	type = RT_IDENTIFIER;

  start = source_ptr;
  len = 1;

	if (last_type)
	{
		for(;;)
		{
			source_ptr++;
			len++;
			car = get_char();
			if (ident_car[car])
				continue;
			if (car == '[')
			{
				car = get_char_offset(1);
				if (car == ']')
				{
					source_ptr++;
					len++;
			  	TABLE_add_symbol(comp->class->table, start, len - 2, NULL, NULL);
					continue;
				}
			}
			
			len--;
			break;
		}
	}
	else
	{
		for(;;)
		{
			source_ptr++;
			car = get_char();
			if (!ident_car[car])
				break;
			len++;
		}
	}

  if (no_res)
  {
    if (get_char() == '}')
      source_ptr++;
    goto IDENTIFIER;
  }

  car = get_char();

  //can_be_reserved = !not_first && TABLE_find_symbol(COMP_res_table, &comp->source[start], len, NULL, &index);
  
  can_be_reserved = !not_first;
  
  if (can_be_reserved)
  {
    index = RESERVED_find_word(start, len);
    can_be_reserved = (index >= 0);
  }
  
  if (can_be_reserved)
  {
    if (index == RS_ME || index == RS_NEW || index == RS_LAST || index == RS_SUPER)
    {
      can_be_reserved = !last_declare;
    }
    else if (index == RS_CLASS)
    {
      can_be_reserved = begin_line && isspace(car);
    }
    else
    {
      is_type = PATTERN_is_type(PATTERN_make(RT_RESERVED, index));

      if (is_type && (car == '[') && (get_char_offset(1) == ']'))
      {
        len += 2;
        source_ptr += 2;
        is_type = FALSE;
        can_be_reserved = FALSE;
      }
      else
      {
        if (index == RS_NEW)
          is_type = TRUE;

        if (last_type)
          can_be_reserved = is_type;
        else if (last_func)
          can_be_reserved = FALSE;
        else
          can_be_reserved = !is_type && (car != ':') && (car != '.') && (car != '!') && (car != '(');
      }
    }
  }

  can_be_subr = flag == 0 && car != '.' && car != '!';

  if (can_be_reserved)
  {
    type = RT_RESERVED;
  }
  else if (can_be_subr && TABLE_find_symbol(COMP_subr_table, start, len, NULL, &index))
  {
    type = RT_SUBR;
  }
	else
  {
  	if (last_type)
  		type = RT_CLASS;
    goto IDENTIFIER;
	}

  add_pattern(type, index);
  return;

IDENTIFIER:

  if (PATTERN_is(last_pattern, RS_EVENT) || PATTERN_is(last_pattern, RS_RAISE))
  {
    start--;
    len++;
    *((char *)start) = ':';
  }

	if (PATTERN_is(last_pattern, RS_EXCL))
	{
  	TABLE_add_symbol(comp->class->string, start, len, NULL, &index);
  	type = RT_STRING;
  }
	else
  	TABLE_add_symbol(comp->class->table, start, len, NULL, &index);
  
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
    if (!isascii(car) || !ispunct(car))
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
  int newline;
  bool jump;
  char *p;
  int i;

  start = source_ptr;
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
      else if (!isspace(car))
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
				else if (car == 'f')
					*p = '\f';
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
                car = (car << 4) | i;
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

  TABLE_add_symbol(comp->class->string, start + 1, len, NULL, &index);

  add_pattern(RT_STRING, index);

  for (i = 0; i < newline; i++)
    add_newline();
}


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


void READ_do(void)
{
  static void *jump_char[9] =
  {
    &&__BREAK, 
    &&__SPACE, 
    &&__COMMENT, 
    &&__STRING, 
    &&__IDENT, 
    &&__QUOTED_IDENT,
		&&__NUMBER,
    &&__ERROR, 
    &&__OTHER
  };
  
  unsigned char car;

  comp = JOB;

  READ_init();

  source_ptr = comp->source;
  source_length = BUFFER_length(comp->source);
  begin_line = TRUE;

  //while (source_ptr < source_length)
  for(;;)
  {
    car = get_char();
    goto *jump_char[(int)first_car[car]];
    
  __ERROR:
    
    THROW("Syntax error");
      
  __SPACE:

    source_ptr++;
    if (car == '\n')
    {
      add_newline();
      begin_line = TRUE;
    }
    continue;

  __COMMENT:
      
    do
    {
      source_ptr++;
      car = get_char();
    }
    while (car != '\n' && car != 0);

    begin_line = FALSE;
    continue;

  __STRING:
      
    add_string();
    begin_line = FALSE;
    continue;

  __IDENT:
    
    add_identifier(FALSE);
    begin_line = FALSE;
    continue;

  __QUOTED_IDENT:
    source_ptr++;
    add_identifier(TRUE);
    begin_line = FALSE;
    continue;
		
	__NUMBER:
		add_number();
		begin_line = FALSE;
		continue;
  
  __OTHER:
  
  	if (car == '#' && begin_line)
  	{
  		add_command();
  		begin_line = FALSE;
  		continue;
  	}
		
#if BIG_COMMENT
		if (car == '/' && get_char_offset(1) == '*')
		{
			for(;;)
			{
				source_ptr++;
				car = get_char();
				if (car == 0)
					break;
				if (car == '*' && get_char_offset(1) == '/')
				{
					source_ptr += 2;
					break;
				}
				if (car == '\n')
					add_newline();
			}

  		begin_line = FALSE;
  		continue;
		}
#endif
  	
    if (is_number())
			goto __NUMBER;

    add_operator();
    begin_line = FALSE;
  }

__BREAK:

  // We add end markers to simplify the compiler job, when it needs to look 
  // at many patterns in one shot.
  
  add_newline();
  add_end();
  add_end();
  add_end();
  add_end();

  READ_exit();
}

