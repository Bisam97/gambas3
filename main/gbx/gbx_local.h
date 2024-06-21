/***************************************************************************

	gbx_local.h

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

#ifndef __GBX_LOCAL_H
#define __GBX_LOCAL_H

#ifndef GBX_INFO

#include <locale.h>
#include "gbx_date.h"
#include "gbx_archive.h"

#endif

#ifndef GBX_INFO

enum {
	LO_HOUR = 1,
	LO_MINUTE = 2,
	LO_SECOND = 3,
	LO_YEAR = 1,
	LO_MONTH = 2,
	LO_DAY = 3
	};

typedef
	struct {
		char decimal_point;
		char currency_decimal_point;
		const char *thousand_sep;
		uchar len_thousand_sep;
		const char *currency_thousand_sep;
		uchar len_currency_thousand_sep;
		char group_size;
		char currency_group_size;
		unsigned char currency_flag;
		uint date_sep[4];
		uint time_sep[4];
		const char *currency_symbol;
		const char *intl_currency_symbol;
		uchar date_order[4];
		uchar time_order[4];
		char *long_date;
		char *medium_date;
		char *short_date;
		char *long_time;
		char *medium_time;
		char *short_time;
		char *general_date;
		char *standard_date;
		char *general_currency;
		char *intl_currency;
		char *true_str;
		uchar len_true_str;
		char *false_str;
		uchar len_false_str;
		unsigned rtl : 1;
		unsigned date_many_sep : 1;
		unsigned date_tail_sep : 1;
		unsigned time_many_sep : 1;
		unsigned time_tail_sep : 1;
		}
	LOCAL_INFO;

#ifndef __GBX_LOCAL_C
EXTERN LOCAL_INFO LOCAL_default, LOCAL_local;
EXTERN char *LOCAL_encoding;
EXTERN bool LOCAL_is_UTF8;
EXTERN char LOCAL_first_day_of_week;
#endif


void LOCAL_init(void);
void LOCAL_exit(void);
bool LOCAL_format_number(double number, int fmt_type, const char *fmt, int len_fmt, char **str, int *len_str, bool local);
bool LOCAL_format_date(const DATE_SERIAL *date, int fmt_type, const char *fmt, int len_fmt, char **str, int *len_str);
const char *LOCAL_get_lang(void);
void LOCAL_set_lang(const char *lang);
const char *LOCAL_gettext(const char *msgid);
//void LOCAL_load_translation(ARCHIVE *arch);
int LOCAL_get_first_day_of_week();
void LOCAL_set_first_day_of_week(char day);
const char *LOCAL_get_format(LOCAL_INFO *info, int type);

#define LOCAL_get(_local) ((_local) ? &LOCAL_local : &LOCAL_default)

#endif

#endif
