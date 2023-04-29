/***************************************************************************

  gcontainer.cpp

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

#include <stdio.h>
#include <unistd.h>

#include "gapplication.h"
#include "gdesktop.h"
#include "gmainwindow.h"
#include "gcontainer.h"

static GList *_arrange_list = NULL;

static gControl *get_next_child_widget (gContainer *gtk_control, int *gtk_index)
{
	gControl *ctrl;
	
	for(;;)
	{
		ctrl = gtk_control->child(*gtk_index);
		if (!ctrl)
			return NULL;
		
		(*gtk_index)++;
		if (ctrl->border && ctrl->widget && ctrl->isVisible())
			return ctrl;
	}
}

#ifdef GTK3
static gboolean cb_draw(GtkWidget *wid, cairo_t *cr, gContainer *data)
{
	CUSERCONTROL_cb_draw(data, cr);
	return false;
}
#else
static gboolean cb_expose(GtkWidget *wid, GdkEventExpose *e, gContainer *data)
{
	CUSERCONTROL_cb_draw(data, e->region, wid->allocation.x, wid->allocation.y);
	return false;
}
#endif

static void cb_map(GtkWidget *widget, gContainer *sender)
{
	//fprintf(stderr, "cb_map: %s %d\n", sender->name(), gApplication::_disable_mapping_events);

	sender->setShown(true);
	sender->arrangeLater();
}

#if GTK3
static void cb_remap_children(GtkWidget *widget, GdkEvent *event, gContainer *sender)
{
	int i;
	gControl *child;
	
	//fprintf(stderr, "cb_remap_children: '%s' <<<\n", sender->name());
	
	for (i = 0; i < sender->childCount(); i++)
	{
		child = sender->child(i);
		if (gtk_widget_get_visible(child->border))
		{
			child->hideButKeepFocus();
			child->showButKeepFocus();
		}
	}

	//fprintf(stderr, "cb_remap_children: >>> '%s'\n", sender->name());
}
#endif

static void cb_unmap(GtkWidget *widget, gContainer *sender)
{
	//fprintf(stderr, "cb_unmap: %s %d\n", sender->name(), gApplication::_disable_mapping_events);
	if (gApplication::_disable_mapping_events)
		return;

	sender->setShown(false);
}

static void resize_container(gContainer *cont, int w, int h)
{
	/*w += cont->width() - cont->containerWidth();
	h += cont->height() - cont->containerHeight();*/
	
	if (cont->isWindow())
		h += cont->height() - cont->containerHeight();

	if (w >= 0 && h >= 0)
		cont->resize(w, h);
}


#define WIDGET_TYPE gControl*
#define CONTAINER_TYPE gContainer*
#define ARRANGEMENT_TYPE gContainerArrangement*
#define IS_RIGHT_TO_LEFT(_object) (gtk_widget_get_direction(((gControl *)_object)->widget) == GTK_TEXT_DIR_RTL)
#define GET_WIDGET(_object) _object
#define GET_CONTAINER(_object) _object
#define GET_ARRANGEMENT(_object) (((gContainer*)_object)->getArrangement())
#define IS_EXPAND(_object) (((gControl*)_object)->isExpand())
#define IS_IGNORE(_object) (((gControl*)_object)->isIgnore())
#define IS_DESIGN(_object) (((gControl*)_object)->isDesign())
#define IS_USER(_object) (((gContainer *)_object)->isUser())
#define IS_WIDGET_VISIBLE(_widget)  (((gControl*)_widget)->isVisible())

#define CAN_ARRANGE(_object) (((gContainer *)_object)->isShown() && !((gControl *)_object)->isDestroyed())
//|| (((gControl *)_object)->isTopLevel() && ((gMainWindow *)_object)->opened))

// BM: ClientX() & ClientY() are relative to the border.
// We need X & Y relative to the container widget.

#define GET_WIDGET_CONTENTS(_widget, _x, _y, _w, _h) _x=((gContainer*)_widget)->containerX(); \
                                                     _y=((gContainer*)_widget)->containerY(); \
                                                     _w=((gContainer*)_widget)->containerWidth(); \
                                                     _h=((gContainer*)_widget)->containerHeight()

