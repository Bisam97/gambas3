/***************************************************************************

  gbx_local.c

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

#define __GBX_LOCAL_C

#include "gb_common.h"
#include "gb_common_buffer.h"
#include "gb_common_case.h"

#include "gb_error.h"
#include "gbx_value.h"
#include "gb_limit.h"

#include <locale.h>
#include <langinfo.h>

#include <time.h>
#include <ctype.h>
#include <float.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libintl.h>

#include "gbx_math.h"
#include "gbx_date.h"
#include "gbx_string.h"
#include "gbx_c_string.h"
#include "gbx_api.h"
#include "gb_file.h"
#include "gbx_component.h"
#include "gbx_exec.h"
#include "gbx_archive.h"

#include "gbx_local.h"

//#define DEBUG_LANG
//#define DEBUG_DATE

static void add_string(const char *src, int len, bool quote);
static void add_string_thousand(const char *src, int len, int *before);

/* System encoding*/
char *LOCAL_encoding = NULL;

/* If system encoding is UTF-8 */
bool LOCAL_is_UTF8;

/* Default 'C' localization */
LOCAL_INFO LOCAL_default = {
	'.', '.',
	NULL, 0, NULL, 0,
	3, 3,
	0,
	{ 0, 0, '/', '/' },
	{ 0, ':', ':', 0 },
	"",
	"",
	{ LO_MONTH, LO_DAY, LO_YEAR },
	{ LO_HOUR, LO_MINUTE, LO_SECOND },
	"dddd mmmm d yyyy",
	"mmm d yyyy",
	"mm/dd/yyyy",
	"hh:nn:ss",
	"hh:nn AM/PM",
	"hh:nn",
	"mm/dd/yyyy hh:nn:ss",
	"mm/dd/yyyy hh:nn:ss",
	"(#,##0.##)",
	"(#,##0.##)",
	"True", 4,
	"False", 5,
	FALSE
	};

/* User language localization */
LOCAL_INFO LOCAL_local = { 0 };

// First day of weekday
char LOCAL_first_day_of_week = -1;

static char *_rtl_lang[] = { "ar", "fa", "he", NULL };

static bool _translation_loaded = FALSE;

static LOCAL_INFO *local_current;

static char env_LC_ALL[MAX_LANG + sizeof "LC_ALL" + 2];
static char env_LANGUAGE[MAX_LANG + sizeof "LANGUAGE" + 2];
static char env_LANG[MAX_LANG + sizeof "LANG" + 2];

static bool _currency;

static char *_lang = NULL;

extern char **environ;
static char **_environ;

enum { PAD_DEFAULT, PAD_NONE, PAD_ZERO, PAD_SPACE };
enum { LAST_NONE, LAST_DATE, LAST_TIME };

#define add_currency_flag(_flag) (LOCAL_local.currency_flag <<= 1, LOCAL_local.currency_flag |= (!!(_flag)))
#define test_currency_flag(_negative, _space, _before, _intl) (!!(LOCAL_local.currency_flag & (1 << ((!!_negative) + ((!!_before) << 1) + ((!!_intl) << 2)))))

static void init_currency_flag(struct lconv *info)
{
#ifndef OS_BSD
	add_currency_flag(info->int_n_cs_precedes); // 7
	add_currency_flag(info->int_p_cs_precedes); // 6
#else
	add_currency_flag(info->n_cs_precedes); // 7
	add_currency_flag(info->p_cs_precedes); // 6
#endif
	add_currency_flag(1); // 5
	add_currency_flag(1); // 4
	add_currency_flag(info->n_cs_precedes); // 3
	add_currency_flag(info->p_cs_precedes); // 2
	add_currency_flag(info->n_sep_by_space); // 1
	add_currency_flag(info->p_sep_by_space); // 0
}

static bool is_currency_before(bool negative, bool intl)
{
	return test_currency_flag(negative, FALSE, TRUE, intl);
}

static bool is_currency_space(bool negative, bool intl)
{
	//fprintf(stderr, "%02X & %02X\n", LOCAL_local.currency_flag, (1 << ((!!negative) + ((!!0) << 1) + ((!!intl) << 2))));
	return test_currency_flag(negative, TRUE, FALSE, intl);
}

static void my_setenv(const char *name, const char *value, char *ptr)
{
	snprintf(ptr, MAX_LANG + strlen(name) + 2, "%s=%s", name, value);
	putenv(ptr);
}

static void begin(void)
{
	COMMON_buffer_init(COMMON_buffer, COMMON_BUF_MAX - 4);
}

static void end(char **str, int *len)
{
	*(COMMON_get_current()) = 0;
	*str = COMMON_buffer;
	*len = COMMON_pos;
}

static void add_thousand_sep(int *before)
{
	int group;
	const char *thsep;
	int lthsep;

	if (before == NULL)
		return;

	thsep = _currency ? local_current->currency_thousand_sep : local_current->thousand_sep;
	lthsep = _currency ? local_current->len_currency_thousand_sep : local_current->len_thousand_sep;
	
	if (thsep && lthsep)
	{
		group = _currency ? local_current->currency_group_size : local_current->group_size;

		if (group > 0 && (*before > 1) && ((*before - 1) == (((*before - 1) / group) * group)))
		{
			if (COMMON_pos > 0 && (COMMON_get_current()[-1] == ' '))
				COMMON_put_char(' ');
			else
				add_string(thsep, lthsep, FALSE);
		}
	}

	(*before)--;
}

static void add_string(const char *src, int len, bool quote)
{
	char c;
	
	if (len <= 0)
		len = strlen(src);

	while (len > 0)
	{
		c = *src++;
		len--;

		if (quote && c == '\\')
		{
			if (len > 0)
			{
				c = *src++;
				len--;
			}
			else
				break;
		}
		
		COMMON_put_char(c);
	}
}

static void add_string_thousand(const char *src, int len, int *before)
{
	char c;
	
	if (len <= 0)
		len = strlen(src);

	while (len > 0)
	{
		c = *src++;
		len--;

		COMMON_put_char(c);

		add_thousand_sep(before);
	}
}

static void add_unicode(uint unicode)
{
	char str[8];
	int len;
	
	if (unicode == 0)
		return;
	
	STRING_utf8_from_unicode(unicode, str);
	len = STRING_utf8_get_char_length(*str);
	
	if (COMMON_pos >= len && strncmp(&COMMON_buffer[COMMON_pos - len], str, len) == 0)
		return;
	
	add_string(str, len, FALSE);
}

static void add_currency(const char *sym)
{
	const char *p = sym;
	char c;

	for(;;)
	{
		c = *p++;
		if (c == 0)
			return;
		if (c != ' ')
			COMMON_put_char(c);
	}
}

static void add_digit_char(char c, int count, int *before)
{
	while (count > 0)
	{
		COMMON_put_char(c);
		count--;

		add_thousand_sep(before);
	}
}

static void add_zero(int count, int *before)
{
	add_digit_char('0', count, before);
}


static void add_sign(char mode, int sign, bool after)
{
	if (after && mode != '(')
		return;

	if (sign < 0)
	{
		if (mode == '(')
			COMMON_put_char(after ? ')' : '(');
		else
			COMMON_put_char('-');
	}
	else if (mode != 0 && mode != '(')
	{
		if (sign > 0)
			COMMON_put_char(mode);
		else
			COMMON_put_char(' ');
	}
}


