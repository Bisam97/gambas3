/***************************************************************************

  gmainwindow.cpp

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

#include <ctype.h>
#include <time.h>

#include "widgets.h"

#ifndef GTK3
#include "x11.h"
#include "sm/sm.h"
#endif

#include "gapplication.h"
#include "gdesktop.h"
#include "gkey.h"
#include "gmenu.h"
#include "gdialog.h"
#include "gmouse.h"
#include "gmainwindow.h"

//#define DEBUG_RESIZE 1

GList *gMainWindow::windows = NULL;
gMainWindow *gMainWindow::_active = NULL;
gMainWindow *gMainWindow::_current = NULL;


#define CHECK_STATE(_var, _state) \
	if (event->changed_mask & _state) \
	{ \
		v = (event->new_window_state & _state) != 0; \
		if (v != data->_var) \
		{ \
			data->_var = v; \
			has_changed = true; \
		} \
	}

static gboolean cb_frame(GtkWidget *widget,GdkEventWindowState *event,gMainWindow *data)
{
	bool has_changed = false;
	bool v;
	
	CHECK_STATE(_minimized, GDK_WINDOW_STATE_ICONIFIED);
	CHECK_STATE(_maximized, GDK_WINDOW_STATE_MAXIMIZED);
	CHECK_STATE(_sticky, GDK_WINDOW_STATE_STICKY);
	CHECK_STATE(_fullscreen, GDK_WINDOW_STATE_FULLSCREEN);

	if (event->changed_mask & GDK_WINDOW_STATE_ABOVE)
	{
		if (event->new_window_state & GDK_WINDOW_STATE_ABOVE)
			data->stack = 1;
		else if (data->stack == 1)
			data->stack = 0;
	}
	if (event->changed_mask & GDK_WINDOW_STATE_BELOW)
	{
		if (event->new_window_state & GDK_WINDOW_STATE_BELOW)
			data->stack = 2;
		else if (data->stack == 2)
			data->stack = 0;
	}

	if (has_changed)
	{
		#ifdef DEBUG_RESIZE
		fprintf(stderr, "cb_frame: min = %d max = %d fs = %d\n", data->_minimized, data->_maximized, data->_fullscreen);
		#endif
		data->_csd_w = data->_csd_h = -1;
		/*data->calcCsdSize();
		data->performArrange();*/
	}

	if (event->changed_mask & (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_STICKY | GDK_WINDOW_STATE_ABOVE | GDK_WINDOW_STATE_BELOW))
		CB_window_state(data);

	return false;
}

static gboolean cb_show(GtkWidget *widget, gMainWindow *data)
{
	if (gApplication::_disable_mapping_events)
		return false;
	
	if (data->_grab_on_show)
	{
		data->_grab_on_show = FALSE;
		gApplication::grabPopup();
	}

	data->emitOpen();

	if (data->_opened)
	{
		data->performArrange();
		#ifdef DEBUG_RESIZE
		fprintf(stderr, "cb_show\n");
		#endif
		data->emitResize();
		CB_window_show(data);
		data->_not_spontaneous = false;
	}
	return false;
}

static gboolean cb_map(GtkWidget *widget, GdkEvent *event, gMainWindow *data)
{
	if (gApplication::_disable_mapping_events)
		return false;
	
	data->_unmap = false;
	return cb_show(widget, data);
}

static gboolean cb_hide(GtkWidget *widget, gMainWindow *data)
{
	if (gApplication::_disable_mapping_events)
		return false;
	
	if (!data->_unmap)
	{
		CB_window_hide(data);
		data->_not_spontaneous = false;
	}

	return false;
}

static gboolean cb_unmap(GtkWidget *widget, GdkEvent *event, gMainWindow *data)
{
	if (gApplication::_disable_mapping_events)
		return false;
	
	bool ret = cb_hide(widget, data);
	data->_unmap = true;
	return ret;
}

static gboolean cb_close(GtkWidget *widget,GdkEvent *event, gMainWindow *data)
{
	if ((!gMainWindow::_current || data == gMainWindow::_current) && gApplication::areInputEventsEnabled())
		data->doClose();

	return true;
}

static gboolean cb_configure(GtkWidget *widget, GdkEventConfigure *event, gMainWindow *data)
{
#if 0
	gint x, y, w, h;

	if (data->_opened)
	{
		if (data->isTopLevel())
		{
			gtk_window_get_position(GTK_WINDOW(data->border), &x, &y);
		}
		else
		{
			x = event->x;
			y = event->y;
		}

		#ifdef DEBUG_RESIZE
		fprintf(stderr, "cb_configure: %s: (%d %d %d %d) -> (%d/%d %d/%d %d %d) window = %p resized = %d send_event = %d\n", data->name(), data->bufX, data->bufY, data->bufW, data->bufH, x, event->x, y, event->y, event->width, event->height, event->window, data->_event_resized, event->send_event);
		#endif

		if (x != data->bufX || y != data->bufY)
		{
			data->bufX = x;
			data->bufY = y;
			if (data->onMove) data->onMove(data);
		}
		
		/*#ifdef GTK3
		//data->_csd_w = data->_csd_h = -1;
		if (data->isTopLevel())
			return false;
		#endif*/

		w = event->width;
		h = event->height;
		
		if ((w != data->bufW) || (h != data->bufH) || (data->_event_resized) || !event->window)
		{
			data->_event_resized = false;
			data->bufW = w;
			data->bufH = h;
			#ifdef DEBUG_RESIZE
			fprintf(stderr, "cb_configure\n");
			#endif
			data->emitResize();
		}
	}
#endif

	int x, y;

	if (!data->isOpened())
		return false;

	if (data->isTopLevel())
	{
		gtk_window_get_position(GTK_WINDOW(data->border), &x, &y);
	}
	else
	{
		x = event->x;
		y = event->y;
	}

	if (x != data->bufX || y != data->bufY)
	{
		data->bufX = x;
		data->bufY = y;
		CB_window_move(data);
	}
	
	#ifdef DEBUG_RESIZE
	fprintf(stderr, "cb_configure: %s: (%d %d %d %d) -> (%d/%d %d/%d %d %d) window = %p send_event = %d\n", data->name(), data->bufX, data->bufY, data->bufW, data->bufH, x, event->x, y, event->y, event->width, event->height, event->window, event->send_event);
	#endif

	data->calcCsdSize();

	data->bufW = event->width - data->_csd_w;
	data->bufH = event->height - data->_csd_h;
	
	if (data->isTopLevel() && !data->_minimized && !data->_maximized && !data->_fullscreen)
	{
		data->_sx = data->bufX;
		data->_sy = data->bufY;
		data->_sw = data->bufW;
		data->_sh = data->bufH;
	}

	#ifdef DEBUG_RESIZE
	fprintf(stderr, "-> %d %d\n", data->bufW, data->bufH);
	#endif
		
	data->emitResize();

	return false;
}

