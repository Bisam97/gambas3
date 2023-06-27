/***************************************************************************

  main.c

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

#define __MAIN_C

#include "main.h"

enum { USE_NOTHING, USE_GB_QT4, USE_GB_QT5 };

const GB_INTERFACE *GB_PTR EXPORT;

// Prevents gbi3 from complaining

GB_DESC *GB_CLASSES[] EXPORT =
{
  NULL
};

char *GB_INCLUDE EXPORT = "gb.qt4|gb.qt5";

static bool _debug = FALSE;


const char *get_name(int use)
{
	switch (use)
	{
		case USE_GB_QT4: return "gb.qt4";
		default: return "gb.qt5";
	}
}

#include "gb_gui_test_temp.h"

int EXPORT GB_INIT(void)
{
	int use = USE_NOTHING;
	int use_other = USE_NOTHING;
	char *env;
	const char *comp;
	const char *fail;
	char not_found[32];

	env = getenv("GB_GUI");
	if (env && *env)
	{
		if (strcmp(env, "gb.qt4") == 0)
			use = USE_GB_QT4;
		else if (strcmp(env, "gb.qt5") == 0)
			use = USE_GB_QT5;
		else
			fprintf(stderr, "gb.gui.qt: warning: '%s' component not supported\n", env);
	}
	
	env = getenv("GB_GUI_DEBUG");
	if (env && strcmp(env, "0")) 
		_debug = TRUE;
	
	if (use == USE_NOTHING)
	{
		use = GUI_should_use();
		if (use == USE_NOTHING)
			use = USE_GB_QT5;
	}

	if (_debug)
		fprintf(stderr, "gb.gui: checking %s...\n", get_name(use));
	
	fail = GUI_can_use(use);
	if (fail)
	{
		strcpy(not_found, fail);

		if (use == USE_GB_QT4)
			use_other = USE_GB_QT5;
		else
			use_other = USE_GB_QT4;
		
		if (!GUI_can_use(use_other))
		{
			fprintf(stderr, "gb.gui.qt: warning: '%s' component not found, using '%s' instead\n", not_found, get_name(use_other));
			use = use_other;
		}
		else
		{
			fprintf(stderr, "gb.gui.qt: error: '%s' component not found, unable to find any QT replacement component\n", not_found);
			exit(1);
		}
	}
	
	comp = get_name(use);
	
	if (GB.Component.Load(comp))
	{
		fprintf(stderr, "gb.gui.qt: error: cannot load component '%s'\n", comp);
		exit(1);
	}
	else
	{
		if (_debug)
			fprintf(stderr, "gb.gui.qt: loading '%s'\n", comp);
	}

	setenv("GB_GUI", comp, TRUE);

  return 0;
}

void EXPORT GB_EXIT()
{
}

