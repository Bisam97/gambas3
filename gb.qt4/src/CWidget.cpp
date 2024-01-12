/***************************************************************************

	CWidget.cpp

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

#define __CWIDGET_CPP

#undef QT3_SUPPORT

#include "gambas.h"
#include "gb_common.h"

#include <stdio.h>
#include <stdlib.h>

#include "CWidget.h"
#include "CFont.h"
#include "CMouse.h"
#include "CKey.h"
#include "CWindow.h"
#include "CConst.h"
#include "CColor.h"
#include "CClipboard.h"
#include "CMenu.h"
#include "CDrawingArea.h"
#include "CTextArea.h"

#include <QApplication>
#include <QObject>
#include <QPalette>
#include <QToolTip>
#include <QPushButton>
#include <QMap>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QPixmap>
#include <QFrame>
#include <QDropEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QWheelEvent>
#include <QHash>
#include <QAbstractScrollArea>
#include <QAbstractEventDispatcher>
#include <QListWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QSet>
#include <QScrollBar>
#include <QLineEdit>
#include <QWindow>

#ifndef NO_X_WINDOW
static QMap<int, int> _x11_to_qt_keycode;
#endif

CWIDGET *CWIDGET_active_control = 0;
CWIDGET *CWIDGET_previous_control = 0;
CWIDGET *CWIDGET_hovered = 0;

int CCONTROL_last_event_type = 0;

static bool _focus_change = false;
static bool _doing_focus_change = false;
static CWIDGET *_old_active_control = 0;
static CWIDGET *_hovered = 0;
static CWIDGET *_official_hovered = 0;

#ifdef QT5
static CWIDGET *_last_entered = NULL;
#else
static QSet<CWIDGET *> *_enter_leave_set = NULL;
#endif

static QT_COLOR_FUNC _after_set_color = NULL;

#define EXT(_ob) ((CWIDGET_EXT *)((CWIDGET *)_ob)->ext)
#define ENSURE_EXT(_ob) (EXT(_ob) ? EXT(_ob) : alloc_ext((CWIDGET *)(_ob)))

#define HANDLE_PROXY(_ob) \
	while (EXT(_ob) && EXT(_ob)->proxy) \
		_ob = (__typeof__ _ob)(EXT(_ob)->proxy);

static CWIDGET_EXT *alloc_ext(CWIDGET *_object)
{
	GB.Alloc(POINTER(&(THIS->ext)), sizeof(CWIDGET_EXT));
	CLEAR(THIS_EXT);
	THIS_EXT->bg = COLOR_DEFAULT;
	THIS_EXT->fg = COLOR_DEFAULT;
	THIS_EXT->tag.type = GB_T_NULL;
	return THIS_EXT;
}
	
static void set_mouse(QWidget *w, int mouse, void *cursor)
{
	QObjectList children;
	QObject *child;
	int i;

	if (mouse == CURSOR_DEFAULT)
		w->unsetCursor();
	else if (mouse == CURSOR_CUSTOM)
	{
		if (cursor)
			w->setCursor(*((CCURSOR *)cursor)->cursor);
		else
			w->unsetCursor();
	}
	else
	{
		Qt::CursorShape shape;
		
		switch(mouse)
		{
			case CURSOR_NONE: shape = Qt::BlankCursor; break;
			case CURSOR_ARROW: shape = Qt::ArrowCursor; break;
			case CURSOR_HELP: shape = Qt::WhatsThisCursor; break;
			case CURSOR_POINTER: shape = Qt::PointingHandCursor; break;
			//case CURSOR_CONTEXT_MENU: shape = ; break;
			case CURSOR_PROGRESS: shape = Qt::BusyCursor; break;
			case CURSOR_WAIT: shape = Qt::WaitCursor; break;
			case CURSOR_CELL: shape = Qt::CrossCursor; break;
			case CURSOR_CROSSHAIR: shape = Qt::CrossCursor; break;
			case CURSOR_TEXT: shape = Qt::IBeamCursor; break;
			case CURSOR_VERTICAL_TEXT: shape = Qt::IBeamCursor; break;
			case CURSOR_ALIAS: shape = Qt::DragLinkCursor; break;
			case CURSOR_COPY: shape = Qt::DragCopyCursor; break;
			case CURSOR_NO_DROP: shape = Qt::ForbiddenCursor; break;
			case CURSOR_MOVE: shape = Qt::SizeAllCursor; break;
			case CURSOR_NOT_ALLOWED: shape = Qt::ForbiddenCursor; break;
			case CURSOR_GRAB: shape = Qt::OpenHandCursor; break;
			case CURSOR_GRABBING: shape = Qt::ClosedHandCursor; break;
			case CURSOR_ALL_SCROLL: shape = Qt::SizeAllCursor; break;
			case CURSOR_COL_RESIZE: shape = Qt::SplitHCursor; break;
			case CURSOR_ROW_RESIZE: shape = Qt::SplitVCursor; break;
			case CURSOR_N_RESIZE: shape = Qt::SizeVerCursor; break;
			case CURSOR_E_RESIZE: shape = Qt::SizeHorCursor; break;
			case CURSOR_S_RESIZE: shape = Qt::SizeVerCursor; break;
			case CURSOR_W_RESIZE: shape = Qt::SizeHorCursor; break;
			case CURSOR_NE_RESIZE: shape = Qt::SizeBDiagCursor; break;
			case CURSOR_NW_RESIZE: shape = Qt::SizeFDiagCursor; break;
			case CURSOR_SW_RESIZE: shape = Qt::SizeBDiagCursor; break;
			case CURSOR_SE_RESIZE: shape = Qt::SizeFDiagCursor; break;
			case CURSOR_EW_RESIZE: shape = Qt::SizeHorCursor; break;
			case CURSOR_NS_RESIZE: shape = Qt::SizeVerCursor; break;
			case CURSOR_NESW_RESIZE: shape = Qt::SizeBDiagCursor; break;
			case CURSOR_NWSE_RESIZE: shape = Qt::SizeFDiagCursor; break;
			//case CURSOR_ZOOM_IN: shape = "zoom-in"; break;
			//case CURSOR_ZOOM_OUT: shape = "zoom-out"; break;
			default: shape = Qt::ArrowCursor;
		}
		
		w->setCursor(QCursor(shape));
	}

	children = w->children();

	for (i = 0; i < children.count(); i++)
	{
		child = children.at(i);
		
		if (child->isWidgetType() && !CWidget::getReal(child))
			set_mouse((QWidget *)child, CURSOR_DEFAULT, 0);
	}
}

void CWIDGET_set_design(CWIDGET *_object, bool ignore)
{
	if (THIS->flag.design)
		return;
	
	//fprintf(stderr, "CWIDGET_set_design: %s %d\n", THIS->name, ignore);
	
	CWidget::removeFocusPolicy(WIDGET);
	set_mouse(WIDGET, CURSOR_DEFAULT, 0);
	
	THIS->flag.design = true;
	THIS->flag.design_ignore = ignore;

	if (GB.Is(THIS, CLASS_Container))
	{
		if (GB.Is(THIS, CLASS_TabStrip))
		{
			THIS->flag.fillBackground = TRUE;
			CWIDGET_reset_color(THIS);
		}
		
		CCONTAINER_update_design((CCONTAINER *)THIS);
	}
}

void CWIDGET_set_name(CWIDGET *_object, const char *name)
{
	CWINDOW *window;
	MyMainWindow *win = 0;
	
	if (GB.Is(THIS, CLASS_Menu))
	{
		if (qobject_cast<MyMainWindow *>(((CMENU *)THIS)->toplevel))
			win = (MyMainWindow *)((CMENU *)THIS)->toplevel;
	}
	else
	{
		window = CWidget::getWindow(THIS);
		if (window)
			win = (MyMainWindow *)QWIDGET(window);

		if (win)
		{
			if (name)
				win->setName(name, THIS);
			else
				win->setName(THIS->name, 0);
		}
	}
		
	GB.FreeString(&THIS->name);
	
	if (name)
		THIS->name = GB.NewZeroString(name);
}

void *CWIDGET_get_parent(void *_object)
{
	QWidget *parent = WIDGET->parentWidget();

	if (!parent || (GB.Is(THIS, CLASS_Window) && ((CWINDOW *)_object)->toplevel))
		return NULL;
	else
		return CWidget::get(parent);
}

void *CWIDGET_get_parent_container(void *_object)
{
	void *parent = CWIDGET_get_parent(THIS);
	
	if (!parent)
		return NULL;
	
	if (EXT(parent) && EXT(parent)->container_for)
		parent = EXT(parent)->container_for;
	
	return parent;
}

uintptr_t CWIDGET_get_handle(void *_object)
{
#ifdef QT5
	return PLATFORM.Window.GetId(WIDGET);
#else
	return (uintptr_t)WIDGET->winId();
#endif
}

bool CWIDGET_is_visible(void *_object)
{
	return THIS->flag.visible; // || !QWIDGET(_object)->isHidden();
}

void CWIDGET_register_proxy(void *_object, void *proxy)
{
	void *check = proxy;

	while (check)
	{
		if (check == THIS)
		{
			GB.Error("Circular proxy chain");	
			return;
		}
		check = EXT(check) ? EXT(check)->proxy : NULL;
	}
	
	if (proxy && THIS_EXT && proxy == THIS_EXT->proxy)
		return;
	else if (!proxy && !THIS_EXT)
		return;
	
	if (proxy && EXT(proxy) && EXT(proxy)->proxy_for)
		EXT(EXT(proxy)->proxy_for)->proxy = NULL;

	//fprintf(stderr, "proxy: (%p %s) -> (%p %s)\n", THIS, THIS->name, proxy, proxy ? ((CWIDGET *)proxy)->name : "NULL");

	if (THIS_EXT && THIS_EXT->proxy && EXT(THIS_EXT->proxy))
		EXT(THIS_EXT->proxy)->proxy_for = NULL;
	
	if (proxy)
		ENSURE_EXT(THIS)->proxy = proxy;
	else if (EXT(THIS))
		EXT(THIS)->proxy = NULL;
	
	if (proxy)
		ENSURE_EXT(proxy)->proxy_for = THIS;
}

int CWIDGET_check(void *_object)
{
	return WIDGET == NULL || THIS->flag.deleted;
}

static QWidget *get_viewport(QWidget *w)
{
	if (qobject_cast<QAbstractScrollArea *>(w))
		return ((QAbstractScrollArea *)w)->viewport();
	//else if (qobject_cast<Q3ScrollView *>(w))
	//	return ((Q3ScrollView *)w)->viewport();
	//else if (qobject_cast<Q3ListView *>(w))
	//	return ((Q3ListView *)w)->viewport();
	else
		return 0;
}

/*void CWIDGET_update_design(CWIDGET *_object)
{
	if (!CWIDGET_test_flag(THIS, WF_DESIGN) && !CWIDGET_test_flag(THIS, WF_DESIGN_LEADER))
		return;

	//qDebug("CWIDGET_update_design: %s %p", GB.GetClassName(THIS), THIS);
	set_design(THIS);
}*/

void CWIDGET_init_name(CWIDGET *_object)
{
	static int n = 0;
	char *name = GB.GetLastEventName();
	
	if (!name)
	{
		char buffer[16];
		n++;
		sprintf(buffer, "#%d", n);
		CWIDGET_set_name(THIS, buffer);
	}
	else
		CWIDGET_set_name(THIS, name);
}

bool CWIDGET_container_for(void *_object, void *container_for)
{
	if (THIS_EXT)
	{
		if (container_for)
		{
			if (!THIS_EXT->container_for)
			{
				THIS_EXT->container_for = container_for;
				return false;
			}
		}
		else
		{
			THIS_EXT->container_for = NULL;
			return false;
		}
	}
	else
	{
		if (container_for)
			ENSURE_EXT(THIS)->container_for = container_for;
		return false;
	}
	
	return true;
}

static void CWIDGET_enter(void *_object)
{
	CWIDGET *parent = (CWIDGET *)CWIDGET_get_parent(THIS);
	
	if (parent && !parent->flag.inside)
		CWIDGET_enter(parent);
	
	if (!THIS->flag.inside)
	{
		//qDebug("CWIDGET_enter: %p %s", THIS, THIS->name);
#ifdef QT5
		_last_entered = THIS;
#endif
		THIS->flag.inside = true;
		GB.Raise(THIS, EVENT_Enter, 0);
	}
}

