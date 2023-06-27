/***************************************************************************

  regexp2.h

  (c) Rob Kudla <pcre-component@kudla.org>
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

#ifndef __REGEXP_H
#define __REGEXP_H

#include "gambas.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

#ifndef __REGEXP_C

extern GB_DESC CRegexpDesc[];
extern GB_DESC CRegexpSubmatchesDesc[];
extern GB_DESC CRegexpSubmatchDesc[];

#else

typedef
	struct 
	{
		GB_BASE ob;
		char *subject;
		char *pattern;
		pcre2_match_data *match;
		PCRE2_SIZE *ovector;
		int count;
		int eopts;
		int copts;
		pcre2_code *code;
		int _submatch;
		int error;
	}
	CREGEXP;

#define THIS OBJECT(CREGEXP)

#endif

bool REGEXP_match(const char *subject, int lsubject, const char *pattern, int lpattern, int coptions, int eoptions);

#endif
