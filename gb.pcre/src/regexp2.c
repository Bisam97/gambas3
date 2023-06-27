/***************************************************************************

  regexp2.c

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

#define __REGEXP_C

#include "gb_common.h"

#include "regexp2.h"
#include "main.h"

//#define DEBUG_REPLACE 1

#define PCRE2_GREEDY 0x80000000 // The highest possible pcre constant. It must be not used by pcre of course!
#define PCRE2_EXTRA  0x80000001

DECLARE_METHOD(RegExp_free);

//---------------------------------------------------------------------------

static void return_error(void *_object, int err_code)
{
	PCRE2_UCHAR8 err_msg[128];
	THIS->error = err_code;
	pcre2_get_error_message(err_code, err_msg, sizeof(err_msg)); 
	GB.Error((char *)err_msg);
}

static void compile(void *_object)
{
	int err_code;
	PCRE2_SIZE err_offset;

	if (!THIS->pattern) {
		GB.Error("No pattern provided");
		return;
	}

	if (THIS->code)
		free(THIS->code);

	THIS->code = pcre2_compile((PCRE2_SPTR)THIS->pattern, GB.StringLength(THIS->pattern), THIS->copts, &err_code, &err_offset, NULL);

	if (!THIS->code)
		return_error(THIS, err_code);
}

static void exec(void *_object, int lsubject)
{
	int ret;
	
	if (!THIS->code) 
	{
		GB.Error("No pattern compiled yet");
		return;
	}
	
	if (lsubject < 0)
		lsubject = GB.StringLength(THIS->subject);

	if (!THIS->subject)
	{
		GB.Error("No subject provided");
		return;
	}

	pcre2_match_data_free(THIS->match); // does nothing if THIS->match is NULL
	THIS->match = pcre2_match_data_create_from_pattern(THIS->code, NULL);
	THIS->ovector = NULL;
	
	THIS->error = 0;
	
	ret = pcre2_match(THIS->code,
			(PCRE2_SPTR)THIS->subject,
			lsubject,
			0,
			THIS->eopts,
			THIS->match,
			NULL);
	
	if (ret > 0)
	{
		THIS->count = ret;
		THIS->ovector = pcre2_get_ovector_pointer(THIS->match);
	}
	else if (ret < 0)
	{
		if (ret == PCRE2_ERROR_NOMATCH)
		{
			THIS->count = 0;
		}
		else
		{
			THIS->error = ret;
			return_error(THIS, ret);
		}
	}
}

static void return_match(void *_object, int index)
{
	int len;

	if (index < 0 || index >= THIS->count)
	{
		GB.Error("Out of bounds");
		return;
	}
	
	index *= 2;
	len = THIS->ovector[index + 1] - THIS->ovector[index];
	if (len <= 0)
		GB.ReturnVoidString();
	else
		GB.ReturnNewString(&THIS->subject[THIS->ovector[index]], len);
}

bool REGEXP_match(const char *subject, int lsubject, const char *pattern, int lpattern, int coptions, int eoptions)
{
	/*
	 * The gb.pcre internal routines don't require the GB_BASE to be
	 * initialised by Gambas!
	 */
	
	CREGEXP tmp;
	bool ret = FALSE;

	if (lsubject <= 0)
		return (lpattern <= 0);

	CLEAR(&tmp);
	tmp.copts = coptions;
	tmp.pattern = GB.NewString(pattern, lpattern);

	compile(&tmp);
	
	if (tmp.code) 
	{
		tmp.eopts = eoptions;
		tmp.subject = GB.NewString(subject, lsubject);

		exec(&tmp, -1);
		ret = (tmp.ovector && tmp.ovector[0] != -1);
	}

	RegExp_free(&tmp, NULL);
	
	return ret;
}

//---------------------------------------------------------------------------

BEGIN_METHOD(RegExp_Compile, GB_STRING pattern; GB_INTEGER coptions)

	THIS->copts = VARGOPT(coptions, 0);

	GB.FreeString(&THIS->pattern);
	THIS->pattern = GB.NewString(STRING(pattern), LENGTH(pattern));
	compile(THIS);

END_METHOD