#ifdef GTK3
static gboolean cb_draw(GtkWidget *wid, cairo_t *cr, gMainWindow *data)
{
	if (data->background() != COLOR_DEFAULT)
	{
		gt_cairo_set_source_color(cr, data->background());
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
	}

	if (data->_picture)
	{
		cairo_pattern_t *pattern;

		pattern = cairo_pattern_create_for_surface(data->_picture->getSurface());
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

		cairo_set_source(cr, pattern);
		cairo_paint(cr);

		cairo_pattern_destroy(pattern);
	}

	return false;
}
#else
static gboolean cb_expose(GtkWidget *wid, GdkEventExpose *e, gMainWindow *data)
{
	bool draw_bg = data->isTransparent();
	bool draw_pic = data->_picture;

	if (!draw_bg && !draw_pic)
		return false;

	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(wid));

	if (draw_bg)
	{
		if (data->background() == COLOR_DEFAULT)
			gt_cairo_set_source_color(cr, 0xFF000000);
		else
			gt_cairo_set_source_color(cr, data->background());
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
	}

	if (draw_pic)
	{
		cairo_pattern_t *pattern;

		gdk_cairo_region(cr, e->region);
		cairo_clip(cr);

		pattern = cairo_pattern_create_for_surface(data->_picture->getSurface());
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

		cairo_set_source(cr, pattern);
		cairo_paint(cr);

		cairo_pattern_destroy(pattern);
	}

	cairo_destroy(cr);
	return false;
}
#endif

static gboolean my_key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  GtkWindow *window = GTK_WINDOW(widget);
  gboolean handled = FALSE;
	gboolean propagated = FALSE;
	GtkWidget *focus;

	focus = gtk_window_get_focus(window);
	//fprintf(stderr, "focus %p = %p\n", window, focus);
	if (focus && gtk_widget_get_realized(focus))
	{
		if (GTK_IS_ENTRY(focus) || GTK_IS_TEXT_VIEW(focus))
		{
			propagated = TRUE;
			handled = gtk_window_propagate_key_event(window, event);
			if (handled)
				return TRUE;
		}
	}
	
  /* handle mnemonics and accelerators */
  handled = gtk_window_activate_key(window, event);
	if (handled)
		return TRUE;

	if (!propagated && focus && gtk_widget_get_realized(focus))
	{
		handled = gtk_window_propagate_key_event(window, event);
		if (handled)
			return TRUE;
	}
	
  /* Chain up, invokes binding set */
	GtkWidgetClass *parent_klass = (GtkWidgetClass*)g_type_class_peek(g_type_parent(GTK_TYPE_WINDOW));
  handled = parent_klass->key_press_event(widget, event);

  return handled;
}


static gboolean my_key_release_event(GtkWidget *widget, GdkEventKey *event)
{
  GtkWindow *window = GTK_WINDOW (widget);
  gboolean handled = FALSE;
	GtkWidget *focus;

	focus = gtk_window_get_focus(window);
	if (focus && !gtk_widget_get_realized(focus))
		return handled;
	
  /* handle focus widget key events */
  if (!handled)
    handled = gtk_window_propagate_key_event(window, event);

  /* Chain up, invokes binding set */
  if (!handled)
	{
		GtkWidgetClass *parent_klass = (GtkWidgetClass*)g_type_class_peek(g_type_parent(GTK_TYPE_WINDOW));
    handled = parent_klass->key_release_event(widget, event);
	}

  return handled;
}


//-------------------------------------------------------------------------

void gMainWindow::initialize()
{
	//fprintf(stderr, "new window: %p in %p\n", this, parent());

	stack = 0;
	accel = NULL;
	_default = NULL;
	_cancel = NULL;
	menuBar = NULL;
	_icon = NULL;
	_picture = NULL;
	_initial_focus = NULL;
	_save_focus = NULL;
	_title = NULL;
	_resize_last_w = _resize_last_h = -1;
	_min_w = _min_h = _default_min_w = _default_min_h = 0;
	_csd_w  = _csd_h = -1;
	_sx = _sy = _sw = _sh = 0;
	_previous = NULL;

	_opened = false;
	_sticky = false;
	_persistent = false;
	_mask = false;
	_masked = false;
	_resized = false;
	_top_only = false;
	_closing = false;
	_closed = false;
	_not_spontaneous = false;
	_skip_taskbar = false;
	_xembed = false;
	_activate = false;
	_hidden = false;
	_hideMenuBar = false;
	_showMenuBar = true;
	_initMenuBar = true;
	_popup = false;
	_maximized = _minimized = _fullscreen = false;
	_transparent = false;
	_utility = false;
	_no_take_focus = false;
	_moved = false;
	_resizable = true;
	_unmap = false;
	_grab_on_show	= false;
	_is_window = true;
	_no_background = true;
	_frame_init = false;
	_set_focus = false;
	
	accel = gtk_accel_group_new();
}

