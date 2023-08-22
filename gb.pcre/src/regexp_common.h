/***************************************************************************

  regexp_common.h

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

#ifndef __REGEXP_COMMON_H
#define __REGEXP_COMMON_H

static CREGEXP *_subst_regexp = NULL;


static void subst_get_submatch(int index, const char **p, int *lp)
{
	if (index <= 0 || index >= _subst_regexp->count)
	{
		*p = NULL;
		*lp = 0;
	}
	else
	{
		index *= 2;
		*p = &_subst_regexp->subject[_subst_regexp->ovector[index]];
		*lp = _subst_regexp->ovector[index + 1] - _subst_regexp->ovector[index];
	}
}


BEGIN_METHOD(RegExp_Replace, GB_STRING subject; GB_STRING pattern; GB_STRING replace; GB_INTEGER coptions; GB_INTEGER eoptions)

	CREGEXP r;
	char *replace;
	char *result = NULL;
	char *subject;
	int offset;

	CLEAR(&r);

	r.copts = VARGOPT(coptions, 0);

#if PCRE2

	if (r.copts & PCRE2_GREEDY)
		r.copts &= ~PCRE2_GREEDY;
	else
		r.copts |= PCRE2_UNGREEDY;

#else

	r.ovecsize = OVECSIZE_INC;
	GB.Alloc(POINTER(&r.ovector), sizeof(int) * r.ovecsize);

	if (r.copts & PCRE_GREEDY)
		r.copts &= ~PCRE_GREEDY;
	else
		r.copts |= PCRE_UNGREEDY;

#endif

	r.pattern = GB.NewString(STRING(pattern), LENGTH(pattern));

	compile(&r);

	if (r.code)
	{
		r.eopts = VARGOPT(eoptions, 0);
		subject = GB.NewString(STRING(subject), LENGTH(subject));

		offset = 0;

		while (offset < LENGTH(subject))
		{
			r.subject = &subject[offset];
			#if DEBUG_REPLACE
			fprintf(stderr, "\nsubject: (%d) %s\n", offset, r.subject);
			#endif
			exec(&r, GB.StringLength(subject) - offset);

			if (r.ovector[0] < 0)
				break;

			_subst_regexp = &r;

			if (r.ovector[0] > 0)
			{
			#if DEBUG_REPLACE
				fprintf(stderr, "add: (%d) %.*s\n", r.ovector[0], r.ovector[0], r.subject);
			#endif
				result = GB.AddString(result, r.subject, r.ovector[0]);
			#if DEBUG_REPLACE
				fprintf(stderr, "result = %s\n", result);
			#endif
			}

			replace = GB.SubstString(STRING(replace), LENGTH(replace), (GB_SUBST_CALLBACK)subst_get_submatch);
			#if DEBUG_REPLACE
			fprintf(stderr, "replace = %s\n", replace);
			#endif
			result = GB.AddString(result, replace, GB.StringLength(replace));
			#if DEBUG_REPLACE
			fprintf(stderr, "result = %s\n", result);
			#endif

			offset += r.ovector[1];

			if (*r.pattern == '^')
				break;
		}

		if (offset < LENGTH(subject))
			result = GB.AddString(result, &subject[offset], LENGTH(subject) - offset);

		_subst_regexp = NULL;

		GB.FreeStringLater(result);
		#if DEBUG_REPLACE
		fprintf(stderr, "result = %s\n", result);
		#endif
		r.subject = subject;
	}

	RegExp_free(&r, NULL);

	GB.ReturnString(result);

END_METHOD


BEGIN_METHOD(RegExp_FindAll, GB_STRING subject; GB_STRING pattern; GB_INTEGER coptions; GB_INTEGER eoptions)

	CREGEXP r;
	GB_ARRAY result;
	char *subject;
	int offset;

	CLEAR(&r);

	r.copts = VARGOPT(coptions, 0);

	/*if (r.copts & PCRE2_GREEDY)
		r.copts &= ~PCRE2_GREEDY;
	else
		r.copts |= PCRE2_UNGREEDY;*/

	r.pattern = GB.NewString(STRING(pattern), LENGTH(pattern));

	compile(&r);

	GB.Array.New(&result, GB_T_STRING, 0);

	if (r.code)
	{
		r.eopts = VARGOPT(eoptions, 0);
		subject = GB.NewString(STRING(subject), LENGTH(subject));

		offset = 0;

		while (offset < LENGTH(subject))
		{
			r.subject = &subject[offset];
			exec(&r, GB.StringLength(subject) - offset);
			if (!r.ovector || r.ovector[0] < 0)
				break;

			*(char **)GB.Array.Add(result) = GB.NewString(&r.subject[r.ovector[0]], r.ovector[1] - r.ovector[0]);

			/*replace = GB.SubstString(STRING(replace), LENGTH(replace), (GB_SUBST_CALLBACK)subst_get_submatch);
			#if DEBUG_REPLACE
			fprintf(stderr, "replace = %s\n", replace);
			#endif
			result = GB.AddString(result, replace, GB.StringLength(replace));
			#if DEBUG_REPLACE
			fprintf(stderr, "result = %s\n", result);
			#endif*/

			offset += r.ovector[1];

			if (*r.pattern == '^')
				break;
		}

		r.subject = subject;
	}

	RegExp_free(&r, NULL);

	GB.ReturnObject(result);

END_METHOD

#endif

