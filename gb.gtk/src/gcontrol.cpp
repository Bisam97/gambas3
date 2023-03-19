/***************************************************************************

	gcontrol.cpp

	(c) 2004-2006 - Daniel Campos Fernández <dcamposf@gmail.com>
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

#include <unistd.h>

#include "widgets.h"
#include "gapplication.h"
#include "gbutton.h"
#include "gdrawingarea.h"
#include "gmainwindow.h"
#include "gscrollbar.h"
#include "gslider.h"
#include "gdesktop.h"
#include "gdrag.h"
#include "gmouse.h"
#include "gmenu.h"

#ifndef GTK3
#include "gplugin.h"
#endif

#include "gcontrol.h"

//#define DEBUG_FOCUS 1
//#define DEBUG_ENTER_LEAVE 1
//#define DEBUG_DESTROY 1

static GList *_destroy_list = NULL;

#if 0
static const char *_cursor_fdiag[] =
{
"16 16 4 1",
"# c None",
"a c #000000",
"b c #c0c0c0",
". c #ffffff",
"..........######",
".aaaaaaaa.######",
".a....ba.#######",
".a...ba.########",
".a....ab.#######",
".a.b...ab.######",
".abaa...ab.###..",
".aa.ba...ab.#.a.",
".a.#.ba...ab.aa.",
"..###.ba...aaba.",
"######.ba...b.a.",
"#######.ba....a.",
"########.ab...a.",
"#######.ab....a.",
"######.aaaaaaaa.",
"######.........."
};

static const char *_cursor_bdiag[] =
{
"16 16 4 1",
". c None",
"a c #000000",
"b c #c0c0c0",
"# c #ffffff",
"......##########",
"......#aaaaaaaa#",
".......#ab####a#",
"........#ab###a#",
".......#ba####a#",
"......#ba###b#a#",
"##...#ba###aaba#",
"#a#.#ba###ab#aa#",
"#aa#ba###ab#.#a#",
"#abaa###ab#...##",
"#a#b###ab#......",
"#a####ab#.......",
"#a###ba#........",
"#a####ba#.......",
"#aaaaaaaa#......",
"##########......"
};
#endif

// Geometry optimization hack - Sometimes fails, so it is disabled...
#define GEOMETRY_OPTIMIZATION 0

// Private structures that allow to implement raise() and lower() methods

#ifdef GTK3

struct _GtkLayoutPrivate
{
  /* Properties */
  guint width;
  guint height;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  /* GtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;

  /* Properties */

  GdkVisibilityState visibility;
  GdkWindow *bin_window;

  GList *children;

  gint scroll_x;
  gint scroll_y;

  guint freeze_count;
};

struct _GtkFixedPrivate
{
  GList *children;
};

typedef struct _GtkLayoutChild GtkLayoutChild;

struct _GtkLayoutChild {
  GtkWidget *widget;
  gint x;
  gint y;
};

#define GET_CHILDREN_LIST(_widget) 

#else

typedef struct _GtkLayoutChild   GtkLayoutChild;

struct _GtkLayoutChild {
  GtkWidget *widget;
  gint x;
  gint y;
};

#endif


#ifdef GTK3
static gboolean cb_background_draw(GtkWidget *wid, cairo_t *cr, gControl *control)
{
	control->drawBackground(cr);
	return false;
}
#else
static gboolean cb_background_expose(GtkWidget *wid, GdkEventExpose *e, gControl *control)
{
	control->drawBackground(e);
	return false;
}
#endif

#ifdef GTK3
static gboolean cb_frame_draw(GtkWidget *wid, cairo_t *cr, gControl *control)
{
	control->drawBorder(cr);
	return false;
}
#else
static gboolean cb_frame_expose(GtkWidget *wid, GdkEventExpose *e, gControl *control)
{
	control->drawBorder(e);
	return false;
}
#endif

#ifndef GTK3

/****************************************************************************

gPlugin

****************************************************************************/

gboolean gPlugin_OnUnplug(GtkSocket *socket,gPlugin *data)
{
	if (data->onUnplug) data->onUnplug(data);
	return true;
}

void gPlugin_OnPlug(GtkSocket *socket,gPlugin *data)
{
	if (data->onPlug) data->onPlug(data);
}


gPlugin::gPlugin(gContainer *parent) : gControl(parent)
{
	border = gtk_socket_new();
	widget = border;
	realize();

	onPlug = NULL;
	onUnplug = NULL;

	g_signal_connect(G_OBJECT(widget), "plug-removed", G_CALLBACK(gPlugin_OnUnplug), (gpointer)this);
	g_signal_connect(G_OBJECT(widget), "plug-added", G_CALLBACK(gPlugin_OnPlug), (gpointer)this);

	ON_DRAW_BEFORE(border, this, cb_background_expose, cb_background_draw);

	setCanFocus(true);
}

int gPlugin::client()
{
	//GdkNativeWindow win = gtk_socket_get_id(GTK_SOCKET(widget));
	//return (long)win;
	GdkWindow *win = gtk_socket_get_plug_window(GTK_SOCKET(widget));
	if (!win)
		return 0;
	else
		return (int)GDK_WINDOW_XID(win);
}

void gPlugin::plug(int id)
{
	void (*func)(gControl *);
	int i;
	Display *d = gdk_x11_display_get_xdisplay(gdk_display_get_default());

	func = onPlug;
	onPlug = NULL;

	for (i = 1; i >= 0; i--)
	{
		if (i == 0)
			onPlug = func;

		gtk_socket_add_id(GTK_SOCKET(widget), (Window)id);
	}

	if (client())
		XAddToSaveSet(d, client());
	else
		emit(SIGNAL(onError));
}

void gPlugin::discard()
{
	#ifdef GDK_WINDOWING_X11
	if (MAIN_display_x11)
	{
		Display *d = gdk_x11_display_get_xdisplay(gdk_display_get_default());

		if (!client()) return;

		XRemoveFromSaveSet(d, client());
		XReparentWindow(d, client(), GDK_ROOT_WINDOW(), 0, 0);
	}
	#else
	stub("no-X11/gPlugin:discard()");
	#endif
}
#endif

/*****************************************************************

CREATION AND DESTRUCTION

******************************************************************/

void gControl::postDelete()
{
	GList *iter;
	gControl *control;

	gMenu::cleanRemovedMenus();
	
	if (!_destroy_list) return;

	for(;;)
	{
		iter = g_list_first(_destroy_list);
		if (!iter)
			break;
		control = (gControl *)iter->data;
#if DEBUG_DESTROY
		fprintf(stderr, "postDelete: %p %s\n", control, control->name());
#endif
		gtk_widget_destroy(control->border);
	}

	_destroy_list = NULL;
}

void gControl::initAll(gContainer *parent)
{
	bufW = 1;
	bufH = 1;
	bufX = -16;
	bufY = -16;
	
	_min_w = _min_h = 1;
	_minimum_size_set = FALSE;
	curs = NULL;
	_font = NULL;
	_resolved_font = NULL;
	_design = false;
	_design_ignore = false;
	_no_design = false;
	_expand = false;
	_ignore = false;
	_inverted = false;
	_accept_drops = false;
	_dragging = false;
	frame_border = 0;
	frame_padding = 0;
	_bg_set = false;
	_fg_set = false;
	have_cursor = false;
	use_base = false;
	_mouse = CURSOR_DEFAULT;
	pr = parent;
	_name = NULL;
	_visible = false;
	_locked = 0;
	_destroyed = false;
	_no_delete = false;
	_action = false;
	_dirty_pos = _dirty_size = false;
	_tracking = false;
	_has_input_method = false;
	_no_default_mouse_event = false;
	_proxy = _proxy_for = NULL;
	_no_tab_focus = false;
	_inside = false;
	_no_auto_grab = false;
	_no_background = false;
	_use_wheel = false;
	_scrollbar = SCROLL_NONE;
	_input_method = NULL;
	_tooltip = NULL;
	_is_container = false;
	_is_window = false;
	_is_button = false;
	_is_drawingarea = false;
	_has_native_popup = false;
	_eat_return_key = false;
	//_hidden_temp = false;
	_allow_show = false;
	_direction = DIRECTION_DEFAULT;

	frame = border = widget = NULL;
	_scroll = NULL;
	hFree = NULL;
	_grab = false;

	_fg = _bg = COLOR_DEFAULT;
#ifdef GTK3
	_css = NULL;
	_has_css_id = false;
	_style_dirty = false;
	_no_style_without_child = false;
#endif
}

gControl::gControl()
{
	initAll(NULL);
}

gControl::gControl(gContainer *parent)
{
	initAll(parent);
}

void gControl::dispose()
{
	gMainWindow *win;
	
	win = window();
	if (win && win->_initial_focus == this)
		win->_initial_focus = NULL;
	
	win = gMainWindow::_current;
	while (win)
	{
		if (win && win->_save_focus == this)
			win->_save_focus = NULL;
		win = win->_previous;
	}

	if (pr)
	{
		pr->remove(this);
		pr = NULL;
	}
}

