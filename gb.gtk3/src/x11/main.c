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

#include "x11.h"
#include "main.h"

const GB_INTERFACE *GB_PTR EXPORT;

//-------------------------------------------------------------------------

static void platform_init(void)
{
	char *env;

	env = getenv("GB_X11_INIT_THREADS");
	if (env && atoi(env))
		XInitThreads();

	X11_init(gdk_x11_display_get_xdisplay(gdk_display_get_default()), gdk_x11_get_default_root_xwindow());
}

static void platform_exit(void)
{
	X11_exit();
}

static GtkWidget *platform_create_plug(Window id)
{
	return gtk_plug_new(id);
}

//-------------------------------------------------------------------------

static long window_get_id(GdkWindow *window)
{
	return window ? GDK_WINDOW_XID(window) : 0;
}

//-------------------------------------------------------------------------

static bool desktop_has_system_tray()
{
	return X11_get_system_tray() != 0;
}

static void desktop_show_tray_icon(GtkStatusIcon *icon, int width, int height)
{
	XSizeHints hints;
	hints.flags = PMinSize;
	hints.min_width = width;
	hints.min_height = height;
	XSetWMNormalHints(gdk_x11_display_get_xdisplay(gdk_display_get_default()), gtk_status_icon_get_x11_window_id(icon), &hints);
}

//-------------------------------------------------------------------------

static GdkFilterReturn x11_event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	((X11_EVENT_FILTER)data)((XEvent *)xevent);
	return GDK_FILTER_CONTINUE;
}

static void x11_set_event_filter(X11_EVENT_FILTER filter)
{
	static X11_EVENT_FILTER save_filter = NULL;
	static GdkEventMask save_mask = (GdkEventMask)0;

	if (save_filter)
	{
		gdk_window_remove_filter(NULL, (GdkFilterFunc)x11_event_filter, (gpointer)save_filter);
		gdk_window_set_events(gdk_get_default_root_window(), save_mask);
	}

	if (filter)
	{
		save_mask = gdk_window_get_events(gdk_get_default_root_window());
		gdk_window_set_events(gdk_get_default_root_window(), (GdkEventMask)(save_mask | GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK));
		gdk_window_add_filter(NULL, (GdkFilterFunc)x11_event_filter, (gpointer)filter);
	}

	save_filter = filter;
}

//-------------------------------------------------------------------------

void *GB_GTK3_X11_1[] EXPORT = {

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
	if (!strcasecmp(key, "DISPLAY"))
	{
		*value = (void *)gdk_x11_display_get_xdisplay(gdk_display_get_default());
		return TRUE;
	}
	else if (!strcasecmp(key, "ROOT_WINDOW"))
	{
		*value = (void *)gdk_x11_get_default_root_xwindow();
		return TRUE;
	}
	else if (!strcasecmp(key, "SET_EVENT_FILTER"))
	{
		*value = (void *)x11_set_event_filter;
		return TRUE;
	}
	else
		return FALSE;
}

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}