void gMainWindow::initWindow()
{
	if (!isTopLevel())
	{
		//g_signal_connect(G_OBJECT(border), "configure-event", G_CALLBACK(cb_configure), (gpointer)this);
		g_signal_connect_after(G_OBJECT(border), "map", G_CALLBACK(cb_show), (gpointer)this);
		g_signal_connect(G_OBJECT(border),"unmap", G_CALLBACK(cb_hide),(gpointer)this);
		//g_signal_connect_after(G_OBJECT(border), "size-allocate", G_CALLBACK(cb_configure), (gpointer)this);
		ON_DRAW_BEFORE(widget, this, cb_expose, cb_draw);
	}
	else
	{
		//g_signal_connect(G_OBJECT(border),"size-request",G_CALLBACK(cb_realize),(gpointer)this);
		//g_signal_connect(G_OBJECT(border), "show", G_CALLBACK(cb_show),(gpointer)this);
		g_signal_connect(G_OBJECT(border), "hide", G_CALLBACK(cb_hide),(gpointer)this);
		g_signal_connect(G_OBJECT(border), "map-event", G_CALLBACK(cb_map),(gpointer)this);
		g_signal_connect(G_OBJECT(border), "unmap-event", G_CALLBACK(cb_unmap),(gpointer)this);
		g_signal_connect(G_OBJECT(border), "delete-event", G_CALLBACK(cb_close),(gpointer)this);
		g_signal_connect(G_OBJECT(border), "window-state-event", G_CALLBACK(cb_frame),(gpointer)this);

		gtk_widget_add_events(widget, GDK_BUTTON_MOTION_MASK | GDK_STRUCTURE_MASK);
		ON_DRAW_BEFORE(widget, this, cb_expose, cb_draw);

		//g_signal_connect(G_OBJECT(border), "configure-event", G_CALLBACK(cb_configure), (gpointer)this);
	}

	gtk_widget_add_events(border, GDK_STRUCTURE_MASK);
	g_signal_connect(G_OBJECT(border), "configure-event", G_CALLBACK(cb_configure), (gpointer)this);

	/*if (!_frame_init)
	{
		#if DEBUG_RESIZE
		fprintf(stderr, "init cb_resize_frame: %s\n", name());
		#endif
		g_signal_connect_after(G_OBJECT(frame), "size-allocate", G_CALLBACK(cb_resize_frame), (gpointer)this);
		_frame_init = true;
	}*/
	
	gtk_window_add_accel_group(GTK_WINDOW(topLevel()->border), accel);

	have_cursor = true; //parent() == 0 && !_xembed;
	setCanFocus(true);
}


// workaround GTK+ accelerator management

static void workaround_accel_management()
{
	static bool _init = FALSE;
	if (_init)
		return;
	
	GtkWidgetClass *klass = (GtkWidgetClass*)g_type_class_peek(GTK_TYPE_WINDOW);
	klass->key_press_event = my_key_press_event;
	klass->key_release_event = my_key_release_event;
	_init = TRUE;
}

gMainWindow::gMainWindow() : gContainer(NULL)
{
  initialize();

	windows = g_list_append(windows, (gpointer)this);

	border = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	workaround_accel_management();

	frame = gtk_fixed_new();
	widget = gtk_fixed_new();

	realize();
	initWindow();

	gtk_widget_show(frame);
	gtk_widget_show(widget);
	gtk_window_resize(GTK_WINDOW(border), 1, 1);
}

gMainWindow::gMainWindow(int plug) : gContainer(NULL)
{
  initialize();

	windows = g_list_append(windows, (gpointer)this);

	_xembed = true;

	#ifdef GTK3
		border = PLATFORM.CreatePlug(plug);
		if (!border)
			return;
	#else
		border = gtk_plug_new(plug);
	#endif

	frame = gtk_fixed_new();
	widget = gtk_fixed_new();

	realize();
	initWindow();

	//gtk_widget_realize(border);
	gtk_widget_show(frame);
	gtk_widget_show(widget);
	gtk_window_resize(GTK_WINDOW(border), 1, 1);
	//gtk_widget_set_size_request(border, 1, 1);
}

gMainWindow::gMainWindow(gContainer *par) : gContainer(par)
{
	initialize();

	border = gtk_event_box_new();
	frame = gtk_fixed_new();
	widget = gtk_fixed_new();

	realize();
	initWindow();
	
	gtk_widget_show(frame);
	gtk_widget_show(widget);
	
	setVisibility(false);
}

gMainWindow::~gMainWindow()
{
	if (!border)
		return;

	gApplication::finishFocus();

	if (_opened)
	{
		CB_window_close(this);
		_opened = false;
		if (GTK_IS_WINDOW(border) && isModal())
			gApplication::exitLoop(this);
	}

	gPicture::assign(&_picture);
	gPicture::assign(&_icon);
	if (_title) g_free(_title);
	g_object_unref(accel);

	if (_active == this)
		_active = NULL;

	if (gApplication::mainWindow() == this)
		gApplication::setMainWindow(NULL);

	windows = g_list_remove(windows, (gpointer)this);
}

int gMainWindow::getStacking()
{
	return stack;
}

void gMainWindow::setSticky(bool vl)
{
	if (!isTopLevel()) return;

	_sticky = vl;

	if (vl) 
		gtk_window_stick(GTK_WINDOW(border));
	else
		gtk_window_unstick(GTK_WINDOW(border));
}

void gMainWindow::setStacking(int vl)
{
  stack=vl;
	if (!isTopLevel()) return;

	switch (vl)
	{
		case 0:
			gtk_window_set_keep_below(GTK_WINDOW(border),FALSE);
			gtk_window_set_keep_above(GTK_WINDOW(border),FALSE);
			break;
		case 1:
			gtk_window_set_keep_below(GTK_WINDOW(border),FALSE);
			gtk_window_set_keep_above(GTK_WINDOW(border),TRUE);
			break;
		case 2:
			gtk_window_set_keep_above(GTK_WINDOW(border),FALSE);
			gtk_window_set_keep_below(GTK_WINDOW(border),TRUE);
			break;
	}
}

void gMainWindow::setRealBackground(gColor color)
{
	if (!_picture)
	{
		gControl::setRealBackground(color);
		gMenu::updateColor(this);
	}
}

void gMainWindow::setRealForeground(gColor color)
{
	gControl::setRealForeground(color);
	gMenu::updateColor(this);
}

void gMainWindow::move(int x, int y)
{
	if (isTopLevel())
	{
		if (!_moved && (x || y))
			_moved = true;

		if (x == bufX && y == bufY)
			return;

		bufX = x;
		bufY = y;

		gtk_window_move(GTK_WINDOW(border), x, y);
	}
	else
	{
		gContainer::move(x,y);
	}
}


void gMainWindow::updateSize()
{
	if (!isTopLevel() || !isOpened())
		return;
	
	#ifdef DEBUG_RESIZE
	fprintf(stderr, "updateSize: %s: %d %d / %d / %d %d\n", name(), width(), height(), isResizable(), _csd_w, _csd_h);
	#endif
	
	if (width() < 1 || height() < 1)
	{
		if (isVisible())
			gtk_widget_hide(border);
	}
	else
	{
		setGeometryHints();
		if (isResizable())
			gtk_window_resize(GTK_WINDOW(border), width(), height());
		else
		{
			//fprintf(stderr, "gMainWindow::updateSize: %s: %d %d\n", name(), width() + Max(_csd_w, 0), height() + Max(_csd_h, 0));
			gtk_widget_set_size_request(border, width() + Max(_csd_w, 0), height() + Max(_csd_h, 0));
		}

		if (isVisible())
			gtk_widget_show(border);
	}
}