static void CWIDGET_leave(void *_object)
{
#ifdef QT5
		if (_last_entered == THIS)
			_last_entered = (CWIDGET *)CWIDGET_get_parent((void *)_last_entered);
#endif
	if (THIS->flag.inside)
	{
		//qDebug("CWIDGET_leave: %p %s", THIS, THIS->name);
		THIS->flag.inside = false;
		GB.Raise(THIS, EVENT_Leave, 0);
	}
}

bool CWIDGET_get_allow_focus(void *_object)
{
	return WIDGET->focusPolicy() != Qt::NoFocus;
}

void CWIDGET_set_allow_focus(void *_object, bool f)
{
	if (f)
	{
		WIDGET->setFocusPolicy(THIS->flag.wheel | GB.CanRaise(THIS, EVENT_MouseWheel) ? Qt::WheelFocus : Qt::StrongFocus);
		WIDGET->setAttribute(Qt::WA_InputMethodEnabled, true);
	}
	else
	{
		WIDGET->setFocusPolicy(Qt::NoFocus);
	}
}


void CWIDGET_new(QWidget *w, void *_object, bool no_show, bool no_filter, bool no_init)
{
	//QAbstractScrollArea *sa;
	
	CWidget::add(w, _object, no_filter);

	//QWidget *p = w->parentWidget();
	//qDebug("CWIDGET_new: %s %p: %p in (%s %p)", GB.GetClassName(THIS), THIS, w, p ? GB.GetClassName(CWidget::get(p)) : "", CWidget::get(p));

	THIS->widget = w;
	//THIS->level = MAIN_loop_level;

	if (!no_init)
		CWIDGET_init_name(THIS);	

	if (qobject_cast<QAbstractScrollArea *>(w)) // || qobject_cast<Q3ScrollView *>(w))
		THIS->flag.scrollview = TRUE;

	//w->setAttribute(Qt::WA_PaintOnScreen, true);
	
	CWIDGET_reset_color(THIS); //w->setPalette(QApplication::palette());
	
	//THIS->flag.fillBackground = GB.Is(THIS, CLASS_Container);
	//w->setAutoFillBackground(THIS->flag.fillBackground);
	
	if (!no_show)
	{
		w->setGeometry(-16, -16, 8, 8);
		CWIDGET_set_visible(THIS, true);
		w->raise();
	}
	
	CCONTAINER_insert_child(THIS);
}


QString CWIDGET_Utf8ToQString(GB_STRING *str)
{
	return QString::fromUtf8((const char *)(str->value.addr + str->value.start), str->value.len);
}

#ifdef QT5
void CWIDGET_leave_popup(void *)
{
	while (_last_entered)
		CWIDGET_leave(_last_entered);
}
#else
void *CWIDGET_enter_popup()
{
	void *save = _enter_leave_set;
	
	_enter_leave_set = new QSet<CWIDGET *>;
	return save;
}

void CWIDGET_leave_popup(void *save)
{
	CWIDGET *_object;
	QSetIterator<CWIDGET *> i(*_enter_leave_set);
	
	while (i.hasNext())
	{
		_object = i.next();
		GB.Unref(POINTER(&_object));
		if (_object)
		{
			if (THIS->flag.inside_later != THIS->flag.inside)
			{
				if (THIS->flag.inside_later)
					CWIDGET_enter(THIS);
				else
					CWIDGET_leave(THIS);
			}
		}
	}
	
	delete _enter_leave_set;
	_enter_leave_set = (QSet<CWIDGET *>*) save;
}

static void insert_enter_leave_event(CWIDGET *control, bool in)
{
	control->flag.inside_later = in;
	
	if (_enter_leave_set->contains(control))
		return;
	
	_enter_leave_set->insert(control);
	GB.Ref(control);
}
#endif

static bool _post_check_hovered = false;
static CWIDGET *_post_check_hovered_window = NULL;

static void post_check_hovered(intptr_t)
{
	void *_object = _post_check_hovered_window;
	
	if (!THIS)
		_object = CWINDOW_Active;
	
	if (THIS && WIDGET)
	{
		//qDebug("post_check_hovered");
		const QPoint globalPos(QCursor::pos());
		QPoint pos = WIDGET->mapFromGlobal(globalPos);
		_hovered = CWidget::getRealExisting(WIDGET->childAt(pos));
		if (_hovered)
			CWIDGET_enter(_hovered);
	}
	
	_post_check_hovered = false;
	_post_check_hovered_window = NULL;
	
}

void CWIDGET_check_hovered()
{
	_post_check_hovered_window = NULL;
	post_check_hovered(0);
}

void CWIDGET_destroy(CWIDGET *_object)
{
	if (!THIS || !WIDGET)
		return;

	if (THIS->flag.deleted)
		return;
	
	if (THIS->flag.dragging)
	{
		GB.Error("Control is being dragged");
		return;
	}

#ifdef QT5
	if (_last_entered == THIS)
		_last_entered = NULL;
#endif
	
	//qDebug("CWIDGET_destroy: %s %p", GB.GetClassName(THIS), THIS);

	CWIDGET_set_visible(THIS, false);
	THIS->flag.deleted = true;

	WIDGET->deleteLater();
}

#define COORD(_c) ((qobject_cast<MyMainWindow *>(WIDGET) && WIDGET->isWindow()) ? ((CWINDOW *)_object)->_c : WIDGET->pos()._c())
#define get_widget(_object) QWIDGET(_object)

static void arrange_parent(CWIDGET *_object)
{
	void *parent = CWIDGET_get_parent(THIS);
	if (!parent)
		return;
	if (CWIDGET_check(parent))
		return;
	CCONTAINER_arrange(parent);
}

void CWIDGET_check_visibility(CWIDGET *_object)
{
	if (!THIS->flag.resized)
	{
		THIS->flag.resized = TRUE;
		//qDebug("CWIDGET_check_visibility: %s %s %d", GB.GetClassName(THIS), THIS->name, THIS->flag.visible);
		CWIDGET_set_visible(THIS, THIS->flag.visible);
	}
}

static void CWIDGET_after_geometry_change(void *_object, bool arrange)
{
	if (arrange)
	{
		if (GB.Is(THIS, CLASS_Container))
			CCONTAINER_arrange(THIS);
		if (GB.Is(THIS, CLASS_DrawingArea))
			((MyDrawingArea *)((CWIDGET *)_object)->widget)->updateBackground();
	}
	
	if (!THIS->flag.ignore)
		arrange_parent(THIS);
}

void CWIDGET_move_resize(void *_object, int x, int y, int w, int h)
{
	QWidget *wid = WIDGET;
	bool arrange = true;

	if (GB.Is(THIS, CLASS_Window))
	{
		CWINDOW_move_resize(THIS, x, y, w, h);
	}
	else
	{
		if (w < 0)
			w = wid->width();

		if (h < 0)
			h = wid->height();

		if (x == wid->x() && y == wid->y() && w == wid->width() && h == wid->height())
			return;
		
		if (w == wid->width() && h == wid->height())
			arrange = false;
		
		wid->setGeometry(x, y, w, h);
	}

	CWIDGET_check_visibility(THIS);
	CWIDGET_after_geometry_change(THIS, arrange);
}

void CWIDGET_move(void *_object, int x, int y)
{
	CWIDGET_move_resize(THIS, x, y, -1, -1);
}

void CWIDGET_resize(void *_object, int w, int h)
{
	CWIDGET_move_resize(THIS, COORD(x), COORD(y), w, h);
}

void CWIDGET_auto_resize(void *_object, int w, int h)
{
	bool dw, dh;
	CCONTAINER_decide(THIS, &dw, &dh);
	CWIDGET_resize(THIS, dw ? -1 : w, dh ? -1 : h);
}

void CWIDGET_auto_move_resize(void *_object, int x, int y, int w, int h)
{
	bool dw, dh;
	CCONTAINER_decide(THIS, &dw, &dh);
	CWIDGET_move_resize(THIS, x, y, dw ? -1 : w, dh ? -1 : h);
}

#if 0
void CWIDGET_check_hovered()
{
	//qDebug("CWIDGET_check_hovered: %p %s -> %p %s", _hovered, _hovered ? _hovered->name : 0, _official_hovered, _official_hovered ? _official_hovered->name : 0);
	
	if (_official_hovered != _hovered)
	{
		if (_official_hovered)
			CWIDGET_leave(_official_hovered);
		
		if (_hovered)
			CWIDGET_enter(_hovered);
		
		_official_hovered = _hovered;
	}
}
#endif

bool CWIDGET_is_design(void *_object)
{
	return THIS->flag.design && !THIS->flag.no_design;
}

static void _cleanup_CWIDGET_raise_event_action(intptr_t object)
{
	GB.Unref(POINTER(&object));
}

void CWIDGET_raise_event_action(void *object, int event)
{
	GB_RAISE_HANDLER handler;

	GB.Ref(object);
	
	handler.callback = _cleanup_CWIDGET_raise_event_action;
	handler.data = (intptr_t)object;
	
	GB.RaiseBegin(&handler);
	GB.Raise(object, event, 0);
	GB.RaiseEnd(&handler);
	
	CACTION_raise(object);
	
	GB.Unref(POINTER(&object));
}


static void update_direction(void *_object)
{
	if (THIS->flag.inverted)
	{
		switch (THIS->flag.direction)
		{
			case DIRECTION_LTR: WIDGET->setLayoutDirection(Qt::RightToLeft); break;
			case DIRECTION_RTL: WIDGET->setLayoutDirection(Qt::LeftToRight); break;
			default:
				WIDGET->unsetLayoutDirection();
				WIDGET->setLayoutDirection(WIDGET->isLeftToRight() ? Qt::RightToLeft : Qt::LeftToRight);
		}
	}
	else
	{
		switch (THIS->flag.direction)
		{
			case DIRECTION_LTR: WIDGET->setLayoutDirection(Qt::LeftToRight); break;
			case DIRECTION_RTL: WIDGET->setLayoutDirection(Qt::RightToLeft); break;
			default: WIDGET->unsetLayoutDirection();
		}
	}
	
	if (GB.Is(THIS, CLASS_Container))
		CCONTAINER_arrange(THIS);
}


void CWIDGET_set_inverted(void *_object, bool v)
{
	if (v == THIS->flag.inverted)
		return;
	
	THIS->flag.inverted = v;
	update_direction(THIS);
}


/*gControl *gControl::nextFocus()
{
	gControl *ctrl;
	
	if (isContainer())
	{
		ctrl = ((gContainer *)this)->firstChild();
		if (ctrl)
			return ctrl;
	}
	
	ctrl = this;
	
	while (!ctrl->next())
	{
		ctrl = ctrl->parent();
		if (ctrl->isTopLevel())
			return ctrl->nextFocus();
	}
	
	return ctrl->next();
}*/

bool CWIDGET_has_no_tab_focus(void *_object)
{
	void *parent;
	
	for(;;)
	{
		parent = CWIDGET_get_parent(THIS);
		HANDLE_PROXY(_object);
		if (THIS->flag.no_tab_focus)
			return true;
		if (!parent)
			return false;
		_object = parent;
	}
}


void *CWIDGET_get_next_focus(void *_object)
{
	void *ob, *next;
	
	//fprintf(stderr, "next: %s\n", CWIDGET_get_name(THIS));
	ob = CCONTAINER_get_first_child(THIS);
	if (ob)
	{
		//fprintf(stderr, "=> %s\n", CWIDGET_get_name(ob));
		return ob;
	}
	
	ob = THIS;
	
	for(;;)
	{
		//fprintf(stderr, "... %s\n", CWIDGET_get_name(ob));
		next = CWIDGET_get_next_previous(ob, true);
		if (next)
		{
			//fprintf(stderr, "=> %s\n", CWIDGET_get_name(next));
			return next;
		}
		
		ob = CWIDGET_get_parent(ob);
		if (!ob)
			return NULL;
		if (!CWIDGET_get_parent(ob))
		{
			ob = CWIDGET_get_next_focus(ob);
			//fprintf(stderr, "=> %s\n", CWIDGET_get_name(ob));
			return ob;
		}
	}
}

/*gControl *gControl::previousFocus()
{
	gControl *ctrl = previous();
	
	if (!ctrl)
	{
		if (!isTopLevel())
			return parent()->previousFocus();
		
		ctrl = this;
	}
	
	while (ctrl->isContainer() && ((gContainer *)ctrl)->childCount())
		ctrl = ((gContainer *)ctrl)->lastChild();

	return ctrl;
}*/