BEGIN_METHOD(RegExp_Exec, GB_STRING subject; GB_INTEGER eoptions)

	THIS->eopts = VARGOPT(eoptions, 0);
	
	GB.FreeString(&THIS->subject);
	THIS->subject = GB.NewString(STRING(subject), LENGTH(subject));
	exec(THIS, -1);

END_METHOD


BEGIN_METHOD(RegExp_new, GB_STRING subject; GB_STRING pattern; GB_INTEGER coptions; GB_INTEGER eoptions)

	if (MISSING(pattern)) // the user didn't provide a pattern.
		return;

	THIS->copts = VARGOPT(coptions, 0);
	THIS->pattern = GB.NewString(STRING(pattern), LENGTH(pattern));
	THIS->code = NULL;

	compile(THIS);
	if (!THIS->code) // we didn't get a compiled pattern back.
			return;

	if (MISSING(subject)) // the user didn't specify any subject text.
		return;

	THIS->eopts = VARGOPT(eoptions, 0);
	THIS->subject = GB.NewString(STRING(subject), LENGTH(subject));

	exec(THIS, -1);

END_METHOD


BEGIN_METHOD_VOID(RegExp_free)

	if (THIS->code)
		free(THIS->code);
	GB.FreeString(&THIS->subject);
	GB.FreeString(&THIS->pattern);
	pcre2_match_data_free(THIS->match); // does nothing if THIS->match is NULL

END_METHOD


BEGIN_METHOD(RegExp_Match, GB_STRING subject; GB_STRING pattern; GB_INTEGER coptions; GB_INTEGER eoptions)

	GB.ReturnBoolean(REGEXP_match(STRING(subject), LENGTH(subject), STRING(pattern), LENGTH(pattern), VARGOPT(coptions, 0), VARGOPT(eoptions, 0)));

END_METHOD


BEGIN_PROPERTY(RegExp_Pattern)

	GB.ReturnString(THIS->pattern);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Subject)

	GB.ReturnString(THIS->subject);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Offset)

	if (THIS->ovector)
		GB.ReturnInteger(THIS->ovector[0]);
	else
		GB.ReturnInteger(0);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Text)

	if (THIS->count == 0)
		GB.ReturnVoidString();
	else
		return_match(THIS, 0);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Error)

	GB.ReturnInteger(THIS->error);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Submatches)

	GB.Deprecated("gb.pcre", "Regexp.Submatches", NULL);
	GB.ReturnSelf(THIS);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Submatches_Count)

	GB.ReturnInteger(THIS->count - 1);

END_PROPERTY


BEGIN_METHOD(RegExp_Submatches_get, GB_INTEGER index)

	int index = VARG(index);

	if (index < 0 || index >= THIS->count)
	{
		GB.Error("Out of bounds");
		return;
	}
	
	THIS->_submatch = index;
	RETURN_SELF();

END_METHOD


BEGIN_PROPERTY(RegExp_Submatch_Text)

	return_match(THIS, THIS->_submatch);

END_PROPERTY


BEGIN_PROPERTY(RegExp_Submatch_Offset)

	GB.ReturnInteger(THIS->ovector ? THIS->ovector[2 * THIS->_submatch] : 0);

END_PROPERTY


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
	if (r.copts & PCRE2_GREEDY)
		r.copts &= ~PCRE2_GREEDY;
	else
		r.copts |= PCRE2_UNGREEDY;
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
			if (!r.ovector || r.ovector[0] < 0)
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

//---------------------------------------------------------------------------