static char *get_languages(void)
{
	char *lang;
	char *lang_list = NULL;
	char *src;
	char *p;

	lang = STRING_new_temp_zero(LOCAL_get_lang());
	p = index(lang, '.');
	if (p) *p = 0;

	lang_list = STRING_add_zero(lang_list, lang);
	lang_list = STRING_add_char(lang_list, ':');

	src = index(lang, '_');
	if (src)
	{
		*src = 0;
		lang_list = STRING_add_zero(lang_list, lang);
		lang_list = STRING_add_char(lang_list, ':');
	}

	lang_list = STRING_add_zero(lang_list, lang);
	lang_list = STRING_add(lang_list, "_*", 2);

	#ifdef DEBUG_LANG
	fprintf(stderr, "Languages = %s\n", lang_list);
	#endif

	return lang_list;
}


static void free_local_info(void)
{
	STRING_free(&LOCAL_local.standard_date);
	STRING_free(&LOCAL_local.long_date);
	STRING_free(&LOCAL_local.medium_date);
	STRING_free(&LOCAL_local.short_date);
	STRING_free(&LOCAL_local.long_time);
	STRING_free(&LOCAL_local.medium_time);
	STRING_free(&LOCAL_local.short_time);
	STRING_free(&LOCAL_local.general_date);
	STRING_free(&LOCAL_local.general_currency);
	STRING_free(&LOCAL_local.intl_currency);
	STRING_free(&LOCAL_local.true_str);
	STRING_free(&LOCAL_local.false_str);
	CLEAR(&LOCAL_local);
}

static const char *fix_separator(const char *str)
{
	if (!str || !*str)
		return " ";
	
	if ((uchar)str[0] == 0xC2 && (uchar)str[1] == 0xA0 && str[2] == 0)
		return " ";

	if ((uchar)str[0] == 0xE2 && (uchar)str[1] == 0x80 && (uchar)str[2] == 0xAF && str[3] == 0)
		return " ";

	return str[1] ? "_" : str;
}

#define FORMAT_ADD(_dst, _str) (LOCAL_local._dst = STRING_add_zero(LOCAL_local._dst, (_str)))
#define FORMAT_ADD_CHAR(_dst, _char) (LOCAL_local._dst = STRING_add_char(LOCAL_local._dst, (_char)))

static void format_add_len(char **pdst, const char *str, int len)
{
	if (len == 1 && isalpha(*str))
		*pdst = STRING_add_char(*pdst, '\\');
	*pdst = STRING_add(*pdst, str, len);
}

#define FORMAT_ADD_LEN(_dst, _str, _len) (format_add_len(&LOCAL_local._dst, (_str), (_len)))

static void format_add_sep(char **pdst, const char *str, char sep)
{
	if (*pdst)
		*pdst = STRING_add_char(*pdst, sep);
	*pdst = STRING_add_zero(*pdst, str);
}

#define FORMAT_ADD_SEP(_dst, _str, _sep) format_add_sep(&LOCAL_local._dst, (_str), (_sep))

static bool order_add(uchar *p, uchar type)
{
	int i;
	
	for (i = 0; i < 3; i++)
	{
		if (p[i] == type)
			return FALSE;
		else if (p[i] == 0)
		{
			p[i] = type;
			return TRUE;
		}
	}
	
	return FALSE;
}

#define ORDER_DATE_ADD(_type) order_add(LOCAL_local.date_order, (_type))
#define ORDER_TIME_ADD(_type) order_add(LOCAL_local.time_order, (_type))

static char *get_strftime(int which)
{
	char *fmt = NULL;
	char *p = nl_langinfo(which);
	char c;
	
	for(;;)
	{
		c = *p++;
		if (!c)
			break;
		if (c != '%')
		{
			fmt = STRING_add_char(fmt, c);
			continue;
		}
		
		c = *p++;
		if (c == 'D')
			fmt = STRING_add_zero(fmt, "%m/%d/%y");
		else if (c == 'F')
			fmt = STRING_add_zero(fmt, "%Y-%m-%d");
		else if (c == 'R')
			fmt = STRING_add_zero(fmt, "%H:%M");
		else if (c == 'r')
			fmt = STRING_add_zero(fmt, "%I:%M:%S %p");
		else if (c == 'T')
			fmt = STRING_add_zero(fmt, "%H:%M:%S");
		else
		{
			fmt = STRING_add_char(fmt, '%');
			fmt = STRING_add_char(fmt, c);
		}
	}
	
	#ifdef DEBUG_DATE
	fprintf(stderr, "fmt = %s\n", fmt);
	#endif
	return fmt;
}

