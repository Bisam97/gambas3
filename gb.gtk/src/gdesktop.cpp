/***************************************************************************

	gdesktop.cpp

	(c) 2004-2006 - Daniel Campos Fernández <dcamposf@gmail.com>

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

#include "widgets.h"
#include "gb.form.font.h"

#ifndef GTK3
#include "x11.h"
#endif

#include "gb.form.const.h"
#include "gapplication.h"
#include "gmainwindow.h"
#include "gdesktop.h"
#include "CScreen.h"

static bool _updating_colors = false;
static int _style_timeout = 0;
static bool _colors_init = false;
static gColor _colors[COLOR_COUNT];
static gColor _colors_disabled[COLOR_COUNT];
static gColor _colors_previous[COLOR_COUNT];

//-------------------------------------------------------------------------

bool gDesktop::rightToLeft()
{
	return MAIN_rtl; //gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
}

gMainWindow* gDesktop::activeWindow()
{
	return gMainWindow::_active ? gMainWindow::_active->topLevel() : NULL;
}

int gDesktop::width()
{
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkRectangle rect;
	gdk_monitor_get_geometry(gdk_display_get_primary_monitor(gdk_display_get_default()), &rect);
	return rect.width;
#else
	return gdk_screen_get_width(gdk_screen_get_default ());
#endif
}

int gDesktop::height()
{
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkRectangle rect;
	gdk_monitor_get_geometry(gdk_display_get_primary_monitor(gdk_display_get_default()), &rect);
	return rect.height;
#else
	return gdk_screen_get_height(gdk_screen_get_default());
#endif
}

int gDesktop::resolution()
{
	gdouble res = gdk_screen_get_resolution(gdk_screen_get_default());
	if (res == -1)
		res = 96;
	return res;
}


gPicture* gDesktop::screenshot(int x, int y, int w, int h)
{
	return gt_grab_window(gdk_get_default_root_window(), x, y, w, h);
}

int gDesktop::count()
{
#if GTK_CHECK_VERSION(3, 22, 0)
	return gdk_display_get_n_monitors(gdk_display_get_default());
#elif defined(GTK3)
	return gdk_screen_get_n_monitors(gdk_screen_get_default());
#else
	return gdk_display_get_n_screens(gdk_display_get_default());
#endif
}

void gDesktop::geometry(int screen, GdkRectangle *rect)
{
	rect->x = rect->y = rect->width = rect->height = 0;
	if (screen < 0 || screen >= count())
		return;

#if GTK_CHECK_VERSION(3, 22, 0)
	gdk_monitor_get_geometry(gdk_display_get_monitor(gdk_display_get_default(), screen), rect);
#elif defined(GTK3)
	gdk_screen_get_monitor_geometry(gdk_screen_get_default(), screen, rect);
#else
	rect->width = gdk_screen_get_width(gdk_display_get_screen(gdk_display_get_default(), screen));
	rect->height = gdk_screen_get_height(gdk_display_get_screen(gdk_display_get_default(), screen));
#endif
}

void gDesktop::availableGeometry(int screen, GdkRectangle *rect)
{
	rect->x = rect->y = rect->width = rect->height = 0;
	if (screen < 0 || screen >= count())
		return;

#if GTK_CHECK_VERSION(3, 22, 0)
	gdk_monitor_get_workarea(gdk_display_get_monitor(gdk_display_get_default(), screen), rect);
#elif defined(GTK3)
	gdk_screen_get_monitor_workarea(gdk_screen_get_default(), screen, rect);
#else
	if (X11_get_available_geometry(screen, &rect->x, &rect->y, &rect->width, &rect->height))
		geometry(screen, rect);
#endif
}

static void update_color(gControl *control)
{
	int i;
	gColor bg = control->background();
	gColor fg = control->foreground();

	if (bg != GB_COLOR_DEFAULT)
	{
		for (i = 0; i < COLOR_COUNT; i++)
		{
			if (bg == _colors_previous[i])
			{
				control->setBackground(_colors[i]);
				break;
			}
		}
	}

	if (fg != GB_COLOR_DEFAULT)
	{
		for (i = 0; i < COLOR_COUNT; i++)
		{
			if (fg == _colors_previous[i])
			{
				control->setForeground(_colors[i]);
				break;
			}
		}
	}
}

#ifdef GTK3

static gColor get_color(GType type, bool fg, GtkStateFlags state, bool disabled)
{
	GtkStyleContext *style = gt_get_style(type, state == STATE_SELECTED ? "selection" : NULL, (type == GTK_TYPE_TOOLTIP && !fg) ? GTK_STYLE_CLASS_BACKGROUND : NULL);
	
	if (disabled)
		state = (GtkStateFlags)(state | STATE_INSENSITIVE);
	
	gtk_style_context_set_state(style, state);
	
	if (!fg)
	{
		cairo_surface_t *image;
		cairo_t *cairo;
		uchar *p;
		
		image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
		cairo = cairo_create(image);
		gtk_render_background(style, cairo, 0, 0, 32, 32);
		cairo_destroy(cairo);
		
		p = (uchar *)cairo_image_surface_get_data(image) + sizeof(uint) * (16 * 33);
		return gt_rgba_to_color(p[2], p[1], p[0], p[3]);
	}
	else
	{
		GdkRGBA rgba;
		gtk_style_context_get_color(style, state, &rgba);
		return gt_to_color(&rgba);
	}
	
	if (state == STATE_SELECTED)
		g_object_unref(G_OBJECT(style));
}

#else

static gColor get_color(GType type, bool fg, GtkStateType state, bool disabled)
{
	GtkStyle *st = gt_get_style(type);
	GdkColor *color;

	if (disabled)
		state = STATE_INSENSITIVE;
	
	if (type == GTK_TYPE_ENTRY)
	{
		if (fg)
			color = &st->text[state];
		else
			color = &st->base[state];
	}
	else
	{
		if (fg)
			color = &st->fg[state];
		else
			color = &st->bg[state];
	}
	
	return gt_gdkcolor_to_color(color);
}

#endif

static void calc_colors(gColor colors[], bool disabled)
{
	colors[COLOR_BACKGROUND] = get_color(GTK_TYPE_WINDOW, false, STATE_NORMAL, disabled);
	colors[COLOR_FOREGROUND] = get_color(GTK_TYPE_WINDOW, true, STATE_NORMAL, disabled);
	colors[COLOR_TEXT_BACKGROUND] = get_color(GTK_TYPE_ENTRY, false, STATE_NORMAL, disabled);
	colors[COLOR_TEXT_FOREGROUND] = get_color(GTK_TYPE_ENTRY, true, STATE_NORMAL, disabled);
	colors[COLOR_SELECTED_BACKGROUND] = get_color(GTK_TYPE_ENTRY, false, STATE_SELECTED, disabled);
	colors[COLOR_SELECTED_FOREGROUND] = get_color(GTK_TYPE_ENTRY, true, STATE_SELECTED, disabled);
	colors[COLOR_BUTTON_BACKGROUND] = get_color(GTK_TYPE_BUTTON, false, STATE_NORMAL, disabled);
	colors[COLOR_BUTTON_FOREGROUND] = get_color(GTK_TYPE_BUTTON, true, STATE_NORMAL, disabled);
	colors[COLOR_TOOLTIP_BACKGROUND] = get_color(GTK_TYPE_TOOLTIP, false, STATE_NORMAL, disabled);
	colors[COLOR_TOOLTIP_FOREGROUND] = get_color(GTK_TYPE_TOOLTIP, true, STATE_NORMAL, disabled);
	#ifdef GTK3
		#if GTK_CHECK_VERSION(3, 12, 0)
			colors[COLOR_LINK_FOREGROUND] = get_color(GTK_TYPE_LINK_BUTTON, true, STATE_LINK, disabled);
			colors[COLOR_VISITED_FOREGROUND] = get_color(GTK_TYPE_LINK_BUTTON, true, (STATE_T)((int)STATE_LINK + (int)STATE_VISITED), disabled);
		#else
			colors[COLOR_LINK_FOREGROUND] = get_color(GTK_TYPE_LINK_BUTTON, true, STATE_NORMAL, disabled);
			colors[COLOR_VISITED_FOREGROUND] = IMAGE.DarkerColor(_colors[COLOR_LINK_FOREGROUND]);
		#endif
	#else
		colors[COLOR_LINK_FOREGROUND] = IMAGE.LighterColor(_colors[COLOR_SELECTED_BACKGROUND]);
		colors[COLOR_VISITED_FOREGROUND] = IMAGE.DarkerColor(_colors[COLOR_LINK_FOREGROUND]);
	#endif
	colors[COLOR_LIGHT_BACKGROUND] = IMAGE.MergeColor(_colors[COLOR_SELECTED_BACKGROUND], _colors[COLOR_SELECTED_FOREGROUND], 0.3);
	colors[COLOR_LIGHT_FOREGROUND] = IMAGE.MergeColor(_colors[COLOR_BACKGROUND], _colors[COLOR_FOREGROUND], 0.3);
}


bool gDesktop::updateColors()
{
	int i, j;
	uchar r, g, b, a;

	_updating_colors = true;

	calc_colors(_colors, false);

	for (i = 0; i < COLOR_COUNT; i++)
	{
		for (j = 0; j < i; j++)
		{
			if (_colors[i] == _colors[j])
			{
				GB_COLOR_SPLIT(_colors[i], r, g, b, a);
				if (g > 127)
					g--;
				else
					g++;
				_colors[i] = GB_COLOR_MAKE(r, g, b, a);
				j = 0;
			}
		}
		//fprintf(stderr, "[%d] %08X -> %08X\n", i, _colors_previous[i], _colors[i]);
	}
	//fprintf(stderr, "\n");

	for (i = 0; i < COLOR_COUNT; i++)
	{
		if (_colors[i] != _colors_previous[i])
		{
			calc_colors(_colors_disabled, true);
			gt_on_theme_change();
			gApplication::updateDarkTheme();
			if (_colors_init)
			{
				//fprintf(stderr, "update controls\n");
				gApplication::forEachControl(update_color);
				APPLICATION_send_change_event(CHANGE_COLOR);
			}
			memmove(_colors_previous, _colors, COLOR_COUNT * sizeof(*_colors));
			_colors_init = true;
			_updating_colors = false;
			return true;
		}
	}

	_updating_colors = false;
	return false;
}

gColor gDesktop::getColor(int color, bool disabled)
{
	return disabled ? _colors_disabled[color] : _colors[color];
}

void gDesktop::screenResolution(int screen, double *x, double *y)
{
	GdkRectangle rect;
	
	if (screen < 0 || screen >= count())
	{
		if (*x) *x = 0;
		if (*y) *y = 0;
		return;
	}

#if GTK_CHECK_VERSION(3, 22, 0)
	GdkMonitor *monitor = gdk_display_get_monitor(gdk_display_get_default(), screen);
	gdk_monitor_get_geometry(monitor, &rect);
	if (x) *x = rect.width / (gdk_monitor_get_width_mm(monitor) / 25.4);
	if (y) *y = rect.height / (gdk_monitor_get_height_mm(monitor) / 25.4);
#else
	gdk_screen_get_monitor_geometry(gdk_screen_get_default(), screen, &rect);
	if (x) *x = rect.width / (gdk_screen_get_monitor_width_mm(gdk_screen_get_default(), screen) / 25.4);
	if (y) *y = rect.height / (gdk_screen_get_monitor_height_mm(gdk_screen_get_default(), screen) / 25.4);
#endif
}

gColor gDesktop::changeColor(gColor color)
{
	int i;

	if (color != GB_COLOR_DEFAULT)
	{
		for (i = 0; i < COLOR_COUNT; i++)
		{
			if (color == _colors_previous[i])
			{
				color = _colors[i];
				break;
			}
		}
	}

	return color;
}

void gDesktop::init()
{
	updateColors();
}

static bool cb_style_change(intptr_t)
{
	//fprintf(stderr, "cb_style_change\n");
	gDesktop::updateColors();
	_style_timeout = 0;
	return false;
}

void gDesktop::onStyleChange()
{
	if (_updating_colors)
		return;
	if (_style_timeout)
		g_source_remove(_style_timeout);
	_style_timeout = g_timeout_add(50, (GSourceFunc)cb_style_change, (gpointer)0);
}