bool gMainWindow::resize(int w, int h, bool no_decide)
{
	if (!isTopLevel())
	{
		if (gContainer::resize(w, h, no_decide))
			return true;
	}
	else
	{
		if (w == bufW && h == bufH)
		{
			_resized = true;
			return true;
		}

		bufW = w < 0 ? 0 : w;
		bufH = h < 0 ? 0 : h;
		
		// we check for _resized to ignore the first resize()
		if (_resized && _default_min_w <= 0 && _default_min_h <= 0)
		{
			_default_min_w = w;
			_default_min_h = h;
		}
		
		updateSize();
	}

	_resized = true;
	return false;
}

bool gMainWindow::emitOpen()
{
	//fprintf(stderr, "emit Open: %p (%d %d) %d resizable = %d fullscreen = %d\n", this, width(), height(), _opened, isResizable(), fullscreen());

	if (_opened)
		return false;
	
	_opened = true;
	_closed = false;
	//_no_resize_event = true; // If the event loop is run during emitOpen(), some spurious configure events are received.

	updateSize();
	//performArrange();

	gtk_widget_realize(border);

	CB_window_open(this);
	if (_closed)
	{
		_opened = false;
		return true;
	}

	//fprintf(stderr, "emit Move & Resize: %p\n", this);
	CB_window_move(this);
	#ifdef DEBUG_RESIZE
	fprintf(stderr, "cb_open\n");
	#endif
	emitResize();

	return false;
}

void gMainWindow::present()
{
	if (_no_take_focus)
		gtk_widget_show(GTK_WIDGET(border));
	else
		gtk_window_present(GTK_WINDOW(border));

	#ifdef GTK3
	updateStyleSheet(false);
	#endif
}

void gMainWindow::afterShow()
{
	if (_activate)
	{
		present();
		_activate = false;
	}
}

void gMainWindow::setTransientFor()
{
	gMainWindow *parent = _current;

	if (!parent)
		parent = _active;

	if (parent)
	{
		parent = parent->topLevel();
		if (parent != this)
		{
			//fprintf(stderr, "setTransientFor: %s -> %s\n", name(), parent->name());
			gtk_window_set_transient_for(GTK_WINDOW(border), GTK_WINDOW(parent->border));
		}
	}
}

void gMainWindow::setVisible(bool vl)
{
	if (!vl)
		_hidden = true;

	if (vl == isVisible())
		return;
	
	if (!isTopLevel())
	{
		gContainer::setVisible(vl);
		if (vl)
		{
			_hidden = false;
			//setActiveWindow(this);
		}
		return;
	}

	if (vl)
	{
		//bool arr = !isVisible();

		emitOpen();
		if (!_opened)
			return;

		_not_spontaneous = !isVisible();
		_visible = true;
		_hidden = false;

		setTransparent(_transparent); // must not call gtk_window_present!

		if (isTopLevel())
		{
			/*if (!_xembed)
			{
				fprintf(stderr, "gtk_window_group_add_window: %p -> %p\n", border, gApplication::currentGroup());
				gtk_window_group_add_window(gApplication::currentGroup(), GTK_WINDOW(border));
				fprintf(stderr, "-> %p\n", gtk_window_get_group(GTK_WINDOW(border)));
			}*/

			// Thanks for Ubuntu's GTK+ patching :-(
			#ifndef GTK3
			//gtk_window_set_has_resize_grip(GTK_WINDOW(border), false);
			if (g_object_class_find_property(G_OBJECT_GET_CLASS(border), "has-resize-grip"))
				g_object_set(G_OBJECT(border), "has-resize-grip", false, (char *)NULL);
			#endif

			gtk_window_move(GTK_WINDOW(border), bufX, bufY);

			/*if (isPopup())
			{
				gtk_widget_show_now(border);
				gtk_widget_grab_focus(border);
			}
			else
			{*/
				present();
			//}

			if (!_title || !*_title)
				gtk_window_set_title(GTK_WINDOW(border), gApplication::defaultTitle());

			if (isUtility())
			{
				setTransientFor();
				if (!_no_take_focus)
					present();
			}

			#ifndef GTK3
			if (gApplication::mainWindow() == this)
			{
				int desktop = session_manager_get_desktop();
				if (desktop >= 0)
				{
					//fprintf(stderr, "X11_window_set_desktop: %d (%d)\n", desktop, true);
					X11_window_set_desktop((Window)handle(), true, desktop);
					session_manager_set_desktop(-1);
				}
			}
			#endif
		}
		else
		{
			gtk_widget_show(border);
			parent()->performArrange();
			performArrange();
		}

		drawMask();
		
		_set_focus = true;

		if (isSkipTaskBar())
			_activate = true;

		/*if (arr)
		{
				fprintf(stderr, "#4\n");
				performArrange();
		}*/
	}
	else
	{
		if (this == _active)
			_initial_focus = gApplication::activeControl();

		_not_spontaneous = isVisible();
		gContainer::setVisible(false);

		if (_popup)
			gApplication::exitLoop(this);

		if (gApplication::_button_grab && !gApplication::_button_grab->isReallyVisible())
				gApplication::setButtonGrab(NULL);
	}
}


void gMainWindow::setMinimized(bool vl)
{
	if (!isTopLevel()) return;

	_minimized = vl;
	if (vl) gtk_window_iconify(GTK_WINDOW(border));
	else    gtk_window_deiconify(GTK_WINDOW(border));
}

void gMainWindow::setMaximized(bool vl)
{
	if (!isTopLevel())
		return;

	_maximized = vl;
	_csd_w = _csd_h = -1;

	if (vl)
		gtk_window_maximize(GTK_WINDOW(border));
	else
		gtk_window_unmaximize(GTK_WINDOW(border));
}

void gMainWindow::setFullscreen(bool vl)
{
	if (!isTopLevel())
		return;

	_fullscreen = vl;
	_csd_w = _csd_h = -1;

	if (vl)
	{
		gtk_window_fullscreen(GTK_WINDOW(border));
		if (isVisible())
			present();
	}
	else
		gtk_window_unfullscreen(GTK_WINDOW(border));
}

void gMainWindow::center()
{
	if (!isTopLevel()) return;

#ifdef GTK3

	if (MAIN_platform_is_wayland)
		gtk_window_set_position(GTK_WINDOW(border), GTK_WIN_POS_CENTER_ON_PARENT);
	
#endif

	GdkRectangle rect;
	int x, y;
	
	gtk_widget_realize(border);
	gDesktop::availableGeometry(screen(), &rect);
	
	x = rect.x + (rect.width - width()) / 2;
	y = rect.y + (rect.height - height()) / 2;

	move(x, y);
}