gControl::~gControl()
{
	//fprintf(stderr, "~gControl: %s %p %s\n", name(), this, GB.GetClassName(hFree));
	CB_control_finish(this);

	dispose();

	if (_proxy)
		_proxy->_proxy_for = NULL;
	if (_proxy_for)
		_proxy_for->_proxy = NULL;

	if (gDrag::getSource() == this)
		gDrag::cancel();

	if (curs)
	{
		delete curs;
		curs=NULL;
	}

	if (_font)
	{
		gFont::assign(&_font);
		gFont::assign(&_resolved_font);
	}

#ifdef GTK3
	if (_css)
		g_object_unref(_css);
#endif

	if (_name)
		g_free(_name);
	if (_tooltip)
		g_free(_tooltip);

#if DEBUG_DESTROY
	fprintf(stderr, "remove from destroy list: %p (%d)\n", this, _destroyed);
#endif
	_destroy_list = g_list_remove(_destroy_list, this);

	#define CLEAN_POINTER(_p) if (_p == this) _p = NULL

	CLEAN_POINTER(gApplication::_enter);
	CLEAN_POINTER(gApplication::_leave);
	CLEAN_POINTER(gApplication::_active_control);
	CLEAN_POINTER(gApplication::_previous_control);
	CLEAN_POINTER(gApplication::_old_active_control);
	CLEAN_POINTER(gApplication::_button_grab);
	CLEAN_POINTER(gApplication::_enter_after_button_grab);
	CLEAN_POINTER(gApplication::_control_grab);
	CLEAN_POINTER(gApplication::_ignore_until_next_enter);
	CLEAN_POINTER(gDrag::_destination);
	CLEAN_POINTER(gDrag::_source);
	CLEAN_POINTER(gDrag::_current);
	CLEAN_POINTER(gMouse::_control);
}

void gControl::destroy()
{
	if (_destroyed)
		return;

#if DEBUG_DESTROY
	fprintf(stderr, "destroy: %p %s (%d)\n", this, name(), _destroyed);
#endif
	hide();
	_destroyed = true;
	dispose();

#if DEBUG_DESTROY
	fprintf(stderr, "added to destroy list: %p %s (%d)\n", this, name(), _destroyed);
	if (g_list_find(_destroy_list, this))
	{
		fprintf(stderr, "already present!!!\n");
		BREAKPOINT();
	}
#endif

	_destroy_list = g_list_prepend(_destroy_list, (gpointer)this);
}


bool gControl::isEnabled() const
{
	return gtk_widget_is_sensitive(border);
}

bool gControl::isReallyVisible()
{
	if (!isTopLevel() && !topLevel()->isReallyVisible())
		return false;

#if GTK_CHECK_VERSION(2, 20, 0)
	return gtk_widget_get_mapped(border);
#else
	return GTK_WIDGET_MAPPED(border);
#endif
}

void gControl::setEnabled(bool vl)
{
	gtk_widget_set_sensitive(border, vl);
}

void gControl::setVisibility(bool vl)
{
	_visible = vl;
	
	if (!_allow_show)
		return;

	if (vl == gtk_widget_get_visible(border))
		return;

	if (vl)
	{
		if (bufW >= minimumWidth() && bufH >= minimumHeight())
		{
			gtk_widget_show(border);
			_dirty_size = true;
			updateGeometry();
	#ifdef GTK3
			updateStyleSheet(false);
	#endif
		}
	}
	else
	{
		if (hasFocus())
		{
			if (parent())
				gcb_focus(widget, GTK_DIR_TAB_FORWARD, this);
			gApplication::setActiveControl(this, false);
		}
		if (gtk_widget_has_grab(border))
			gtk_grab_remove(border);
		gtk_widget_hide(border);
	}

	if (!isIgnore() && pr) pr->performArrange();
}

void gControl::setVisible(bool vl)
{
	setVisibility(vl);
	checkVisibility();
}

/*****************************************************************

POSITION AND SIZE

******************************************************************/

bool gControl::getScreenPos(int *x, int *y)
{
	if (!gtk_widget_get_window(border))
	{
		if (pr)
		{
			pr->getScreenPos(x, y);
			x += pr->clientX() + bufX;
			y += pr->clientY() + bufY;
			return false;
		}
		
		*x = *y = 0; // widget is not realized
		return true;
	}

	gdk_window_get_origin(gtk_widget_get_window(border), x, y);

	//fprintf(stderr, "getScreenPos: %s: %d %d: %d\n", name(), *x, *y, gtk_widget_get_has_window(border));

	#if GTK_CHECK_VERSION(2, 18, 0)
	if (!gtk_widget_get_has_window(border))
	{
		GtkAllocation a;
		gtk_widget_get_allocation(border, &a);
		*x += a.x;
		*y += a.y;
	}
	#endif

	//fprintf(stderr, "getScreenPos: --> %d %d\n", *x, *y);
	return false;
}

int gControl::screenX()
{
	if (isTopLevel())
	{
		GdkWindow *window = gtk_widget_get_window(border);
		int x = 0;
		GtkAllocation a;
		
		if (window)
			gdk_window_get_origin(window, &x, NULL);
		
		gtk_widget_get_allocation(widget, &a);
		
		return x + a.x - ((gContainer *)this)->clientX();
	}
	
	return pr->screenX() + x() - pr->clientX() - pr->scrollX();
}

int gControl::screenY()
{
	if (isTopLevel())
	{
		GdkWindow *window = gtk_widget_get_window(border);
		int y = 0;
		GtkAllocation a;
		
		if (window)
			gdk_window_get_origin(window, NULL, &y);
		
		gtk_widget_get_allocation(widget, &a);
		
		return y + a.y - ((gContainer *)this)->clientY();
	}
	
	return pr->screenY() + y() + pr->clientY() - pr->scrollY();
}

static void send_configure(gControl *control)
{
	GtkWidget *widget;
	GdkEvent *event;

	widget = control->border;

	if (!gtk_widget_get_realized(widget))
		return;

// 	if (control->isWindow())
// 	 g_debug("send configure to window: %s", control->name());

	event = gdk_event_new(GDK_CONFIGURE);

	event->configure.window = NULL; //(GdkWindow *)g_object_ref(widget->window);
	event->configure.send_event = TRUE;
	event->configure.x = control->x();
	event->configure.y = control->y();
	event->configure.width = control->width();
	event->configure.height = control->height();

	gtk_widget_event(widget, event);
	gdk_event_free(event);
}

void gControl::move(int x, int y)
{
	//GtkLayout *fx;

	if (x == bufX && y == bufY)
		return;

	bufX = x;
	bufY = y;

	//g_debug("move: %p: %d %d", this, x, y);
	_dirty_pos = true;
	if (pr && !isIgnore())
	{
		// TODO: check the following optimization to see if it can be enabled again
		//if (gtk_widget_get_parent(border) == pr->getContainer())
			pr->performArrange();
	}

	#if GEOMETRY_OPTIMIZATION
	gApplication::setDirty();
	#else
	updateGeometry();
	#endif
	
	checkVisibility();
	
	send_configure(this); // needed for Watcher and Form Move events
}

void gControl::hideButKeepFocus()
{
	//fprintf(stderr, "gControl::hideButKeepFocus: %s\n", gApplication::_active_control ? gApplication::_active_control->name() : "NULL");

	if (gtk_widget_get_visible(border))
	{
		gApplication::_keep_focus = true;
		gApplication::_disable_mapping_events = true;
		gtk_widget_hide(border);
		gApplication::_disable_mapping_events = false;
		gApplication::_keep_focus = false;
	}
}

void gControl::showButKeepFocus()
{
	gControl *focus;

	//fprintf(stderr, "gControl::showButKeepFocus: %s\n", gApplication::_active_control ? gApplication::_active_control->name() : "NULL");

	if (_allow_show && !gtk_widget_get_visible(border))
	{
		gApplication::_disable_mapping_events = true;
		gtk_widget_show(border);
		gApplication::_disable_mapping_events = false;
	}
	
	focus = gApplication::_active_control;
	gApplication::_active_control = NULL;
	if (focus && !focus->hasFocus())
		focus->setFocus();
	gApplication::_active_control = focus;

	//_hidden_temp = false;
}

bool gControl::resize(int w, int h, bool no_decide)
{
	bool decide_w, decide_h;
	
	if (w < 0 && h < 0)
		return true;
	
	if (pr && !no_decide)
	{
		pr->decide(this, &decide_w, &decide_h);

		if (w < 0 || decide_w)
			w = width();

		if (h < 0 || decide_h)
			h = height();
	}

	if (w < 0) w = 0;
	if (h < 0) h = 0;

	if (width() == w && height() == h)
		return true;
	
	bufW = w;
	bufH = h;
	
	if (w < minimumWidth() || h < minimumHeight())
	{
		hideButKeepFocus();
	}
	else
	{
		//g_debug("resize: %p %s: %d %d", this, name(), w, h);
		_dirty_size = true;

		#if GEOMETRY_OPTIMIZATION
		gApplication::setDirty();
		#else
		updateGeometry();
		#endif
		
		if (isVisible() && !isReallyVisible())
		{
			showButKeepFocus();
#ifdef GTK3
			updateStyleSheet(false);
#endif
		}
	}

	checkVisibility();
	
	if (pr && !isIgnore())
		pr->performArrange();

	send_configure(this); // needed for Watcher and Form Resize events
	return false;
}

void gControl::moveResize(int x, int y, int w, int h, bool no_decide)
{
	if (pr)
		pr->disableArrangement();

	move(x, y);
	resize(w, h, no_decide);

	if (pr)
		pr->enableArrangement();
}

