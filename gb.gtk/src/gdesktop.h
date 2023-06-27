/***************************************************************************

  gdesktop.h

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

#ifndef __GDESKTOP_H
#define __GDESKTOP_H

class gPicture;
class gMainWindow;
class gControl;
class gFont;

class gDesktop
{
public:

	enum {
		BACKGROUND,
		FOREGROUND,
		TEXT_BACKGROUND,
		TEXT_FOREGROUND,
		SELECTED_BACKGROUND,
		SELECTED_FOREGROUND,
		BUTTON_BACKGROUND,
		BUTTON_FOREGROUND,
		LIGHT_BACKGROUND,
		LIGHT_FOREGROUND,
		TOOLTIP_BACKGROUND,
		TOOLTIP_FOREGROUND,
		LINK_FOREGROUND,
		VISITED_FOREGROUND,
		NUM_COLORS
	};
	
	static gFont* font() { return gFont::desktopFont(); }
	static void setFont(gFont *vl) { gFont::setDesktopFont(vl); }
	static int scale() { return gFont::desktopScale(); }
	
	static int height();
	static int width();
	static int resolution();
	static gPicture* screenshot(int x = 0, int y = 0, int w = 0, int h = 0);
	static gMainWindow* activeWindow();

	static bool rightToLeft();
	
	static int count();
	static void geometry(int screen, GdkRectangle *rect);
	static void availableGeometry(int screen, GdkRectangle *rect);
	
	static void geometry(GdkRectangle *rect) { geometry(0, rect); }
	static void availableGeometry(GdkRectangle *rect) { availableGeometry(0, rect); }
	
	static void screenResolution(int screen, double *x, double *y);
	
	static gColor getColor(int color, bool disabled = false);
	
	static void onThemeChange();
	
private:

	static bool _colors_valid;
	static gColor _colors[NUM_COLORS];
	static gColor _colors_disabled[NUM_COLORS];
	
	static void calc_colors(gColor colors[], bool disabled);
};

#endif
