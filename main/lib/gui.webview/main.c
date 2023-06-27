/***************************************************************************

  main.c

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

#define __MAIN_C

#include "main.h"

GB_INTERFACE GB EXPORT;

GB_DESC *GB_CLASSES[] EXPORT =
{
  NULL
};

char *GB_INCLUDE EXPORT = "gb.qt5.webview|gb.gtk3.webview|gb.qt4.webview";

int EXPORT GB_INIT(void)
{
	const char *comp = NULL;
	char *env;
	
	env = getenv("GB_GUI");
	if (env)
	{
		if (strcmp(env, "gb.qt5") == 0)
			comp = "gb.qt5.webview";
		else if (strcmp(env, "gb.qt4") == 0)
			comp = "gb.qt4.webview";
		else if (strcmp(env, "gb.gtk3") == 0)
			comp = "gb.gtk3.webview";
	}
	
	if (!comp)
	{
		// GB_GUI should be set by gb.gui
		if (!env || !*env)
			fprintf(stderr, "gb.gui.webview: error: no component specified in GB_GUI environment variable\n");
		else
			fprintf(stderr, "gb.gui.webview: error: unsupported component specified in GB_GUI environment variable\n");
		exit(1);
	}
		
	if (GB.Component.Load(comp))
		fprintf(stderr, "gb.gui.webview: unable to load '%s' component\n", comp);
  
  return 0;
}

void EXPORT GB_EXIT()
{
}