#define GET_WIDGET_X(_widget)  (((gControl*)_widget)->left())
#define GET_WIDGET_Y(_widget)  (((gControl*)_widget)->top())
#define GET_WIDGET_W(_widget)  (((gControl*)_widget)->width())
#define GET_WIDGET_H(_widget)  (((gControl*)_widget)->height())
#define MOVE_WIDGET(_object, _widget, _x, _y)  (((gControl*)_widget)->move( _x, _y))
#define RESIZE_WIDGET(_object, _widget, _w, _h)  (((gControl*)_widget)->resize( _w, _h, true))
#define MOVE_RESIZE_WIDGET(_object, _widget, _x, _y, _w, _h) (((gControl*)_widget)->moveResize(_x, _y, _w, _h, true))
#define RESIZE_CONTAINER(_object, _cont, _w, _h)  resize_container((gContainer *)(_cont), _w, _h) 

#define INIT_CHECK_CHILDREN_LIST(_widget) \
	gContainer *gtk_control = (gContainer*)_widget; \
	int gtk_index = 0; \

#define HAS_CHILDREN() (gtk_control->childCount() != 0)

#define RESET_CHILDREN_LIST() gtk_index = 0;

#define GET_NEXT_CHILD_WIDGET() get_next_child_widget(gtk_control, &gtk_index)

#define GET_OBJECT_FROM_WIDGET(_widget) ((void*)_widget)

#define GET_OBJECT_NAME(_object) (((gControl *)_object)->name())

#define RAISE_ARRANGE_EVENT(_object) CB_container_arrange((gContainer *)_object);
#define RAISE_BEFORE_ARRANGE_EVENT(_object) CB_container_before_arrange((gContainer *)_object);

#define DESKTOP_SCALE gDesktop::scale()

#define FUNCTION_NAME arrangeContainer

#include "gb.form.arrangement.h"

void gContainer::performArrange()
{
	if (_no_arrangement)
		_did_arrangement = true;
	else
	{
		_did_arrangement = false;
		//fprintf(stderr, "performArrange: %p / %p [%d] '%s'\n", this, hFree, isShown(), name());
		arrangeContainer((void *)this);
	}
}


static int _max_w, _max_h;
static int _gms_x, _gms_y, _gms_w, _gms_h;

static void gms_move_widget(gControl *wid, int x, int y)
{
	int w = x + wid->width();
	int h = y + wid->height();

	if (w > _max_w) _max_w = w;
	if (h > _max_h) _max_h = h;
}

static void gms_move_resize_widget(gControl *wid, int x, int y, int w, int h)
{
	w += x;
	h += y;

	if (w > _max_w) _max_w = w;
	if (h > _max_h) _max_h = h;
}

#undef MOVE_WIDGET
#define MOVE_WIDGET(_object, _widget, _x, _y) gms_move_widget(_widget, _x, _y)
#undef RESIZE_WIDGET
#define RESIZE_WIDGET(_object, _widget, _w, _h) (0)
#undef MOVE_RESIZE_WIDGET
#define MOVE_RESIZE_WIDGET(_object, _widget, _x, _y, _w, _h) gms_move_resize_widget(_widget, _x, _y, _w, _h)
#undef RAISE_BEFORE_ARRANGE_EVENT
#define RAISE_BEFORE_ARRANGE_EVENT(_object) (0)
#undef RAISE_ARRANGE_EVENT
#define RAISE_ARRANGE_EVENT(_object) (0)
#undef FUNCTION_NAME
#define FUNCTION_NAME get_max_size
#undef RESIZE_CONTAINER
#define RESIZE_CONTAINER(_object, _cont, _w, _h) (0)
#undef GET_WIDGET_CONTENTS
#define GET_WIDGET_CONTENTS(_widget, _x, _y, _w, _h) \
	_x = _gms_x; \
	_y = _gms_y; \
	_w = _gms_w; \
	_h = _gms_h;

//#undef IS_WIDGET_VISIBLE
//#define IS_WIDGET_VISIBLE(_cont) (1)
#define GET_MAX_SIZE

#include "gb.form.arrangement.h"

void gContainer::getMaxSize(int xc, int yc, int wc, int hc, int *w, int *h)
{
	int add;
	gContainerArrangement *arr = getArrangement();
	bool locked;

	locked = arr->locked;
	arr->locked = false;

	_max_w = 0;
	_max_h = 0;
	_gms_x = xc;
	_gms_y = yc;
	_gms_w = wc;
	_gms_h = hc;
	get_max_size((void *)this);

	if (arr->margin)
		add = arr->padding ? arr->padding : gDesktop::scale();
	else if (!arr->spacing)
		add = arr->padding;
	else
		add = 0;

	*w = _max_w + add;
	*h = _max_h + add;

	arr->locked = locked;
}

