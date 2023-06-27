/***************************************************************************

  gbx_test.c

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

#define __GBX_TEST_C

#include "gb_common.h"
#include "gbx_class.h"
#include "gbx_api.h"
#include "gbx_project.h"
#include "gbx_exec.h"
#include "gbx_test.h"

static CLASS_DESC_METHOD *find_method(const char *name)
{
	CLASS_DESC_METHOD *method = (CLASS_DESC_METHOD *)CLASS_get_symbol_desc_kind(PROJECT_class, name, CD_STATIC_METHOD, 0, T_ANY);
	if (!method)
		ERROR_panic("Test.%s() method unavailable", name);
	return method;
}

static void error_test_run(CARRAY *tests)
{
	OBJECT_UNREF(tests);
}

void TEST_run(const char *test_list)
{
	CLASS_DESC_METHOD *startup;
	CARRAY *tests;
	int i;
	char *test;
	char *p;
	char *name;
	CLASS *class;
	

	if (!test_list || !*test_list)
	{
		EXEC_public_desc(PROJECT_class, NULL, find_method("_list"), 0);
		return;
	}
	
	startup = find_method("_add");
	
	tests = STRING_split(test_list, strlen(test_list), ",", 1, NULL, 0, TRUE, FALSE);

	ON_ERROR_1(error_test_run, tests)
	{
		for (i = 0; i < tests->count; i++)
		{
			test = *(char **)CARRAY_get_data_unsafe(tests, i);
			p = index(test, '.');
			if (p)
				name = STRING_new_temp(test, p - test);
			else
				name = test;

			class = CLASS_find(name);
			CLASS_load(class);
			if (!class->is_test)
				continue;
			
			GB_Push(2, T_OBJECT, class, T_STRING, test, STRING_length(test));
			EXEC_public_desc(PROJECT_class, NULL, startup, 2);
		}
	}
	END_ERROR

	OBJECT_UNREF(tests);
	
	EXEC_public_desc(PROJECT_class, NULL, find_method("_run"), 0);
}