void gControl::updateGeometry(bool force)
{
// 	if (_dirty_pos)
// 	{
// 		g_debug("move: %p -> %d %d", this, x(), y());
// 		_dirty_pos = false;
// 		GtkLayout *fx = GTK_LAYOUT(gtk_widget_get_parent(border));
// 		gtk_layout_move(fx, border, x(), y());
// 	}
//
// 	if (_dirty_size)
// 	{
// 		GtkAllocation a = { x(), y(), width(), height() };
// 		g_debug("resize: %p -> %d %d", this, width(), height());
// 		_dirty_size = false;
// 		//gtk_widget_set_size_request(border, width(), height());
// 		gtk_widget_size_allocate(border,
// 	}
	if (force || _dirty_pos || _dirty_size)
	{
		//g_debug("move-resize: %s: %d %d %d %d", this->name(), x(), y(), width(), height());
		if (force || _dirty_pos)
		{
			if (pr)
				pr->moveChild(this, x(), y());

			_dirty_pos = false;
		}
		if ((force || _dirty_size) && isVisible())
		{
			gtk_widget_set_size_request(border, width(), height());
			_dirty_size = false;
		}
	}
}

/*****************************************************************

APPEARANCE

******************************************************************/


void gControl::setExpand(bool vl)
{
	if (vl == _expand)
		return;

	_expand = vl;
	checkVisibility();

	if (pr && !_ignore)
		pr->performArrange();
}

void gControl::setIgnore(bool vl)
{
	if (vl == _ignore)
		return;

	_ignore = vl;
	if (pr)
		pr->performArrange();
}

void gControl::setTooltip(char *vl)
{
	char *pango;

	if (_tooltip) g_free(_tooltip);
	_tooltip = NULL;
	if (vl && *vl) _tooltip = g_strdup(vl);

	if (_tooltip)
	{
		pango = gt_html_to_pango_string(_tooltip, -1, true);
		gtk_widget_set_tooltip_markup(border, pango);
		g_free(pango);
	}
	else
		gtk_widget_set_tooltip_markup(border, NULL);
}

gFont* gControl::font() const
{
	if (_resolved_font)
	{
		//fprintf(stderr, "%s: font -> _resolved_font\n", name());
		return _resolved_font;
	}
	else if (pr)
	{
		//fprintf(stderr, "%s: font -> parent\n", name());
		return pr->font();
	}
	else
	{
		//fprintf(stderr, "%s: font -> desktop\n", name());
		return gDesktop::font();
	}
}

void gControl::actualFontTo(gFont *ft)
{
	font()->copyTo(ft);
}

void gControl::resolveFont()
{
	gFont *font;

	if (_font)
	{
		font = new gFont();
		font->mergeFrom(_font);
		if (pr)
			font->mergeFrom(pr->font());
		else
			font->mergeFrom(gDesktop::font());

		gFont::set(&_resolved_font, font);
	}
	else
		gFont::assign(&_resolved_font);
}

void gControl::setFont(gFont *ft)
{
	//fprintf(stderr, "setFont: %s: %s\n", name(), ft->toFullString());
	
	if (ft)
	{
		if (_font && _font->equals(ft))
			return;
		
		gFont::assign(&_font, ft);
	}
	else
	{
		if (!_font)
			return;
		
		gFont::assign(&_font);
	}

	gFont::assign(&_resolved_font);

	updateFont();

	resize();

	//fprintf(stderr, "--> %s: _font = %s\n", name(), _font ? _font->toFullString() : NULL);
}

#ifdef GTK3

void gControl::updateFont()
{
	resolveFont();
	updateStyleSheet(true);
	updateSize();
}

#else

static void cb_update_font(GtkWidget *widget, gpointer data)
{
	PangoFontDescription *desc = (PangoFontDescription *)data;
	gtk_widget_modify_font(widget, desc);
}

void gControl::updateFont()
{
	resolveFont();
	gtk_widget_modify_font(widget, font()->desc());
	if (!isContainer() && GTK_IS_CONTAINER(widget))
		gtk_container_forall(GTK_CONTAINER(widget), (GtkCallback)cb_update_font, (gpointer)font()->desc());
	refresh();
	updateSize();
}

#endif

void gControl::updateSize()
{
}

int gControl::mouse()
{
	if (_proxy)
		return _proxy->mouse();
	else
		return _mouse;
}

gCursor* gControl::cursor()
{
	if (_proxy)
		return _proxy->cursor();

	if (!curs) return NULL;
	return new gCursor(curs);
}

void gControl::setCursor(gCursor *vl)
{
	if (_proxy)
	{
		_proxy->setCursor(vl);
		return;
	}

	if (curs) { delete curs; curs=NULL;}
	if (!vl)
	{
		setMouse(CURSOR_DEFAULT);
		return;
	}
	curs=new gCursor(vl);
	setMouse(CURSOR_CUSTOM);
}

void gControl::updateCursor(GdkCursor *cursor)
{
	if (GDK_IS_WINDOW(gtk_widget_get_window(border)) && _inside)
	{
		if (cursor || isWindow())
			gdk_window_set_cursor(gtk_widget_get_window(border), cursor);
		
		if (!cursor && parent())
			parent()->updateCursor(parent()->getGdkCursor());
	}
}

GdkCursor *gControl::getGdkCursor()
{
	const char *name;
	GdkCursor *cr = NULL;
	int m = _mouse;

	if (gApplication::isBusy())
		m = CURSOR_WAIT;

	if (m == CURSOR_CUSTOM)
	{
		if (curs && curs->cur)
			return curs->cur;
	}

	if (m != CURSOR_DEFAULT)
	{
		switch(m)
		{
			case CURSOR_NONE: name = "none"; break;
			case CURSOR_ARROW: name = "default"; break;
			case CURSOR_HELP: name = "help"; break;
			case CURSOR_POINTER: name = "pointer"; break;
			case CURSOR_CONTEXT_MENU: name = "context-menu"; break;
			case CURSOR_PROGRESS: name = "progress"; break;
			case CURSOR_WAIT: name = "wait"; break;
			case CURSOR_CELL: name = "cell"; break;
			case CURSOR_CROSSHAIR: name = "crosshair"; break;
			case CURSOR_TEXT: name = "text"; break;
			case CURSOR_VERTICAL_TEXT: name = "vertical-text"; break;
			case CURSOR_ALIAS: name = "alias"; break;
			case CURSOR_COPY: name = "copy"; break;
			case CURSOR_NO_DROP: name = "no-drop"; break;
			case CURSOR_MOVE: name = "move"; break;
			case CURSOR_NOT_ALLOWED: name = "not-allowed"; break;
			case CURSOR_GRAB: name = "grab"; break;
			case CURSOR_GRABBING: name = "grabbing"; break;
			case CURSOR_ALL_SCROLL: name = "all-scroll"; break;
			case CURSOR_COL_RESIZE: name = "col-resize"; break;
			case CURSOR_ROW_RESIZE: name = "row-resize"; break;
			case CURSOR_N_RESIZE: name = "n-resize"; break;
			case CURSOR_E_RESIZE: name = "e-resize"; break;
			case CURSOR_S_RESIZE: name = "s-resize"; break;
			case CURSOR_W_RESIZE: name = "w-resize"; break;
			case CURSOR_NE_RESIZE: name = "ne-resize"; break;
			case CURSOR_NW_RESIZE: name = "nw-resize"; break;
			case CURSOR_SW_RESIZE: name = "sw-resize"; break;
			case CURSOR_SE_RESIZE: name = "se-resize"; break;
			case CURSOR_EW_RESIZE: name = "ew-resize"; break;
			case CURSOR_NS_RESIZE: name = "ns-resize"; break;
			case CURSOR_NESW_RESIZE: name = "nesw-resize"; break;
			case CURSOR_NWSE_RESIZE: name = "nwse-resize"; break;
			case CURSOR_ZOOM_IN: name = "zoom-in"; break;
			case CURSOR_ZOOM_OUT: name = "zoom-out"; break;
			default: name = "default";
		}

		cr = gdk_cursor_new_from_name(gdk_display_get_default(), name);
		if (!cr)
			cr = gdk_cursor_new_for_display(gdk_display_get_default(), (GdkCursorType)m);
	}

	return cr;
}

void gControl::setMouse(int m)
{
	if (_proxy)
	{
		_proxy->setMouse(m);
		return;
	}

	//fprintf(stderr, "setMouse: %s\n", name());
	_mouse = m;
	updateCursor(getGdkCursor());
}



/*****************************************************************

HANDLES

******************************************************************/

gMainWindow* gControl::window() const
{
	if (isWindow())
		return (gMainWindow *)this;

	if (!pr)
		return NULL;
	else
		return pr->window();
}

gMainWindow* gControl::topLevel() const
{
	const gControl *child = this;

	while (!child->isTopLevel())
		child = child->parent();

	return (gMainWindow *)child;
}

long gControl::handle()
{
#ifdef GTK3
	gtk_widget_realize(border);
	GdkWindow *window = gtk_widget_get_window(border);
	return PLATFORM.Window.GetId(window);
#else
	if (MAIN_display_x11)
	{
		GdkWindow *window = gtk_widget_get_window(border);
		return window ? GDK_WINDOW_XID(window) : 0;
	}
	else
		return 0;
#endif
}

/*****************************************************************

MISC

******************************************************************/

