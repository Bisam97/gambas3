/***************************************************************************

  gdrawingarea.h

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

#ifndef __GDRAWINGAREA_H
#define __GDRAWINGAREA_H

class gDrawingArea : public gContainer
{
public:
	gDrawingArea(gContainer *parent);
	virtual ~gDrawingArea();

	int getBorder() const { return getFrameBorder(); }
	bool cached() const { return _cached; }
	bool hasNoBackground() const { return _no_background; }
	bool useTablet() const { return _use_tablet; }

	void setBorder(int vl) { setFrameBorder(vl); }
	void setCached(bool vl);
	void setNoBackground(bool vl);
	void setUseTablet(bool vl);

	bool inDrawEvent() const { return _in_draw_event; }
	static bool inAnyDrawEvent() { return _in_any_draw_event; }

//"Methods"
	void clear();
#ifndef GTK3
	virtual void setBackground(gColor color = GB_COLOR_DEFAULT);
#endif
	virtual void setRealBackground(gColor color);
	virtual void updateFont();
	virtual uintptr_t handle();


//"Private"
	void create();
	void updateCache();
	void resizeCache();
	void refreshCache();
	void setCache();
	void updateUseTablet();

#ifdef GTK3
	cairo_surface_t *buffer;
#else
	GdkPixmap *buffer;
#endif
	GtkWidget *box;
	unsigned _cached : 1;
	unsigned _resize_cache : 1;
	unsigned _in_draw_event : 1;
	unsigned _use_tablet : 1;
	unsigned _own_window : 1;
	static int _in_any_draw_event;
};


// Callbacks

#ifdef GTK3
	void CB_drawingarea_expose(gDrawingArea *sender, cairo_t *cr);
#else
	void CB_drawingarea_expose(gDrawingArea *sender, GdkRegion *region, int dx, int dy);
#endif
	void CB_drawingarea_font(gDrawingArea *sender);

#endif
