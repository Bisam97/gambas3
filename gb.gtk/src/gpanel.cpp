/***************************************************************************

  gpanel.cpp

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
#include "gapplication.h"
#include "gpanel.h"

/****************************************************************************

Panel

****************************************************************************/

void gPanel::create(void)
{
	int i;
	GtkWidget *ch, *box;
	bool doReparent = false;
	bool was_visible = isVisible();
	GdkRectangle rect = { 0 };
	int bg, fg;
	gControl *nextSibling;
	
	if (border)
	{
		getGeometry(&rect);
		bg = background();
		fg = foreground();
		nextSibling = next();
		parent()->remove(this);
		
		for (i = 0; i < childCount(); i++)
		{
			ch = child(i)->border;
			g_object_ref(G_OBJECT(ch));
			gtk_container_remove(GTK_CONTAINER(widget), ch);
		}
		
		doReparent = true;
	}
	
	if (_bg_set)
	{
		createBorder(gtk_event_box_new());
		widget = gtk_fixed_new();
		box = widget;
		//gtk_widget_set_app_paintable(border, TRUE);
		//gtk_widget_set_app_paintable(box, TRUE);
	}
	else
	{
		createBorder(gtk_fixed_new());
		widget = border;
		box = NULL;
	}

	frame = border;
	realize(true);
	
	//g_signal_connect(G_OBJECT(border), "size-allocate", G_CALLBACK(cb_size), (gpointer)this);
	//g_signal_connect(G_OBJECT(border), "expose-event", G_CALLBACK(cb_expose), (gpointer)this);
	
	if (doReparent)
	{
		if (isUser())
			setUser();
	
		if (box)
			gtk_widget_realize(box);
		
		setNext(nextSibling);
		setBackground(bg);
		setForeground(fg);
		updateFont();
		bufX = bufY = bufW = bufH = -1;
		setGeometry(&rect);
		
		for (i = 0; i < childCount(); i++)
		{
			ch = child(i)->border;
			gtk_container_add(GTK_CONTAINER(widget), ch);
			moveChild(child(i), child(i)->x(), child(i)->y());
			g_object_unref(G_OBJECT(ch));
		}
		
		if (was_visible)
			show();
		else
			hide();
		
		//gApplication::checkHoveredControl(this);
		
		if (_inside)
		{
			_inside = false;
			if (gApplication::_enter == this)
				gApplication::_enter = NULL;
			gApplication::_ignore_until_next_enter = this;
		}
	}
}

gPanel::gPanel(gContainer *parent) : gContainer(parent)
{
	border = NULL;
#ifdef GTK3
	_no_style_without_child = true;
#endif
	create();
}

#ifdef GTK3
#else
void gPanel::setBackground(gColor color)
{
	bool set = _bg_set;
	
	gContainer::setBackground(color);
	
	if (set != _bg_set)
		create();
}
#endif