void *CWIDGET_get_previous_focus(void *_object)
{
	void *ob, *ctrl;
	
	//fprintf(stderr, "previous: %s\n", CWIDGET_get_name(THIS));
	ctrl = CWIDGET_get_next_previous(THIS, false);
	
	if (!ctrl)
	{
		ob = CWIDGET_get_parent(THIS);
		if (ob)
		{
			ob = CWIDGET_get_previous_focus(ob);
			//fprintf(stderr, "=> %s\n", CWIDGET_get_name(ob));
			return ob;
		}
		
		ctrl = THIS;
	}
	
	for(;;)
	{
		/*if (!CWIDGET_is_visible(ctrl))
			return ctrl;*/
		//fprintf(stderr, "... %s\n", CWIDGET_get_name(ctrl));
		ob = CCONTAINER_get_last_child(ctrl);
		if (!ob)
		{
			//fprintf(stderr, "=> %s\n", CWIDGET_get_name(ctrl));
			return ctrl;
		}
		ctrl = ob;
	}
}


//---------------------------------------------------------------------------

BEGIN_METHOD_VOID(Control_new)

	MAIN_CHECK_INIT();

END_METHOD

BEGIN_PROPERTY(Control_X)

	if (READ_PROPERTY)
		GB.ReturnInteger(COORD(x));
	else
	{
		CWIDGET_move(_object, VPROP(GB_INTEGER), COORD(y));
		/*if (WIDGET->isWindow())
			qDebug("X: %d ==> X = %d", PROPERTY(int), WIDGET->x());*/
	}

END_PROPERTY


BEGIN_PROPERTY(Control_ScreenX)

	int x = WIDGET->mapToGlobal(QPoint(0, 0)).x();

#ifdef QT5
	if (MAIN_platform_is_wayland)
	{
		QWindow *win = WIDGET->window()->windowHandle();
		if (win)
			x += win->frameMargins().left();
	}
#endif

  GB.ReturnInteger(x);

END_PROPERTY


BEGIN_PROPERTY(Control_Y)

	if (READ_PROPERTY)
		GB.ReturnInteger(COORD(y));
	else
		CWIDGET_move(_object, COORD(x), VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_ScreenY)

	int y = WIDGET->mapToGlobal(QPoint(0, 0)).y();

#ifdef QT5
	if (MAIN_platform_is_wayland)
	{
		QWindow *win = WIDGET->window()->windowHandle();
		if (win)
			y += win->frameMargins().top();
	}
#endif

  GB.ReturnInteger(y);

END_PROPERTY


BEGIN_PROPERTY(Control_Width)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->width());
	else
		CWIDGET_auto_resize(_object, VPROP(GB_INTEGER), -1);

END_PROPERTY


BEGIN_PROPERTY(Control_Height)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->height());
	else
		CWIDGET_auto_resize(_object, -1, VPROP(GB_INTEGER));

END_PROPERTY

void *CWIDGET_get_real_font(CWIDGET *_object)
{
	if (THIS->font)
		return CFONT_create(*((CFONT *)THIS->font)->font);

	CWIDGET *parent = (CWIDGET *)CWIDGET_get_parent(THIS);
	if (parent)
		return CWIDGET_get_real_font(parent);
	else
		return CFONT_create(qApp->font());
}

BEGIN_PROPERTY(Control_Font)

	CFONT *font;
	
	if (!THIS->font)
	{
		THIS->font = CFONT_create(WIDGET->font(), 0, THIS);
		GB.Ref(THIS->font);
	}
	
	if (READ_PROPERTY)
	{
		*(((CFONT *)THIS->font)->font) = WIDGET->font();
		GB.ReturnObject(THIS->font);
	}
	else
	{
		font = (CFONT *)VPROP(GB_OBJECT);

		if (!font)
		{
			WIDGET->setFont(QFont());
			GB.Unref(POINTER(&THIS->font));
			THIS->font = NULL;
		}
		else
		{
			WIDGET->setFont(*(font->font));
			*(((CFONT *)THIS->font)->font) = WIDGET->font();
		}
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Design)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CWIDGET_is_design(THIS));
	else
	{
		if (VPROP(GB_BOOLEAN))
			CWIDGET_set_design(THIS);
		else if (CWIDGET_is_design(THIS))
			GB.Error("Design property cannot be reset");
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Enabled)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->isEnabled());
	else
		WIDGET->setEnabled(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Control_HasFocus)

	HANDLE_PROXY(_object);
	
	GB.ReturnBoolean(WIDGET->hasFocus());

END_PROPERTY

BEGIN_PROPERTY(Control_Hovered)

	if (!CWIDGET_is_visible(THIS))
		GB.ReturnBoolean(false);
	else
		GB.ReturnBoolean(THIS->flag.inside);

END_PROPERTY

BEGIN_PROPERTY(Control_Expand)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->flag.expand);
	else if (THIS->flag.expand != VPROP(GB_BOOLEAN))
	{
		THIS->flag.expand = VPROP(GB_BOOLEAN);
		CWIDGET_check_visibility(THIS);
		if (!THIS->flag.ignore)
			arrange_parent(THIS);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Ignore)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->flag.ignore);
	else if (THIS->flag.ignore != VPROP(GB_BOOLEAN))
	{
		THIS->flag.ignore = VPROP(GB_BOOLEAN);
		arrange_parent(THIS);
	}

END_PROPERTY


BEGIN_METHOD(Control_Move, GB_INTEGER x; GB_INTEGER y; GB_INTEGER w; GB_INTEGER h)

	CWIDGET_auto_move_resize(_object, VARG(x), VARG(y), VARGOPT(w, -1), VARGOPT(h, -1));

END_METHOD


BEGIN_METHOD(Control_Resize, GB_INTEGER w; GB_INTEGER h)

	CWIDGET_auto_resize(_object, VARG(w), VARG(h));

END_METHOD


BEGIN_METHOD(Control_MoveScaled, GB_FLOAT x; GB_FLOAT y; GB_FLOAT w; GB_FLOAT h)

	int x, y, w, h;
	int scale = MAIN_scale;
	
	x = (int)(VARG(x) * scale + 0.5);
	y = (int)(VARG(y) * scale + 0.5);
	w = (MISSING(w) ? -1 : (VARG(w) * scale + 0.5));
	h = (MISSING(h) ? -1 : (VARG(h) * scale + 0.5));
	
	if (w == 0) w = 1;
	if (h == 0) h = 1;

	CWIDGET_auto_move_resize(_object, x, y, w, h);

END_METHOD


BEGIN_METHOD(Control_ResizeScaled, GB_FLOAT w; GB_FLOAT h)

	int w, h;
	int scale = MAIN_scale;

	w = (int)(VARG(w) * scale + 0.5);
	h = (int)(VARG(h) * scale + 0.5);
	
	if (w == 0) w = 1;
	if (h == 0) h = 1;

	CWIDGET_auto_resize(_object, w , h);

END_METHOD


BEGIN_METHOD_VOID(Control_Delete)

	//if (WIDGET)
	//  qDebug("CWIDGET_delete: %p (%p)", THIS, WIDGET);

	CWIDGET_destroy(THIS);

END_METHOD


void CWIDGET_set_visible(CWIDGET *_object, bool v)
{
	bool arrange = false;
	
	THIS->flag.visible = v;

	if (!THIS->flag.resized)
		return;

	if (THIS->flag.visible)
	{
		arrange = !WIDGET->isVisible();
		QWIDGET(_object)->show();
		if (GB.Is(THIS, CLASS_Container))
			CCONTAINER_arrange(THIS);
	}
	else
	{
		arrange = !WIDGET->isHidden();
		QWIDGET(_object)->hide();
	}
	
	if (arrange && !THIS->flag.ignore)
		arrange_parent(THIS);
}



BEGIN_PROPERTY(Control_Visible)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CWIDGET_is_visible(THIS));
	else
	{
		CWIDGET_set_visible(THIS, VPROP(GB_BOOLEAN));
		CWIDGET_check_visibility(THIS);
	}

END_PROPERTY


BEGIN_METHOD_VOID(Control_Show)

	CWIDGET_set_visible(THIS, true);
	CWIDGET_check_visibility(THIS);

END_METHOD


BEGIN_METHOD_VOID(Control_Hide)

	CWIDGET_set_visible(THIS, false);
	CWIDGET_check_visibility(THIS);

END_METHOD


BEGIN_METHOD_VOID(Control_Raise)

	QWIDGET(_object)->raise();
	arrange_parent(THIS);

END_METHOD


BEGIN_METHOD_VOID(Control_Lower)

	QWIDGET(_object)->lower();
	arrange_parent(THIS);

END_METHOD


BEGIN_METHOD(Control_Move_under, GB_OBJECT control)

	CWIDGET *ob = (CWIDGET *)VARG(control);

	if (GB.CheckObject(ob))
		return;

	WIDGET->stackUnder(ob->widget);

END_METHOD


void *CWIDGET_get_next_previous(void *_object, bool next)
{
	QWidget *parent;
	QObjectList children;
	int i;
	void *current;

	parent = WIDGET->parentWidget();
	if (!parent)
		return NULL;
	
	children = WIDGET->parentWidget()->children();
	i = children.indexOf(WIDGET);
	for(;;)
	{
		if (next)
		{
			i++;
			if (i >= children.count())
				return NULL;
		}
		else
		{
			i--;
			if (i < 0)
				return NULL;
		}
		
		current = CWidget::getRealExisting(children.at(i));
		if (current)
			return current;
	}
}

BEGIN_PROPERTY(Control_Next)

	if (READ_PROPERTY)
	{
		GB.ReturnObject(CWIDGET_get_next_previous(THIS, true));
	}
	else
	{
		CWIDGET *ob = (CWIDGET *)VPROP(GB_OBJECT);
		
		if (!ob)
			WIDGET->raise();
		else
		{
			if (GB.CheckObject(ob))
				return;
			
			WIDGET->stackUnder(ob->widget);
		}
		arrange_parent(THIS);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Previous)

	if (READ_PROPERTY)
	{
		GB.ReturnObject(CWIDGET_get_next_previous(THIS, false));
	}
	else
	{
		CWIDGET *ob = (CWIDGET *)VPROP(GB_OBJECT);

		if (!ob)
		{
			WIDGET->lower();
		}
		else
		{
			if (GB.CheckObject(ob))
				return;

			ob = (CWIDGET *)CWIDGET_get_next_previous(ob, true);
			if (ob)
				WIDGET->stackUnder(QWIDGET(ob));
		}
		arrange_parent(THIS);
	}

END_PROPERTY


BEGIN_METHOD_VOID(Control_Refresh) //, GB_INTEGER x; GB_INTEGER y; GB_INTEGER w; GB_INTEGER h)

	QWIDGET(_object)->update();
	if (THIS->flag.scrollview)
		get_viewport(WIDGET)->update();

END_METHOD


void CWIDGET_set_focus(void *_object)
{
	CWINDOW *win;

	HANDLE_PROXY(_object);
	
	win = CWidget::getTopLevel(THIS);

	if (win->opened && QWIDGET(win)->isVisible())
	{
		if (qobject_cast<QLineEdit *>(WIDGET) || qobject_cast<QTextEdit *>(WIDGET))
			WIDGET->setFocus(Qt::MouseFocusReason);
		else
			WIDGET->setFocus(Qt::TabFocusReason);
		//WIDGET->window()->setAttribute(Qt::WA_KeyboardFocusChange, false);
	}
	else if ((CWIDGET *)win != THIS)
	{
		//qDebug("delayed focus on %s for %s", THIS->name, ((CWIDGET *)win)->name);
		GB.Unref(POINTER(&win->focus));
		win->focus = THIS;
		GB.Ref(THIS);
	}
}

BEGIN_METHOD_VOID(Control_SetFocus)

	CWIDGET_set_focus(THIS);

END_METHOD


BEGIN_PROPERTY(Control_Tag)

	if (READ_PROPERTY)
	{
		if (THIS_EXT)
			GB.ReturnVariant(&THIS_EXT->tag);
		else
		{
			GB.ReturnNull();
			GB.ReturnConvVariant();
		}
	}
	else
		GB.StoreVariant(PROP(GB_VARIANT), POINTER(&(ENSURE_EXT(THIS)->tag)));

END_METHOD


BEGIN_PROPERTY(Control_Mouse)

	QWidget *wid;

	HANDLE_PROXY(_object);
	
	wid = QWIDGET(_object);
	
	if (READ_PROPERTY)
		GB.ReturnInteger(THIS_EXT ? THIS_EXT->mouse : CURSOR_DEFAULT);
	else
	{
		int mouse = VPROP(GB_INTEGER);
		
		if (mouse != CURSOR_DEFAULT || THIS_EXT)
			ENSURE_EXT(THIS)->mouse = mouse;
		
		set_mouse(wid, mouse, THIS_EXT ? THIS_EXT->cursor : NULL);
	}