void gContainer::decide(gControl *child, bool *width, bool *height)
{
	*width = *height = FALSE;
	
	if (!isShown() || !child->_allow_show || child->isIgnore() || autoResize())
		return;
	
	if ((arrange() == ARRANGE_VERTICAL)
	    || (arrange() == ARRANGE_HORIZONTAL && child->isExpand())
	    || (arrange() == ARRANGE_ROW && child->isExpand())
		  || (arrange() == ARRANGE_FILL))
		*width = TRUE;
	
	if ((arrange() == ARRANGE_HORIZONTAL)
	    || (arrange() == ARRANGE_VERTICAL && child->isExpand())
	    || (arrange() == ARRANGE_COLUMN && child->isExpand())
		  || (arrange() == ARRANGE_FILL))
		*height = TRUE;
}

void gContainer::initialize()
{
	_children = g_ptr_array_new();
	
	radiogroup = NULL;
	
	_proxyContainer = NULL;
	_proxyContainerFor = NULL;
	_client_x = -1;
	_client_y = -1;
	_client_w = 0;
	_client_h = 0;
	_no_arrangement = 0;
	_did_arrangement = false;
	_is_container = true;
	_user = false;
	_user_container = false;
	_shown = false;
	_arrange_later = false;
	
	arrangement.mode = 0;
	arrangement.spacing = false;
	arrangement.padding = 0;
	arrangement.autoresize = false;
	arrangement.locked = false;
	arrangement.margin = false;
	arrangement.indent = false;
	arrangement.centered = false;
	arrangement.invert = false;
}

gContainer::gContainer() 
{
	//g_print("gContainer() %d\n",this);
	initialize();
}

gContainer::gContainer(gContainer *parent) : gControl(parent)
{
	//g_print("gContainer(parent) %d par: %d\n",this,parent);
	initialize();
}

gContainer::~gContainer()
{
	int i;
	
	resetArrangeLater();
	
	for (i = 0; i < childCount(); i++)
		child(i)->removeParent();
	
	g_ptr_array_unref(_children);
	_children = NULL;
	
	if (radiogroup) { g_object_unref(G_OBJECT(radiogroup)); radiogroup=NULL; }
}

int gContainer::childCount() const
{
	return _children->len;
}

gControl* gContainer::child(int index) const
{
	if (index < 0 || index >= (int)_children->len)
		return NULL;
	else
		return (gControl *)g_ptr_array_index(_children, index);
}

int gContainer::childIndex(gControl *ch) const
{
	int i;
	
	for (i = 0; i < childCount(); i++)
	{
		if (child(i) == ch)
			return i;
	}
	
	return -1;
}

void gContainer::setArrange(int vl)
{
	switch(vl)
	{
		case ARRANGE_NONE:
		case ARRANGE_HORIZONTAL:
		case ARRANGE_VERTICAL:
		case ARRANGE_LEFT_RIGHT:
		case ARRANGE_TOP_BOTTOM:
		case ARRANGE_FILL:
			if (vl != arrangement.mode)
			{
				arrangement.mode = vl;
				updateScrollBar();
				performArrange();
			}
		default: 
			break;
	}
}

void gContainer::setPadding(int vl)
{
	if (vl >= 0 && vl <= 255 && vl != arrangement.padding) 
	{
		arrangement.padding = vl;
		performArrange();
	}
}

void gContainer::setSpacing(bool vl)
{
	if (vl != arrangement.spacing)
	{
		arrangement.spacing = vl;
		performArrange();
	}
}

void gContainer::setMargin(bool vl)
{
	if (vl != arrangement.margin)
	{
		arrangement.margin = vl;
		performArrange();
	}
}

void gContainer::setIndent(bool vl)
{
	if (vl != arrangement.indent) 
	{
		arrangement.indent = vl;
		performArrange();
	}
}

void gContainer::setCentered(bool vl)
{
	if (vl != arrangement.centered) 
	{
		arrangement.centered = vl;
		performArrange();
	}
}

void gContainer::setAutoResize(bool vl)
{
	if (vl != arrangement.autoresize)
	{
		arrangement.autoresize = vl;
		performArrange();
	}
}

void gContainer::setUser()
{
	_user = true;
	performArrange();
	updateDesignChildren();
	ON_DRAW_BEFORE(border, this, cb_expose, cb_draw);
}

void gContainer::setInvert(bool vl)
{
	if (vl != arrangement.invert)
	{
		arrangement.invert = vl;
		performArrange();
	}
}

