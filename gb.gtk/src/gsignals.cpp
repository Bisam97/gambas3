/***************************************************************************

  gsignals.cpp

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
#include <gdk/gdkkeysyms.h>

#include "gapplication.h"
#include "gdrawingarea.h"
#include "gkey.h"
#include "gmouse.h"
#include "gmainwindow.h"
#include "gdrag.h"
#include "gdesktop.h"

//#define DEBUG_DND 1

static void cb_destroy(GtkWidget *object, gControl *data)
{
	if (data->_no_delete)
		return;
		
	//if (!data->_destroyed)
	delete data;
}

static gboolean cb_menu(GtkWidget *widget, gControl *data)
{
	return CB_control_mouse(data, gEvent_MouseMenu);
}

gboolean gcb_focus_in(GtkWidget *widget, GdkEventFocus *event, gControl *data)
{
	//fprintf(stderr, "gcb_focus_in: %s\n", data->name());
	
	gApplication::setActiveControl(data, true);
	if (data->frame)
		data->refresh();

	return false;
}

gboolean gcb_focus_out(GtkWidget *widget, GdkEventFocus *event, gControl *data)
{	
	//fprintf(stderr, "gcb_focus_out: %s\n", data->name());
	
	/*if (!::strcmp(data->name(), "txtName"))
		BREAKPOINT();*/

	if (!gApplication::_keep_focus)
		gApplication::setActiveControl(data, false);
	if (data->frame)
		data->refresh();
	
	return false;
}

gboolean gcb_focus(GtkWidget *widget, GtkDirectionType direction, gControl *data)
{
	gControl *ctrl;
	
	if (direction == GTK_DIR_TAB_FORWARD || direction == GTK_DIR_TAB_BACKWARD)
	{
		ctrl = gApplication::activeControl();

		if (!ctrl)
			return true;
		
		if (ctrl->topLevel() != data)
			return true;
		
		for(;;)
		{
			if (direction == GTK_DIR_TAB_FORWARD)
				ctrl = ctrl->nextFocus();
			else
				ctrl = ctrl->previousFocus();
			
			if (!ctrl)
				break;
			
			//fprintf(stderr, "gcb_focus: %s / %d %d %d %d\n", ctrl->name(), ctrl->isReallyVisible(), ctrl->isEnabled(), ctrl->canFocus(), !ctrl->isNoTabFocusRec());
			
			if (!ctrl->isTopLevel() && ctrl->isReallyVisible() && ctrl->isEnabled() && ctrl->canFocus() && !ctrl->isNoTabFocusRec())
			{
				//fprintf(stderr, "cb_focus: --> %s / %d\n", ctrl->name(), ctrl->isNoTabFocusRec());
				ctrl->setFocus();
				break;
			}
			if (ctrl == gApplication::activeControl())
				break;
		}
	}

	return true;
}


/****************************************************
 Drag 
*****************************************************/

static void cb_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *dt, guint i, guint time, gControl *data)
{
	char *text;
	int len;
	gPicture *pic;
	//g_debug("sg_drag_data_get\n");

	#if DEBUG_DND
	fprintf(stderr, "cb_drag_data_get: %s\n", data->name());
	#endif

	context = gDrag::enable(context, data, time);
	
	text = gDrag::getText(&len, NULL, true);
	if (text)
	{
		#if DEBUG_DND
		char *atom = gdk_atom_name(gtk_selection_data_get_target(dt));
		fprintf(stderr, "target = %s\n", atom);
		g_free(atom);
		fprintf(stderr, "gtk_selection_data_set_text: %.*s\n", len, text);
		#endif

		if (!gtk_selection_data_set_text(dt, text, len))
	    gtk_selection_data_set (dt, gtk_selection_data_get_target(dt), 8, (guchar *)text, len);
		return;
	}
	
	pic = gDrag::getImage(true);
	if (pic)
	{
		gtk_selection_data_set_pixbuf(dt, pic->getPixbuf());
	}
	
	gDrag::disable(context);
}

static void cb_drag_end(GtkWidget *widget, GdkDragContext *ct, gControl *data)
{
	#if DEBUG_DND
	fprintf(stderr, "cb_drag_end: %s\n", data->name());
	#endif
	
	gDrag::end();
}