END_METHOD


BEGIN_PROPERTY(Control_Cursor)

	HANDLE_PROXY(_object);
	
	if (READ_PROPERTY)
		GB.ReturnObject(THIS_EXT ? THIS_EXT->cursor : NULL);
	else
	{
		GB.StoreObject(PROP(GB_OBJECT), &(ENSURE_EXT(THIS)->cursor));
		set_mouse(WIDGET, CURSOR_CUSTOM, THIS_EXT->cursor);
		ENSURE_EXT(THIS)->mouse = CURSOR_CUSTOM;
	}

END_PROPERTY


BEGIN_PROPERTY(Control_NoTabFocus)

	HANDLE_PROXY(_object);
	
	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->flag.no_tab_focus);
	else
		THIS->flag.no_tab_focus = VPROP(GB_BOOLEAN);

END_PROPERTY


BEGIN_PROPERTY(Control_Direction)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->flag.direction);
	else
	{
		int dir = VPROP(GB_INTEGER);
		if (dir < 0 || dir > 2) dir = 0;
		THIS->flag.direction = dir;
		update_direction(THIS);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_RightToLeft)

	GB.ReturnBoolean(WIDGET->isRightToLeft());

END_PROPERTY


static QWidget *get_color_widget(CWIDGET *_object)
{
	QWidget *view = get_viewport(WIDGET);
	if (view)
		return view;

	return WIDGET;
}

QT_COLOR_FUNC CWIDGET_after_set_color(QT_COLOR_FUNC func)
{
	QT_COLOR_FUNC old = _after_set_color;
	_after_set_color = func;
	return old;
}


void CWIDGET_reset_color(CWIDGET *_object)
{
	GB_COLOR fg, bg;
	QPalette palette;
	QWidget *w;
	
	HANDLE_PROXY(_object);
	//qDebug("reset_color: %s", THIS->name);
	//qDebug("set_color: (%s %p) bg = %08X (%d) fg = %08X (%d)", GB.GetClassName(THIS), THIS, THIS->bg, w->backgroundRole(), THIS->fg, w->foregroundRole());
	
	w = get_color_widget(THIS);
	
	if (!THIS_EXT || (THIS_EXT->bg == COLOR_DEFAULT && THIS_EXT->fg == COLOR_DEFAULT))
	{
		w->setPalette(QPalette());
		w->setAutoFillBackground(THIS->flag.autoFillBackground); //!THIS->flag.noBackground && THIS->flag.fillBackground);
	}
	else
	{
		bg = THIS_EXT->bg;
		fg = THIS_EXT->fg;
		
		if (GB.Is(THIS, CLASS_TextArea))
		{
			palette = QPalette();

			if (bg != COLOR_DEFAULT)
			{
				palette.setColor(QPalette::Base, TO_QCOLOR(bg));
				palette.setColor(QPalette::Window, TO_QCOLOR(bg));
				palette.setColor(QPalette::Button, TO_QCOLOR(bg));
			}

			if (fg != COLOR_DEFAULT)
			{
				palette.setColor(QPalette::Text, TO_QCOLOR(fg));
				palette.setColor(QPalette::WindowText, TO_QCOLOR(fg));
				palette.setColor(QPalette::ButtonText, TO_QCOLOR(fg));
			}

			w->setPalette(palette);

			CTEXTAREA_set_foreground(THIS);
		}
		else
		{
			palette = QPalette();
		
			if (bg != COLOR_DEFAULT)
			{
				/*if (GB.Is(THIS, CLASS_Container))
				{
					palette.setColor(QPalette::Base, TO_QCOLOR(bg));
					palette.setColor(QPalette::Window, TO_QCOLOR(bg));
					palette.setColor(QPalette::Button, TO_QCOLOR(bg));
				}
				else*/
				palette.setColor(w->backgroundRole(), TO_QCOLOR(bg));
				w->setAutoFillBackground(!THIS->flag.noBackground && (THIS->flag.fillBackground || w->backgroundRole() == QPalette::Window));
			}
			else
				w->setAutoFillBackground(THIS->flag.autoFillBackground);
			
			if (fg != COLOR_DEFAULT)
			{
				//if (GB.Is(THIS, CLASS_Container))
				{
					palette.setColor(QPalette::Text, TO_QCOLOR(fg));
					palette.setColor(QPalette::WindowText, TO_QCOLOR(fg));
					palette.setColor(QPalette::ButtonText, TO_QCOLOR(fg));
				}
				//else
					//palette.setColor(w->foregroundRole(), TO_QCOLOR(fg));
			}
		
			w->setPalette(palette);
		}

	}
	
	//w->setAutoFillBackground(THIS->bg != COLOR_DEFAULT);
	
	
	if (_after_set_color)
		(*_after_set_color)(THIS);

	if (!GB.Is(THIS, CLASS_Container))
		return;
	
	if (GB.Is(THIS, CLASS_Window))
		CWINDOW_define_mask((CWINDOW *)THIS);
}


void CWIDGET_set_color(CWIDGET *_object, int bg, int fg, bool handle_proxy)
{
	if (handle_proxy) { HANDLE_PROXY(_object); }

	ENSURE_EXT(THIS);
	THIS_EXT->bg = bg;
	THIS_EXT->fg = fg;
	
	CWIDGET_reset_color(THIS);
}


GB_COLOR CWIDGET_get_background(CWIDGET *_object, bool handle_proxy)
{
	if (handle_proxy) { HANDLE_PROXY(_object); }

	return THIS_EXT ? THIS_EXT->bg : COLOR_DEFAULT;
}


GB_COLOR CWIDGET_get_real_background(CWIDGET *_object)
{
	GB_COLOR bg = CWIDGET_get_background(THIS);
	
	if (bg != COLOR_DEFAULT)
		return bg;

	return WIDGET->palette().color(WIDGET->backgroundRole()).rgb() & 0xFFFFFF;
	
	CWIDGET *parent = (CWIDGET *)CWIDGET_get_parent(THIS);
	
	if (parent)
		return CWIDGET_get_real_background(parent);
	else
		return QApplication::palette().color(QPalette::Window).rgb() & 0xFFFFFF;
}


GB_COLOR CWIDGET_get_foreground(CWIDGET *_object, bool handle_proxy)
{
	if (handle_proxy) { HANDLE_PROXY(_object); }

	return THIS_EXT ? THIS_EXT->fg : COLOR_DEFAULT;
}


GB_COLOR CWIDGET_get_real_foreground(CWIDGET *_object)
{
	GB_COLOR fg = CWIDGET_get_foreground(THIS);
	
	if (fg != COLOR_DEFAULT)
		return fg;

	CWIDGET *parent = (CWIDGET *)CWIDGET_get_parent(THIS);
	
	if (parent)
		return CWIDGET_get_real_foreground(parent);
	else
		return QApplication::palette().color(QPalette::WindowText).rgb() & 0xFFFFFF;
}


BEGIN_PROPERTY(Control_Background)

	if (THIS_EXT && THIS_EXT->proxy)
	{
		if (READ_PROPERTY)
			GB.GetProperty(THIS_EXT->proxy, "Background");
		else
		{
			GB_VALUE value;
			value.type = GB_T_INTEGER;
			value._integer.value = VPROP(GB_INTEGER);
			GB.SetProperty(THIS_EXT->proxy, "Background", &value);
		}
		
		return;
	}

	if (READ_PROPERTY)
		GB.ReturnInteger(CWIDGET_get_background(THIS));
	else
	{
		GB_COLOR col = VPROP(GB_INTEGER);
		if (col != CWIDGET_get_background(THIS))
			CWIDGET_set_color(THIS, col, CWIDGET_get_foreground(THIS));
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Foreground)

	if (THIS_EXT && THIS_EXT->proxy)
	{
		if (READ_PROPERTY)
			GB.GetProperty(THIS_EXT->proxy, "Foreground");
		else
		{
			GB_VALUE value;
			value.type = GB_T_INTEGER;
			value._integer.value = VPROP(GB_INTEGER);
			GB.SetProperty(THIS_EXT->proxy, "Foreground", &value);
		}

		return;
	}

	if (READ_PROPERTY)
		GB.ReturnInteger(CWIDGET_get_foreground(THIS));
	else
	{
		GB_COLOR col = VPROP(GB_INTEGER);
		if (col != CWIDGET_get_foreground(THIS))
			CWIDGET_set_color(THIS, CWIDGET_get_background(THIS), col);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Parent)

	GB.ReturnObject(CWIDGET_get_parent_container(THIS));

END_PROPERTY


BEGIN_PROPERTY(Control__Parent)

	GB.ReturnObject(CWIDGET_get_parent(THIS));

END_PROPERTY


BEGIN_PROPERTY(Control_Window)

	GB.ReturnObject(CWidget::getWindow(THIS));

END_PROPERTY


BEGIN_PROPERTY(Control_Id)

	GB.ReturnLong(CWIDGET_get_handle(THIS));

END_PROPERTY


/*static QString remove_ampersand(const QString &s)
{
	QString r;
	uint i;

	for (i = 0; i < s.length(); i++)
	{
		if (s[i] == '&')
		{
			i++;
			if (i < s.length())
				r += s[i];
		}
		else
		{
			r += s[i];
		}
	}

	return r;
}*/


BEGIN_PROPERTY(Control_Tooltip)

	//QWidget *w;

	if (READ_PROPERTY)
		RETURN_NEW_STRING(WIDGET->toolTip());
	else
	{
		QString tip = QSTRING_PROP();
		if (THIS->flag.inside)
		{
			if (tip.isEmpty())
				QToolTip::hideText();
			else if (QToolTip::isVisible())
			{
				QToolTip::hideText();
				QToolTip::showText(QCursor::pos(), tip, WIDGET);
			}
		}
		WIDGET->setToolTip(tip);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Name)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->name);
	else
		CWIDGET_set_name(THIS, GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(Control_Action)

	char *current = THIS_EXT ? THIS_EXT->action : NULL;

	if (READ_PROPERTY)
		GB.ReturnString(current);
	else
	{
		char *action = PLENGTH() ? GB.NewString(PSTRING(), PLENGTH()) : NULL;
		
		CACTION_register(THIS, current, action);
		
		if (THIS_EXT)
			GB.FreeString(&THIS_EXT->action);
		
		if (action)
			ENSURE_EXT(THIS)->action = action;
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Proxy)

	if (READ_PROPERTY)
		GB.ReturnObject(THIS_EXT ? THIS_EXT->proxy : NULL);
	else
		CWIDGET_register_proxy(THIS, VPROP(GB_OBJECT));

END_PROPERTY


BEGIN_PROPERTY(Control_PopupMenu)

	if (READ_PROPERTY)
		GB.ReturnString(THIS_EXT ? THIS_EXT->popup : NULL);
	else
		GB.StoreString(PROP(GB_STRING), &(ENSURE_EXT(THIS)->popup));

END_PROPERTY


/*BEGIN_METHOD_VOID(Control_Screenshot)

	GB.ReturnObject(CPICTURE_grab(QWIDGET(_object)));

END_METHOD*/


BEGIN_METHOD(Control_Drag, GB_VARIANT data; GB_STRING format)

	static GB_FUNCTION func;
	static bool init = FALSE;

	if (!init)
	{
		GB.GetFunction(&func, (void *)GB.FindClass("Drag"), "_call", NULL, NULL);
		init = TRUE;
	}

	GB.Push(2, GB_T_OBJECT, THIS, GB_T_VARIANT, &VARG(data));
	if (MISSING(format))
	{
		GB.Call(&func, 2, FALSE);
	}
	else
	{
		GB.Push(1, GB_T_STRING, STRING(format), LENGTH(format));
		GB.Call(&func, 3, FALSE);
	}

END_METHOD


BEGIN_METHOD(Control_Reparent, GB_OBJECT container; GB_INTEGER x; GB_INTEGER y)

	QPoint p(WIDGET->pos());
	bool show;

	if (!MISSING(x) && !MISSING(y))
	{
		p.setX(VARG(x));
		p.setY(VARG(y));
	}

	if (GB.CheckObject(VARG(container)))
		return;

	show = CWIDGET_is_visible(THIS);
	CWIDGET_set_visible(THIS, false);
	WIDGET->setParent(QCONTAINER(VARG(container)));
	WIDGET->move(p);
	CCONTAINER_insert_child(THIS);
	CWIDGET_set_visible(THIS, show);

END_METHOD


BEGIN_PROPERTY(Control_Drop)

	HANDLE_PROXY(_object);

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->flag.drop);
	else
	{
		THIS->flag.drop = VPROP(GB_BOOLEAN);
		if (THIS->flag.scrollview)
			get_viewport(WIDGET)->setAcceptDrops(VPROP(GB_BOOLEAN));
		else
			WIDGET->setAcceptDrops(VPROP(GB_BOOLEAN));
	}

