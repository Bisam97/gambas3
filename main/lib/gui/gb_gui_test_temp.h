/***************************************************************************

  gb_gui_test_temp.h

  (c) 2000-2020 Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __GB_GUI_TEST_TEMP_H

static char *make_name(const char *prefix, const char *suffix)
{
	static char buffer[32];
	
	snprintf(buffer, sizeof(buffer), "%s.%s", prefix, suffix);
	return buffer;
}

static const char *GUI_can_use(int use)
{
	static const char *ext[] = { "ext", "webkit", "opengl", "webview", NULL };
	const char **pext;
	const char *name;
	char *child;
	
	name = get_name(use);
	
	if (!GB.Component.CanLoadLibrary(name))
		return name;

	for (pext = ext; *pext; pext++)
	{
		if (!GB.Component.Exist(make_name("gb.gui", *pext)))
			continue;
		
		child = make_name(name, *pext);
		if (!GB.Component.CanLoadLibrary(child))
			return child;
		if (_debug)
			fprintf(stderr, "  %s OK\n", child);
	}
	
	return NULL;
}

static int GUI_should_use()
{
	const char *env = getenv("XDG_CURRENT_DESKTOP");
	
	if (env && strncasecmp(env, "KDE", 3) == 0)
	{
		env = getenv("KDE_FULL_SESSION");
		
		if (env && !strcmp(env, "true"))
		{
			env = getenv("KDE_SESSION_VERSION");
			if (env)
			{
				if (strcmp(env, "4") == 0)
					return USE_GB_QT4;
				else if (strcmp(env, "5") == 0)
					return USE_GB_QT5;
			}
		}
	}

	return USE_NOTHING;	
}

 