/****************************************************
 Drop 
*****************************************************/

// BM: What for?
//static guint32 _drag_time = 0;

static void cb_drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gControl *data)
{
	if (!gDrag::isCurrent(data))
		return;
	
	#if DEBUG_DND
	fprintf(stderr, "\ncb_drag_leave: %s\n", data->name());
	#endif

	gDrag::setCurrent(NULL);
	gDrag::hide(data);
}

static gboolean cb_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gControl *data)
{
	bool cancel;
	
	GdkModifierType mask;
	GdkDragAction dnd_action;
	int action;
	gControl *source;
	
#ifdef GTK3
	
	GdkDevice *pointer;
	
	#if GDK_MAJOR_VERSION > 3 || (GDK_MAJOR_VERSION == 3 && GDK_MINOR_VERSION >= 20)
	pointer = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default()));
	#else
	pointer = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_display_get_default()));
	#endif
	
	gdk_window_get_device_position(gtk_widget_get_window(widget), pointer, NULL, NULL, &mask);
	
#else

	gdk_window_get_pointer(gtk_widget_get_window (widget), NULL, NULL, &mask);
	
#endif
	
	mask = (GdkModifierType)(mask & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_META_MASK));
	
	if (mask == GDK_CONTROL_MASK)
	{
		dnd_action = GDK_ACTION_COPY;
		action = DRAG_COPY;
	}
	else if (mask == GDK_SHIFT_MASK)
	{
		dnd_action = GDK_ACTION_LINK;
		action = DRAG_LINK;
	}
	else
	{
		dnd_action = GDK_ACTION_MOVE;
		action = DRAG_MOVE;
	}
	
	#if DEBUG_DND
	fprintf(stderr, "\ncb_drag_motion: %s / %d / %d\n", data->name(), action, gdk_drag_context_get_selected_action(context));
	#endif	
	
	gApplication::checkHoveredControl(data);

	source = gApplication::controlItem(gtk_drag_get_source_widget(context));
	gDrag::setDropData(action, x, y, source, NULL);
	
	context = gDrag::enable(context, data, time);
	
	cancel = gDrag::setCurrent(data);
	#if DEBUG_DND
	fprintf(stderr, "setCurrent -> %d\n", cancel);
	#endif

	if (!cancel)
	{
		//fprintf(stderr, "cb_drag_motion: onDragMove: %p\n", widget);
		gControl *control = data;
		
		//while (control->_proxy)
		//	control = control->_proxy;
		
		while (control)
		{
			#if DEBUG_DND
			fprintf(stderr, "send DragMove %s\n", control->name());
			#endif
			if (CB_control_can_raise(control, gEvent_DragMove))
			{
				cancel = CB_control_drag_move(control);
				if (cancel)
					break;
			}
			control = control->_proxy_for;
		}
	}
	
	context = gDrag::disable(context);
	
	if (cancel)
	{
		#if DEBUG_DND
		fprintf(stderr, "cb_drag_motion: cancel\n");
		#endif
		gDrag::hide(data);
	}
	else
	{
		#if DEBUG_DND
		fprintf(stderr, "cb_drag_motion: accept %d / %d\n", action, gdk_drag_context_get_selected_action(context));
		#endif
		gdk_drag_status(context, dnd_action, time);
		return true;
	}
	
	return false;
}


static gboolean cb_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gControl *data)
{
	gControl *source;

	#if DEBUG_DND
	fprintf(stderr, "\ncb_drag_drop: %s\n", data->name());
	#endif
	
	/*if (!gDrag::isCurrent(data))
		return false;*/
	
	// cb_drag_leave() is automatically called when a drop occurs
	//cb_drag_leave(widget, context, time, data);
	
	/*if (!data->_accept_drops) //acceptDrops())
		return false;*/

	/*if (!CB_control_can_raise(data, gEvent_Drop))
	{
		gtk_drag_finish(context, false, false, time);
		return false;
	}*/

	source = gApplication::controlItem(gtk_drag_get_source_widget(context));

	gDrag::setDropData(gDrag::getAction(), x, y, source, data);
	
	context = gDrag::enable(context, data, time);
	
	while (data)
	{
		if (CB_control_drop(data))
			break;
		data = data->_proxy_for;
	}
	
	context = gDrag::disable(context);

	//fprintf(stderr, "cancel = %d\n", cancel);
	
	gtk_drag_finish(context, true, false, time);
	
	//data->_drag_enter = false;

	return true;
}