static void fill_local_info(void)
{
	struct lconv *info;
	char *p;
	char *fmt;
	char c;
	char pad;
	char last;
	uchar last_elt;
	char *codeset;
	int len;
	int last_long_date;

	free_local_info();

	/* local encoding */

	if (!LOCAL_is_UTF8)
		STRING_free(&LOCAL_encoding);

	codeset = nl_langinfo(CODESET);
	if (!codeset || !*codeset)
		codeset = "US-ASCII";

	LOCAL_is_UTF8 = (strcasecmp(codeset, "UTF-8") == 0);
	if (LOCAL_is_UTF8)
		LOCAL_encoding = SC_UTF8;
	else
		LOCAL_encoding = STRING_new_zero(codeset);

	#ifdef DEBUG_LANG
	fprintf(stderr, "LOCAL_encoding = %s\n", LOCAL_encoding == SC_UTF8 ? "UTF-8" : LOCAL_encoding);
	#endif

	/* Numeric information */

	info = localeconv();

	//fprintf(stderr, "'%s' '%s' %d %d\n", info->thousands_sep, info->mon_thousands_sep, *info->thousands_sep, *info->grouping);
	//fprintf(stderr, "'%s' '%s'\n", nl_langinfo(THOUSANDS_SEP), nl_langinfo(MON_THOUSANDS_SEP));

	LOCAL_local.decimal_point = *(info->decimal_point);
	LOCAL_local.thousand_sep = fix_separator(info->thousands_sep);
	LOCAL_local.len_thousand_sep = strlen(LOCAL_local.thousand_sep);
	
	LOCAL_local.group_size = *(info->grouping);
	if (LOCAL_local.group_size == 0)
		LOCAL_local.group_size = 3;

	/*LOCAL_local.currency_symbol = STRING_conv_to_UTF8(info->currency_symbol, 0);
	STRING_ref(LOCAL_local.currency_symbol);
	LOCAL_local.intl_currency_symbol = STRING_conv_to_UTF8(info->int_curr_symbol, 0);
	STRING_ref(LOCAL_local.intl_currency_symbol);*/

	// Date & time format

	#ifdef DEBUG_DATE
	fprintf(stderr, "D_T_FMT: %s\n", nl_langinfo(D_T_FMT));
	fprintf(stderr, "D_FMT: %s\n", nl_langinfo(D_FMT));
	fprintf(stderr, "T_FMT: %s\n", nl_langinfo(T_FMT));
	#endif
	
	// gb.GeneralDate / gb.LongDate
	
	p = fmt = get_strftime(D_T_FMT);
	last = LAST_NONE;
	last_elt = 0;
	last_long_date = 0;
	
	for(;;)
	{
		c = *p++;
		if (!c)
			break;
		
		if (c == '%')
		{
			c = *p++;
			if (c != '%')
			{
				pad = PAD_DEFAULT;
				
				while (c && !isalpha(c))
				{
					if (c == '-')
						pad = PAD_NONE;
					else if (c == '_')
						pad = PAD_SPACE;
					else if (c == '0')
						pad = PAD_ZERO;
					c = *p++;
				}
				
				if (c == 'E' || c == 'O')
					c = *p++;
				
				switch (c)
				{
					case 'a': case 'A':
						FORMAT_ADD(general_date, (c == 'a') ? "ddd" : "dddd");
						FORMAT_ADD(long_date, "dddd");
						last = LAST_DATE;
						break;
						
					case 'b': case 'h': case 'B':
						FORMAT_ADD(general_date, (c == 'b') ? "mmm" : "mmmm");
						FORMAT_ADD(long_date, "mmmm");
						last = LAST_DATE;
						break;
						
					case 'd': case 'e':
						if (pad == PAD_DEFAULT)
							pad = (c == 'e') ? PAD_NONE : PAD_ZERO;
						FORMAT_ADD(general_date, (pad == PAD_ZERO) ? "dd" : "d");
						FORMAT_ADD(long_date, (pad == PAD_ZERO) ? "dd" : "d");
						last = LAST_DATE;
						break;
					
					case 'H': case 'I': case 'k': case 'l':
						if (pad == PAD_DEFAULT)
							pad = (c == 'k' || c == 'l') ? PAD_NONE : PAD_ZERO;
						FORMAT_ADD(general_date, (pad == PAD_ZERO) ? "hh" : "h");
						last = LAST_TIME;
						break;
						
					case 'm':
						if (pad == PAD_DEFAULT)
							pad = PAD_ZERO;
						FORMAT_ADD(general_date, (pad == PAD_ZERO) ? "mm" : "m");
						FORMAT_ADD(long_date, (pad == PAD_ZERO) ? "mm" : "m");
						last = LAST_DATE;
						break;
						
					case 'M':
						if (pad == PAD_DEFAULT)
							pad = PAD_ZERO;
						FORMAT_ADD(general_date, (pad == PAD_ZERO) ? "nn" : "n");
						last = LAST_TIME;
						break;
						
					case 'P': case 'p':
						FORMAT_ADD(general_date, (c == 'P') ? "am/pm" : "AM/PM");
						last = LAST_TIME;
						break;
						
					case 'S':
						if (pad == PAD_DEFAULT)
							pad = PAD_ZERO;
						FORMAT_ADD(general_date, (pad == PAD_ZERO) ? "ss" : "s");
						last = LAST_TIME;
						break;
						
					case 'Y': case 'y':
						FORMAT_ADD(general_date, "yyyy");
						FORMAT_ADD(long_date, "yyyy");
						last = LAST_DATE;
						break;
						
					case 'z':
						FORMAT_ADD(general_date, "t");
						FORMAT_ADD(long_date, "t");
						last = LAST_DATE;
						break;
						
					case 'Z':
						FORMAT_ADD(general_date, "tt");
						FORMAT_ADD(long_date, "t");
						last = LAST_DATE;
						break;
				}
				
				if (last == LAST_DATE)
					last_long_date = STRING_length(LOCAL_local.long_date);

				continue;
			}
		}
		
		len = STRING_utf8_get_char_length(c);
		p--;
		
		FORMAT_ADD_LEN(general_date, p, len);
		
		if (last == LAST_DATE)
			FORMAT_ADD_LEN(long_date, p, len);
		
		p += len;
	}
	
	STRING_free(&fmt);
	
	// Remove final separators of long_date
	
	fmt = LOCAL_local.long_date;
	LOCAL_local.long_date = STRING_new(fmt, last_long_date);
	STRING_free(&fmt);
	
	// Other date formats
	
	p = fmt = get_strftime(D_FMT);
	last = LAST_NONE;
	last_elt = 0;
	
	for(;;)
	{
		c = *p++;
		if (!c)
			break;
		
		if (c == '%')
		{
			c = *p++;
			if (c != '%')
			{
				pad = PAD_DEFAULT;
				
				while (c && !isalpha(c))
				{
					if (c == '-')
						pad = PAD_NONE;
					else if (c == '_')
						pad = PAD_SPACE;
					else if (c == '0')
						pad = PAD_ZERO;
					c = *p++;
				}
				
				if (c == 'E' || c == 'O')
					c = *p++;
				
				switch (c)
				{
					case 'a': case 'A':
						if (ORDER_DATE_ADD(LO_DAY))
						{
							FORMAT_ADD_SEP(medium_date, "dd", '/');
							FORMAT_ADD_SEP(short_date, "d", '/');
							last = LAST_DATE;
							last_elt = LO_DAY;
						}
						break;
						
					case 'b': case 'h': case 'B':
						if (ORDER_DATE_ADD(LO_MONTH))
						{
							FORMAT_ADD_SEP(medium_date, "mm", '/');
							FORMAT_ADD_SEP(short_date, "m", '/');
							last = LAST_DATE;
							last_elt = LO_MONTH;
						}
						break;
						
					case 'd': case 'e':
						if (pad == PAD_DEFAULT)
							pad = (c == 'e') ? PAD_NONE : PAD_ZERO;
						if (ORDER_DATE_ADD(LO_DAY))
						{
							FORMAT_ADD_SEP(medium_date, "dd", '/');
							FORMAT_ADD_SEP(short_date, "d", '/');
							last = LAST_DATE;
							last_elt = LO_DAY;
						}
						break;
					
					case 'm':
						if (pad == PAD_DEFAULT)
							pad = PAD_ZERO;
						if (ORDER_DATE_ADD(LO_MONTH))
						{
							FORMAT_ADD_SEP(medium_date, "mm", '/');
							FORMAT_ADD_SEP(short_date, "m", '/');
							last = LAST_DATE;
							last_elt = LO_MONTH;
						}
						break;
						
					case 'Y': case 'y':
						FORMAT_ADD_SEP(medium_date, "yyyy", '/');
						FORMAT_ADD_SEP(short_date, "yyyy", '/');
						if (ORDER_DATE_ADD(LO_YEAR))
						{
							last = LAST_DATE;
							last_elt = LO_YEAR;
						}							
						break;
						
					default:
						last = LAST_NONE;
				}
			
				if (last == LAST_DATE)
					LOCAL_local.date_tail_sep = FALSE;
				
				continue;
			}
		}
		
		len = STRING_utf8_get_char_length(c);
		p--;
		
		if (last == LAST_DATE)
		{
			if (LOCAL_local.date_sep[last_elt] == 0)
				LOCAL_local.date_sep[last_elt] = STRING_utf8_to_unicode(p, len);
			LOCAL_local.date_tail_sep = c != ' ';
		}
		
		p += len;
	}
	
	if (LOCAL_local.date_tail_sep)
	{
		FORMAT_ADD_CHAR(medium_date, '/');
		FORMAT_ADD_CHAR(short_date, '/');
	}
	
	LOCAL_local.date_many_sep = LOCAL_local.date_sep[LOCAL_local.date_order[0]] != LOCAL_local.date_sep[LOCAL_local.date_order[1]];

	STRING_free(&fmt);

	// Other time formats
	
	p = fmt = get_strftime(T_FMT);
	last = LAST_NONE;
	last_elt = 0;
	
	for(;;)
	{
		c = *p++;
		if (!c)
			break;
		
		if (c == '%')
		{
			c = *p++;
			if (c != '%')
			{
				pad = PAD_DEFAULT;
				
				while (c && !isalpha(c))
				{
					if (c == '-')
						pad = PAD_NONE;
					else if (c == '_')
						pad = PAD_SPACE;
					else if (c == '0')
						pad = PAD_ZERO;
					c = *p++;
				}
				
				if (c == 'E' || c == 'O')
					c = *p++;
				
				switch (c)
				{
					case 'H': case 'I': case 'k': case 'l':
						if (pad == PAD_DEFAULT)
							pad = (c == 'k' || c == 'l') ? PAD_NONE : PAD_ZERO;
						if (ORDER_TIME_ADD(LO_HOUR))
						{
							FORMAT_ADD_SEP(long_time, "hh", ':');
							FORMAT_ADD_SEP(medium_time, (pad == PAD_ZERO) ? "hh" : "h", ':');
							FORMAT_ADD_SEP(short_time, "h", ':');
							last = LAST_TIME;
							last_elt = LO_HOUR;
						}
						break;
						
					case 'M':
						if (pad == PAD_DEFAULT)
							pad = PAD_ZERO;
						if (ORDER_TIME_ADD(LO_MINUTE))
						{
							FORMAT_ADD_SEP(long_time, "nn", ':');
							FORMAT_ADD_SEP(medium_time, (pad == PAD_ZERO) ? "nn" : "n", ':');
							FORMAT_ADD_SEP(short_time, "nn", ':');
							last = LAST_TIME;
							last_elt = LO_MINUTE;
						}
						break;
						
					case 'P': case 'p':
						FORMAT_ADD_SEP(medium_time, (c == 'P') ? "am/pm" : "AM/PM", ' ');
						break;
						
					case 'S':
						if (pad == PAD_DEFAULT)
							pad = PAD_ZERO;
						if (ORDER_TIME_ADD(LO_SECOND))
						{
							FORMAT_ADD_SEP(long_time, "ss", ':');
							FORMAT_ADD_SEP(medium_time, (pad == PAD_ZERO) ? "ss" : "s", ':');
							last = LAST_TIME;
							last_elt = LO_SECOND;
						}
						break;
					
					default:
						last = LAST_NONE;
				}
			
				if (last == LAST_TIME)
					LOCAL_local.time_tail_sep = FALSE;
				
				continue;
			}
		}
		
		len = STRING_utf8_get_char_length(c);
		p--;
		
		if (last == LAST_TIME)
		{
			if (LOCAL_local.time_sep[last_elt] == 0)
				LOCAL_local.time_sep[last_elt] = STRING_utf8_to_unicode(p, len);
			LOCAL_local.time_tail_sep = c != ' ';
		}
		
		p += len;
	}
	
	if (LOCAL_local.time_tail_sep)
	{
		FORMAT_ADD_CHAR(long_time, ':');
		FORMAT_ADD_CHAR(medium_time, ':');
		FORMAT_ADD_CHAR(short_time, ':');
	}
	
	LOCAL_local.time_many_sep = LOCAL_local.time_sep[LOCAL_local.time_order[0]] != LOCAL_local.time_sep[LOCAL_local.time_order[1]];

	STRING_free(&fmt);
	
	LOCAL_local.standard_date = STRING_copy(LOCAL_local.medium_date);
	LOCAL_local.standard_date = STRING_add_char(LOCAL_local.standard_date, ' ');
	LOCAL_local.standard_date = STRING_add_string(LOCAL_local.standard_date, LOCAL_local.long_time);

	#ifdef DEBUG_DATE
	fprintf(stderr, "date_tail_sep = %u\n", LOCAL_local.date_tail_sep);
	fprintf(stderr, "time_tail_sep = %u\n\n", LOCAL_local.time_tail_sep);
	
	fprintf(stderr, "general_date: '%s'\n", LOCAL_local.general_date);
	fprintf(stderr, "long_date:    '%s'\n", LOCAL_local.long_date);
	fprintf(stderr, "medium_date:  '%s'\n", LOCAL_local.medium_date);
	fprintf(stderr, "short_date:   '%s'\n", LOCAL_local.short_date);
	fprintf(stderr, "long_time:    '%s'\n", LOCAL_local.long_time);
	fprintf(stderr, "medium_time:  '%s'\n", LOCAL_local.medium_time);
	fprintf(stderr, "short_time:   '%s'\n", LOCAL_local.short_time);
	#endif
	
	// Fix missing seconds

	/*if (!got_second)
	{
		*tp++ = LO_SECOND;
		stradd_sep(long_time, "ss", ':');
	}*/

	#if 0
	// Do not fix the french date separator anymore

	lang = LOCAL_get_lang();
	if (strcmp(lang, "fr") == 0 || strncmp(lang, "fr_", 3) == 0)
		LOCAL_local.date_sep[LO_DAY] = LOCAL_local.date_sep[LO_MONTH] = '/';
	#endif

	/*stradd_sep(general_date, LOCAL_local.long_time, ' ');
	am_pm = nl_langinfo(AM_STR);
	if (am_pm && *am_pm)
	{
		am_pm = nl_langinfo(PM_STR);
		if (am_pm && *am_pm)
		{
			stradd_sep(medium_time, "AM/PM", ' ');
		}
	}*/
	
	// Currency format

	LOCAL_local.currency_thousand_sep = fix_separator(info->mon_thousands_sep);
	LOCAL_local.len_currency_thousand_sep = strlen(LOCAL_local.currency_thousand_sep);
	
	LOCAL_local.currency_group_size = *(info->mon_grouping);
	if (LOCAL_local.currency_group_size == 0)
		LOCAL_local.currency_group_size = 3;

	LOCAL_local.currency_decimal_point = *(info->mon_decimal_point);
	LOCAL_local.currency_symbol = info->currency_symbol;
	LOCAL_local.intl_currency_symbol = info->int_curr_symbol;

	LOCAL_local.general_currency = STRING_new_zero("($,0.");
	LOCAL_local.general_currency = STRING_add(LOCAL_local.general_currency, "########", Min(8, info->frac_digits));
	LOCAL_local.general_currency = STRING_add_char(LOCAL_local.general_currency, ')');

	LOCAL_local.intl_currency = STRING_new_zero("($$,0.");
	LOCAL_local.intl_currency = STRING_add(LOCAL_local.intl_currency, "########", Min(8, info->int_frac_digits));
	LOCAL_local.intl_currency = STRING_add_char(LOCAL_local.intl_currency, ')');

	init_currency_flag(info);

	LOCAL_local.true_str = STRING_new_zero(LOCAL_gettext(LOCAL_default.true_str));
	LOCAL_local.len_true_str = STRING_length(LOCAL_local.true_str);
	LOCAL_local.false_str = STRING_new_zero(LOCAL_gettext(LOCAL_default.false_str));
	LOCAL_local.len_false_str = STRING_length(LOCAL_local.false_str);
}