void gControl::refresh()
{
	//fprintf(stderr, "%s: refresh %s\n", GB.Debug.GetCurrentPosition(), name());
	gtk_widget_queue_draw(border);
	if (frame != border && GTK_IS_WIDGET(frame))
		gtk_widget_queue_draw(frame);
	if (widget != frame  && GTK_IS_WIDGET(widget))
		gtk_widget_queue_draw(widget);

	afterRefresh();
}

void gControl::refresh(int x, int y, int w, int h)
{
	GtkAllocation a;
	/*GdkRectangle r;
	GtkAllocation a;*/

	//gtk_widget_get_allocation(border, &a);

	if (x < 0 || y < 0 || w <= 0 || h <= 0)
	{
		x = y = 0;
		w = width();
		h = height();
	}
	
	if (w <= 0 || h <= 0)
		return;

	gtk_widget_get_allocation(widget, &a);
	
	gtk_widget_queue_draw_area(widget, x + a.x, y + a.y, w, h);
	
	afterRefresh();
}

void gControl::afterRefresh()
{
}

void gControl::setDesign(bool ignore)
{
	if (_design)
		return;
	
	//fprintf(stderr, "setDesign: %s %d\n", name(), ignore);
	setCanFocus(false);
	setMouse(GDK_LEFT_PTR);
	setTooltip(NULL);
	_design = true;
	_design_ignore = ignore;
}

void gControl::updateDesign()
{
	if (!_design)
		return;
	
	_design = false;
	setDesign(_design_ignore);
}

gControl *gControl::ignoreDesign()
{
	//fprintf(stderr, "ignoreDesign: %s", name());
	
	if (!isDesignIgnore())
		return this;
	
	gControl *ctrl = this;
	while (ctrl && ctrl->isDesignIgnore())
		ctrl = ctrl->parent();

	//fprintf(stderr, " --> %s\n", ctrl->name());
	return ctrl;
}

bool gControl::canFocus() const
{
	/*#if DEBUG_FOCUS
	fprintf(stderr, "canFocus: %s -> %d\n", name(), gtk_widget_get_can_focus(widget));
	#endif*/
	
#if GTK_CHECK_VERSION(2, 18, 0)
	return gtk_widget_get_can_focus(widget);
#else
	return GTK_WIDGET_CAN_FOCUS(widget);
#endif
}

bool gControl::canFocusOnClick() const
{
	/*if (_proxy)
		return _proxy->canFocusOnClick();*/
	if (isWindow())
		return false;
	if (!GTK_IS_BUTTON(widget))
		return true;
	return gt_get_focus_on_click(widget);
}

void gControl::setCanFocus(bool vl)
{
	/*#if DEBUG_FOCUS
	fprintf(stderr, "setCanFocus: %s %d ?\n", name(), vl);
	#endif*/
	if (isDesign() || vl == canFocus())
		return;

	/*if (_proxy)
		_proxy->setCanFocus(vl);
	else*/
	{
		/*#if DEBUG_FOCUS
		fprintf(stderr, "setCanFocus: %s %p %d\n", name(), this, vl);
		#endif*/
		gtk_widget_set_can_focus(widget, vl);
	}

	/*_has_input_method = vl;

	if (_input_method && !vl)
	{
		g_object_unref(_input_method);
		_input_method = NULL;
	}
	else if (!_input_method && vl)
	{
		_input_method = gtk_im_multicontext_new();
	}*/

}

void gControl::setFocus()
{
	#if DEBUG_FOCUS
	fprintf(stderr, "setFocus %s ?\n", name());
	#endif
	
	if (_proxy)
	{
		_proxy->setFocus();
		return;
	}

	if (!canFocus() || hasFocus())
		return;
	
	gMainWindow *win = window();

	if (!win)
		return;

	if (win->isVisible())
	{
		gtk_widget_grab_focus(widget);
		
		#if DEBUG_FOCUS
		fprintf(stderr, "setFocus NOW: %s %s %p -> %d\n", GB.GetClassName(hFree), name(), hFree, hasFocus());
		#endif
	}
	else
	{
		#if DEBUG_FOCUS
		fprintf(stderr, "setFocus later: %s %s %p\n", GB.GetClassName(hFree), name(), hFree);
		#endif
		win->_initial_focus = this;
	}
}

bool gControl::hasFocus() const
{
	if (_proxy)
		return _proxy->hasFocus();
	else
#if GTK_CHECK_VERSION(2, 18, 0)
		return (border && gtk_widget_has_focus(border)) || (widget && gtk_widget_has_focus(widget)) || gApplication::activeControl() == this;
#else
		return (border && GTK_WIDGET_HAS_FOCUS(border)) || (widget && GTK_WIDGET_HAS_FOCUS(widget)) || gApplication::activeControl() == this;
#endif
}

#if GTK_CHECK_VERSION(3, 2, 0)
bool gControl::hasVisibleFocus() const
{
	if (_proxy)
		return _proxy->hasVisibleFocus();
	else
		return (border && gtk_widget_has_visible_focus(border)) || (widget && gtk_widget_has_visible_focus(widget));
}
#endif

gControl* gControl::next()
{
	int index;

	if (!pr)
		return NULL;

	index = pr->childIndex(this);
	if (index < 0 || index >= pr->childCount())
		return NULL;
	else
		return pr->child(index + 1);
}

gControl* gControl::previous()
{
	int index;

	if (!pr)
		return NULL;

	index = pr->childIndex(this);
	if (index <= 0)
		return NULL;
	else
		return pr->child(index - 1);
}

static int find_child_fixed(GtkFixedChild *data, GtkWidget *widget)
{
	return !(data->widget == widget);
}
	
static int find_child_layout(GtkLayoutChild *data, GtkWidget *widget)
{
	return !(data->widget == widget);
}

static GList **get_children_list(GtkContainer *parent)
{
#ifdef GTK3
	if (GTK_IS_LAYOUT(parent))
		return &((GtkLayout *)parent)->priv->children;
	else
		return &((GtkFixed *)parent)->priv->children;
#else
	if (GTK_IS_LAYOUT(parent))
		return &((GtkLayout *)parent)->children;
	else
		return &((GtkFixed *)parent)->children;
#endif
}

void gControl::restack(bool raise)
{
	GtkContainer *parent;
	GList **children;
	GList *find;
	gpointer *p;

	if (!pr) 
		return;

	parent = GTK_CONTAINER(gtk_widget_get_parent(border));
	
	//fprintf(stderr, "%s: %s -> %s (%s%s)\n", raise ? "raise" : "lower", name(), pr->name(), GTK_IS_LAYOUT(parent) ? "L" : "F", gtk_widget_get_has_window(border) ? "W" : "");
	
	children = get_children_list(parent);
	
	if (GTK_IS_LAYOUT(parent))
		find = g_list_find_custom(*children, border, (GCompareFunc)find_child_layout);
	else if (GTK_IS_FIXED(parent))
		find = g_list_find_custom(*children, border, (GCompareFunc)find_child_fixed);
	else
		return;
	
	if (_visible)
		hideButKeepFocus();
	
	*children = g_list_remove_link(*children, find);
	if (raise)
		*children = g_list_concat(*children, find);
	else
		*children = g_list_concat(find, *children);
	
	if (gtk_widget_get_has_window(border))
	{
		if (raise)
			gdk_window_raise(gtk_widget_get_window(border));
		else
			gdk_window_lower(gtk_widget_get_window(border));
	}
	
	g_ptr_array_remove(pr->_children, this);
	
	if (raise)
	{
		g_ptr_array_add(pr->_children, this);
	}
	else
	{
		g_ptr_array_add(pr->_children, NULL);
		p = pr->_children->pdata;
		memmove(&p[1], &p[0], (pr->_children->len - 1) * sizeof(gpointer));
		p[0] = this;
	}
	
	if (_visible)
		showButKeepFocus();

	updateGeometry(true);
	pr->performArrange();
	pr->refresh();
}

void gControl::setNext(gControl *ctrl)
{
	GPtrArray *ch;
	uint i;

	if (!ctrl)
	{
		raise();
		return;
	}

	if (ctrl == this || isTopLevel() || ctrl->parent() != parent())
		return;

	if (gtk_widget_get_has_window(ctrl->border) && gtk_widget_get_has_window(border))
		gdk_window_restack(gtk_widget_get_window(border), gtk_widget_get_window(ctrl->border), FALSE);

	ch = pr->_children;
	g_ptr_array_remove(ch, this);
	g_ptr_array_add(ch, NULL);

	for (i = 0; i < ch->len; i++)
	{
		if (g_ptr_array_index(ch, i) == ctrl)
		{
			memmove(&ch->pdata[i + 1], &ch->pdata[i], (ch->len - i - 1) * sizeof(gpointer));
			ch->pdata[i] = this;
			break;
		}
	}

	pr->performArrange();
}

void gControl::setPrevious(gControl *ctrl)
{
	if (!ctrl)
		lower();
	else
		setNext(ctrl->next());
}

/*********************************************************************

Drag & Drop

**********************************************************************/

/*static void print_full_name(gControl *ctrl)
{
	if (ctrl->parent())
	{
		print_full_name(ctrl->parent());
		fprintf(stderr, ">");
	}

	fprintf(stderr, "%s", ctrl->name() ? ctrl->name() : "?");
}*/