bool gMainWindow::isModal() const
{
	if (!isTopLevel()) return false;

	return gtk_window_get_modal(GTK_WINDOW(border));
}

void gMainWindow::showModal()
{
	if (!isTopLevel()) return;
	if (isModal() || isPopup()) return;

	gApplication::finishFocus();
	gMouse::finishEvent();
	
	//show();
	setType(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_modal(GTK_WINDOW(border), true);
	setTransientFor();

	_save_focus = gApplication::activeControl();
	_previous = _current;
	_current = this;

	center();
	show();
	//gtk_grab_add(border);
	gApplication::enterLoop(this);

	_current = _previous;
	_previous = NULL;

	//gtk_grab_remove(border);
	gtk_window_set_modal(GTK_WINDOW(border), false);

	if (!_persistent)
		destroy();
	else
		hide();
	
	if (_save_focus)
	{
		gApplication::setActiveControl(_save_focus, true);
		_save_focus = NULL;
	}
}

void gMainWindow::showPopup(int x, int y)
{
	bool has_border;
	int oldx, oldy;
	GdkWindowTypeHint type;

	if (!isTopLevel()) return;
	if (isModal() || isPopup()) return;

	gApplication::finishFocus();
	gMouse::finishEvent();

	//gtk_widget_unrealize(border);
	//((GtkWindow *)border)->type = GTK_WINDOW_POPUP;
	//gtk_widget_realize(border);

	oldx = left();
	oldy = top();

	_popup = true;
	setType(GTK_WINDOW_POPUP);
	
	has_border = gtk_window_get_decorated(GTK_WINDOW(border));
	type = gtk_window_get_type_hint(GTK_WINDOW(border));

	gtk_window_set_decorated(GTK_WINDOW(border), false);
	gtk_window_set_type_hint(GTK_WINDOW(border), GDK_WINDOW_TYPE_HINT_COMBO);
	
	setTransientFor();

	_save_focus = gApplication::activeControl();
	_previous = _current;
	_current = this;

	gtk_window_resize(GTK_WINDOW(border), bufW, bufH);
  move(x, y);
	//raise();
	setFocus();
	
	gApplication::enterPopup(this);

	_current = _previous;
	_previous = NULL;
	_popup = false;

	if (!_persistent)
		destroy();
	else
	{
		hide();

		gtk_window_set_decorated(GTK_WINDOW(border), has_border);
		gtk_window_set_type_hint(GTK_WINDOW(border), type);

		move(oldx, oldy);
	}
	
	if (_save_focus)
	{
		gApplication::setActiveControl(_save_focus, true);
		_save_focus = NULL;
	}
}

void gMainWindow::showActivate()
{
	bool v = isTopLevel() && isVisible() && !_no_take_focus;

	setType(GTK_WINDOW_TOPLEVEL);

	if (!_moved)
		center();
	emitOpen();
	if (!_opened)
		return;
	show();
	if (v)
		present();
}

void gMainWindow::activate()
{
	if (isTopLevel() && isVisible())
		present();
}

void gMainWindow::showPopup()
{
	int x, y;
	gMouse::getScreenPos(&x, &y);
	showPopup(x, y);
}

void gMainWindow::restack(bool raise)
{
	if (!isTopLevel())
	{
		gControl::restack(raise);
		return;
	}
	
	if (raise)
		present();
	else
		gdk_window_lower(gtk_widget_get_window(border));
}

const char* gMainWindow::text()
{
	return _title;
}

void gMainWindow::setText(const char *txt)
{
	if (txt != _title)
	{
		if (_title) 
		{
			g_free(_title);
			_title = NULL;
		}
		
		if (txt && *txt)
			_title = g_strdup(txt);
	}

	if (isTopLevel())
		gtk_window_set_title(GTK_WINDOW(border), _title ? _title : "");
}

bool gMainWindow::hasBorder()
{
	if (isTopLevel())
		return gtk_window_get_decorated(GTK_WINDOW(border));
	else
		return false;
}

bool gMainWindow::isResizable()
{
	if (isTopLevel())
		return _resizable;
	else
		return false;
}

void gMainWindow::setBorder(bool b)
{
	if (!isTopLevel())
		return;

	gtk_window_set_decorated(GTK_WINDOW(border), b);
}

void gMainWindow::setResizable(bool b)
{
	if (!isTopLevel())
		return;

	if (b == isResizable())
		return;

	_resizable = b;
	updateSize();
}

void gMainWindow::setSkipTaskBar(bool b)
{
	if (!isTopLevel()) return;
	_skip_taskbar = b;
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(border), b);
}


/*gPicture* gMainWindow::icon()
{
	GdkPixbuf *buf;
	gPicture *pic;

	if (!isTopLevel()) return NULL;

	buf=gtk_window_get_icon(GTK_WINDOW(border));
	if (!buf) return NULL;

	pic=gPicture::fromPixbuf(buf);

	return pic;
}*/

void gMainWindow::setIcon(gPicture *pic)
{
  gPicture::assign(&_icon, pic);

	if (!isTopLevel()) return;
  gtk_window_set_icon(GTK_WINDOW(border), pic ? pic->getPixbuf() : NULL);
}


void gMainWindow::setTopOnly(bool vl)
{
	if (!isTopLevel()) return;

	_top_only = vl;
	gtk_window_set_keep_above (GTK_WINDOW(border), vl);
}


void gMainWindow::setMask(bool vl)
{
	if (_mask == vl)
		return;

	_mask = vl;
	drawMask();
}

void gMainWindow::setPicture(gPicture *pic)
{
  gPicture::assign(&_picture, pic);
  drawMask();
}

void gMainWindow::remap()
{
	if (!isVisible())
		return;

	gtk_widget_unmap(border);
	gtk_widget_map(border);

	if (_skip_taskbar) { setSkipTaskBar(false);	setSkipTaskBar(true); }
	if (_top_only) { setTopOnly(false); setTopOnly(true); }
	if (_sticky) { setSticky(false); setSticky(true); }
	if (stack) { setStacking(0); setStacking(stack); }
}