void LOCAL_init(void)
{
	_environ = environ;
	LOCAL_set_lang(NULL);
}

void LOCAL_exit(void)
{
	if (environ == _environ)
	{
		unsetenv("LANG");
		unsetenv("LC_ALL");
		unsetenv("LANGUAGE");
	}
	
	if (!LOCAL_is_UTF8)
		STRING_free(&LOCAL_encoding);
	
	STRING_free(&_lang);
	free_local_info();
}


const char *LOCAL_get_lang(void)
{
	char *lang;

	if (!_lang)
	{
		lang = getenv("LC_ALL");
		if (!lang || !*lang)
			lang = getenv("LANG");
		if (!lang || !*lang)
			lang = "C";
		_lang = STRING_new_zero(lang);
	}

	return _lang;
}

void LOCAL_set_lang(const char *lang)
{
	char **l;
	int rtl;
	char *var;
	bool set_lang;

	if (lang && (strlen(lang) > MAX_LANG))
		THROW_ARG();
	
	#ifdef DEBUG_LANG
	fprintf(stderr, "******** LOCAL_set_lang: %s / LC_ALL = %s\n", lang ? lang : "(null)", getenv("LC_ALL"));
	#endif

	set_lang = lang && *lang;
	
	STRING_free(&_lang);
	lang = LOCAL_get_lang();

	#ifdef DEBUG_LANG
	fprintf(stderr, "lang = %s\n", lang);
	#endif

	if (set_lang)
	{
		#ifdef DEBUG_LANG
		fprintf(stderr, "LOCAL_set_lang: set LC_ALL=%s\n", lang);
		#endif
		my_setenv("LC_ALL", lang, env_LC_ALL);
		my_setenv("LANGUAGE", lang, env_LANGUAGE);
		my_setenv("LANG", lang, env_LANG);
	}
	
	if (setlocale(LC_ALL, ""))
	{
		_translation_loaded = FALSE;
		COMPONENT_translation_must_be_reloaded();
		CLASS_translation_must_be_reloaded();
	}
	else
	{
		char *err = strerror(errno);
		ERROR_warning("cannot switch to language '%s': %s. Did you install the corresponding locale packages?", lang ? lang : LOCAL_get_lang(), err);
		setlocale(LC_ALL, "C");
	}

	DATE_init_local();
	fill_local_info();

	/* If language is right to left written */

	rtl = FALSE;
	for (l = _rtl_lang; *l; l++)
	{
		if (strncmp(*l, lang, 2) == 0)
		{
			rtl = TRUE;
			break;
		}
	}

	var = getenv("GB_REVERSE");
	if (var && !(var[0] == '0' && var[1] == 0))
		rtl = !rtl;

	HOOK(lang)(lang, rtl);
	LOCAL_local.rtl = rtl;
}