static void cb_show(GtkWidget *widget, gContainer *data)
{
	//fprintf(stderr, "cb_show: %s '%s'\n", GB.GetClassName(data->hFree), data->name());
	data->performArrange();
}

void gControl::borderSignals()
{
	GtkWidget *w;

	g_signal_connect_after(G_OBJECT(border), "destroy", G_CALLBACK(cb_destroy), (gpointer)this);
	//g_signal_connect(G_OBJECT(border),"drag-data-received",G_CALLBACK(sg_drag_data_received),(gpointer)this);
	//g_signal_connect(G_OBJECT(border),"enter-notify-event",G_CALLBACK(sg_enter),(gpointer)this);
	//g_signal_connect(G_OBJECT(border),"leave-notify-event",G_CALLBACK(sg_enter),(gpointer)this);
	
	//g_signal_connect_after(G_OBJECT(border),"size-allocate",G_CALLBACK(sg_size),(gpointer)this);
	
	if (isContainer())
		g_signal_connect(G_OBJECT(border), "show", G_CALLBACK(cb_show), (gpointer)this);

	if (border != widget && !_scroll)
	{
		/*if (!_no_default_mouse_event)
		{
			g_signal_connect(G_OBJECT(border),"button-release-event",G_CALLBACK(gcb_button_release),(gpointer)this);
			g_signal_connect(G_OBJECT(border),"button-press-event",G_CALLBACK(gcb_button_press),(gpointer)this);
		}*/
		g_signal_connect(G_OBJECT(border), "popup-menu", G_CALLBACK(cb_menu), (gpointer)this);	
		//g_signal_connect_after(G_OBJECT(border),"motion-notify-event",G_CALLBACK(sg_motion),(gpointer)this);
		//g_signal_connect(G_OBJECT(border),"scroll-event",G_CALLBACK(sg_scroll),(gpointer)this);
	}

	w = _scroll ? widget : border;

	g_signal_connect(G_OBJECT(w), "drag-motion", G_CALLBACK(cb_drag_motion),(gpointer)this);
	g_signal_connect(G_OBJECT(w), "drag-leave", G_CALLBACK(cb_drag_leave),(gpointer)this);
	g_signal_connect(G_OBJECT(w), "drag-drop", G_CALLBACK(cb_drag_drop),(gpointer)this);
	g_signal_connect(G_OBJECT(w), "drag-data-get", G_CALLBACK(cb_drag_data_get),(gpointer)this);
	g_signal_connect(G_OBJECT(w), "drag-end", G_CALLBACK(cb_drag_end),(gpointer)this);
}

void gControl::initSignals()
{	
	borderSignals();

	if (!(border != widget && !_scroll))
	{
		//g_signal_connect(G_OBJECT(widget),"scroll-event",G_CALLBACK(sg_scroll),(gpointer)this);
		/*if (!_no_default_mouse_event)
		{
			g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(gcb_button_release),(gpointer)this);
			g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(gcb_button_press),(gpointer)this);
		}*/
		//g_signal_connect(G_OBJECT(widget),"motion-notify-event",G_CALLBACK(sg_motion),(gpointer)this);
		g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(cb_menu), (gpointer)this);
	}

	g_signal_connect(G_OBJECT(widget), "key-press-event", G_CALLBACK(gcb_key_event), (gpointer)this);
	g_signal_connect(G_OBJECT(widget), "key-release-event", G_CALLBACK(gcb_key_event), (gpointer)this);
	g_signal_connect(G_OBJECT(widget), "focus", G_CALLBACK(gcb_focus), (gpointer)this);
	g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(gcb_focus_in), (gpointer)this);
	g_signal_connect(G_OBJECT(widget), "focus-out-event", G_CALLBACK(gcb_focus_out), (gpointer)this);
	//g_signal_connect(G_OBJECT(widget),"event",G_CALLBACK(sg_event),(gpointer)this);

	/*if (widget != border)
	{
		g_signal_connect(G_OBJECT(widget), "drag-end", G_CALLBACK(cb_drag_end), (gpointer)this);
	}*/
}