GB_DESC CRegexpDesc[] =
{
	GB_DECLARE("RegExp", sizeof(CREGEXP)),

	GB_METHOD("_new", NULL, RegExp_new, "[(Subject)s(Pattern)s(CompileOptions)i(ExecOptions)i]"),
	GB_METHOD("_free", NULL, RegExp_free, NULL),
	
	GB_METHOD("Compile", NULL, RegExp_Compile, "(Pattern)s[(CompileOptions)i]"),
	GB_METHOD("Exec", NULL, RegExp_Exec, "(Subject)s[(ExecOptions)i]"),

	GB_STATIC_METHOD("Match", "b", RegExp_Match, "(Subject)s(Pattern)s[(CompileOptions)i(ExecOptions)i]"),
	GB_STATIC_METHOD("Replace", "s", RegExp_Replace, "(Subject)s(Pattern)s(Replace)s[(CompileOptions)i(ExecOptions)i]"),

	GB_CONSTANT("Caseless", "i", PCRE2_CASELESS),
	GB_CONSTANT("MultiLine", "i", PCRE2_MULTILINE),
	GB_CONSTANT("DotAll", "i", PCRE2_DOTALL),
	GB_CONSTANT("Extended", "i", PCRE2_EXTENDED),
	GB_CONSTANT("Anchored", "i", PCRE2_ANCHORED),
	GB_CONSTANT("DollarEndOnly", "i", PCRE2_DOLLAR_ENDONLY),
	GB_CONSTANT("Extra", "i", PCRE2_EXTRA),
	GB_CONSTANT("NotBOL", "i", PCRE2_NOTBOL),
	GB_CONSTANT("NotEOL", "i", PCRE2_NOTEOL),
	GB_CONSTANT("Ungreedy", "i", PCRE2_UNGREEDY),
	GB_CONSTANT("NotEmpty", "i", PCRE2_NOTEMPTY),
	GB_CONSTANT("UTF8", "i", PCRE2_UTF),
	GB_CONSTANT("NoAutoCapture", "i", PCRE2_NO_AUTO_CAPTURE),
	GB_CONSTANT("NoUTF8Check", "i", PCRE2_NO_UTF_CHECK),
	
	GB_CONSTANT("NoMatch", "i", PCRE2_ERROR_NOMATCH),
	GB_CONSTANT("Null", "i", PCRE2_ERROR_NULL),
	GB_CONSTANT("BadOption", "i", PCRE2_ERROR_BADOPTION),
	GB_CONSTANT("BadMagic", "i", PCRE2_ERROR_BADMAGIC),
	GB_CONSTANT("UnknownNode", "i", PCRE2_ERROR_INTERNAL),
	GB_CONSTANT("NoMemory", "i", PCRE2_ERROR_NOMEMORY),
	GB_CONSTANT("NoSubstring", "i", PCRE2_ERROR_NOSUBSTRING),
	GB_CONSTANT("MatchLimit", "i", PCRE2_ERROR_MATCHLIMIT),
	GB_CONSTANT("Callout", "i", PCRE2_ERROR_CALLOUT),
	GB_CONSTANT("BadUTF8", "i", PCRE2_ERROR_UTF8_ERR1),
	GB_CONSTANT("BadUTF8Offset", "i", PCRE2_ERROR_BADUTFOFFSET),
	GB_CONSTANT("Greedy", "i", PCRE2_GREEDY),

	GB_PROPERTY_READ("SubMatches", ".Regexp.Submatches", RegExp_Submatches),
	
	GB_PROPERTY_READ("Text", "s", RegExp_Text), /* this is the string matched by the entire pattern */
	GB_PROPERTY_READ("Offset", "i", RegExp_Offset), /* this is the string matched by the entire pattern */
	GB_PROPERTY_READ("Pattern", "s", RegExp_Pattern),
	GB_PROPERTY_READ("Subject", "s", RegExp_Subject),
	GB_PROPERTY_READ("Error", "i", RegExp_Error),

	GB_METHOD("_get", ".Regexp.Submatch", RegExp_Submatches_get, "(Index)i"),
	GB_PROPERTY_READ("Count", "i", RegExp_Submatches_Count),

	GB_END_DECLARE
};

GB_DESC CRegexpSubmatchesDesc[] =
{
	GB_DECLARE(".Regexp.Submatches", 0), GB_VIRTUAL_CLASS(),

	GB_METHOD("_get", ".Regexp.Submatch", RegExp_Submatches_get, "(Index)i"),
	GB_PROPERTY_READ("Count", "i", RegExp_Submatches_Count),

	GB_END_DECLARE
};

GB_DESC CRegexpSubmatchDesc[] =
{
	GB_DECLARE(".Regexp.Submatch", 0), GB_VIRTUAL_CLASS(),

	GB_PROPERTY_READ("Offset", "i", RegExp_Submatch_Offset),
	GB_PROPERTY_READ("Text", "s", RegExp_Submatch_Text),

	GB_END_DECLARE
};