static int int_to_string(uint64_t nbr, char **addr)
{
	static char buf[32];
	char *ptr;
	int digit, len;
	bool neg;

	len = 0;
	ptr = &buf[sizeof(buf)];

	if (nbr == 0)
	{
		ptr -= 2;
		*addr = ptr;
		ptr[0] = '0';
		ptr[1] = 0;
		return 1;
	}
	
	neg = (nbr & (1ULL << 63)) != 0;
	
	if (neg)
		nbr = 1 + ~nbr;
	
	while (nbr > 0)
	{
		digit = nbr % 10;
		nbr /= 10;

		ptr--;
		len++;

		*ptr = '0' + digit;
	}

	if (neg)
	{
		len++;
		ptr--;
		*ptr = '-';
	}
	
	*addr = ptr;
	return len;
}

const char *LOCAL_get_format(LOCAL_INFO *info, int type)
{
	switch(type)
	{
		case LF_GENERAL_NUMBER: return "0.###############E@#";
		case LF_SHORT_NUMBER: return "0.#######E@#";
		case LF_FIXED: return "0.00";
		case LF_PERCENT: return "###%";
		case LF_SCIENTIFIC: return "0.################E+0";
		case LF_CURRENCY: return info->general_currency;
		case LF_INTERNATIONAL: return info->intl_currency;
		case LF_GENERAL_DATE: return info->general_date;
		case LF_LONG_DATE: return info->long_date;
		case LF_MEDIUM_DATE: return info->medium_date;
		case LF_SHORT_DATE: return info->short_date;
		case LF_LONG_TIME: return info->long_time;
		case LF_MEDIUM_TIME: return info->medium_time;
		case LF_SHORT_TIME: return info->short_time;
		default: return NULL;
	}
}