void gMainWindow::drawMask()
{
	bool do_remap = false;

	if (!isVisible())
		return;

#ifdef GTK3

	cairo_region_t *mask;

	if (_mask && _picture)
		mask = gdk_cairo_region_create_from_surface(_picture->getSurface());
	else
		mask = NULL;

	gdk_window_shape_combine_region(gtk_widget_get_window(border), mask, 0, 0);
	if (mask)
		cairo_region_destroy(mask);

	refresh();

#else

	GdkBitmap *mask = (_mask && _picture) ? _picture->getMask() : NULL;
	do_remap = !mask && _masked;

	gdk_window_shape_combine_mask(border->window, mask, 0, 0);

#endif

	if (_picture)
	{
		gtk_widget_set_app_paintable(border, TRUE);
		gtk_widget_realize(border);
		gtk_widget_realize(widget);
		// What for??
		/*for (int i = 0; i < controlCount(); i++)
			getControl(i)->refresh();*/
	}
	else if (!_transparent)
	{
		gtk_widget_set_app_paintable(border, FALSE);
		setRealBackground(background());
	}

	_masked = mask != NULL;

	if (do_remap)
		remap();
	else
	{
		if (!_skip_taskbar)
		{
			setSkipTaskBar(true);
			setSkipTaskBar(false);
		}
	}
}

int gMainWindow::menuCount()
{
	if (!menuBar) return 0;
	return gMenu::winChildCount(this);
}

void gMainWindow::setPersistent(bool vl)
{
  _persistent = vl;
}

bool gMainWindow::doClose(bool destroying)
{
	if (_closing || _closed)
		return false;

	if (!isTopLevel())
	{
		if (_opened)
		{
			_closing = true;
			_closed = !CB_window_close(this);
			_closing = false;
			_opened = !_closed;
		}
		else
			_closed = true;

		if (_closed)
		{
			if (_persistent || destroying)
				hide();
			else
				destroy();
		}
	}
	else
	{
		if (_opened)
		{
			if (isModal() && !gApplication::hasLoop(this))
				return true;

			_closing = true;
			_closed = !CB_window_close(this);
			_closing = false;
			_opened = !_closed;

			if (!_opened && isModal())
				gApplication::exitLoop(this);
		}

		if (!_opened) // && !modal())
		{
			if (_active == this)
				setActiveWindow(NULL);

			if (!isModal())
			{
				if (_persistent || destroying)
					hide();
				else
					destroy();
			}
		}
	}
	
	return _opened;
}


bool gMainWindow::close()
{
	return doClose();
}

static void hide_hidden_children(gContainer *cont)
{
	int i;
	gControl *child;

	for (i = 0;; i++)
	{
		child = cont->child(i);
		if (!child)
			break;
		if (!child->isVisible())
			gtk_widget_hide(child->border);
		else if (child->isContainer())
			hide_hidden_children((gContainer *)child);
	}
}

void gMainWindow::createWindow(GtkWidget *new_border)
{
	gt_widget_reparent(frame, new_border);
	createBorder(new_border);
	updateEventMask();
	registerControl();
}

void gMainWindow::reparent(gContainer *newpr, int x, int y)
{
	int w, h;
	gColor fg, bg;
	bool was_visible;

	if (_xembed)
		return;

	bg = background();
	fg = foreground();
	was_visible = isVisible();

	if (isTopLevel() && newpr)
	{
		windows = g_list_remove(windows, (gpointer)this);
		gtk_window_remove_accel_group(GTK_WINDOW(topLevel()->border), accel);

		createWindow(gtk_event_box_new());

		setParent(newpr);
		connectParent();
		//newpr->insert(this);
		borderSignals();

		initWindow();
		
		setBackground(bg);
		setForeground(fg);
		setFont(font());

		checkMenuBar();

		bufX = bufY = 0;
		move(x, y);

		gtk_widget_set_size_request(border, width(), height());

		if (was_visible)
			gtk_widget_show(border);
		// Hidden children are incorrectly shown. Fix that!
		hideHiddenChildren();
		
		if (!isIgnore())
			newpr->performArrange();
	}
	else if ((!isTopLevel() && !newpr)
	         || (isTopLevel() && isPopup()))
	{
		windows = g_list_append(windows, (gpointer)this);
		gtk_window_remove_accel_group(GTK_WINDOW(topLevel()->border), accel);
		// TODO: test that
		
		createWindow(gtk_window_new(GTK_WINDOW_TOPLEVEL));
		
		if (parent())
		{
			parent()->remove(this);
			if (!isIgnore())
				parent()->performArrange();
			setParent(NULL);
		}

		borderSignals();
		initWindow();
		
		setBackground(bg);
		setForeground(fg);
		setFont(font());
		setText(text());

		move(x, y);
		w = width();
		h = height();
		bufW = bufH = -1;
		gtk_widget_set_size_request(border, 1, 1);
		resize(w, h);

		gtk_widget_set_sensitive(frame, FALSE);
		gtk_widget_set_sensitive(frame, TRUE);

		if (was_visible)
			present();
		hideHiddenChildren();
		
		_popup = false; //type == GTK_WINDOW_POPUP;
	}
	else
	{
		gContainer::reparent(newpr, x, y);
	}
}

void gMainWindow::setType(GtkWindowType type)
{
	int w, h;
	gColor bg, fg;

	if (!isTopLevel())
		return;
	if (gtk_window_get_window_type(GTK_WINDOW(border)) == type)
		return;
	
	bg = background();
	fg = foreground();

	gtk_window_remove_accel_group(GTK_WINDOW(border), accel);
	// TODO: test that
	
	createWindow(gtk_window_new(type));

	initWindow();
	borderSignals();
	setBackground(bg);
	setForeground(fg);
	setFont(font());

	w = width();
	h = height();
	bufW = bufH = -1;
	gtk_widget_set_size_request(border, 1, 1);
	resize(w, h);

	hideHiddenChildren();
}

static void fill_children_list(gContainer *cont, GPtrArray *list)
{
	int i;
	gControl *control;
	
	for (i = 0; i < cont->childCount(); i++)
	{
		control = cont->child(i);
		if (control->isContainer()) // && !control->isWindow())
			fill_children_list((gContainer *)control, list);
		g_ptr_array_add(list, control);
	}
}

GPtrArray *gMainWindow::getControlList()
{
	GPtrArray *list = g_ptr_array_new();
	fill_children_list(this, list);
	return list;
}

gControl *gMainWindow::getControl(const char *name)
{
	GPtrArray *list = getControlList();
	uint i;
	gControl *ctrl;

	for (i = 0; i < list->len; i++)
	{
		ctrl = (gControl *)g_ptr_array_index(list, i);
		if (!ctrl->isDestroyed() && !strcasecmp(ctrl->name(), name))
			break;
		ctrl = NULL;
	}

	g_ptr_array_unref(list);
	return ctrl;
}

int gMainWindow::clientX()
{
	return 0;
}

int gMainWindow::containerX()
{
	return 0;
}