int gContainer::clientX()
{
	gint xc, yc;
	GtkWidget *cont = getContainer();
	
	if (_client_x >= 0)
		return _client_x;
	

	if (!_scroll && gtk_widget_get_window(cont) && gtk_widget_get_window(border))
	{
		gtk_widget_translate_coordinates(cont, border, 0, 0, &xc, &yc);
		xc += containerX();
	}
	else
		xc = getFrameWidth();
		
	return xc;
}

int gContainer::containerX()
{
	GtkWidget *cont = getContainer();
	
	if (cont == widget && widget == frame)
		return getFrameWidth();
	else
		return 0;
}

int gContainer::clientY()
{
	gint xc, yc;
	GtkWidget *cont = getContainer();
	
	if (_client_y >= 0)
		return _client_y;
	
	if (!_scroll && gtk_widget_get_window(cont) && gtk_widget_get_window(border))
	{
		gtk_widget_translate_coordinates(cont, border, 0, 0, &xc, &yc);
		yc += containerY();
	}
	else
		yc = getFrameWidth();
		
	return yc;
}

#if 0
int gContainer::clientY()
{
	gint xc, yc;
	GtkWidget *cont = getContainer();
	
	if (!cont->window || !border->window || cont == widget)
		return getFrameWidth();
	
// 	if (width() != border->allocation.width || height() != border->allocation.height)
// 	{
// 		updateGeometry();
// 		GtkAllocation a = { x(), y(), width(), height() };
// 		gtk_widget_size_allocate(widget, &a);
// 	}

	gtk_widget_translate_coordinates(cont, border, 0, 0, &xc, &yc);
	if (cont == widget)
		yc += getFrameWidth();
	
	return yc; // + getFrameWidth();
}
#endif

int gContainer::containerY()
{
	GtkWidget *cont = getContainer();
	
	if (cont == widget && widget == frame)
		return getFrameWidth();
	else
		return 0;
}

int gContainer::clientWidth()
{
	GtkWidget *cont = getContainer();
	
	if (_client_w > 0)
		return _client_w;
	
	if (cont != widget && gtk_widget_get_window(cont))
	{
		GtkAllocation a;

		gtk_widget_get_allocation(widget, &a);

		if ((width() != a.width || height() != a.height)
		    && a.width > 0 && a.height > 0)
		{
			//g_debug("clientWidth: %s: %d", name(), width());
			a.width = width(); a.height = height();
			gt_disable_warnings(true);
			gtk_widget_size_allocate(widget, &a);
			gt_disable_warnings(false);
		}
		//g_debug("ClientWidth: %s -> %d", this->name(), cont->allocation.width);

		gtk_widget_get_allocation(cont, &a);

		if (a.width > 0)
			return a.width;
	}
	
	if (_scroll)
		return (int)gtk_adjustment_get_page_size(gtk_scrolled_window_get_hadjustment(_scroll));
	
	return width() - getFrameWidth() * 2;
}

int gContainer::containerWidth()
{
	return clientWidth();
}

int gContainer::clientHeight()
{
	GtkWidget *cont = getContainer();
	
	if (_client_h > 0)
		return _client_h;
	
	if (cont != widget && gtk_widget_get_window(cont))
	{
		GtkAllocation a;

		gtk_widget_get_allocation(widget, &a);

		if ((width() != a.width || height() != a.height)
		    && a.width > 0 && a.height > 0)
		{
			//g_debug("clientHeight: %s: %d", name(), height());
			a.width = width(); a.height = height();
			//gt_disable_warnings(true);
			gtk_widget_size_allocate(widget, &a);
			//gt_disable_warnings(false);
			//gtk_container_resize_children(GTK_CONTAINER(widget));
		}
		//g_debug("ClientHeight: %s -> %d", this->name(), cont->allocation.height);
		gtk_widget_get_allocation(cont, &a);

		if (a.height > 0)
			return a.height;
	}
	
	if (_scroll)
		return (int)gtk_adjustment_get_page_size(gtk_scrolled_window_get_vadjustment(_scroll));
	
	return height() - getFrameWidth() * 2;
}

int gContainer::containerHeight()
{
	return clientHeight();
}