END_PROPERTY

static bool has_tracking(CWIDGET *_object)
{
	HANDLE_PROXY(_object);
	return THIS->flag.tracking;
}

BEGIN_PROPERTY(Control_Tracking)

	HANDLE_PROXY(_object);

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->flag.tracking);
	else
	{
		if (VPROP(GB_BOOLEAN) != THIS->flag.tracking)
		{
			THIS->flag.tracking = VPROP(GB_BOOLEAN);
			if (THIS->flag.tracking)
			{
				THIS->flag.old_tracking = WIDGET->hasMouseTracking();
				WIDGET->setMouseTracking(true);
			}
			else
			{
				WIDGET->setMouseTracking(THIS->flag.old_tracking);
			}
		}
	}
	
END_PROPERTY


BEGIN_PROPERTY(CWIDGET_border_full)

	QFrame *wid = (QFrame *)QWIDGET(_object);
	int border, lw;

	if (READ_PROPERTY)
	{
		if (wid->frameStyle() == (QFrame::Box + QFrame::Plain) && wid->lineWidth() == 1)
			border = BORDER_PLAIN;
		else if (wid->frameStyle() == (QFrame::StyledPanel + QFrame::Sunken))
			border = BORDER_SUNKEN;
		else if (wid->frameStyle() == (QFrame::StyledPanel + QFrame::Raised))
			border = BORDER_RAISED;
		else if (wid->frameStyle() == (QFrame::StyledPanel + QFrame::Plain))
			border = BORDER_ETCHED;
		else
			border = BORDER_NONE;

		GB.ReturnInteger(border);
	}
	else
	{
		switch (VPROP(GB_INTEGER))
		{
			case BORDER_PLAIN: border = QFrame::Box + QFrame::Plain; lw = 1; break;
			case BORDER_SUNKEN: border = QFrame::StyledPanel + QFrame::Sunken; lw = 2; break;
			case BORDER_RAISED: border = QFrame::StyledPanel + QFrame::Raised; lw = 2; break;
			case BORDER_ETCHED: border = QFrame::StyledPanel + QFrame::Plain; lw = 2; break;
			default: border = QFrame::NoFrame; lw = 0; break;
		}

		wid->setFrameStyle(border);
		wid->setLineWidth(lw);
		wid->update();
	}

END_PROPERTY


BEGIN_PROPERTY(CWIDGET_border_simple)

	QFrame *wid = (QFrame *)QWIDGET(_object);

	if (READ_PROPERTY)
	{
		GB.ReturnBoolean(wid->frameStyle() != QFrame::NoFrame);
	}
	else
	{
		//qDebug("frameStyle = %d", wid->frameStyle());

		if (VPROP(GB_BOOLEAN))
		{
			wid->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
			//wid->setFrameStyle(QFrame::LineEditPanel);
			wid->setLineWidth(2);
		}
		else
		{
			wid->setFrameStyle(QFrame::NoFrame);
			wid->setLineWidth(0);
		}

		//qDebug("--> %s %d %d %d %d", THIS->name, wid->contentsRect().x(), wid->contentsRect().y(), wid->contentsRect().width(), wid->contentsRect().height());
		//wid->style()->polish(wid);
		wid->update();
	}

END_PROPERTY