bool LOCAL_format_number(double number, int fmt_type, const char *fmt, int len_fmt, char **str, int *len_str, bool local)
{
	int i;
	unsigned char c;
	int n;
	char *buf_addr;
	int pos;
	int thousand;
	int *thousand_ptr;
	char sign;
	bool comma;
	bool point;
	int before, before_zero;
	int after, after_zero;
	char exponent;
	int exp_zero;
	bool exp_sign;
	
	double fabs_number;
	int number_sign;
	uint64_t mantisse;
	uint64_t power;
	double number_mant;
	int number_exp;
	int number_real_exp;
	int ndigit;
	int pos_first_digit;

	bool intl_currency;

	if (local)
		local_current = &LOCAL_local;
	else
		local_current = &LOCAL_default;

	switch(fmt_type)
	{
		case LF_USER:
			break;
			
		case LF_STANDARD:
			fmt_type = LF_GENERAL_NUMBER;
			// continue

		default:
			fmt = LOCAL_get_format(local_current, fmt_type);
			if (!fmt)
				return TRUE;
	}

	if (len_fmt == 0)
		len_fmt = strlen(fmt);

	if (len_fmt >= COMMON_BUF_MAX)
		return TRUE;

	sign = 0;
	comma = FALSE;
	before = 0;
	before_zero = 0;
	point = FALSE;
	after = 0;
	after_zero = 0;
	exponent = 0;
	exp_sign = FALSE;
	exp_zero = 0;
	_currency = FALSE;
	intl_currency = FALSE;
	fabs_number = fabs(number);

	begin();

	// Search for the first formatting character
	
	for (i = 0; i < len_fmt; i++)
	{
		c = fmt[i];
		if (c == '\\')
		{
			i++;
			continue;
		}
		
		if (c == '-' || c == '+' || c == '#' || c == '0' || c == '.' || c == ',' || c == '(' || c == '$')
			break;
	}
	
	if (i >= len_fmt)
		return TRUE;

	pos = i;
	if (pos > 0)
		add_string(fmt, pos, TRUE);

	// Search if there is a percent format character
	
	for (i = pos; i < len_fmt; i++)
	{
		c = fmt[i];
		if (c == '\\')
		{
			i++;
			continue;
		}
		
		if (c == '%')
		{
			number *= 100;
			break;
		}
	}
	
	// specify the sign

	if (fmt[pos] == '-')
	{
		sign = ' ';
		pos++;
	}
	else if (fmt[pos] == '+')
	{
		sign = '+';
		pos++;
	}
	else if (fmt[pos] == '(')
	{
		sign = '(';
		pos++;
	}

	if (pos >= len_fmt)
		return TRUE;

	// currency

	if (fmt[pos] == '$')
	{
		_currency = TRUE;
		pos++;

		if (fmt[pos] == '$')
		{
			intl_currency = TRUE;
			pos++;
		}
	}

	// decimal digits

	for(; pos < len_fmt; pos++)
	{
		c = fmt[pos];

		if (c == ',')
		{
			comma = TRUE;
			continue;
		}

		if (c == '#' || c == '0')
		{
			before++;
			if (c == '0' || before_zero > 0)
				before_zero++;
			continue;
		}

		break;
	}
	
	if (before > 0 && before_zero == 0)
		before_zero = 1;

	if (pos >= len_fmt)
		goto _FORMAT;

	// decimal point

	if (fmt[pos] != '.')
		goto _FORMAT;

	pos++;
	point = TRUE;

	if (pos >= len_fmt)
		goto _FORMAT;

	// digits after decimal point

	for(; pos < len_fmt; pos++)
	{
		c = fmt[pos];

		if (c == '#' || c == '0')
		{
			after++;
			if (c == '0')
				after_zero = after;
			continue;
		}

		break;
	}

	if (pos >= len_fmt)
		goto _FORMAT;

	// exponent

	if (fmt[pos] == 'e' || fmt[pos] == 'E')
	{
		bool exp_optional = FALSE;
		
		exponent = fmt[pos];

		pos++;
		if (pos >= len_fmt)
			return TRUE;
		
		if (fmt[pos] == '-' || fmt[pos] == '+' || fmt[pos] == '@')
		{
			exp_sign = TRUE;
			exp_optional = fmt[pos] == '@';
			pos++;
		}

		if (pos >= len_fmt)
			return TRUE;

		for(; pos < len_fmt; pos++)
		{
			c = fmt[pos];

			if (c == '#' || c == '0')
			{
				if (c == '0' || exp_zero > 0)
					exp_zero++;
				continue;
			}

			break;
		}
		
		if (exp_optional)
		{
			if (number == 0.0 || (fabs_number > 1E-4 && fabs_number < 1E10))
				exponent = 0;
		}
	}

_FORMAT:

	if (before == 0 && after == 0)
		return TRUE;

	// sign

	number_sign = fsgn(number);

	add_sign(sign, number_sign, FALSE);

	// currency (before)

	if (_currency && is_currency_before(number_sign < 0, intl_currency))
	{
		add_currency(intl_currency ? local_current->intl_currency_symbol : local_current->currency_symbol);
		if (is_currency_space(number_sign < 0, intl_currency))
			COMMON_put_char(' ');
	}

	// We note where the first digit will be printed

	pos_first_digit = COMMON_pos;

	// the number

	if (isfinite(number))
	{
		number_mant = frexp10(fabs(number), &number_exp);
		ndigit = after;
		
		if (!exponent) 
			ndigit += number_exp;
		else
			ndigit++;
		
		ndigit = MinMax(ndigit, 0, MAX_FLOAT_DIGIT);
		//fprintf(stderr, "number_mant = %.24g  number_exp = %d  ndigit = %d\n", number_mant, number_exp, ndigit);

		power = pow10_uint64_p(ndigit + 1);

		mantisse = number_mant * power;
		if ((mantisse % 10) >= 5)
			mantisse += 10;

		//fprintf(stderr, "-> power = %" PRId64 " mantisse = %" PRId64 "\n", power, mantisse);

		if (mantisse >= power)
		{
			ndigit = int_to_string(mantisse, &buf_addr);
			buf_addr--;
			buf_addr[0] = buf_addr[1];
			buf_addr[1] = '.';
			ndigit++;
		}
		else
		{
			ndigit = int_to_string(mantisse, &buf_addr);
			buf_addr -= 2;
			buf_addr[0] = '0';
			buf_addr[1] = '.';
			ndigit += 2;
		}

		ndigit--;
		buf_addr[ndigit] = 0;

		// 0.0 <= number_mant < 1.0

		//number_exp++; /* simplifie les choses */

		number_real_exp = number_exp;
		if (exponent)
			number_exp = number != 0.0;

		// should return "0[.]...", or "1[.]..." if the number is rounded up.

		if (buf_addr[0] == '1') // the number has been rounded up.
		{
			if (exponent)
				number_real_exp++;
			else
				number_exp++;
		}

		if (ndigit > 1) // so there is a point
		{
			if (buf_addr[0] == '0')
			{
				buf_addr += 2;
				ndigit -= 2;
			}
			else
			{
				buf_addr[1] = buf_addr[0];
				ndigit--;
				buf_addr++;
			}

			while (ndigit > 0 && buf_addr[ndigit - 1] == '0')
				ndigit--;
		}

		// digits before point

		thousand = Max(before, Max(before_zero, number_exp));
		thousand_ptr = comma ? &thousand : NULL;

		if (number_exp > 0)
		{
			add_digit_char(' ', before - Max(before_zero, number_exp), thousand_ptr);
			add_zero(before_zero - number_exp, thousand_ptr);

			add_string_thousand(buf_addr, Min(number_exp, ndigit), thousand_ptr);

			if (number_exp > ndigit)
				add_zero(number_exp - ndigit, thousand_ptr);
		}
		else
		{
			add_digit_char(' ', before - before_zero, thousand_ptr);
			add_zero(before_zero, thousand_ptr);
		}

		// decimal point

		if (point)
			COMMON_put_char(local_current->decimal_point);

		// digits after the decimal point

		if ((ndigit - number_exp) > 0)
		{
			if (number_exp < 0)
			{
				n = Min(after, (- number_exp));
				if (n == after)
				{
					add_zero(after_zero, NULL);
					goto _EXPONENT;
				}
				else
				{
					add_zero(n, NULL);
					after -= n;
					after_zero -= n;
				}
			}

			if (number_exp > 0)
			{
				buf_addr += number_exp;
				ndigit -= number_exp;
			}

			n = Min(ndigit, after);
			if (n > 0)
			{
				add_string(buf_addr, n, FALSE);
				after -= n;
				after_zero -= n;
			}

			if (after_zero > 0)
				add_zero(after_zero, NULL);
		}
		else
			add_zero(after_zero, NULL);

	_EXPONENT:

		// The decimal point is removed if it is located at the end

		COMMON_pos--;
		if (COMMON_look_char() != local_current->decimal_point)
			COMMON_pos++;

		// exponent

		if (exponent != 0) // && number != 0.0)
		{
			number_real_exp--;

			COMMON_put_char(exponent);
			if (exp_sign && number_real_exp >= 0)
				COMMON_put_char('+');
			if (number_real_exp < 0)
			{
				COMMON_put_char('-');
				number_real_exp = (- number_real_exp);
			}
			n = int_to_string(number_real_exp, &buf_addr);
			while (exp_zero > n)
			{
				COMMON_put_char('0');
				exp_zero--;
			}
			add_string(buf_addr, n, FALSE);
		}
	}
	else // isfinite
	{
		if (isnan(number))
			add_string("NaN", 3, FALSE);
		else if (isinf(number))
			add_string("Inf", 3, FALSE);
	}

	// currency (after)

	if (_currency && !is_currency_before(number_sign < 0, intl_currency))
	{
		if (is_currency_space(number_sign < 0, intl_currency))
			COMMON_put_char(' ');
		add_currency(intl_currency ? local_current->intl_currency_symbol : local_current->currency_symbol);
	}

	// The last format brace is ignored

	if (sign == '(' && fmt[pos] == ')')
		pos++;

	// The sign after

	add_sign(sign, number_sign, TRUE);

	// print at least a zero

	if (COMMON_pos == pos_first_digit)
		COMMON_put_char('0');

	// format suffix

	if (pos < len_fmt)
		add_string(&fmt[pos], len_fmt - pos, TRUE);

	// return the result

	end(str, len_str);
	return FALSE;
}