void gContainer::insert(gControl *child, bool realize)
{
	//fprintf(stderr, "insert %s into %s\n", child->name(), name());

	if (!gtk_widget_get_parent(child->border))
	{
		//gtk_layout_put(GTK_LAYOUT(getContainer()), child->border, 0, 0);
		gtk_container_add(GTK_CONTAINER(getContainer()), child->border);
	}
	child->bufX = child->bufY = 0;
	
	g_ptr_array_add(_children, child);
	
	if (realize)
		child->_visible = true;
    
	//g_debug("gContainer::insert: visible = %d", isReallyVisible());
	if (!realize)
		performArrange();
	//fprintf(stderr, "--> %d %d %d %d\n", child->x(), child->y(), child->width(), child->height());

	if (realize)
	{
    //gtk_widget_realize(child->border);
		//gtk_widget_show(child->border);
		if (child->frame)
			gtk_widget_show(child->frame);
		if (child->widget != child->border)
			gtk_widget_show(child->widget);
	}

#ifndef GTK3
	if (hasBackground() && !child->_bg_set) child->setBackground();
	if (hasForeground() && !child->_fg_set) child->setForeground();
#endif
  child->updateFont();
	
	if ((isUser() && isDesign()) || isDesignIgnore())
		child->setDesign(true);
	
	child->updateDirection();
}

void gContainer::remove(gControl *child)
{
	g_ptr_array_remove(_children, child);
}


gControl *gContainer::find(int x, int y)
{
	int i;
	gControl *ch;
	
	//fprintf(stderr, "gContainer::find: %s (C %d %d %d %d) (S %d %d): %d %d\n", name(), clientX(), clientY(), clientWidth(), clientHeight(), scrollX(), scrollY(), x, y);

	x -= clientX();
	y -= clientY();
	
	if (gApplication::_button_grab != this)
	{
		if (x < 0 || y < 0 || x >= clientWidth() || y >= clientHeight())
			return NULL;
	}
	
	if (_scroll)
	{
		x += scrollX();
		y += scrollY();
	}
	
	for (i = childCount() - 1; i >= 0; i--)
	{
		ch = child(i);
		//fprintf(stderr, "test: %s %d: %d %d %d %d / %d %d\n", ch->name(), ch->isVisible(), ch->x(), ch->y(), ch->width(), ch->height(), x, y);
		if (ch->isVisible() && x >= ch->left() && y >= ch->top() && x < (ch->left() + ch->width()) && y < (ch->top() + ch->height()))
		{
			//fprintf(stderr, "--> %s\n", ch->name());
			return ch;
		}
	}
	
	//fprintf(stderr, "--> NULL\n");
	return NULL;
}


#ifndef GTK3
void gContainer::setBackground(gColor color)
{
	int i;
	gControl *ch;
	
	gControl::setBackground(color);

	for (i = 0; i < childCount(); i++)
	{
		ch = gContainer::child(i);
		if (!ch->_bg_set)
			ch->setBackground();
	}
}
#endif

/*#ifdef GTK3
void gContainer::updateColor()
{
	int i;

	for (i = 0; i < childCount(); i++)
		gContainer::child(i)->updateColor();
}
#endif*/

void gContainer::setForeground(gColor color)
{
	int i;
	gControl *ch;
	
	gControl::setForeground(color);
	
	for (i = 0; i < childCount(); i++)
	{
		ch = gContainer::child(i);
		if (!ch->_fg_set)
			ch->setForeground();
	}	
}

GtkWidget *gContainer::getContainer()
{
	return widget;
}

void gContainer::borderSignals()
{
	gControl::borderSignals();
	
	g_signal_connect(G_OBJECT(border), "map", G_CALLBACK(cb_map), (gpointer)this);	
	g_signal_connect(G_OBJECT(border), "unmap", G_CALLBACK(cb_unmap), (gpointer)this);	
#ifdef GTK3
	g_signal_connect_after(G_OBJECT(border), "map-event", G_CALLBACK(cb_remap_children), (gpointer)this);	
#endif
}

bool gContainer::resize(int w, int h, bool no_decide)
{
	if (gControl::resize(w, h, no_decide))
		return true;

	_client_w = 0;
	_client_h = 0;
	
	if (isUser())
		CUSERCONTROL_cb_resize(this);

	performArrange();
	return false;
}

void gContainer::setVisible(bool vl)
{
	//bool arr;
	
	if (vl == isVisible())
		return;
	
	gControl::setVisible(vl);
	
	/*if (arr)
		performArrange();*/
}