void gControl::updateAcceptDrops()
{
	GtkWidget *w = _scroll ? widget : border;

	if (_accept_drops)
	{
		gtk_drag_dest_set(w, (GtkDestDefaults)0, NULL, 0, (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK));
		gtk_drag_dest_set_track_motion(w, TRUE);
	}
	else
	{
		gtk_drag_dest_unset(w);
	}

	/*fprintf(stderr, "updateAcceptDrops: ");
	print_full_name(this);
	fprintf(stderr, " -> %d\n", _accept_drops);*/
}

bool gControl::acceptDrops() const
{
	if (_proxy)
		return _proxy->acceptDrops();
	else
		return _accept_drops;
}

void gControl::setAcceptDrops(bool vl)
{
	if (_proxy)
	{
		_proxy->setAcceptDrops(vl);
		return;
	}
	
	_accept_drops = vl;
	updateAcceptDrops();
}

/*********************************************************************

Internal

**********************************************************************/

void gControl::connectParent()
{
	if (pr)
		pr->insert(this, true);

	// BM: Widget has been created, so we can set its cursor if application is busy
	if (gApplication::isBusy() && mustUpdateCursor())
		setMouse(mouse());
}

gColor gControl::getFrameColor()
{
	return gDesktop::getColor(gDesktop::LIGHT_FOREGROUND);
}

#ifdef GTK3
void gControl::drawBorder(cairo_t *cr)
{
	/*if (getFrameBorder() != BORDER_NONE)
		fprintf(stderr, "gControl::drawBorder: %s: %d %d\n", name(), width(), height());*/
	gt_draw_border(cr, gtk_widget_get_style_context(widget), gtk_widget_get_state_flags(widget), getFrameBorder(), getFrameColor(), 0, 0, width(), height(), use_base);
}
#else
void gControl::drawBorder(GdkEventExpose *e)
{
	GdkWindow *win;
	GtkShadowType shadow;
	gint x, y, w, h;
	cairo_t *cr;
	GtkWidget *wid;
	GtkAllocation a;

	if (getFrameBorder() == BORDER_NONE)
		return;

	x = 0;
	y = 0;
	w = width();
	h = height();

	if (frame)
		wid = frame;
	else
		wid = widget;

	if (GTK_IS_LAYOUT(wid))
		win = gtk_layout_get_bin_window(GTK_LAYOUT(wid));
	else
		win = gtk_widget_get_window(wid);

	gtk_widget_get_allocation(wid, &a);
	x = a.x;
	y = a.y;

	if (w < 1 || h < 1)
		return;

	switch (getFrameBorder())
	{
		case BORDER_PLAIN:

			cr = gdk_cairo_create(win);
			gt_cairo_draw_rect(cr, x, y, w, h, getFrameColor());
			cairo_destroy(cr);
			return;

		case BORDER_SUNKEN: shadow = GTK_SHADOW_IN; break;
		case BORDER_RAISED: shadow = GTK_SHADOW_OUT; break;
		case BORDER_ETCHED: shadow = GTK_SHADOW_ETCHED_IN; break;

		default:
			return;
	}

	GdkRectangle clip;
	gdk_region_get_clipbox(e->region, &clip);
	GtkStyle *st = gtk_widget_get_style(widget);
	if (use_base)
		gtk_paint_box(st, win, GTK_STATE_NORMAL, shadow, &clip, widget, "entry", x, y, w, h);
	else
		gtk_paint_shadow(st, win, GTK_STATE_NORMAL, shadow, &clip, widget, NULL, x, y, w, h);
}
#endif

/*static void cb_size_allocate(GtkWidget *wid, GtkAllocation *a, gContainer *container)
{
	if (!container->isTopLevel())
		container->performArrange();
}*/


/*
	The different cases managed by gControl::realize()

	border     frame      widget
		0          0          W
		B          0          W
		0          F          W
		B          F          W
*/

static void add_container(GtkWidget *parent, GtkWidget *child)
{
	GtkWidget *ch;

	for(;;)
	{
		if (!GTK_IS_BIN(parent))
			break;

		ch = gtk_bin_get_child(GTK_BIN(parent));
		if (!ch)
			break;

		parent = ch;
	}

	gtk_container_add(GTK_CONTAINER(parent), child);
}

void gControl::registerControl()
{
	gt_register_control(border, this);
}

#ifndef GTK3

static gboolean cb_clip_children(GtkWidget *wid, GdkEventExpose *e, gContainer *d)
{
	GdkRegion *me;
	GtkAllocation a;

	gtk_widget_get_allocation(wid, &a);
	me = gdk_region_rectangle((GdkRectangle *)&a);

	gdk_region_intersect(e->region, me);

	gdk_region_destroy(me);

	if (gdk_region_empty(e->region))
		return TRUE;

	return FALSE;
}

#endif

#if 0
static gboolean cb_clip_by_parent(GtkWidget *wid, GdkEventExpose *e, gControl *d)
{
	GdkRegion *preg;
	GdkRectangle prect = { 0, 0, d->parent()->width() - d->x(), d->parent()->height() - d->y() };

	fprintf(stderr, "area = %d %d %d %d  prect = %d %d %d %d\n",
					e->area.x, e->area.y, e->area.width, e->area.height,
					prect.x, prect.y, prect.width, prect.height);

	preg = gdk_region_rectangle(&prect);

	gdk_region_intersect(e->region, preg);

	gdk_region_destroy(preg);

	if (gdk_region_empty(e->region))
		return TRUE;

	gdk_region_get_clipbox(e->region, &prect);
	e->area = prect;
	fprintf(stderr, "--> %d %d %d %d\n", prect.x, prect.y, prect.width, prect.height);

	return FALSE;
}
#endif

#ifdef GTK3

//fprintf(stderr, "get_preferred_width [%p %s] %p\n", klass, G_OBJECT_TYPE_NAME(widget), klass->_gtk_reserved2);
//fprintf(stderr, "get_preferred_height [%p %s] %p\n", klass, G_OBJECT_TYPE_NAME(widget), klass->_gtk_reserved3);

//#define must_patch(_widget) (gt_get_control(_widget) != NULL)

static bool _do_not_patch = false;

static bool must_patch(GtkWidget *widget)
{
	GtkWidget *parent;
	gControl *parent_control;

	if (_do_not_patch)
		return false;

	if (gt_get_control(widget))
	{
		//fprintf(stderr, "must_patch: %p -> 1\n", widget);
		return true;
	}

	parent = gtk_widget_get_parent(widget);
	if (!parent)
	{
		//fprintf(stderr, "must_patch: %p -> 0\n", widget);
		return false;
	}
	
	if (GTK_IS_NOTEBOOK(parent) && GTK_IS_FIXED(widget))
		return true;
	
	if (GTK_IS_SCROLLED_WINDOW(parent))
	{
		parent = gtk_widget_get_parent(parent);
		if (!parent)
			return false;
	}
	
	if (GTK_IS_ENTRY(widget))
	{
		parent = gtk_widget_get_parent(parent);
		if (GTK_IS_COMBO_BOX(parent))
			return true;
	}

	parent_control = gt_get_control(parent);
	if (!parent_control)
		return false;

	return (parent_control->widget == widget || (GtkWidget *)parent_control->_scroll == widget);
}

static gboolean draw_container(GtkWidget *widget, cairo_t *cr)
{
	GList *children;
	GtkFixedChild *child;
	GtkAllocation a;

	gtk_widget_get_allocation(widget, &a);
	
	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, a.width, a.height);
	cairo_clip(cr);	
	
  for (children = *get_children_list(GTK_CONTAINER(widget)); children; children = children->next)
	{
		child = (GtkFixedChild *)children->data;
		cairo_save(cr);
		gtk_container_propagate_draw (GTK_CONTAINER(widget), child->widget, cr);
		cairo_restore(cr);
	}
  
  cairo_restore(cr);
  return FALSE;
}

#include "gb.gtk.patch.h"

PATCH_DECLARE(GTK_TYPE_WINDOW)
PATCH_DECLARE(GTK_TYPE_ENTRY)
PATCH_DECLARE(GTK_TYPE_COMBO_BOX)
PATCH_DECLARE(GTK_TYPE_SPIN_BUTTON)
PATCH_DECLARE(GTK_TYPE_BUTTON)
PATCH_DECLARE(GTK_TYPE_FIXED)
PATCH_DECLARE(GTK_TYPE_EVENT_BOX)
PATCH_DECLARE(GTK_TYPE_BOX)
PATCH_DECLARE(GTK_TYPE_TOGGLE_BUTTON)
PATCH_DECLARE(GTK_TYPE_SCROLLED_WINDOW)
PATCH_DECLARE(GTK_TYPE_CHECK_BUTTON)
PATCH_DECLARE(GTK_TYPE_RADIO_BUTTON)
PATCH_DECLARE(GTK_TYPE_NOTEBOOK)
PATCH_DECLARE(GTK_TYPE_TEXT_VIEW)
PATCH_DECLARE(GTK_TYPE_SCROLLBAR)
PATCH_DECLARE(GTK_TYPE_SCALE)

#if GTK_CHECK_VERSION(3,10,0)
PATCH_DECLARE_BASELINE(GTK_TYPE_ENTRY)
PATCH_DECLARE_BASELINE(GTK_TYPE_COMBO_BOX)
PATCH_DECLARE_BASELINE(GTK_TYPE_SPIN_BUTTON)
PATCH_DECLARE_BASELINE(GTK_TYPE_BUTTON)
#endif

