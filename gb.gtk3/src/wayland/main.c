/***************************************************************************

  main.c

  (c) Benoît Minisini <gambas@users.sourceforge.net>

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

#include <gdk/gdkwayland.h>
#include "main.h"

//-------------------------------------------------------------------------

static void platform_init(void)
{
}

static void platform_exit(void)
{
}

static GtkWidget *platform_create_plug(long wid)
{
	return NULL;
}

//-------------------------------------------------------------------------

static uintptr_t window_get_id(GdkWindow *window)
{
	return (uintptr_t)gdk_wayland_window_get_wl_surface(window);
}

//-------------------------------------------------------------------------

bool desktop_has_system_tray()
{
	return FALSE;
}

void desktop_show_tray_icon(GtkStatusIcon *icon, int width, int height)
{
}
//-------------------------------------------------------------------------

const GB_INTERFACE *GB_PTR EXPORT;

void *GB_GTK3_WAYLAND_1[] EXPORT = {

  (void *)GTK_PLATFORM_INTERFACE_VERSION,
  
  (void *)platform_init,
  (void *)platform_exit,
  
  (void *)platform_create_plug,
  
  (void *)window_get_id,
  
  (void *)desktop_has_system_tray,
  (void *)desktop_show_tray_icon,

  /*(void *)release_grab,
  (void *)unrelease_grab,
  (void *)get_last_key_code,
  
  (void *)desktop_get_resolution_x,
  (void *)desktop_get_resolution_y,
  (void *)desktop_screenshot,
  
  (void *)window_get_virtual_desktop,
  (void *)window_set_virtual_desktop,
  (void *)window_remap,
  (void *)window_set_properties,
  (void *)window_set_user_time,
  (void *)window_set_transient_for,*/
  
  NULL
  };


int EXPORT GB_INFO(const char *key, void **value)
{
	/*if (!strcasecmp(key, "DISPLAY"))
	{
		*value = (void *)QX11Info::display();
		return TRUE;
	}
	else if (!strcasecmp(key, "ROOT_WINDOW"))
	{
		*value = (void *)QX11Info::appRootWindow();
		return TRUE;
	}
	else if (!strcasecmp(key, "TIME"))
	{
		*value = (void *)QX11Info::appTime();
		return TRUE;
	}
	else*/
		return FALSE;
}

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}