/*void gContainer::updateFocusChain()
{
	GList *chain = NULL;
	int i;
	gControl *ch;
	
	for (i = 0; i < childCount(); i++)
	{
		ch = child(i);
		if (ch->isNoTabFocus())
			continue;
		chain = g_list_prepend(chain, ch->border);
	}

	chain = g_list_reverse(chain);
	
	gtk_container_set_focus_chain(GTK_CONTAINER(widget), chain);
	
	g_list_free(chain);
}*/

void gContainer::updateFont()
{
	int i;

	gControl::updateFont();

	for (i = 0; i < childCount(); i++)
		child(i)->updateFont();

	if (isUser())
		CUSERCONTROL_cb_font(this);
}

void gContainer::moveChild(gControl *child, int x, int y)
{
	GtkWidget *cont = gtk_widget_get_parent(child->border); //getContainer();
	
	if (GTK_IS_LAYOUT(cont))
		gtk_layout_move(GTK_LAYOUT(cont), child->border, x, y);
	else
		gtk_fixed_move(GTK_FIXED(cont), child->border, x, y);
}

bool gContainer::hasBackground() const
{
	return _bg_set || (parent() && parent()->hasBackground());
}

bool gContainer::hasForeground() const
{
	return _fg_set || (parent() && parent()->hasForeground());
}

void gContainer::setFullArrangement(gContainerArrangement *arr)
{
	bool locked = arrangement.locked;
	
	arrangement = *arr;
	arrangement.locked = locked;
	performArrange();
}

void gContainer::disableArrangement()
{
	if (_no_arrangement == 0)
		_did_arrangement = false;
	
	_no_arrangement++;
}

void gContainer::enableArrangement()
{
	_no_arrangement--;
	if (_no_arrangement == 0 && _did_arrangement)
		performArrange();
}

void gContainer::hideHiddenChildren()
{
	int i;
	gControl *child;
	
	for (i = 0;; i++)
	{
		child = gContainer::child(i);
		if (!child)
			break;
		if (!child->isVisible())
			gtk_widget_hide(child->border);
		else if (child->isContainer())
			((gContainer *)child)->hideHiddenChildren();
	}
}

void gContainer::reparent(gContainer *newpr, int x, int y)
{
	gControl::reparent(newpr, x, y);
	hideHiddenChildren();
}


void gContainer::clear()
{
	gContainer *cont = proxyContainer();
	gControl *ch;
	
	for(;;)
	{
		ch = cont->child(0);
		if (!ch)
			break;
		ch->destroy();
	}
}

void gContainer::updateDesignChildren()
{
	int i;
	gContainer *cont;

	if (!isDesign())
		return;
	
	if (!isUser() && !isDesignIgnore())
		return;
	
	if (isUserContainer() && !_proxyContainer)
		return;
	
	cont = isDesignIgnore() ? this : proxyContainer();
	
	for (i = 0; i < cont->childCount(); i++)
		cont->child(i)->setDesign(true);
}

void gContainer::setDesign(bool ignore)
{
	if (isDesign())
		return;
	
	gControl::setDesign(ignore);
	updateDesignChildren();
}

void gContainer::setProxyContainer(gContainer *proxy)
{
	if (_proxyContainer != this)
		_proxyContainer = proxy;
	else
		_proxyContainer = NULL;
	
	updateDesignChildren();
}

void gContainer::resetArrangeLater()
{
	if (_arrange_later)
	{
		//fprintf(stderr, "resetArrangeLater: %p / %p '%s'\n", this, hFree, name());
		
		_arrange_later = false;
		_arrange_list = g_list_remove(_arrange_list, this);
	}
}

void gContainer::arrangeLater()
{
	if (!_arrange_later)
	{
		//fprintf(stderr, "arrangeLater: %p / %p '%s'\n", this, hFree, name());
	
		_arrange_later = true;
		_arrange_list = g_list_prepend(_arrange_list, (gpointer)this);
	}
}

void gContainer::postArrange()
{
	GList *iter;
	gContainer *cont;

	if (!_arrange_list) return;

	for(;;)
	{
		iter = g_list_first(_arrange_list);
		if (!iter)
			break;
		cont = (gContainer *)iter->data;
		cont->resetArrangeLater();
		cont->performArrange();
	}

	g_list_free(_arrange_list);
	_arrange_list = NULL;
}

void gContainer::updateDirection()
{
	int i;
	gControl *child;
	
	gControl::updateDirection();
	
	for (i = 0;; i++)
	{
		child = gContainer::child(i);
		if (!child)
			break;
		child->updateDirection();
	}
	
	performArrange();
}