void gt_patch_control(GtkWidget *widget)
{
	PATCH_CLASS(widget, GTK_TYPE_WINDOW)
	else PATCH_CLASS_BASELINE(widget, GTK_TYPE_ENTRY)
	else PATCH_CLASS_BASELINE(widget, GTK_TYPE_SPIN_BUTTON)
	else PATCH_CLASS_BASELINE(widget, GTK_TYPE_BUTTON)
	else PATCH_CLASS(widget, GTK_TYPE_FIXED)
	else PATCH_CLASS(widget, GTK_TYPE_EVENT_BOX)
	else PATCH_CLASS(widget, GTK_TYPE_BOX)
	else PATCH_CLASS(widget, GTK_TYPE_TOGGLE_BUTTON)
	else PATCH_CLASS(widget, GTK_TYPE_SCROLLED_WINDOW)
	else PATCH_CLASS(widget, GTK_TYPE_CHECK_BUTTON)
	else PATCH_CLASS(widget, GTK_TYPE_RADIO_BUTTON)
	else PATCH_CLASS(widget, GTK_TYPE_NOTEBOOK)
	else PATCH_CLASS(widget, GTK_TYPE_TEXT_VIEW)
	else PATCH_CLASS(widget, GTK_TYPE_SCROLLBAR)
	else PATCH_CLASS(widget, GTK_TYPE_SCALE)
	else PATCH_CLASS_BASELINE(widget, GTK_TYPE_COMBO_BOX)
	else PATCH_CLASS(widget, GTK_TYPE_TEXT_VIEW)
		
	PATCH_CLASS_FIXED(widget, GTK_TYPE_FIXED)
}

#endif

void gControl::setMinimumSize()
{
	#ifdef GTK3

	if (isContainer())
	{
		_min_w = _min_h = 1;
	}
	else
	{
		GtkRequisition minimum_size, natural_size;
		bool mapped = gtk_widget_get_mapped(border);
		
		if (!mapped)
		{
			gApplication::_disable_mapping_events = true;
			gtk_widget_show(border);
		}
			
		_do_not_patch = true;
		gtk_widget_get_preferred_size(widget, &minimum_size, &natural_size);
		_do_not_patch = false;
		
		if (!mapped)
		{
			gtk_widget_hide(border);
			gApplication::_disable_mapping_events = false;
		}
		
		//fprintf(stderr, "gtk_widget_get_preferred_size: %s: min = %d %d / nat = %d %d\n", GB.GetClassName(hFree), minimum_size.width, minimum_size.height, natural_size.width, natural_size.height);

		_min_w = minimum_size.width;
		_min_h = minimum_size.height;
	}
	
	#else
	
	_min_w = _min_h = 1;
		
	#endif
}


void gControl::updateEventMask()
{
	gtk_widget_add_events(widget, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
		| GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK
		| GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

	if (widget != border && (GTK_IS_WINDOW(border) || (GTK_IS_EVENT_BOX(border) && !gtk_event_box_get_visible_window(GTK_EVENT_BOX(border)))))
	{
		gtk_widget_add_events(border, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK
			| GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
	}
}

void gControl::realize(bool draw_frame)
{
	if (!_scroll)
	{
		if (!border)
			border = widget;

		if (frame)
		{
			if (border != frame && border != widget)
				add_container(border, frame);
			if (frame != widget)
				add_container(frame, widget);
		}
		else if (border != widget)
			add_container(border, widget);
	}

#ifdef GTK3
	gt_patch_control(border);
	if (widget && widget != border)
		gt_patch_control(widget);
#endif

	initSignals();
	connectParent();
	
	setMinimumSize();
	resize(Max(8, _min_w), Max(8, _min_h), true);
	
	if (!_no_background && !gtk_widget_get_has_window(border))
		ON_DRAW_BEFORE(border, this, cb_background_expose, cb_background_draw);

	if (draw_frame && frame)
		ON_DRAW_BEFORE(frame, this, cb_frame_expose, cb_frame_draw);
	
#ifndef GTK3
	if (isContainer() && !gtk_widget_get_has_window(widget))
		g_signal_connect(G_OBJECT(widget), "expose-event", G_CALLBACK(cb_clip_children), (gpointer)this);
#endif

	updateEventMask();
	registerControl();
	updateFont();
}

void gControl::realizeScrolledWindow(GtkWidget *wid, bool doNotRealize)
{
	_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));

#ifdef GTK3
	PATCH_CLASS(_scroll, GTK_TYPE_SCROLLED_WINDOW)
	PATCH_CLASS(wid, GTK_TYPE_TEXT_VIEW)
#endif

#ifdef GTK3
		border = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_hexpand(wid, TRUE);
#else
		border = gtk_alignment_new(0, 0, 1, 1);
#endif
	gtk_widget_set_redraw_on_allocate(border, TRUE);
	widget = wid;
	frame = border;
	_no_auto_grab = true;

	//gtk_container_add(GTK_CONTAINER(border), GTK_WIDGET(_scroll));
	gtk_scrolled_window_set_policy(_scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(_scroll, GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(border), GTK_WIDGET(_scroll));
	gtk_container_add(GTK_CONTAINER(_scroll), widget);

	if (!doNotRealize)
		realize(true);
	else
		registerControl();

	updateFont();
	
	gtk_widget_show_all(border);
}

void gControl::updateBorder()
{
	int pad;

	if (!frame)
		return;

#if GTK3
	if (!GTK_IS_BOX(frame))
#else
	if (!GTK_IS_ALIGNMENT(frame))
#endif
	{
		refresh();
		return;
	}

	switch (frame_border)
	{
		case BORDER_NONE: pad = 0; break;
		case BORDER_PLAIN: pad = 1; break;
		default: pad = gApplication::getFrameWidth(); break;
	}

	if ((int)frame_padding > pad)
		pad = frame_padding;

#if GTK3
	g_object_set(widget, "margin", pad, NULL);
#else
	gtk_alignment_set_padding(GTK_ALIGNMENT(frame), pad, pad, pad, pad);
	refresh();
#endif
	//gtk_widget_queue_draw(frame);
}

int gControl::getFrameWidth() const
{
	guint p;

	if (frame)
	{
#if GTK3
		if (GTK_IS_BOX(frame))
		{
			g_object_get(widget, "margin", &p, NULL);
			return p;
		}
#else
		if (GTK_IS_ALIGNMENT(frame))
		{
			gtk_alignment_get_padding(GTK_ALIGNMENT(frame), &p, NULL, NULL, NULL);
			return p;
		}
#endif
	}

	/*if (_scroll)
	{
		if (gtk_scrolled_window_get_shadow_type(_scroll) == GTK_SHADOW_NONE)
			return 0;
		else
			return gApplication::getFrameWidth();
	}*/

	switch (frame_border)
	{
		case BORDER_NONE: p = 0; break;
		case BORDER_PLAIN: p = 1; break;
		default: p = gApplication::getFrameWidth(); break;
	}
	return p;
}

void gControl::setFrameBorder(int border)
{
	if (border < BORDER_NONE || border > BORDER_ETCHED)
		return;

	frame_border = border;
	updateBorder();
}

bool gControl::hasBorder() const
{
	return getFrameBorder() != BORDER_NONE;
}

void gControl::setBorder(bool vl)
{
	setFrameBorder(vl ? BORDER_SUNKEN : BORDER_NONE);
	_has_border = vl;
}


void gControl::setFramePadding(int padding)
{
	if (padding < 0)
		padding = 0;
	frame_padding = padding;
	updateBorder();
}


void gControl::setName(char *name)
{
	if (_name) g_free(_name);
	_name = NULL;
	if (name) _name = g_strdup(name);
}

gColor gControl::defaultBackground() const
{
	return gDesktop::getColor(gDesktop::BACKGROUND, !isEnabled());
}

#ifdef GTK3

GtkWidget *gControl::getStyleSheetWidget()
{
	return border;
}

const char *gControl::getStyleSheetColorNode()
{
	return "";
}

const char *gControl::getStyleSheetFontNode()
{
	return "";
}

void gControl::customStyleSheet(GString *css)
{
}

void gControl::setStyleSheetNode(GString *css, const char *node)
{
	if (node == _css_node)
		return;
	
	if (node && _css_node && !::strcmp(node, _css_node))
		return;
	
	if (_css_node)
		g_string_append(css, "}\n");
	
	_css_node = node;
	
	if (!node)
		return;
	
	if (!_has_css_id)
	{
		gt_widget_set_name(getStyleSheetWidget(), name());
		_has_css_id = true;
	}

	g_string_append_printf(css, "#%s %s {\ntransition:none;\n", gtk_widget_get_name(getStyleSheetWidget()), node);
}

void gControl::updateStyleSheet(bool dirty)
{
	GString *css;
	gColor bg, fg;
	
	if (dirty)
		_style_dirty = true;

	if (isContainer())
	{
		gContainer *cont = (gContainer *)this;
		
		if (_no_style_without_child && cont->childCount() == 0)
			return;

		if (!dirty)
		{
			for (int i = 0; i < cont->childCount(); i++)
				cont->child(i)->updateStyleSheet(false);
		}
	}
	
	if (!isReallyVisible() || !_style_dirty)
		return;

	bg = _no_background ? background() : COLOR_DEFAULT;
	fg = foreground(); //realForeground();

	css = g_string_new(NULL);
	_css_node = NULL;
	
	if (bg != COLOR_DEFAULT || fg != COLOR_DEFAULT)
	{
		setStyleSheetNode(css, getStyleSheetColorNode());
		gt_css_add_color(css, bg, fg);
	}
	
	if (_resolved_font)
	{
		setStyleSheetNode(css, getStyleSheetFontNode());
		gt_css_add_font(css, _resolved_font);
	}

	customStyleSheet(css);

	setStyleSheetNode(css, NULL);
	
	gt_define_style_sheet(&_css, css);
	
	/*if (_css)
	{
		char *css_str = gtk_css_provider_to_string(GTK_CSS_PROVIDER(_css));
		fprintf(stderr, "---- %s\n%s", gtk_widget_get_name(getStyleSheetWidget()), css_str);
		g_free(css_str);
	}*/
	
	_style_dirty = false;
}

gColor gControl::realBackground(bool no_default)
{
	if (_bg != COLOR_DEFAULT)
		return _bg;
	
	if (!no_default)
		return COLOR_DEFAULT;
	
	if (!use_base && parent())
		return parent()->realBackground(true);
	
	return defaultBackground();
}

void gControl::setRealBackground(gColor color)
{
}

void gControl::setBackground(gColor color)
{
	if (_bg == color)
		return;
	
	_bg = color;
	updateStyleSheet(true);
	updateColor();
}

gColor gControl::realForeground(bool no_default)
{
	if (_fg != COLOR_DEFAULT)
		return _fg;
	else if (pr)
		return pr->realForeground(no_default);
	else
		return no_default ? gDesktop::getColor(gDesktop::FOREGROUND) : COLOR_DEFAULT;
}

void gControl::setRealForeground(gColor color)
{
}

void gControl::setForeground(gColor color)
{
	if (_fg == color)
		return;
	
	_fg = color;
	_fg_set = color != COLOR_DEFAULT;
#ifdef GTK3
	updateStyleSheet(true);
#endif
	//gt_widget_set_color(border, TRUE, _fg, _fg_name, &_fg_default);
	updateColor();
	/*if (::strcmp(name(), "dwgInfo") == 0)
		fprintf(stderr, "setForeground: %08X\n", _fg);*/
}

#else

gColor gControl::realBackground(bool no_default)
{
	if (_bg_set)
		return use_base ? get_gdk_base_color(widget, isEnabled()) : get_gdk_bg_color(widget, isEnabled());
	else
		return no_default ? defaultBackground() : COLOR_DEFAULT;
}

static void set_background(GtkWidget *widget, gColor color, bool use_base)
{
	if (use_base)
		set_gdk_base_color(widget, color);
	else
		set_gdk_bg_color(widget, color);
}

void gControl::setRealBackground(gColor color)
{
	set_background(border, color, use_base);
	if (border != frame && GTK_IS_WIDGET(frame))
		set_background(frame, color, use_base);
	if (frame != widget)
		set_background(widget, color, use_base);
}

void gControl::setBackground(gColor color)
{
	_bg = color;
	_bg_set = color != COLOR_DEFAULT;

	if (!_bg_set)
	{
		if (pr && !use_base)
			color = pr->realBackground();
	}

	setRealBackground(color);
}

gColor gControl::realForeground(bool no_default)
{
	if (_fg_set)
		return use_base ? get_gdk_text_color(widget, isEnabled()) : get_gdk_fg_color(widget, isEnabled());
	else if (pr)
		return pr->realForeground(no_default);
	else
		return no_default ? gDesktop::getColor(gDesktop::FOREGROUND) : COLOR_DEFAULT;
}

static void set_foreground(GtkWidget *widget, gColor color, bool use_base)
{
	if (use_base)
		set_gdk_text_color(widget, color);
	else
		set_gdk_fg_color(widget, color);
}

void gControl::setRealForeground(gColor color)
{
	set_foreground(widget, color, use_base);
}

void gControl::setForeground(gColor color)
{
	_fg = color;
	_fg_set = color != COLOR_DEFAULT;

	if (!_fg_set)
	{
		if (pr)
			color = pr->realForeground();
	}

	setRealForeground(color);
}

#endif

void gControl::emit(void *signal)
{
	if (!signal || locked())
		return;
	(*((void (*)(gControl *))signal))(this);
}

void gControl::emit(void *signal, intptr_t arg)
{
	if (!signal || locked())
		return;
	(*((void (*)(gControl *, intptr_t))signal))(this, arg);
}

void gControl::reparent(gContainer *newpr, int x, int y)
{
	gContainer *oldpr;
	bool was_visible = isVisible();

	// newpr can be equal to pr: for example, to move a control for one
	// tab to another tab of the same TabStrip!

	if (!newpr || !newpr->getContainer())
		return;

	if (pr == newpr && gtk_widget_get_parent(border) == newpr->getContainer())
	{
		move(x, y);
		return;
	}

	if (was_visible) hide();
	//gtk_widget_unrealize(border);

	oldpr = pr;
	pr = newpr;

	if (oldpr == newpr)
	{
		gt_widget_reparent(border, newpr->getContainer());
		oldpr->performArrange();
	}
	else
	{
		if (oldpr)
		{
			gt_widget_reparent(border, newpr->getContainer());
			oldpr->remove(this);
			oldpr->performArrange();
		}

		newpr->insert(this);
	}

	//gtk_widget_realize(border);
	bufX = !x;
	move(x, y);
	if (was_visible)
	{
		//fprintf(stderr, "was_visible\n");
		show();
	}
}

int gControl::scrollX()
{
	if (!_scroll)
		return 0;

	return (int)gtk_adjustment_get_value(gtk_scrolled_window_get_hadjustment(_scroll));
}

int gControl::scrollY()
{
	if (!_scroll)
		return 0;

	return (int)gtk_adjustment_get_value(gtk_scrolled_window_get_vadjustment(_scroll));
}

void gControl::setScrollX(int vl)
{
	GtkAdjustment* adj;
	int max;

	if (!_scroll)
		return;

	adj = gtk_scrolled_window_get_hadjustment(_scroll);

	max = (int)(gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));

	if (vl < 0)
		vl = 0;
	else if (vl > max)
		vl = max;

	gtk_adjustment_set_value(adj, (gdouble)vl);
}