static void add_strftime(const char *format, struct tm *tm)
{
	int n;

	n = strftime(COMMON_get_current(), COMMON_get_size_left(), format, tm);
	COMMON_pos += n;
}


static void add_number(int value, int pad)
{
	static char temp[10] = { 0 };
	int i, n;
	bool minus = FALSE;

	if (value < 0)
	{
		value = (-value);
		minus = TRUE;
	}

	n = 0;
	for (i = 9; i >= 2; i--)
	{
		n++;
		if (value < 10)
		{
			temp[i] = value + '0';
			value = 0;
			if (n >= pad)
				break;
		}
		else
		{
			temp[i] = (value % 10) + '0';
			value /= 10;
		}
	}

	if (minus)
	{
		i--;
		temp[i] = '-';
		n++;
	}

	add_string(&temp[i], n, FALSE);
}

static bool add_date_time_token(DATE_SERIAL *date, char token, int count)
{
	struct tm tm = {0};
	char buf[8];
	int n;
	bool date_token;

	date_token = token == 'd' || token == 'm' || token == 'y';

	if ((date_token && DATE_SERIAL_has_no_date(date))) // || (!date_token && DATE_SERIAL_has_no_time(date)))
		return TRUE;

	switch (token)
	{
		case 'd':

			if (count <= 2)
			{
				add_number(date->day, (count == 1 ? 0 : 2));
			}
			else
			{
				tm.tm_wday = date->weekday;
				add_strftime(count == 3 ? "%a" : "%A", &tm);
			}
			break;

		case 'm':

			if (count <= 2)
			{
				add_number(date->month, (count == 1 ? 0 : 2));
			}
			else
			{
				tm.tm_mon = date->month - 1;
				add_strftime(count == 3 ? "%b" : "%B", &tm);
			}
			break;

		case 'y':

			if (count <= 2 && date->year >= 1939 && date->year <= 2038)
				add_number(date->year - (date->year >= 2000 ? 2000 : 1900), 2);
			else
				add_number(date->year, (count == 1 ? 0 : count));
			break;

		case 'h':
			
			add_number(date->hour, (count == 1 ? 0 : 2));
			break;
			
		case 'n':

			add_number(date->min, (count == 1 ? 0 : 2));
			break;
			
		case 's':
			
			add_number(date->sec, (count == 1 ? 0 : 2));
			break;

		case 'u':

			if (date->msec || count == 2)
			{
				if (count >= 2)
					add_number(date->msec, 3);
				else
				{
					n = snprintf(buf, sizeof(buf), "%03d", date->msec);
					while (buf[n - 1] == '0')
						n--;
					buf[n] = 0;
					add_string(buf, n, FALSE);
				}
			}
			break;

		case 't':

			if (count <= 2)
			{
				time_t t = time(NULL);
				localtime_r(&t, &tm);
				add_strftime(count == 2 ? "%z" : "%Z", &tm);
			}
			break;
	}

	return FALSE;
}


static uint find_first_separator(uint psep[4])
{
	int i;
	
	for (i = 1; i <= 3; i++)
	{
		if (psep[i])
			return psep[i];
	}
	
	return 0;
}


static void add_date_separator(char token)
{
	uchar index;
	uint sep;
	
	switch (token)
	{
		case 'y': index = LO_YEAR; break;
		case 'm': index = LO_MONTH; break;
		case 'd': index = LO_DAY; break;
		default: return;
	}
			
	sep = local_current->date_sep[index];
	if (!sep && !local_current->date_many_sep)
		sep = find_first_separator(local_current->date_sep);
	if (sep) add_unicode(sep);
}


static void add_time_separator(char token)
{
	uchar index;
	uint sep;
	
	switch (token)
	{
		case 'h': index = LO_HOUR; break;
		case 'n': index = LO_MINUTE; break;
		case 's': index = LO_SECOND; break;
		default: return;
	}
			
	sep = local_current->time_sep[index];
	if (!sep && !local_current->time_many_sep)
		sep = find_first_separator(local_current->time_sep);
	if (sep) add_unicode(sep);
}


bool LOCAL_format_date(const DATE_SERIAL *date, int fmt_type, const char *fmt, int len_fmt, char **str, int *len_str)
{
	DATE_SERIAL vdate;
	char c;
	int pos;
	int pos_ampm = -1;
	struct tm date_tm;
	char token;
	int token_count;
	bool quote;
	
	local_current = &LOCAL_local;
	vdate = *date;

	switch(fmt_type)
	{
		case LF_USER:
			break;

		case LF_STANDARD:
			fmt = local_current->standard_date;
			break;
			
		case LF_GENERAL_DATE:
			if (date->year == 0)
			{
				if (date->hour == 0 && date->min == 0 && date->sec == 0)
				{
					*str = NULL;
					*len_str = 0;
					return FALSE;
				}
				fmt = local_current->long_time;
			}
			else
				fmt = local_current->general_date;
			break;

		default:
			fmt = LOCAL_get_format(local_current, fmt_type);
			if (!fmt)
				return TRUE;
	}

	if (len_fmt == 0)
		len_fmt = strlen(fmt);

	if (len_fmt >= COMMON_BUF_MAX)
		return TRUE;

	// looking for AM/PM

	for (pos = 0; pos < len_fmt - 4; pos++)
	{
		if (fmt[pos] == '\\')
		{
			pos++;
			continue;
		}

		if (strncasecmp(&fmt[pos], "am/pm", 5) == 0)
		{
			pos_ampm = pos;
			if (vdate.hour > 12)
				vdate.hour -= 12;
			else if (vdate.hour == 0)
				vdate.hour = 12;
			break;
		}
	}

	// formatting

	begin();

	token = 0;
	token_count = 0;
	quote = FALSE;

	for (pos = 0; pos < len_fmt; pos++)
	{
		c = fmt[pos];
		
		if (c == '\\')
		{
			pos++;
			c = fmt[pos];
			if (!c)
				break;
			c = 1; // something not null that cannot be a token
			quote = TRUE;
		}
		
		if (c == token)
		{
			token_count++;
			continue;
		}
		
		if (token)
			add_date_time_token(&vdate, token, token_count);
		
		if (c == 'd' || c == 'm' || c == 'y' || c == 'h' || c == 'n' || c == 's' || c == 'u' || c == 't')
		{
			token = c;
			token_count = 1;
			continue;
		}
		
		if (c == '/')
		{
			if (!DATE_SERIAL_has_no_date(date))
				add_date_separator(token);
		}
		else if (c == ':')
			add_time_separator(token);
		else if (pos == pos_ampm)
		{
			date_tm.tm_sec = date->sec;
			date_tm.tm_min = date->min;
			date_tm.tm_hour = date->hour;
			date_tm.tm_mday = 1;
			date_tm.tm_mon = 0;
			date_tm.tm_year = 0;

			add_strftime((c == 'a' ? "%P" : "%p"), &date_tm);

			pos += 4;
		}
		else
		{
			if (c != ' ' || token == 0 || COMMON_pos > 0)
				COMMON_put_char(quote ? fmt[pos] : c);
		}
		
		token = 0;
	}
	
	if (token)
		add_date_time_token(&vdate, token, token_count);

	// return the result

	end(str, len_str);
	return FALSE;
}