BEGIN_PROPERTY(CWIDGET_scrollbar)

	QAbstractScrollArea *wid = qobject_cast<QAbstractScrollArea *>(WIDGET);
	//Q3ScrollView *sw = qobject_cast<Q3ScrollView *>(WIDGET);
	int scroll;

	if (wid)
	{
		if (READ_PROPERTY)
		{
			scroll = 0;
			if (wid->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded)
				scroll += 1;
			if (wid->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded)
				scroll += 2;
	
			GB.ReturnInteger(scroll);
		}
		else
		{
			scroll = VPROP(GB_INTEGER) & 3;
			wid->setHorizontalScrollBarPolicy( (scroll & 1) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
			wid->setVerticalScrollBarPolicy( (scroll & 2) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
		}
	}
	/*else if (sw)
	{
		if (READ_PROPERTY)
		{
			scroll = 0;
			if (sw->hScrollBarMode() == Q3ScrollView::Auto)
				scroll += 1;
			if (sw->vScrollBarMode() == Q3ScrollView::Auto)
				scroll += 2;
	
			GB.ReturnInteger(scroll);
		}
		else
		{
			scroll = VPROP(GB_INTEGER) & 3;
			sw->setHScrollBarMode( (scroll & 1) ? Q3ScrollView::Auto : Q3ScrollView::AlwaysOff);
			sw->setVScrollBarMode( (scroll & 2) ? Q3ScrollView::Auto : Q3ScrollView::AlwaysOff);
		}
	}*/

END_PROPERTY

void CWIDGET_grab(CWIDGET *_object)
{
	QEventLoop eventLoop;
	QEventLoop *old;
	
	if (THIS->flag.grab)
		return;
	
	THIS->flag.grab = true;
	WIDGET->grabMouse(WIDGET->cursor());
	WIDGET->grabKeyboard();

	old = MyApplication::eventLoop;
	MyApplication::eventLoop = &eventLoop;
	eventLoop.exec();
	MyApplication::eventLoop = old;
	
	WIDGET->releaseMouse();
	WIDGET->releaseKeyboard();
	THIS->flag.grab = false;

}

BEGIN_METHOD_VOID(Control_Grab)

	CWIDGET_grab(THIS);

END_METHOD


/* Classe CWidget */

CWidget CWidget::manager;
QHash<QObject *, CWIDGET *> CWidget::dict;
bool CWidget::real;

#if 0
bool haveChildren;

void CWidget::installFilter(QObject *o)
{
	QObjectList *children;
	QObject *child;

	children = (QObjectList *)(o->children());

	o->installEventFilter(&manager);

	if (!children)
		return;

	child = children->first();
	while (child)
	{
		if (child->isWidgetType())
		{
			haveChildren = true;
			CWidget::installFilter(child);
		}

		child = children->next();
	}
}

void CWidget::removeFilter(QObject *o)
{
	QObjectList *children = (QObjectList *)(o->children());
	QObject *child;

	if (!o->isWidgetType())
		return;

	o->removeEventFilter(&manager);

	if (!children)
		return;

	child = children->first();
	while (child)
	{
		CWidget::removeFilter(child);
		child = children->next();
	}
}
#endif

void CWidget::removeFocusPolicy(QWidget *w)
{
	QObjectList children;
	int i;
	QObject *child;

	w->clearFocus();
	w->setFocusPolicy(Qt::NoFocus);

	children = w->children();

	for (i = 0; i < children.count(); i++)
	{
		child = children.at(i);
		
		if (child->isWidgetType())
			CWidget::removeFocusPolicy((QWidget *)child);
	}
}


void CWidget::add(QObject *o, void *object, bool no_filter)
{
	//if (!no_filter)
	QObject::connect(o, SIGNAL(destroyed()), &manager, SLOT(destroy()));

	dict.insert(o, (CWIDGET *)object);

	/*
	if (!no_filter)
	{
		haveChildren = false;
		CWidget::installFilter(o);
		if (haveChildren)
			CWIDGET_set_flag(object, WF_NO_EVENT);
	}
	*/

	GB.Ref(object);
}

CWIDGET *CWidget::get(QObject *o)
{
	CWIDGET *ob;

	real = true;

	while (o)
	{
		ob = dict[o];
		if (ob)
			return ob;
		if (((QWidget *)o)->isWindow())
			return NULL;

		o = o->parent();
		real = false;
	}

	return NULL;
}

CWIDGET *CWidget::getRealExisting(QObject *o)
{
	CWIDGET *_object = dict[o];
	
	if (THIS && THIS->flag.deleted)
		_object = NULL;
	
	return _object;
}


CWIDGET *CWidget::getDesign(QObject *o)
{
	CWIDGET *ob;

	if (!o->isWidgetType())
		return NULL;

	real = true;

	while (o)
	{
		ob = dict[o];
		if (ob && !ob->flag.design_ignore)
			return ob;
		if (((QWidget *)o)->isWindow())
			return NULL;

		o = o->parent();
		real = false;
	}

	return NULL;
}

QWidget *CWidget::getContainerWidget(CCONTAINER *object)
{
	if (GB.CheckObject(object))
		GB.Propagate();

	if (object->container == NULL)
	{
		GB.Error("Null container");
		GB.Propagate();
	}

	//fprintf(stderr, "container: (%p %s)\n", object, object->widget.name);

	return (object->container);
}

CWINDOW *CWidget::getWindow(CWIDGET *ob)
{
	//QWidget *p = w->parentWidget();
	for(;;)
	{
		if (GB.Is(ob, CLASS_Window)) // && ((CWINDOW *)ob)->window)
			break;

		ob = CWidget::get(QWIDGET(ob)->parentWidget());
		if (!ob)
			break;
	}

	return (CWINDOW *)ob;
}


CWINDOW *CWidget::getTopLevel(CWIDGET *ob)
{
	//QWidget *p = w->parentWidget();
	for(;;)
	{
		if (GB.Is(ob, CLASS_Window) && ((CWINDOW *)ob)->toplevel)
			break;

		ob = CWidget::get(QWIDGET(ob)->parentWidget());
		if (!ob)
			break;
	}

	return (CWINDOW *)ob;
}


#if 0
void CWidget::setName(CWIDGET *object, const char *name)
{
	QWidget *w = QWIDGET(object);
	CTOPLEVEL *top = (CTOPLEVEL *)CWidget::get(w->topLevelWidget());

	if (QWIDGET(top) == w)
		return;

	if (w->name() != NULL)
	{
		/*qDebug("- %s", w->name());*/
		top->dict->remove(w->name());
	}

	if (name != NULL)
	{
		top->dict->insert((const char *)name, object);
		w->setName(name);
		/*qDebug("+ %s", w->name());*/
	}
}
#endif

#define CLEAN_POINTER(_ptr) if ((_ptr) == THIS) _ptr = NULL

void CWidget::destroy()
{
	QWidget *w = (QWidget *)sender();
	CWIDGET *_object = CWidget::get(w);
	CWINDOW *win;

	if (!THIS)
		return;

	/*fprintf(stderr, "CWidget::destroy: (%s %p) %s / proxy = %p / proxy_for = %p\n", GB.GetClassName(THIS), THIS, THIS->name,
					THIS_EXT ? THIS_EXT->proxy : NULL, THIS_EXT ? THIS_EXT->proxy_for : NULL);*/

	if (!_post_check_hovered)
	{
		CWIDGET *top = (CWIDGET *)CWidget::getTopLevel(THIS);
		if (top == THIS)
			top = NULL;
		_post_check_hovered = true;
		_post_check_hovered_window = top;
		GB.Post((void (*)())post_check_hovered, (intptr_t)0);
	}
	
	CLEAN_POINTER(_hovered);
	CLEAN_POINTER(_official_hovered);
	CLEAN_POINTER(_post_check_hovered_window);
	CLEAN_POINTER(CWIDGET_active_control);
	CLEAN_POINTER(CWIDGET_previous_control);
	CLEAN_POINTER(CWIDGET_hovered);
	CLEAN_POINTER(_old_active_control);
#if QT5
	CLEAN_POINTER(_last_entered);
#endif
	
	win = CWINDOW_Current;
	while (win)
	{
		CLEAN_POINTER(win->save_focus);
		win = win->previous;
	}

	if (THIS_EXT)
	{
		if (THIS_EXT->proxy)
			EXT(THIS_EXT->proxy)->proxy_for = NULL;
		if (THIS_EXT->proxy_for)
			EXT(THIS_EXT->proxy_for)->proxy = NULL;
		
		CACTION_register(THIS, THIS_EXT->action, NULL);
		GB.FreeString(&THIS_EXT->action);

		if (THIS_EXT->container_for)
		{
			((CCONTAINER *)THIS_EXT->container_for)->container = ((CWIDGET *)THIS_EXT->container_for)->widget;
			THIS_EXT->container_for = NULL;
		}
	
		GB.Unref(POINTER(&THIS_EXT->cursor));
		GB.FreeString(&THIS_EXT->popup);
		GB.StoreVariant(NULL, &THIS_EXT->tag);
		GB.Free(POINTER(&THIS->ext));
	}
	
	CWIDGET_set_name(THIS, 0);

	dict.remove(w);

	QWIDGET(THIS) = NULL;
	GB.Unref(POINTER(&THIS->font));
	
	//qDebug(">> CWidget::destroy %p (%p) :%p:%ld #2", ob, ob->widget, ob->ob.klass, ob->ob.ref);
	//if (!CWIDGET_test_flag(ob, WF_NODETACH))
	GB.Detach(THIS);
	
	GB.Unref(POINTER(&_object));
}

void CWidget::each(void (*func)(CWIDGET *))
{
	QHashIterator<QObject *, CWIDGET *> i(dict);
	while (i.hasNext())
	{
		i.next();
		(*func)(i.value());
	}
}

/*static void post_dblclick_event(void *control)
{
	GB.Raise(control, EVENT_DblClick, 0);
	GB.Unref(&control);
}*/

static void post_focus_change(void *)
{
	CWIDGET *current, *control;
	
	//fprintf(stderr, "post_focus_change: %p / %d %d\n", CWIDGET_active_control, !_focus_change, _doing_focus_change);

	if (!_focus_change || _doing_focus_change)
		return;
	
	_doing_focus_change = true;
	
	for(;;)
	{
		current = CWIDGET_active_control;
		if (current == _old_active_control)
			break;

		control = _old_active_control;
		//if (control) fprintf(stderr, "check focus out %s\n", control->name);
		while (control)
		{
			//fprintf(stderr, "post_focus_change: %s lost focus\n", control->name);
			GB.Raise(control, EVENT_LostFocus, 0);
			control = (CWIDGET *)(EXT(control) ? EXT(control)->proxy_for : NULL);
		}
		
		_old_active_control = current;
		CWINDOW_activate(current);
		
		control = current;
		//if (control) fprintf(stderr, "check focus in %s\n", control->name);
		while (control)
		{
			//fprintf(stderr, "post_focus_change: %s got focus\n", control->name);
			GB.Raise(control, EVENT_GotFocus, 0);
			control = (CWIDGET *)(EXT(control) ? EXT(control)->proxy_for : NULL);
		}
	}
	
	_focus_change = false;
	_doing_focus_change = false;
	
	//fprintf(stderr, "post_focus_change: END\n");
}

static void handle_focus_change()
{
	if (_focus_change)
		return;
	
	_focus_change = TRUE;
	GB.Post((void (*)())post_focus_change, (intptr_t)NULL);
}

void CWIDGET_finish_focus(void)
{
	post_focus_change(NULL);
}

void CWIDGET_handle_focus(CWIDGET *control, bool on) 
{
	if (on == (CWIDGET_active_control == control))
		return;
	
	//fprintf(stderr, "CWIDGET_handle_focus: %s %d / %d\n", control->name, on, _focus_change);
	
	if (CWIDGET_active_control && !_focus_change)
		CWIDGET_previous_control = CWIDGET_active_control;
	CWIDGET_active_control = on ? control : NULL;
	handle_focus_change();
}

static bool raise_key_event_to_parent_window(CWIDGET *control, int event)
{
	for(;;)
	{
		if (!control || control->flag.deleted || !QWIDGET(control))
			break;

		control = (CWIDGET *)CWIDGET_get_parent(control);
		
		if (!control || control->flag.deleted || !QWIDGET(control))
			break;
		
		control = (CWIDGET *)CWidget::getWindow(control);
		if (GB.Raise(control, event, 0))
			return true;
	}
	
	return false;
}

bool CWidget::eventFilter(QObject *widget, QEvent *event)
{
	CWIDGET *control;
	int event_id;
	int type = event->type();
	bool real;
	bool design;
	bool original;
	bool cancel;
	QPoint p;
	void *jump;
	bool parent_got_it;
	double timer;

	CCONTROL_last_event_type = type;

	//if (widget->isA("MyMainWindow"))
	//	getDesignDebug(widget);
	switch (type)
	{
		case QEvent::Enter: 
			jump = &&__ENTER; break;
		case QEvent::Leave: 
			jump = &&__LEAVE; break;
		case QEvent::FocusIn:
			jump = &&__FOCUS_IN; break;
		case QEvent::FocusOut:
			jump = &&__FOCUS_OUT; break;
		case QEvent::ContextMenu:
			jump = &&__CONTEXT_MENU; break;
		case QEvent::MouseButtonPress:
		case QEvent::MouseMove:
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseButtonRelease:
			jump = &&__MOUSE; break;
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			jump = &&__KEY; break;
		case QEvent::Shortcut:
			jump = &&_DESIGN; break;
		case QEvent::InputMethod:
			jump = &&__INPUT_METHOD; break;
		case QEvent::Wheel:
			jump = &&__MOUSE_WHEEL; break;
		case QEvent::DragEnter:
			jump = &&__DRAG_ENTER; break;
		case QEvent::DragMove:
			jump = &&__DRAG_MOVE; break;
		case QEvent::Drop:
			jump = &&__DROP; break;
		case QEvent::DragLeave:
			jump = &&__DRAG_LEAVE; break;
		case QEvent::DeferredDelete:
			control = CWidget::getDesign(widget);
			if (!control || control->flag.deleted)
			{
				QObject::eventFilter(widget, event); 
				return false;
			}
			else
				goto _STANDARD;
		case QEvent::TabletMove:
		case QEvent::TabletPress:
		case QEvent::TabletRelease:
			jump = &&__TABLET; break;
		default:
			goto _STANDARD;
	}
	
	control = CWidget::getDesign(widget);
	//for(;;)
	//{
		if (!control || GB.Is(control, CLASS_Menu))
			goto _STANDARD;
	//	if (control->widget->isEnabled())
	//		break;
	//	control = (CWIDGET *)CWIDGET_get_parent(control);
	//}

	real = CWidget::real;
	design = CWIDGET_is_design(control); //CWIDGET_test_flag(control, WF_DESIGN); // && !GB.Is(control, CLASS_Container);
	original = event->spontaneous();
	
	goto *jump;

	__ENTER:
	{
#ifndef QT5
		QWidget *popup = qApp->activePopupWidget();
#endif
		
		//qDebug("enter %p %s real = %d inside = %d", widget, control->name, real, control->flag.inside);
		
		if (real)
		{
#ifdef QT5
			CWIDGET_enter(control);
#else
			if (!popup || CWidget::getReal(popup))
				CWIDGET_enter(control);
			else if (_enter_leave_set)
				insert_enter_leave_event(control, true);
#endif
		}
		
		goto __NEXT;
	}

	__LEAVE:
	{
#ifndef QT5
		QWidget *popup = qApp->activePopupWidget();
#endif
		
		//qDebug("leave %p %s real = %d inside = %d", widget, control->name, real, control->flag.inside);
		
		if (real)
		{
#ifdef QT5
			CWIDGET_leave(control);
#else
			if (!popup || CWidget::getReal(popup))
				CWIDGET_leave(control);
			else if (_enter_leave_set)
				insert_enter_leave_event(control, false);
#endif
		}
		
		goto __NEXT;
	}
	
	__FOCUS_IN:
	{
		CWIDGET_handle_focus(control, true);
		goto __NEXT;
	}
	
	__FOCUS_OUT:
	{
		CWIDGET_handle_focus(control, false);
		goto __NEXT;
	}
	
	__CONTEXT_MENU:
	{
		while (EXT(control) && EXT(control)->proxy_for)
			control = (CWIDGET *)(EXT(control)->proxy_for);

	__MENU_TRY_PROXY:
	
		// if (real && GB.CanRaise(control, EVENT_Menu))
		//qDebug("Menu event! %p %d", control, EVENT_Menu);
		if (GB.CanRaise(control, EVENT_Menu))
		{
			int old = MENU_popup_count;

			((QContextMenuEvent *)event)->accept();

			if (GB.Raise(control, EVENT_Menu, 0) || MENU_popup_count != old)
				return true;
			//else
			//	goto __NEXT;
		}

		if (EXT(control) && EXT(control)->popup)
		{
			CWINDOW *window = CWidget::getWindow(control);
			CMENU *menu = CWindow::findMenu(window, EXT(control)->popup);
			if (menu)
				CMENU_popup(menu, QCursor::pos());
			return true;
		}

		if (EXT(control) && EXT(control)->proxy)
		{
			control = (CWIDGET *)(EXT(control)->proxy);
			goto __MENU_TRY_PROXY;
		}
		
		goto __NEXT;		
	}
	
	__MOUSE:
	{
		QMouseEvent *mevent = (QMouseEvent *)event;
		
		if (!original)
			goto _DESIGN;
		
		/*if (type == QEvent::MouseButtonPress)
		{
			qDebug("mouse event on [%s %s %p] (%s %p) %s%s%s", widget->metaObject()->className(), qPrintable(widget->objectName()), widget,
						control ? GB.GetClassName(control) : "-", control, real ? "REAL " : "", design ? "DESIGN " : "", original ? "ORIGINAL ": "");
			//getDesignDebug(widget);
			fprintf(stderr, "MouseButtonPress: %s (%p)\n", control ? GB.GetClassName(control) : "-", control);
		}*/
		
		if (!real)
		{
			CWIDGET *cont = CWidget::get(widget);
			if (cont->flag.scrollview)
			{
				if (qobject_cast<QScrollBar *>(widget))
					goto _STANDARD;
				/*if (widget != get_viewport(QWIDGET(cont)))
				{
					if (!widget->objectName().isNull())
						goto _STANDARD;
				}*/
			}
		}
		
		if (type == QEvent::MouseMove)
		{
			// Ignore spurious move events

			static int last_move_x = -1;
			static int last_move_y = -1;

			if (last_move_x == mevent->globalX() && last_move_y == mevent->globalY())
			{
				//fprintf(stderr, "same mouse move! %d %d\n", last_move_x, last_move_y);
				goto _STANDARD;
			}

			last_move_x = mevent->globalX();
			last_move_y = mevent->globalY();
		}

		if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick)
		{
			GB.GetTime(&timer, TRUE);
			if (((timer - MOUSE_timer) * 1000) < QApplication::doubleClickInterval() 
				  && abs(mevent->globalX() - MOUSE_click_x) < MAIN_scale
				  && abs(mevent->globalY() - MOUSE_click_y) < MAIN_scale)
				MOUSE_click_count++;
			else
			{
				MOUSE_click_x = mevent->globalX();
				MOUSE_click_y = mevent->globalY();
				MOUSE_click_count = 1;
			}
			MOUSE_timer = timer;
		}
		
		//while (control->proxy_for)
		//	control = (CWIDGET *)control->proxy_for;

	__MOUSE_TRY_PROXY:
	
		if (!design && !QWIDGET(control)->isEnabled())
			goto __NEXT;
	
		p.setX(mevent->globalX());
		p.setY(mevent->globalY());
		p = QWIDGET(control)->mapFromGlobal(p);
		
		switch(type)
		{
			case QEvent::MouseButtonPress:
				event_id = EVENT_MouseDown;
				//state = mevent->buttons();
				MOUSE_info.sx = p.x();
				MOUSE_info.sy = p.y();
				CMOUSE_set_control(control);
				break;
				
			case QEvent::MouseButtonDblClick:
				event_id = EVENT_DblClick;
				break;
				
			case QEvent::MouseButtonRelease:
				event_id = EVENT_MouseUp;
				CMOUSE_set_control(NULL);
				break;
				
			default:
				event_id = EVENT_MouseMove;
				if (mevent->buttons() == Qt::NoButton && !has_tracking(control))
					goto _DESIGN;
		}

		/* GB.Raise() can free the control, so we must reference it as we may raise two successive events now */
		GB.Ref(control);
		cancel = false;

		if (GB.CanRaise(control, event_id) || (event_id == EVENT_DblClick && GB.CanRaise(control, EVENT_MouseDown)))
		{
			/*if (!design && CWIDGET_test_flag(control, WF_SCROLLVIEW))
			{
				if (widget != ((QScrollView *)QWIDGET(control))->viewport()
						&& widget->name(0))
				{
					qDebug("cancel");
					goto _DESIGN;
				}
			}*/
			
			CMOUSE_clear(true);
			MOUSE_info.x = p.x();
			MOUSE_info.y = p.y();
			MOUSE_info.screenX = mevent->globalX();
			MOUSE_info.screenY = mevent->globalY();
			MOUSE_info.button = mevent->button();
			MOUSE_info.state = mevent->buttons();
			MOUSE_info.modifier = mevent->modifiers();

			if (event_id == EVENT_DblClick)
				cancel = GB.Raise(control, EVENT_MouseDown, 0); //, GB_T_INTEGER, p.x(), GB_T_INTEGER, p.y(), GB_T_INTEGER, state);
				
			if (!cancel)
				cancel = GB.Raise(control, event_id, 0); //, GB_T_INTEGER, p.x(), GB_T_INTEGER, p.y(), GB_T_INTEGER, state);

			CMOUSE_clear(false);
			
			/*if (CDRAG_dragging)
				return true;*/
		}
		
		if (event_id == EVENT_MouseMove && !cancel && (mevent->buttons() != Qt::NoButton) && GB.CanRaise(control, EVENT_MouseDrag) && !CDRAG_dragging
				&& ((abs(p.x() - MOUSE_info.sx) + abs(p.y() - MOUSE_info.sy)) > 8)) // QApplication::startDragDistance()))
		{		
			/*if (!design && CWIDGET_test_flag(control, WF_SCROLLVIEW))
			{
				if (widget != ((QScrollView *)QWIDGET(control))->viewport()
						&& widget->name(0))
				{
					goto _DESIGN;
				}
			}*/
			
			CMOUSE_clear(true);
			MOUSE_info.x = p.x();
			MOUSE_info.y = p.y();
			MOUSE_info.screenX = mevent->globalX();
			MOUSE_info.screenY = mevent->globalY();
			MOUSE_info.button = mevent->button();
			MOUSE_info.state = mevent->buttons();
			MOUSE_info.modifier = mevent->modifiers();
		
			//qDebug("MouseDrag: %s", control->name);
			cancel = GB.Raise(control, EVENT_MouseDrag, 0);
			
			CMOUSE_clear(false);
		}
	
		GB.Unref(POINTER(&control));
		
		if (!control)
			goto __MOUSE_RETURN_TRUE;

		if (control->flag.grab && event_id == EVENT_MouseUp)
			MyApplication::eventLoop->exit();
		
		if (cancel)
			goto __MOUSE_RETURN_TRUE;

		if (EXT(control) && EXT(control)->proxy_for)
		{
			control = (CWIDGET *)(EXT(control)->proxy_for);
			goto __MOUSE_TRY_PROXY;
		}
		
		CMOUSE_reset_translate();
		goto __NEXT;

	__MOUSE_RETURN_TRUE:

		CMOUSE_reset_translate();
		return true;
	}
	
	__TABLET:
	{
		QTabletEvent *tevent = (QTabletEvent *)event;

		if (!original)
			goto _DESIGN;

		if (!real)
		{
			CWIDGET *cont = CWidget::get(widget);
			if (cont->flag.scrollview)
			{
				if (qobject_cast<QScrollBar *>(widget))
					goto _STANDARD;
				/*if (widget != get_viewport(QWIDGET(cont)))
				{
					if (!widget->objectName().isNull())
						goto _STANDARD;
				}*/
			}
		}
		
		if (!control->flag.use_tablet)
			goto __NEXT;
		
	__TABLET_TRY_PROXY:
	
		p.setX(tevent->globalX());
		p.setY(tevent->globalY());
		p = QWIDGET(control)->mapFromGlobal(p);
		
		if (type == QEvent::TabletPress)
		{
			//qDebug("MouseDown on %p (%s %p) %s%s", widget, control ? GB.GetClassName(control) : "-", control, real ? "REAL " : "", design ? "DESIGN " : "");

			event_id = EVENT_MouseDown;
			//state = mevent->buttons();
			
			//MOUSE_info.sx = p.x();
			//MOUSE_info.sy = p.y();
			
			control->flag.tablet_pressed = true;
			//qDebug("MouseEvent: %d %d", mevent->x(), mevent->y());
		}
		else if (type == QEvent::TabletMove)
		{
			//if (!control->flag.tracking && !control->flag.tablet_pressed)
			//	return false;
			
			event_id = EVENT_MouseMove;
		}
		else //if (type == QEvent::TabletRelease)
		{
			event_id = EVENT_MouseUp;
			//state = mevent->buttons();
		}

		//if (event_id == EVENT_MouseMove && mevent->buttons() == Qt::NoButton && !QWIDGET(control)->hasMouseTracking())
		//	goto _DESIGN;


		cancel = false;

		if (GB.CanRaise(control, event_id))
		{
			//MOUSE_info.x = p.x();
			//MOUSE_info.y = p.y();
			//POINTER_info.screenX = tevent->globalX();
			//POINTER_info.screenY = tevent->globalY();
			//MOUSE_info.modifier = tevent->modifiers();
#ifdef QT5
			POINTER_info.tx = tevent->globalPosF().x();
			POINTER_info.ty = tevent->globalPosF().y();
#else
			POINTER_info.tx = tevent->hiResGlobalX();
			POINTER_info.ty = tevent->hiResGlobalY();
#endif
			POINTER_info.pressure = tevent->pressure();
			POINTER_info.rotation = tevent->rotation();
			POINTER_info.xtilt = tevent->xTilt();
			POINTER_info.ytilt = tevent->yTilt();
			
			switch(tevent->pointerType())
			{
				case QTabletEvent::Pen: POINTER_info.type = POINTER_PEN; break;
				case QTabletEvent::Eraser: POINTER_info.type = POINTER_ERASER; break;
				case QTabletEvent::Cursor: POINTER_info.type = POINTER_CURSOR; break;
				default: POINTER_info.type = POINTER_MOUSE;
			}

			//cancel = GB.Raise(control, event_id, 0);

			//CMOUSE_clear(false);
		}
		
		//if (control->flag.grab && event_id == EVENT_MouseUp)
		//	MyApplication::eventLoop->exit();
		
		if (event_id == EVENT_MouseUp)
			control->flag.tablet_pressed = false;
		
		//if (cancel)
		//	return true;
		
		if (EXT(control) && EXT(control)->proxy_for)
		{
			control = (CWIDGET *)(EXT(control)->proxy_for);
			goto __TABLET_TRY_PROXY;
		}
		
		CMOUSE_reset_translate();
		return false; // We fill the information, and then expect Qt to generate a Mouse event from the Tablet event
	}
	
	__KEY:
	{
		QKeyEvent *kevent = (QKeyEvent *)event;

		if (MAIN_key_debug)
		{
			qDebug(QT_NAME ": %s: real = %d original = %d no_keyboard = %d",
				(type == QEvent::KeyRelease ? "KeyRelease" :
				 (type == QEvent::KeyPress ? "KeyPress" : "?")),
				real, original, control->flag.no_keyboard);
		}
		
		#if QT_VERSION <= 0x030005
		if (!real || !original)
			goto _DESIGN;
		#endif

		if (control->flag.no_keyboard)
			goto _DESIGN;

		event_id = (type == QEvent::KeyRelease) ? EVENT_KeyRelease : EVENT_KeyPress;
		cancel = false;
		parent_got_it = false;

		#if QT_VERSION > 0x030005
		if (!original && type != QEvent::InputMethod)
			goto _DESIGN; //_ACCEL;
		#endif

		if (type == QEvent::KeyRelease && kevent->isAutoRepeat())
			goto __NEXT;
		
		if (MAIN_key_debug)
		{
			qDebug(QT_NAME ": (%s %s) -> %d `%s' %s",
				GB.GetClassName(control), control->name,
				kevent->key(), (const char *)kevent->text().toLatin1(), kevent->isAutoRepeat() ? "AR" : "--");
		}
		//qDebug("CWidget::eventFilter: KeyPress on %s %p", GB.GetClassName(control), control);
			
	__KEY_TRY_PROXY:
			
		CKEY_clear(true);

		GB.FreeString(&CKEY_info.text);
		CKEY_info.text = NEW_STRING(kevent->text());
		CKEY_info.state = kevent->modifiers();
		CKEY_info.code = kevent->key();
		CKEY_info.release = type == QEvent::KeyRelease;
		
		#ifndef NO_X_WINDOW
		{
			#ifdef QT5
				int last = PLATFORM.GetLastKeyCode();
			#else
				int last = MAIN_x11_last_key_code;
			#endif
			if (type == QEvent::KeyPress && CKEY_info.code)
				_x11_to_qt_keycode.insert(last, CKEY_info.code);
			else if (type == QEvent::KeyRelease && CKEY_info.code == 0)
			{
				if (_x11_to_qt_keycode.contains(last))
				{
					CKEY_info.code = _x11_to_qt_keycode[last];
					_x11_to_qt_keycode.remove(last);
				}
			}
		}
		#endif
		
		GB.Ref(control);
		
		if (!parent_got_it)
		{
			parent_got_it = true;
			if (!cancel)
				cancel = raise_key_event_to_parent_window(control, event_id);
		}
		
		if (!cancel)
			cancel = GB.Raise(control, event_id, 0);

		GB.Unref(POINTER(&control));
		
		CKEY_clear(false);

		if ((cancel && (type != QEvent::KeyRelease)) || !control)
			return true;

		if (EXT(control) && EXT(control)->proxy_for)
		{
			control = (CWIDGET *)(EXT(control)->proxy_for);
			goto __KEY_TRY_PROXY;
		}
		
		if (control->flag.grab && event_id == EVENT_KeyPress && kevent->key() == Qt::Key_Escape)
			MyApplication::eventLoop->exit();

		goto __NEXT;
	}
	
	__INPUT_METHOD:
	{
		QInputMethodEvent *imevent = (QInputMethodEvent *)event;

		if (MAIN_key_debug)
		{
			qDebug(QT_NAME ": InputMethod: real = %d original = %d no_keyboard = %d",
				real, original, control->flag.no_keyboard);
		}
		
		#if QT_VERSION <= 0x030005
		if (!real || !original)
			goto _DESIGN;
		#endif

		if (!imevent->commitString().isEmpty())
		{
			if (MAIN_key_debug)
			{
				qDebug(QT_NAME ": (%s %s) -> `%s'",
					GB.GetClassName(control), control->name,
					(const char *)imevent->commitString().toUtf8());
			}
		
			event_id = EVENT_KeyPress;
			cancel = false;
			
		__IM_TRY_PROXY:
	
			if (GB.CanRaise(control, event_id))
			{
				CKEY_clear(true);
	
				GB.FreeString(&CKEY_info.text);
				//qDebug("IMEnd: %s", imevent->text().latin1());
				CKEY_info.text = NEW_STRING(imevent->commitString());
				CKEY_info.state = Qt::KeyboardModifiers();
				CKEY_info.code = 0;
	
				if (EXT(control) && EXT(control)->proxy_for)
					cancel = GB.Raise(EXT(control)->proxy_for, event_id, 0);
				if (!cancel)
					cancel = GB.Raise(control, event_id, 0);
	
				CKEY_clear(false);
	
				if (cancel)
					return true;
			}

			if (EXT(control) && EXT(control)->proxy_for)
			{
				control = (CWIDGET *)(EXT(control)->proxy_for);
				goto __IM_TRY_PROXY;
			}
		}
		
		goto __NEXT;
	}
	
	__MOUSE_WHEEL:
	{
		QWheelEvent *ev = (QWheelEvent *)event;
		bool eat_wheel;
		int d;

		//qDebug("Event on %p %s%s%s", widget,
		//  real ? "REAL " : "", design ? "DESIGN " : "", child ? "CHILD " : "");

		if (!original)
			goto _DESIGN;

		eat_wheel = control->flag.wheel;

	__MOUSE_WHEEL_TRY_PROXY:
		
		if (design || QWIDGET(control)->isEnabled())
		{
			if (GB.CanRaise(control, EVENT_MouseWheel))
			{
				// Automatic focus for wheel events
				CWIDGET_set_focus(control);
				
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
				p.setX(ev->position().x());
				p.setY(ev->position().y());
#else
				p.setX(ev->x());
				p.setY(ev->y());
#endif

				p = ((QWidget *)widget)->mapTo(QWIDGET(control), p);

				CMOUSE_clear(true);
				MOUSE_info.x = p.x();
				MOUSE_info.y = p.y();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
				MOUSE_info.screenX = ev->globalPosition().x();
				MOUSE_info.screenY = ev->globalPosition().y();
#else
				MOUSE_info.screenX = ev->globalX();
				MOUSE_info.screenY = ev->globalY();
#endif
				MOUSE_info.state = ev->buttons();
				MOUSE_info.modifier = ev->modifiers();

#ifdef QT5
				QPoint delta = ev->angleDelta();
#else
				QPoint delta;
				if (ev->orientation() == Qt::Horizontal)
					delta.setX(ev->delta());
				else
					delta.setY(ev->delta());
#endif

				if (delta.x())
				{
					MOUSE_info.orientation = Qt::Horizontal;
					ENSURE_EXT(control)->wheel_x += delta.x();
					MOUSE_info.delta = d = (EXT(control)->wheel_x >= 120) ? 120 : ((EXT(control)->wheel_x <= -120) ? -120 : 0);

					while (abs(EXT(control)->wheel_x) >= 120)
					{
						cancel = GB.Raise(control, EVENT_MouseWheel, 0);
						if (cancel)
						{
							EXT(control)->wheel_x = 0;
							break;
						}
						EXT(control)->wheel_x -= d;
					}
				}

				if (delta.y())
				{
					MOUSE_info.orientation = Qt::Vertical;
					ENSURE_EXT(control)->wheel_y += delta.y();
					MOUSE_info.delta = d = (EXT(control)->wheel_y >= 120) ? 120 : ((EXT(control)->wheel_y <= -120) ? -120 : 0);

					while (abs(EXT(control)->wheel_y) >= 120)
					{
						cancel = GB.Raise(control, EVENT_MouseWheel, 0);
						if (cancel)
						{
							EXT(control)->wheel_y = 0;
							break;
						}
						EXT(control)->wheel_y -= d;
					}
				}

				CMOUSE_clear(false);
				
				if (cancel)
				{
					event->accept();
					return true;
				}
			}

			if (EXT(control) && EXT(control)->proxy_for)
			{
				control = (CWIDGET *)(EXT(control)->proxy_for);
				goto __MOUSE_WHEEL_TRY_PROXY;
			}
		}
		
		if (!eat_wheel)
		{
			control = (CWIDGET *)CWIDGET_get_parent(control);
			if (control)
				goto __MOUSE_WHEEL_TRY_PROXY;
		}
		
		goto __NEXT;
	}
	
	__DRAG_ENTER:
	{
		if (!control->flag.drop)
			goto __NEXT;
			
		for(;;)
		{
			if (CDRAG_drag_enter((QWidget *)widget, control, (QDropEvent *)event))
			{
				if (!((QDropEvent *)event)->isAccepted())
					CDRAG_hide_frame(control);
				return true;
			}
			
			if (!EXT(control) || !EXT(control)->proxy_for)
				break;
				
			control = (CWIDGET *)(EXT(control)->proxy_for);
		}
		
		goto __NEXT;
	}
	
	__DRAG_MOVE:
	{
		if (!control->flag.drop)
			goto __NEXT;

		for(;;)
		{
			if (GB.CanRaise(control, EVENT_DragMove))
			{
				if (CDRAG_drag_move(QWIDGET(control), control, (QDropEvent *)event))
				{
					if (!((QDropEvent *)event)->isAccepted())
						CDRAG_hide_frame(control);
					break;
				}
			}

			if (!EXT(control) || !EXT(control)->proxy_for)
				break;
				
			control = (CWIDGET *)(EXT(control)->proxy_for);
		}
		
		if (GB.CanRaise(control, EVENT_Drop))
			return true;

		goto __NEXT;
	}
	
	__DRAG_LEAVE:
	{
		if (!control->flag.drop)
			goto __NEXT;
			
		for(;;)
		{
			CDRAG_drag_leave(control);
			
			if (!EXT(control) || !EXT(control)->proxy_for)
				break;
				
			control = (CWIDGET *)(EXT(control)->proxy_for);
		}
		goto __NEXT;
	}
	
	__DROP:
	{
		if (!control->flag.drop)
			goto __NEXT;

		for(;;)
		{
			CDRAG_drag_leave(control);
			if (CDRAG_drag_drop((QWidget *)widget, control, (QDropEvent *)event))
				return true;
		
			if (!EXT(control) || !EXT(control)->proxy_for)
				break;
				
			control = (CWIDGET *)(EXT(control)->proxy_for);
		}

		goto __NEXT;
	}
	
	__NEXT:
	
	if (!control || control->flag.deleted)
	{
		QObject::eventFilter(widget, event); 
		return (type != QEvent::DeferredDelete);
	}
	
	/*if (CWIDGET_check(control))
	{
		qDebug("CWidget::eventFilter: %p was destroyed", control);
		return true;
	}*/

_DESIGN:

	if (design)
	{
		if ((type == QEvent::MouseButtonPress)
				|| (type == QEvent::MouseButtonRelease)
				|| (type == QEvent::MouseButtonDblClick)
				|| (type == QEvent::MouseMove)
				|| (type == QEvent::Wheel)
				|| (type == QEvent::ContextMenu)
				|| (type == QEvent::KeyPress)
				|| (type == QEvent::KeyRelease)
				|| (type == QEvent::InputMethod)
				|| (type == QEvent::Shortcut)
				|| (type == QEvent::Enter)
				|| (type == QEvent::Leave)
				|| (type == QEvent::FocusIn)
				|| (type == QEvent::FocusOut)
				|| (type == QEvent::DragEnter)
				|| (type == QEvent::DragMove)
				|| (type == QEvent::DragLeave)
				|| (type == QEvent::Drop)
				|| (type == QEvent::TabletMove)
				|| (type == QEvent::TabletPress)
				|| (type == QEvent::TabletRelease)
				)
		return true;
	}

_STANDARD:

	return QObject::eventFilter(widget, event);    // standard event processing
}

/** Action *****************************************************************/

#define HAS_ACTION(_control) ((CWIDGET *)(_control))->flag.has_action
#define SET_ACTION(_control, _flag) (((CWIDGET *)(_control))->flag.has_action = (_flag))

#include "gb.form.action.h"

#if 0
static void gray_image(QImage &img)
{
	uchar *b(img.bits());
	uchar *g(img.bits() + 1);
	uchar *r(img.bits() + 2);

	uchar * end(img.bits() + img.numBytes());

	while (b != end) {

			*b = *g = *r = 0x80 | (((*r + *b) >> 1) + *g) >> 2; // (r + b + g) / 3

			b += 4;
			g += 4;
			r += 4;
	}
}
#endif

void CWIDGET_iconset(QIcon &icon, const QPixmap &pixmap, int size)
{
	QImage img;
	//QPixmap disabled;
	QPixmap normal;

	if (pixmap.isNull())
		return;
	
	if (size > 0)
	{
		img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
		size = ((size + 1) & ~3);
		img = img.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		normal = QPixmap::fromImage(img);
	}
	else
		normal = pixmap;
		
	icon = QIcon(normal);
	
	/*gray_image(img);
	
	disabled.convertFromImage(img);
	icon.setPixmap(disabled, QIcon::Small, QIcon::Disabled);*/
}


GB_DESC CControlDesc[] =
{
	GB_DECLARE("Control", sizeof(CCONTROL)), GB_NOT_CREATABLE(),

	GB_HOOK_CHECK(CWIDGET_check),

	GB_METHOD("_new", NULL, Control_new, NULL),
	GB_METHOD("_free", NULL, Control_Delete, NULL),

	GB_METHOD("Move", NULL, Control_Move, "(X)i(Y)i[(Width)i(Height)i]"),
	GB_METHOD("Resize", NULL, Control_Resize, "(Width)i(Height)i"),

	GB_METHOD("MoveScaled", NULL, Control_MoveScaled, "(X)f(Y)f[(Width)f(Height)f]"),
	GB_METHOD("ResizeScaled", NULL, Control_ResizeScaled, "(Width)f(Height)f"),

	GB_METHOD("Delete", NULL, Control_Delete, NULL),
	GB_METHOD("Show", NULL, Control_Show, NULL),
	GB_METHOD("Hide", NULL, Control_Hide, NULL),

	GB_METHOD("Raise", NULL, Control_Raise, NULL),
	GB_METHOD("Lower", NULL, Control_Lower, NULL),

	GB_PROPERTY("Next", "Control", Control_Next),
	GB_PROPERTY("Previous", "Control", Control_Previous),

	GB_METHOD("SetFocus", NULL, Control_SetFocus, NULL),
	GB_METHOD("Refresh", NULL, Control_Refresh, NULL),
	GB_METHOD("Drag", "Control", Control_Drag, "(Data)v[(Format)s]"),
	GB_METHOD("Grab", NULL, Control_Grab, NULL),

	GB_METHOD("Reparent", NULL, Control_Reparent, "(Parent)Container;[(X)i(Y)i]"),

	GB_PROPERTY("X", "i", Control_X),
	GB_PROPERTY("Y", "i", Control_Y),
	GB_PROPERTY_READ("ScreenX", "i", Control_ScreenX),
	GB_PROPERTY_READ("ScreenY", "i", Control_ScreenY),
	GB_PROPERTY("W", "i", Control_Width),
	GB_PROPERTY("H", "i", Control_Height),
	GB_PROPERTY("Left", "i", Control_X),
	GB_PROPERTY("Top", "i", Control_Y),
	GB_PROPERTY("Width", "i", Control_Width),
	GB_PROPERTY("Height", "i", Control_Height),

	GB_PROPERTY("Visible", "b", Control_Visible),
	GB_PROPERTY("Enabled", "b", Control_Enabled),
	GB_PROPERTY_READ("HasFocus", "b", Control_HasFocus),
	GB_PROPERTY_READ("Hovered", "b", Control_Hovered),
	
	GB_PROPERTY("Expand", "b", Control_Expand),
	GB_PROPERTY("Ignore", "b", Control_Ignore),

	GB_PROPERTY("Font", "Font", Control_Font),
	GB_PROPERTY("Background", "i", Control_Background),
	GB_PROPERTY("Foreground", "i", Control_Foreground),

	GB_PROPERTY("Design", "b", Control_Design),
	GB_PROPERTY("Name", "s", Control_Name),
	GB_PROPERTY("Tag", "v", Control_Tag),
	GB_PROPERTY("Tracking", "b", Control_Tracking),
	GB_PROPERTY("Mouse", "i", Control_Mouse),
	GB_PROPERTY("Cursor", "Cursor", Control_Cursor),
	GB_PROPERTY("Tooltip", "s", Control_Tooltip),
	GB_PROPERTY("Drop", "b", Control_Drop),
	GB_PROPERTY("Action", "s", Control_Action),
	GB_PROPERTY("PopupMenu", "s", Control_PopupMenu),
	GB_PROPERTY("Proxy", "Control", Control_Proxy),
	GB_PROPERTY("NoTabFocus", "b", Control_NoTabFocus),
	GB_PROPERTY("Direction", "i", Control_Direction),
	GB_PROPERTY_READ("RightToLeft", "b", Control_RightToLeft),

	GB_PROPERTY_READ("Parent", "Container", Control_Parent),
	GB_PROPERTY_READ("_Parent", "Container", Control__Parent),
	GB_PROPERTY_READ("Window", "Window", Control_Window),
	GB_PROPERTY_READ("Id", "l", Control_Id),
	GB_PROPERTY_READ("Handle", "l", Control_Id),

	GB_EVENT("Enter", NULL, NULL, &EVENT_Enter),
	GB_EVENT("GotFocus", NULL, NULL, &EVENT_GotFocus),
	GB_EVENT("LostFocus", NULL, NULL, &EVENT_LostFocus),
	GB_EVENT("KeyPress", NULL, NULL, &EVENT_KeyPress),
	GB_EVENT("KeyRelease", NULL, NULL, &EVENT_KeyRelease),
	GB_EVENT("Leave", NULL, NULL, &EVENT_Leave),
	GB_EVENT("MouseDown", NULL, NULL, &EVENT_MouseDown),
	GB_EVENT("MouseMove", NULL, NULL, &EVENT_MouseMove),
	GB_EVENT("MouseDrag", NULL, NULL, &EVENT_MouseDrag),
	GB_EVENT("MouseUp", NULL, NULL, &EVENT_MouseUp),
	GB_EVENT("MouseWheel", NULL, NULL, &EVENT_MouseWheel),
	GB_EVENT("DblClick", NULL, NULL, &EVENT_DblClick),
	GB_EVENT("Menu", NULL, NULL, &EVENT_Menu),
	GB_EVENT("Drag", NULL, NULL, &EVENT_Drag),
	GB_EVENT("DragMove", NULL, NULL, &EVENT_DragMove),
	GB_EVENT("Drop", NULL, NULL, &EVENT_Drop),
	GB_EVENT("DragLeave", NULL, NULL, &EVENT_DragLeave),

	CONTROL_DESCRIPTION,

	GB_END_DECLARE
};