void gControl::setScrollY(int vl)
{
	GtkAdjustment* adj;
	int max;

	if (!_scroll)
		return;

	adj = gtk_scrolled_window_get_vadjustment(_scroll);

	max = (int)(gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));

	if (vl < 0)
		vl = 0;
	else if (vl > max)
		vl = max;

	gtk_adjustment_set_value(adj, (gdouble)vl);
}

void gControl::scroll(int x, int y)
{
	setScrollX(x);
	setScrollY(y);
}

/*int gControl::scrollWidth()
{
	return widget->requisition.width;
}

int gControl::scrollHeight()
{
	return widget->requisition.height;
}*/

void gControl::setScrollBar(int vl)
{
	if (!_scroll)
		return;

	_scrollbar = vl & 3;
	updateScrollBar();
}

void gControl::updateScrollBar()
{
	if (!_scroll)
		return;

	switch(_scrollbar)
	{
		case SCROLL_NONE:
			gtk_scrolled_window_set_policy(_scroll, GTK_POLICY_NEVER, GTK_POLICY_NEVER);
			break;
		case SCROLL_HORIZONTAL:
			gtk_scrolled_window_set_policy(_scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
			break;
		case SCROLL_VERTICAL:
			gtk_scrolled_window_set_policy(_scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
			break;
		case SCROLL_BOTH:
			gtk_scrolled_window_set_policy(_scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
			break;
	}
}

bool gControl::isTracking() const
{
	if (_proxy)
		return _proxy->isTracking();
	else
		return _tracking;
}

void gControl::setTracking(bool v)
{
	if (_proxy)
		_proxy->setTracking(v);
	else
		_tracking = v;
	/*
	GtkWidget *wid;

	if (GTK_IS_EVENT_BOX(border))
		wid = border;
	else
		wid = widget;

	if (v != _tracking)
	{
		uint event_mask = gtk_widget_get_events(wid);
		_tracking = v;
		if (v)
		{
			_old_tracking = event_mask & GDK_POINTER_MOTION_MASK;
			event_mask |= GDK_POINTER_MOTION_MASK;
		}
		else
		{
			event_mask &= ~GDK_POINTER_MOTION_MASK;
		}

		if (!_old_tracking)
		{
			gtk_widget_unrealize(wid);
			gtk_widget_set_events(wid, event_mask);
			gtk_widget_realize(wid);
		}
	}
	*/
}

bool gControl::grab()
{
	gControl *old_control_grab;
	bool save_tracking;

	if (_grab)
		return false;

	if (gt_grab(border, FALSE, gApplication::lastEventTime()))
		return true;

	_grab = true;
	save_tracking = _tracking;
	_tracking = true;

	old_control_grab = gApplication::_control_grab;
	gApplication::_control_grab = this;

	gApplication::enterLoop(this);

	gApplication::_control_grab = old_control_grab;

	gt_ungrab();

	_tracking = save_tracking;
	_grab = false;
	return false;
}

bool gControl::hovered()
{
	//int x, y, xm, ym;

	if (!isVisible())
		return false;
	else
		return _inside;

	/*getScreenPos(&x, &y);
	gMouse::getScreenPos(&xm, &ym);

	return (xm >= x && ym >= y && xm < (x + width()) && ym < (y + height()));*/
}

bool gControl::setProxy(gControl *proxy)
{
	gControl *check = proxy;

	while (check)
	{
		if (check == this)
			return true;

		check = check->_proxy;
	}
	
	if (proxy == _proxy)
		return false;

	//fprintf(stderr, "proxy: (%p %s) -> (%p %s)\n", this, name(), proxy, proxy ? proxy->name() : "NULL");
	
	if (proxy && proxy->_proxy_for)
		proxy->_proxy_for->_proxy = NULL;

	if (_proxy)
		_proxy->_proxy_for = NULL;

	_proxy = proxy;

	if (_proxy)
		_proxy->_proxy_for = this;

	return false;
}

void gControl::setNoTabFocus(bool v)
{
	if (_proxy)
	{
		_proxy->setNoTabFocus(v);
		return;
	}
	
	_no_tab_focus = v;
}

bool gControl::isNoTabFocus() const
{
	if (_proxy)
		return _proxy->isNoTabFocus();
	else
		return _no_tab_focus;
}

bool gControl::isNoTabFocusRec() const
{
	return isNoTabFocus() || (parent() && parent()->isNoTabFocusRec());
}

#ifdef GTK3
void gControl::onEnterEvent()
{
}

void gControl::onLeaveEvent()
{
}
#endif

void gControl::emitEnterEvent(bool no_leave)
{
	gContainer *cont;
	
	#if DEBUG_ENTER_LEAVE
	fprintf(stderr, "========== START ENTER %s (%d)\n", name(), no_leave);
	#endif

	if (parent())
		parent()->emitEnterEvent(true);

	if (!no_leave && isContainer())
	{
		cont = (gContainer *)this;
		int i;

		for (i = 0; i < cont->childCount(); i++)
			cont->child(i)->emitLeaveEvent();
	}

	gApplication::_enter = this;

	if (gApplication::_leave)
	{
		if (gApplication::_leave == this || gApplication::_leave->isAncestorOf(this))
			gApplication::_leave = NULL;
	}

	if (_inside)
		return;
	
	_inside = true;
	
	#ifdef GTK3
	onEnterEvent();
	#endif

	if (!no_leave)
		setMouse(mouse());

	#if DEBUG_ENTER_LEAVE
	fprintf(stderr, ">>>>>>>>>> END ENTER %s\n", name());
	#endif

	if (gApplication::_ignore_until_next_enter)
	{
		#if DEBUG_ENTER_LEAVE
		fprintf(stderr, "ignore next enter for %s\n", name());
		#endif
		if (gApplication::_ignore_until_next_enter == this)
			gApplication::_ignore_until_next_enter = NULL;
		return;
	}

	//fprintf(stderr, "RAISE ENTER: %s\n", name());
	CB_control_enter_leave(this, gEvent_Enter);
}

void gControl::emitLeaveEvent()
{
	if (gApplication::_enter == this)
		gApplication::_enter = NULL;

	if (!_inside)
		return;

	#if DEBUG_ENTER_LEAVE
	fprintf(stderr, "========== START LEAVE %s\n", name());
	#endif

	if (isContainer())
	{
		gContainer *cont = (gContainer *)this;
		int i;

		for (i = 0; i < cont->childCount(); i++)
			cont->child(i)->emitLeaveEvent();
	}

	_inside = false;
	
	#ifdef GTK3
	onLeaveEvent();
	#endif

	#if DEBUG_ENTER_LEAVE
	fprintf(stderr, ">>>>>>>>>> END LEAVE %s\n", name());
	#endif

	if (parent()) parent()->setMouse(parent()->mouse());

	if (gApplication::_ignore_until_next_enter)
	{
		#if DEBUG_ENTER_LEAVE
		fprintf(stderr, "ignore next leave for %s\n", name());
		#endif
		return;
	}

	//fprintf(stderr, "RAISE LEAVE: %s\n", name());
	CB_control_enter_leave(this, gEvent_Leave);
}

bool gControl::isAncestorOf(gControl *child)
{
	if (!isContainer())
		return false;

	for(;;)
	{
		child = child->parent();
		if (!child)
			return false;
		else if (child == this)
			return true;
	}
}

#ifdef GTK3
void gControl::drawBackground(cairo_t *cr)
{
	gColor col= background();
	
	if (col == COLOR_DEFAULT)
	{
		if (!gtk_widget_get_has_window(border))
			return;
		col = realBackground(true);
	}

	gt_cairo_set_source_color(cr, col);
	cairo_rectangle(cr, 0, 0, width(), height());
	cairo_fill(cr);
}
#else
void gControl::drawBackground(GdkEventExpose *e)
{
	GtkAllocation a;

	if (background() == COLOR_DEFAULT)
		return;

	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(border));

	gdk_cairo_region(cr, e->region);
	cairo_clip(cr);
	gt_cairo_set_source_color(cr, background());

	gtk_widget_get_allocation(border, &a);
	cairo_rectangle(cr, a.x, a.y, width(), height());
	cairo_fill(cr);

	cairo_destroy(cr);
}
#endif

#ifdef GTK3
void gControl::updateColor()
{
}

/*void gControl::setColorNames(const char *bg_names[], const char *fg_names[])
{
	_bg_name_list = bg_names;
	_fg_name_list = fg_names;

	if (!bg_names)
	{
		_bg_name = NULL;
		_fg_name = NULL;
		use_base = FALSE;
		return;
	}

	gt_style_lookup_color(gtk_widget_get_style_context(widget), bg_names, &_bg_name, &_bg_default);
	gt_style_lookup_color(gtk_widget_get_style_context(widget), fg_names, &_fg_name, &_fg_default);
}

void gControl::setColorBase()
{
	static const char *bg_names[] = { "base_color", "theme_base_color", NULL };
	static const char *fg_names[] = { "text_color", "theme_text_color", NULL };
	setColorNames(bg_names, fg_names);
	use_base = TRUE;
}

void gControl::setColorButton()
{
	const char *bg_names[] = { "button_bg_color", "theme_button_bg_color", "theme_bg_color", NULL };
	const char *fg_names[] = { "button_fg_color", "theme_button_fg_color", "theme_fg_color", NULL };
	setColorNames(bg_names, fg_names);
	use_base = FALSE;
}*/
#endif

GtkIMContext *gControl::getInputMethod()
{
	return _input_method;
}

gControl *gControl::nextFocus()
{
	gControl *ctrl;
	gControl *next_ctrl;
	
	ctrl = this;
	
	if (ctrl->isContainer() && ((gContainer *)ctrl)->childCount())
		return ((gContainer *)ctrl)->firstChild();
	
	for(;;)
	{
		next_ctrl = ctrl->next();
		if (next_ctrl)
			return next_ctrl;
		if (ctrl->isTopLevel())
			return ctrl;
		ctrl = ctrl->parent();
	}
}

gControl *gControl::previousFocus()
{
	gControl *ctrl;
	gControl *next_ctrl;
	
	ctrl = this;
	
	if (ctrl->isContainer() && ((gContainer *)ctrl)->childCount())
		return ((gContainer *)ctrl)->lastChild();
	
	for(;;)
	{
		next_ctrl = ctrl->previous();
		if (next_ctrl)
			return next_ctrl;
		if (ctrl->isTopLevel())
			return ctrl;
		ctrl = ctrl->parent();
	}
}

void gControl::createWidget()
{
#ifdef GTK3
	if (_css)
	{
		g_object_unref(_css);
		_css = NULL;
	}
#endif
}

void gControl::createBorder(GtkWidget *new_border, bool keep_widget)
{
	GtkWidget *old = border;
	
	border = new_border;
	
	if (keep_widget && widget)
		gt_widget_reparent(widget, border);
	
	if (old)
	{
		_no_delete = true;
		gtk_widget_destroy(old);
		_no_delete = false;
		createWidget();
	}

	updateAcceptDrops();
}

int gControl::actualDirection() const
{
	const gControl *ctrl = this;
	
	for(;;)
	{
		if (ctrl->direction() != DIRECTION_DEFAULT)
			return ctrl->direction();
		if (ctrl->isWindow())
			return DIRECTION_DEFAULT;
		ctrl = ctrl->parent();
	}
}

void gControl::updateDirection()
{
	GtkTextDirection dir;
	
	switch(actualDirection())
	{
		case DIRECTION_LTR: dir = GTK_TEXT_DIR_LTR; break;
		case DIRECTION_RTL: dir = GTK_TEXT_DIR_RTL; break;
		default: dir = MAIN_rtl ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR;
	}
	
	if (_inverted)
		dir = dir == GTK_TEXT_DIR_LTR ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR;
	
	gtk_widget_set_direction(widget, dir);
}

bool gControl::isRightToLeft() const
{
	return gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL;
}


void gControl::setInverted(bool v)
{
	if (v == _inverted)
		return;
	
	_inverted = v;
	updateDirection();
}

void gControl::setDirection(int v)
{
	if (v == _direction)
		return;
	
	_direction = v;
	updateDirection();
}

void gControl::checkVisibility()
{
	if (_allow_show)
		return;
	
	_allow_show = true;
	setVisibility(_visible);
}