static void LOCAL_load_translation(ARCHIVE *arch)
{
	char *domain = NULL;
	char *lang_list;
	char *lang;
	char *src;
	char *test;
	char c;
	FILE *file;
	char *addr;
	int len;
	COMPONENT *save = COMPONENT_current;
	char *path;
	const char *dir;
	char *dst;

	/* We must force GB_LoadFile() to look in our archive, because all translation
		files of one language have the same path!
	*/

	if (arch)
	{
		domain = arch->domain;
		COMPONENT_current = COMPONENT_find(arch->name);
	}

	if (!domain)
		domain = "gb";

	#ifdef DEBUG_LANG
	fprintf(stderr, "LOCAL_load_translation: %s / domain = %s\n", arch ? arch->name : "NULL", domain);
	#endif

	lang_list = get_languages();

	lang = strtok(lang_list, ":");

	for(;;)
	{
		if (!lang)
			break;

		if (*lang)
		{
			test = STRING_new_zero(lang);
			src = test;

			for(;;)
			{
				c = *src;
				if (c == 0 || c == '_')
					break;
				*src = tolower(c);
				src++;
			}

			test = STRING_add(test, ".mo", 3);

			dir = arch ? ".lang" : ".../.lang";
			
			FILE_dir_first(dir, test, 0);
			STRING_free_real(test);
			
			if (!FILE_dir_next(&path, &len))
			{
				dst = STRING_new_zero(dir);
				dst = STRING_add_char(dst, '/');
				dst = STRING_add(dst, path, len);
				break;
			}
			
			/*if (!arch)
				dst = FILE_cat("...", ".lang", test, NULL);
			else
				dst = FILE_cat(".lang", test, NULL);
			
			dst = FILE_set_ext(dst, "mo");

			#ifdef DEBUG_LANG
			fprintf(stderr, "trying %s\n", dst);
			#endif

			if (FILE_exist(dst))
				break;*/
		}

		lang = strtok(NULL, ":");
	}

	if (!lang)
	{
		#ifdef DEBUG_LANG
		fprintf(stderr, "No translation\n");
		#endif
		goto __NOTRANS;
	}

	#ifdef DEBUG_LANG
	fprintf(stderr, "Loading %s\n", dst);
	#endif

	STRING_free_later(dst);
	if (GB_LoadFile(dst, 0, &addr, &len))
	{
		#ifdef DEBUG_LANG
		fprintf(stderr, "Cannot load %s\n", dst);
		#endif
		goto __ERROR;
	}

	// These temporary files have predictable names because they
	// are *.mo files read by the gettext system.

	dir = FILE_cat(FILE_make_temp(NULL, NULL), "tr", NULL);
	mkdir(dir, S_IRWXU);

	dir = FILE_cat(FILE_make_temp(NULL, NULL), "tr", LOCAL_get_lang(), NULL);
	mkdir(dir, S_IRWXU);

	dir = FILE_cat(dir, "LC_MESSAGES", NULL);
	mkdir(dir, S_IRWXU);

	dir = FILE_cat(dir, domain, NULL);
	strcat((char *)dir, ".mo");

	unlink(dir);

	#ifdef DEBUG_LANG
	fprintf(stderr, "Writing to %s (%d bytes)\n", dir, len);
	#endif

	// No need to test previous system calls as the failure will be detected now

	file = fopen(dir, "w");
	if (file)
	{
		fwrite(addr, len, 1, file);
		fclose(file);
	}

	GB_ReleaseFile(addr, len);

__ERROR:

	// If the *.mo was not copied, then the following functions will failed

	#ifdef DEBUG_LANG

		fprintf(stderr, "bindtextdomain: %s\n", bindtextdomain(domain, FILE_cat(FILE_make_temp(NULL, NULL), "tr", NULL)));
		fprintf(stderr, "bind_textdomain_codeset: %s\n", bind_textdomain_codeset(domain, "UTF-8"));
		if (!arch)
			fprintf(stderr, "textdomain: %s\n", textdomain(domain));

	#else

		bindtextdomain(domain, FILE_cat(FILE_make_temp(NULL, NULL), "tr", NULL));
		#ifdef OS_SOLARIS
		fprintf(stderr, "Warning: bind_textdomain_codeset() unavailable.\n");
		#else
		bind_textdomain_codeset(domain, "UTF-8");
		#endif
		if (!arch)
			textdomain(domain); /* default domain */

	#endif

__NOTRANS:

	STRING_free(&lang_list);

	if (arch)
		arch->translation_loaded = TRUE;
	else
		_translation_loaded = TRUE;

	COMPONENT_current = save;
}


const char *LOCAL_gettext(const char *msgid)
{
	const char *tr = msgid;
	ARCHIVE *arch = NULL;

	/*
			If LOCAL_gettext() is called, then we are in the context of
			the archive, so the translation loaded will be the good one.
	*/

	if (!msgid)
		return "";

	if (!ARCHIVE_get_current(&arch))
	{
		#ifdef DEBUG_LANG
		fprintf(stderr, "dgettext(\"%s\", \"%s\")\n", arch->domain, msgid);
		#endif
		if (!arch->translation_loaded)
			LOCAL_load_translation(arch);
		tr = dgettext(arch->domain, msgid);
	}

	if (tr == msgid)
	{
		#ifdef DEBUG_LANG
		fprintf(stderr, "dgettext(\"%s\", \"%s\")\n", "gb", msgid);
		#endif
		if (!_translation_loaded)
			LOCAL_load_translation(NULL);
		tr = dgettext("gb", msgid);
		//tr = gettext(msgid);
	}

	/*printf("tr: %s -> %s\n", msgid, tr);*/

	if (!tr || tr[0] == 0 || (tr[0] == '-' && (tr[1] == 0 || (tr[1] == '\n' && tr[2] == 0))))
		tr = msgid;

	#ifdef DEBUG_LANG
	fprintf(stderr, "--> \"%s\"\n", tr);
	#endif
	
	return tr;
}

int LOCAL_get_first_day_of_week()
{
	const char *lang;

	if (LOCAL_first_day_of_week >= 0)
		return LOCAL_first_day_of_week;

	lang = LOCAL_get_lang();

	if (strcmp(lang, "en") == 0 || strncmp(lang, "en_", 3) == 0 || strcmp(lang, "C") == 0)
		return 0;
	else
		return 1;
}

void LOCAL_set_first_day_of_week(char day)
{
	if (day >= -1 && day <= 6)
		LOCAL_first_day_of_week = day;
}