int gMainWindow::clientY()
{
	if (isMenuBarVisible())
		return menuBarHeight();
	else
		return 0;
}

int gMainWindow::containerY()
{
	return 0;
}


int gMainWindow::clientWidth()
{
	return width();
}


int gMainWindow::menuBarHeight()
{
	int h = 0;

	if (menuBar)
	{
		//gtk_widget_show(GTK_WIDGET(menuBar));
		//fprintf(stderr, "menuBarHeight: gtk_widget_get_visible: %d\n", gtk_widget_get_visible(GTK_WIDGET(menuBar)));
#ifdef GTK3
		gtk_widget_get_preferred_height(GTK_WIDGET(menuBar), NULL, &h);
#else
		GtkRequisition req = { 0, 0 };
		gtk_widget_size_request(GTK_WIDGET(menuBar), &req);
		h = req.height;
#endif
		//fprintf(stderr, "menuBarHeight: %d\n", h);
	}

	return h;
}

int gMainWindow::clientHeight()
{
	if (isMenuBarVisible())
		return height() - menuBarHeight();
	else
		return height();
}

void gMainWindow::setActiveWindow(gControl *control)
{
	_active = CB_window_activate(control);
}

#ifdef GDK_WINDOWING_X11
bool gMainWindow::isUtility() const
{
	return _utility;
}

void gMainWindow::setUtility(bool v)
{
	bool remap = false;

	if (!isTopLevel())
		return;

	// TODO: works only if the window is not mapped!

	_utility = v;
#if GTK_CHECK_VERSION(2, 20, 0)
	if (gtk_widget_get_mapped(border))
#else
    if (GTK_WIDGET_MAPPED(border))
#endif
	{
		remap = true;
		gtk_widget_unmap(border);
	}

	gtk_window_set_type_hint(GTK_WINDOW(border), v ? GDK_WINDOW_TYPE_HINT_DIALOG : GDK_WINDOW_TYPE_HINT_NORMAL);

	if (remap)
		gtk_widget_map(border);
}
#else
bool gMainWindow::isUtility()
{
	return _utility;
}

void gMainWindow::setUtility(bool v)
{
	_utility = v;
}
#endif

void gMainWindow::configure()
{
	static bool init = FALSE;
	static GB_FUNCTION _init_menubar_shortcut_func;

	int h;

	if (bufW < 1 || bufH < 1)
		return;

	if (_initMenuBar != isMenuBarVisible())
	{
		_initMenuBar = !_initMenuBar;

		if (!init)
		{
			GB.GetFunction(&_init_menubar_shortcut_func, (void *)GB.FindClass("_Gui"), "_InitMenuBarShortcut", NULL, NULL);
			init = TRUE;
		}

		GB.Push(1, GB_T_OBJECT, hFree);
		GB.Call(&_init_menubar_shortcut_func, 1, FALSE);
	}

	h = menuBarHeight();

	#ifdef DEBUG_RESIZE
	fprintf(stderr, "configure: %s: menu = %d h = %d / %d x %d\n", name(), isMenuBarVisible(), h, width(), height());
	#endif

	if (isMenuBarVisible())
	{
		gtk_fixed_move(GTK_FIXED(frame), GTK_WIDGET(menuBar), 0, 0);
		if (h > 1)
			gtk_widget_set_size_request(GTK_WIDGET(menuBar), width(), h);
		gtk_fixed_move(GTK_FIXED(frame), widget, 0, h);
		gtk_widget_set_size_request(widget, width(), Max(0, height() - h));
	}
	else
	{
		if (menuBar)
			gtk_fixed_move(GTK_FIXED(frame), GTK_WIDGET(menuBar), -width(), -h);
		gtk_fixed_move(GTK_FIXED(frame), widget, 0, 0);
		gtk_widget_set_size_request(widget, width(), height());
	}
}

bool gMainWindow::setMenuBarVisible(bool v)
{
	if (_showMenuBar == v)
		return true;

	_showMenuBar = v;

	if (!menuBar)
		return true;

	configure();
	performArrange();

	return false;
}

bool gMainWindow::isMenuBarVisible()
{
	//fprintf(stderr, "isMenuBarVisible: %d\n", !!(menuBar && !_hideMenuBar && _showMenuBar));
	return menuBar && !_hideMenuBar && _showMenuBar; //|| (menuBar && GTK_WIDGET_MAPPED(GTK_WIDGET(menuBar)));
}

void gMainWindow::updateFont()
{
	gContainer::updateFont();
	gMenu::updateFont(this);
	CB_window_font(this);
}

void gMainWindow::checkMenuBar()
{
	int i;
	gMenu *menu;

	//fprintf(stderr, "gMainWindow::checkMenuBar\n");

	if (menuBar)
	{
		_hideMenuBar = true;
		for (i = 0;; i++)
		{
			menu = gMenu::winChildMenu(this, i);
			if (!menu)
				break;
			if (menu->isVisible() && !menu->isSeparator())
			{
				_hideMenuBar = false;
				break;
			}
		}
	}

	configure();
	performArrange();
}

void gMainWindow::embedMenuBar(GtkWidget *border)
{
	if (menuBar)
	{
		g_object_ref(G_OBJECT(menuBar));

		if (gtk_widget_get_parent(GTK_WIDGET(menuBar)))
			gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(menuBar))), GTK_WIDGET(menuBar));

		gtk_fixed_put(GTK_FIXED(frame), GTK_WIDGET(menuBar), 0, 0);

		g_object_unref(G_OBJECT(menuBar));

		gtk_widget_show(GTK_WIDGET(menuBar));

		gMenu::updateFont(this);
		gMenu::updateColor(this);

		checkMenuBar();
	}
}

/*bool gMainWindow::getScreenPos(int *x, int *y)
{
	return gContainer::getScreenPos(x, y);
}*/

double gMainWindow::opacity()
{
	if (isTopLevel())
#if GTK_CHECK_VERSION(3, 8, 0)
		return gtk_widget_get_opacity(border);
#else
		return gtk_window_get_opacity(GTK_WINDOW(border));
#endif
	else
		return 1.0;
}

void gMainWindow::setOpacity(double v)
{
	if (isTopLevel())
#if GTK_CHECK_VERSION(3, 8, 0)
		gtk_widget_set_opacity(border, v);
#else
		gtk_window_set_opacity(GTK_WINDOW(border), v);
#endif
}

int gMainWindow::screen()
{
	gMainWindow *tl = topLevel();
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkWindow *window = gtk_widget_get_window(tl->border);
	if (window)
		return gt_find_monitor(gdk_display_get_monitor_at_window(gdk_display_get_default(), window));
	else
		return -1;
#else
	return gdk_screen_get_number(gtk_window_get_screen(GTK_WINDOW(tl->border)));
#endif
}

void gMainWindow::emitResize()
{
	if (bufW == _resize_last_w && bufH == _resize_last_h)
		return;

	#ifdef DEBUG_RESIZE
	fprintf(stderr, "emitResize: %s: %d %d\n", name(), bufW, bufH);
	#endif
	_resize_last_w = bufW;
	_resize_last_h = bufH;
	configure();
	performArrange();
	CB_window_resize(this);
}

static void emit_resize_later(gMainWindow *window)
{
	window->emitResize();
}

void gMainWindow::emitResizeLater()
{
	GB.Post((GB_CALLBACK)emit_resize_later, (intptr_t)this);
}

void gMainWindow::setGeometryHints()
{
	GdkGeometry geometry;
	int min_w, min_h;
	
	if (isTopLevel())
	{
		min_w = _min_w;
		min_h = _min_h;

		if (isResizable())
		{
			if (isModal() || isUtility())
			{
				if (!min_w && !min_h)
				{
					min_w = _default_min_w;
					min_h = _default_min_h;
				}
			}

			geometry.min_width = min_w + Max(_csd_w, 0);
			geometry.min_height = min_h + Max(_csd_h, 0);
			
			geometry.max_width = 32767;
			geometry.max_height = 32767;
		}
		else
		{
			geometry.max_width = geometry.min_width = width() + Max(_csd_w, 0);
			geometry.max_height = geometry.min_height = height() + Max(_csd_h, 0);
		}

		#if DEBUG_RESIZE
		fprintf(stderr, "setGeometryHints: %s: min size: %d %d (%d x %d)\n", name(), geometry.min_width, geometry.min_height, width(), height());
		#endif
		gtk_window_set_geometry_hints(GTK_WINDOW(border), NULL, &geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
		//gdk_window_set_geometry_hints(gtk_widget_get_window(border), &geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_POS));
	}
}

void gMainWindow::setBackground(gColor vl)
{
	if (!_transparent)
		gControl::setBackground(vl);
	else
		_bg = vl;
}

void gMainWindow::setTransparent(bool vl)
{
	if (!vl)
		return;

	_transparent = TRUE;
	
	/*#ifdef GTK3
	if (MAIN_platform_is_wayland)
		return;
	#endif*/

	if (!isVisible())
		return;

#ifdef GTK3
	GdkScreen *screen = NULL;
	GdkVisual *visual = NULL;

	screen = gtk_widget_get_screen(border);
	visual = gdk_screen_get_rgba_visual(screen);
	if (visual == NULL)
		return;
#else
	GdkScreen *screen;
	GdkColormap *colormap;

	screen = gtk_widget_get_screen(border);
	colormap = gdk_screen_get_rgba_colormap(screen);
	if (colormap == NULL)
		return;
#endif

	gtk_widget_unrealize(border);

	gtk_widget_set_app_paintable(border, TRUE);

#ifdef GTK3
	gtk_widget_set_visual(border, visual);
#else
	gtk_widget_set_colormap(border, colormap);
#endif

	gtk_widget_realize(border);

	/*int w = width();
	int h = height();

	bufW = w - 1;
	resize(w, h);*/

	//gtk_window_present(GTK_WINDOW(border));
	//updateSize();
}

bool gMainWindow::closeAll()
{
	int i;
	gMainWindow *win;

	for(i = 0; i < count(); i++)
	{
		win = get(i);
		if (!win)
			break;
		if (!win->isTopLevel())
			continue;
		if (win == gApplication::mainWindow())
			continue;
		if (win->close())
			return true;
	}

	return false;
}

void gMainWindow::setNoTakeFocus(bool v)
{
	_no_take_focus = v;
	if (isTopLevel())
		gtk_window_set_focus_on_map(GTK_WINDOW(border), !_no_take_focus);
}

void gMainWindow::calcCsdSize()
{
	GtkAllocation ba;
	GtkAllocation wa;
	
	if (_csd_w >= 0)
		return;
		
	if (!isTopLevel())
	{
		_csd_w = _csd_h = 0;
		return;
	}
		
	gtk_widget_get_allocation(border, &ba);
	if (ba.width <= 1 && ba.height <= 1)
		return;

	gtk_widget_get_allocation(frame, &wa);
	if (wa.width <= 1 && wa.height <= 1)
		return;
	
	_csd_w = ba.width - wa.width;
	_csd_h = ba.height - wa.height;
	#ifdef DEBUG_RESIZE
	fprintf(stderr, "calcCsdSize: border: %d %d layout: %d %d\n", ba.width, ba.height, wa.width, wa.height);
	fprintf(stderr, "calcCsdSize: --> %s: csd = %d %d\n", name(), _csd_w, _csd_h);
	#endif
	
	if (!isResizable())
		updateSize();
	else
		setGeometryHints();
}

void gMainWindow::destroy()
{
	doClose(true);
	gControl::destroy();
}

void gMainWindow::setCustomMinimumSize(int w, int h)
{
	w = Max(0, w);
	h = Max(0, h);
	if (w == _min_w && h == _min_h)
		return;
	_min_w = w;
	_min_h = h;
	updateSize();
}

void gMainWindow::getCustomMinimumSize(int *w, int *h) const
{
	*w = _min_w;
	*h = _min_h;
}

gControl *gMainWindow::getInitialFocus()
{
	gControl *ctrl;
	
	if (!_set_focus)
		return this;
	
	_set_focus = false;
	
	if (_initial_focus)
	{
		ctrl = _initial_focus;
		_initial_focus = NULL;
		//fprintf(stderr, "focus = %p %s\n", focus->border, focus->name());
		//focus->setFocus();
		//fprintf(stderr, "focus of window %p -> %p\n", border, gtk_window_get_focus(GTK_WINDOW(border)));
		//focus = NULL;
	}
	else
	{
		ctrl = this;
		
		for(;;)
		{
			ctrl = ctrl->nextFocus();
			if (!ctrl)
				break;
			
			if (ctrl->isReallyVisible() && ctrl->isEnabled() && !ctrl->isWindow() && ctrl->canFocus())
				break;
			
			if (ctrl == this)
				break;
		}
	}
	
	return ctrl ? ctrl : this;
}

#ifdef GTK3
GtkWidget *gMainWindow::getStyleSheetWidget()
{
	return frame;
}
#endif
