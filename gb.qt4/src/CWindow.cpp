/***************************************************************************

  CWindow.cpp

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

#define __CWINDOW_CPP

#include <QScreen>

#include <qnamespace.h>
#include <qapplication.h>
#include <qmenubar.h>
#include <qsizepolicy.h>
#include <qnamespace.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qobject.h>

//Added by qt3to4:
#include <QMoveEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QLayout>
#include <QEventLoop>
#include <QAction>
#include <QTimer>

#include "main.h"

#ifndef NO_X_WINDOW
#ifndef QT5
#include <QX11Info>
#include <QX11EmbedWidget>
#include <QX11EmbedContainer>
#endif
#endif

#ifdef QT5
#include <QWindow>
#endif

#ifndef QT6
#include <QDesktopWidget>
#endif

#include "gambas.h"

#include "CWidget.h"
#include "CMouse.h"
#include "CMenu.h"
#include "CKey.h"
#include "CDraw.h"
#include "CImage.h"
#include "CColor.h"
#include "CStyle.h"
#include "CWindow.h"

#ifndef NO_X_WINDOW
	#ifndef QT5
#include "x11.h"
#undef FontChange
	#endif
#endif

#ifdef NO_X_WINDOW
enum
{
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_MENU,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	_NET_WM_WINDOW_TYPE_POPUP_MENU,
	_NET_WM_WINDOW_TYPE_TOOLTIP,
	_NET_WM_WINDOW_TYPE_NOTIFICATION,
	_NET_WM_WINDOW_TYPE_COMBO,
	_NET_WM_WINDOW_TYPE_DND
};
#endif

//#define DEBUG_WINDOW 1

DECLARE_EVENT(EVENT_Open);
DECLARE_EVENT(EVENT_Close);
DECLARE_EVENT(EVENT_Activate);
DECLARE_EVENT(EVENT_Deactivate);
DECLARE_EVENT(EVENT_Move);
DECLARE_EVENT(EVENT_Resize);
DECLARE_EVENT(EVENT_Show);
DECLARE_EVENT(EVENT_Hide);
DECLARE_EVENT(EVENT_Title);
DECLARE_EVENT(EVENT_Icon);
DECLARE_EVENT(EVENT_Font);
DECLARE_EVENT(EVENT_State);

DECLARE_METHOD(Window_Show);

CWINDOW *CWINDOW_Main = 0;
int CWINDOW_MainDesktop = -1;
CWINDOW *CWINDOW_Current = 0;
CWINDOW *CWINDOW_LastActive = 0;
CWINDOW *CWINDOW_Active = 0;

#ifndef QT5
int CWINDOW_Embedder = 0;
bool CWINDOW_Embedded = false;
static int CWINDOW_EmbedState = 0;
#endif

// Fix a QT little boring visual bug on menubars
#if 0
void CWINDOW_fix_menubar(CWINDOW *window)
{
	if (window && window->menuBar)
	{
		QWidget *save = qApp->focusWidget();
		window->menuBar->setFocus();
		QKeyEvent e(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
		qApp->sendEvent(window->menuBar, &e);
		if (save)
			save->setFocus();
	}
}
#endif

/*---- Utility routines --------------------------------------------------------------*/

static void clear_mask(CWINDOW *_object)
{
	WINDOW->clearMask();

	if (THIS->toplevel)
	{
		#ifndef NO_X_WINDOW
			bool v = !WINDOW->isHidden() && WINDOW->isVisible();
			//WINDOW->setBorder(WINDOW->hasBorder(), true);
			//WINDOW->setResizable(WINDOW->isResizable(), true);
			if (v && THIS->reallyMasked)
			{
				#ifdef QT5
					PLATFORM.Window.Remap(WINDOW);
				#else
					X11_window_remap(WINDOW->effectiveWinId());
				#endif
				WINDOW->initProperties(PROP_ALL);
			}
		#endif
	}
}

void CWINDOW_define_mask(CWINDOW *_object)
{
	QPixmap background;
	QColor c;
	QPalette palette;

	//qDebug("define_mask: (%s %p)  picture = %p  masked = %d", GB.GetClassName(THIS), THIS, THIS->picture, THIS->masked);

	//if (THIS->embedded)
	//	return;

	if (THIS->picture)
		background = *(THIS->picture->pixmap);

	if (background.isNull())
	{
		clear_mask(THIS);
		THIS->reallyMasked = false;
		THIS->container->setPixmap(0);
		//THIS->container->setPalette(WINDOW->palette());
	}
	else
	{
		if (THIS->masked && background.hasAlpha())
		{
			THIS->reallyMasked = true;
			WINDOW->setBetterMask(background);
		}
		else
		{
			clear_mask(THIS);
			THIS->reallyMasked = false;
		}

		THIS->container->setPixmap(THIS->picture->pixmap);
	}

	THIS->container->update();
}

static bool emit_open_event(void *_object)
{
	//if (THIS->opening)
	//	return true;

	if (THIS->opened)
		return false;

	THIS->closed = false;
	THIS->opened = true;

	#if DEBUG_WINDOW
	qDebug("emit_open_event: %s %p", GB.GetClassName(THIS), THIS);
	#endif
	//THIS->opening = true;
	//WINDOW->configure();
	GB.Raise(THIS, EVENT_Open, 0);
	//THIS->opening = false;
	if (THIS->closed)
	{
		#if DEBUG_WINDOW
		qDebug("emit_open_event: %s %p [CANCELED]", GB.GetClassName(THIS), THIS);
		#endif
		THIS->opened = false;
		return true;
	}

	THIS->opened = true;
	THIS->hidden = false;
	return false;
}

static void handle_focus(CWINDOW *_object)
{
	if (THIS->focus)
	{
		//qDebug("handle_focus on %s", THIS->focus->name);
		CWIDGET_set_focus(THIS->focus);
		GB.Unref(POINTER(&THIS->focus));
		THIS->focus = NULL;
	}
}

static void raise_resize_event(void *_object)
{
	if (WINDOW->width() != THIS->last_resize_w || WINDOW->height() != THIS->last_resize_h)
	{
		THIS->last_resize_w = WINDOW->width();
		THIS->last_resize_h = WINDOW->height();
		GB.Raise(THIS, EVENT_Resize, 0);
	}
}

static void post_show_event(void *_object)
{
	THIS->sx = THIS->x;
	THIS->sy = THIS->y;
	GB.Raise(THIS, EVENT_Move, 0);
	raise_resize_event(THIS);
	handle_focus(THIS);
}

static void reparent_window(CWINDOW *_object, void *parent, bool move, int x = 0, int y = 0)
{
	QPoint p;
	QWidget *newParentWidget;
	bool moved = THIS->moved;

	if (move)
	{
		p.setX(x);
		p.setY(y);
		moved = true;
	}
	else if (THIS->toplevel)
  {
		p.setX(THIS->x);
		p.setY(THIS->y);
  }
  else
    p = WIDGET->pos();

	if (!parent)
		newParentWidget = 0;
	else
	{
		if (GB.CheckObject(parent))
			return;
		newParentWidget = QCONTAINER(parent);
	}

	if (newParentWidget != WINDOW->parentWidget())
	{
		//qDebug("reparent_window: -> %s %p", parent ? ((CWIDGET *)parent)->name : "", parent);
		WINDOW->doReparent(newParentWidget, &p);
	}
	else
		CWIDGET_move(THIS, p.x(), p.y());

	THIS->moved = moved;
}

void CWINDOW_ensure_active_window()
{
	void *_object = CWINDOW_Active;

	if (THIS)
		WINDOW->activate();
}


static void show_later(CWINDOW *_object)
{
	/* If the user has explicitely hidden the window since the posting of this routines
		then do nothing
	*/

	//qDebug("show_later %s %p: hidden = %d", GB.GetClassName(THIS), THIS, THIS->hidden);
	if (!THIS->hidden && WIDGET)
	{
		if (!emit_open_event(THIS))
			CWIDGET_set_visible((CWIDGET *)THIS, true);
	}
	GB.Unref(POINTER(&_object));
}


void CWINDOW_move_resize(void *_object, int x, int y, int w, int h)
{
	bool move, resize;

	move = x != THIS->x || y != THIS->y || !THIS->moved;
	
	if (w < 0)
		w = THIS->w;

	if (h < 0)
		h = THIS->h;
	
	resize = w != THIS->w || h != THIS->h || !THIS->resized;
	
	if (!move && !resize)
		return;
	
	THIS->x = x;
	THIS->y = y;
	THIS->w = w;
	THIS->h = h;
	
	if (!THIS->moved && (x || y))
		THIS->moved = TRUE;

	if (move)
		WINDOW->move(x, y);
	
	if (resize)
	{
		bool resizable = false;
	
		if (WINDOW->isWindow() && !WINDOW->isResizable())
		{
			resizable = true;
			WINDOW->setResizable(true);
		}

		WINDOW->resize(w, h);

		THIS->resized = TRUE;
		if (THIS->default_minw <= 0 && THIS->default_minh <= 0)
		{
			THIS->default_minw = w;
			THIS->default_minh = h;
		}
	
		if (resizable)
			WINDOW->setResizable(false);
		
		WINDOW->configure();
	}
}

static int get_screen_number(QWidget *widget)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	return QApplication::screens().indexOf(widget->screen());
#else
	return QApplication::desktop()->screenNumber(widget);
#endif
}

//-- Window ---------------------------------------------------------------

BEGIN_METHOD(Window_new, GB_OBJECT parent)

	static bool app_icon = false;

	MyMainWindow *win = 0;
	MyContainer *container;
#ifndef NO_X_WINDOW
#ifndef QT5
	QX11EmbedWidget *client = 0;
#endif
#endif
	const char *name = GB.GetClassName(THIS);

	//THIS->widget.flag.fillBackground = true;

	if (MISSING(parent) || !VARG(parent))
	{
#ifndef NO_X_WINDOW
#ifndef QT5
		if (CWINDOW_Embedder && !CWINDOW_Embedded)
		{
			client = new QX11EmbedWidget;

			win = new MyMainWindow(client, name, true);
			container = new MyContainer(win);
			container->raise();
			THIS->embedded = true;
			THIS->toplevel = false;
			THIS->xembed = true;
		}
		else
#endif
#endif
		{
			//win = new MyMainWindow(CWINDOW_Main ? (MyMainWindow *)QWIDGET(CWINDOW_Main) : 0, name);
			win = new MyMainWindow(0, name);
			container = new MyContainer(win);
			container->raise();
			THIS->embedded = false;
			THIS->toplevel = true;
			THIS->xembed = false;
		}
	}
	else
	{
		if (GB.Conv((GB_VALUE *)(void *)ARG(parent), (GB_TYPE)CLASS_Container))
			return;

		//frame = new MyEmbeddedWindow(QCONTAINER(VARG(parent)));
		//frame->setName(name);
		//container = frame;
		//THIS->embedded = true;
		//THIS->toplevel = false;
		//container->installEventFilter(&CWindow::manager);
		//CWIDGET_new(frame, (void *)_object, NULL);

		win = new MyMainWindow(QCONTAINER(VARG(parent)), name, true);
		container = new MyContainer(win);
		container->raise();
		THIS->embedded = true;
		THIS->toplevel = false;
		THIS->xembed = false;
	}
	/*else
	{
		GB.Error("The parent of a Window must be a Container or a Workspace");
		return;
	}*/

	THIS->container = container;
	CWIDGET_new(win, (CWIDGET *)THIS, true);
	THIS->widget.flag.resized = TRUE;

	if (win)
	{
		win->_object = THIS;
		win->installEventFilter(&CWindow::manager);
	}

	if (THIS->toplevel || THIS->xembed)
	{
		CWindow::insertTopLevel(THIS);

		/*if (CWINDOW_Main == 0)
		{
			#if DEBUG_WINDOW
			qDebug("CWINDOW_Main -> %p", THIS);
			#endif
			CWINDOW_Main = THIS;
		}*/
	}

#ifndef NO_X_WINDOW
#ifndef QT5
	if (THIS->xembed)
	{
		CWINDOW_Embedded = true;

		QObject::connect(XEMBED, SIGNAL(embedded()), &CWindow::manager, SLOT(embedded()));
		QObject::connect(XEMBED, SIGNAL(containerClosed()), &CWindow::manager, SLOT(closed()));
		QObject::connect(XEMBED, SIGNAL(error(int)), &CWindow::manager, SLOT(error()));

		//qDebug("XEMBED: EmbedInto %ld", CWINDOW_Embedder);
		XEMBED->embedInto(CWINDOW_Embedder);
		//qDebug("XEMBED: show");
		//XEMBED->show();
		//define_mask(THIS);

		for(;;)
		{
			MAIN_process_events();
			if (CWINDOW_EmbedState)
				break;
			usleep(10000);
		}

		//qDebug("XEMBED: EmbedState: %d", CWINDOW_EmbedState);

		if (CWINDOW_EmbedState == EMBED_ERROR)
		{
			CWINDOW_Embedded = false;
			CWINDOW_Embedder  = 0;
			GB.Error("Embedding has failed");
		}
	}
#endif
#endif

	#if 1
	if (THIS->embedded && !THIS->xembed)
	{
		/* ### This can call post_show_event() directly, whereas the function is not terminated */
		#if DEBUG_WINDOW
		qDebug("post show_later %s %p", GB.GetClassName(THIS), THIS);
		#endif
		GB.Ref(THIS);
		//show_later(THIS);
		GB.Post((void (*)())show_later, (intptr_t)THIS);
		//WIDGET->show();
	}
	//else
	//	THIS->hidden = TRUE;
	#endif

	THIS->showMenuBar = true;

	if (!app_icon)
	{
		app_icon = true;
		CIMAGE_set_default_window_icon();
	}
	
END_METHOD


BEGIN_METHOD_VOID(CFORM_new)

	if (!GB.Parent(_object))
		GB.Attach(_object, _object, "Form");

	CWIDGET_set_name((CWIDGET *)THIS, GB.GetClassName((void *)THIS));

END_METHOD


BEGIN_METHOD_VOID(Form_Main)

	CWINDOW *form = (CWINDOW *)GB.AutoCreate(GB.GetClass(NULL), 0);

	if (!form->hidden)
		Window_Show(form, NULL);

END_METHOD


BEGIN_METHOD(Form_Load, GB_OBJECT parent)

	//qDebug("Form_Load");
	reparent_window((CWINDOW *)GB.AutoCreate(GB.GetClass(NULL), 0), VARGOPT(parent, 0), false);

END_METHOD


BEGIN_METHOD_VOID(Window_free)

	//qDebug("Window_free");

	GB.StoreObject(NULL, POINTER(&(THIS->icon)));
	GB.StoreObject(NULL, POINTER(&(THIS->picture)));
	GB.Unref(POINTER(&THIS->focus));

END_METHOD


BEGIN_METHOD_VOID(CWINDOW_next)

	int index = ENUM(int);

	if (index >= CWindow::list.count())
	{
		GB.StopEnum();
		return;
	}

	GB.ReturnObject(CWindow::list.at(index));
	ENUM(int) = index + 1;

END_METHOD


BEGIN_PROPERTY(CWINDOW_count)

	GB.ReturnInteger(CWindow::list.count());

END_PROPERTY


BEGIN_METHOD(CWINDOW_get_from_id, GB_INTEGER id)

	QWidget *wid = QWidget::find(VARG(id));

	//qDebug("id = %d wid = %p", PARAM(id), wid);

	if (wid != 0 && wid->isWindow())
	{
		//qDebug("-> %p", CWidget::getReal(wid));
		GB.ReturnObject(CWidget::getReal(wid));
	}
	else
	{
		//qDebug("-> %p", 0);
		GB.ReturnNull();
	}

END_METHOD


static bool do_close(CWINDOW *_object, int ret, bool destroyed = false)
{
	bool closed;

	#if DEBUG_WINDOW
	fprintf(stderr, "do_close: (%s %p) %d %d\n", GB.GetClassName(THIS), THIS, THIS->closing, THIS->closed);
	#endif

	if (THIS->closing || THIS->closed) // || WIDGET->isHidden())
		return false;

	if (!THIS->toplevel)
	{
		//qDebug("Close event: %s %p opened = %d", GB.GetClassName(THIS), THIS, THIS->opened);
		if (THIS->opened)
		{
			THIS->closing = true;
			closed = !GB.Raise(THIS, EVENT_Close, 0);
			THIS->closing = false;
		}
		else
			closed = true;

		if (destroyed || closed)
		{
			THIS->closed = true;
			THIS->opened = false;
		}

		if (closed)
		{
			WIDGET->hide();
			if (!THIS->persistent)
				CWIDGET_destroy((CWIDGET *)THIS);
		}
	}
	else
	{
		if (!THIS->opened)
		{
			#if DEBUG_WINDOW
			qDebug("send close event");
			#endif
			QCloseEvent e;
			QApplication::sendEvent(WINDOW, &e);
			closed = e.isAccepted();
		}
		else
		{
			#if DEBUG_WINDOW
			qDebug("call WINDOW->close()");
			#endif
			closed = WINDOW->close();
		}
		#if DEBUG_WINDOW
		qDebug("--> closed = %d", closed);
		#endif
	}

	if (closed)
		THIS->ret = ret;

	return (!closed);
}

BEGIN_METHOD(Window_Close, GB_INTEGER ret)

	int ret = VARGOPT(ret, 0);

	GB.ReturnBoolean(do_close(THIS, ret));

END_METHOD


BEGIN_METHOD_VOID(Window_Raise)

	if (!THIS->toplevel)
	{
		if (!WIDGET->isVisible())
			CWIDGET_set_visible((CWIDGET *)THIS, true);
		WIDGET->raise();
	}
	else
	{
		if (!WINDOW->isVisible())
			WINDOW->showActivate();
		else
			WINDOW->raise();
	}

END_METHOD

static bool check_opened(CWINDOW *_object, bool modal)
{
	if (THIS->toplevel && THIS->opened)
	{
		if (modal || THIS->modal)
		{
			GB.Error("Window is already opened");
			return TRUE;
		}
	}
	
	return FALSE;
}

BEGIN_METHOD_VOID(Window_Show)

	if (check_opened(THIS, FALSE))
		return;

	if (emit_open_event(THIS))
		return;

	if (!THIS->toplevel)
	{
		CWIDGET_set_visible((CWIDGET *)THIS, true);
		CWIDGET_check_visibility((CWIDGET *)THIS);
#ifndef NO_X_WINDOW
#ifndef QT5
		if (THIS->xembed)
			XEMBED->show();
#endif
#endif
		post_show_event(THIS);
	}
	else
	{
		WINDOW->showActivate();
	}

END_METHOD


BEGIN_METHOD_VOID(Window_Hide)

	THIS->hidden = true;

	if (THIS->toplevel && THIS->modal)
	{
		do_close(THIS, 0);
		//THIS->widget.flag.visible = false;
	}
	else
		CWIDGET_set_visible((CWIDGET *)THIS, false);

END_METHOD


BEGIN_METHOD_VOID(Window_ShowModal)

	if (check_opened(THIS, TRUE))
		return;
	
	if (!THIS->toplevel)
	{
		GB.Error("The window is not top-level");
		return;
	}

	THIS->ret = 0;
	THIS->modal = TRUE;

	if (!emit_open_event(THIS))
		WINDOW->showModal();

	THIS->modal = FALSE;
	
	GB.ReturnInteger(THIS->ret);

END_METHOD


BEGIN_METHOD(Window_ShowPopup, GB_INTEGER x; GB_INTEGER y)

	QPoint pos;

	if (check_opened(THIS, TRUE))
		return;

	if (MISSING(x) || MISSING(y))
		pos = QCursor::pos();
	else
		pos = QPoint(VARG(x), VARG(y));

	THIS->ret = 0;

	if (THIS->toplevel)
	{
		THIS->modal = THIS->popup = TRUE;
		if (!emit_open_event(THIS))
			WINDOW->showPopup(pos);
		THIS->modal = THIS->popup = FALSE;
	}

	GB.ReturnInteger(THIS->ret);

END_METHOD


#if 0
BEGIN_METHOD(Window_ShowPopupAt, GB_OBJECT control; GB_INTEGER pos; GB_INTEGER align)

	static GB_FUNCTION func;

	if (!GB_FUNCTION_IS_VALID(&func))
		GB.GetFunction(&func, (void *)GB.FindClass("_Gui"), "_ShowPopupAt", NULL, NULL);

	GB.Push(4,
		GB_T_OBJECT, THIS,
		GB_T_OBJECT, VARG(control),
		GB_T_INTEGER, VARGOPT(pos, ALIGN_BOTTOM),
		GB_T_INTEGER, VARGOPT(align, ALIGN_NORMAL));

	GB.Call(&func, 4, FALSE);

END_METHOD
#endif

BEGIN_PROPERTY(Window_Modal)

	if (THIS->toplevel)
		GB.ReturnBoolean(THIS->modal);
	else
		GB.ReturnBoolean(false);

END_PROPERTY


BEGIN_PROPERTY(Window_TopLevel)

	GB.ReturnBoolean(THIS->toplevel);

END_PROPERTY


BEGIN_PROPERTY(Window_Persistent)

	if (!THIS->toplevel)
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(true);
	}
	else
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(THIS->persistent);
		else
			THIS->persistent = VPROP(GB_BOOLEAN);
	}

END_PROPERTY


BEGIN_PROPERTY(Window_Text)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(WIDGET->windowTitle());
	else
	{
		QString s = QSTRING_PROP();
		THIS->title = s.length() > 0;
		WIDGET->setWindowTitle(s);
		GB.Raise(THIS, EVENT_Title, 0);
	}

END_PROPERTY


BEGIN_PROPERTY(Window_Border)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WINDOW->hasBorder());
	else
		WINDOW->setBorder(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Window_Resizable)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WINDOW->isResizable());
	else
		WINDOW->setResizable(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Window_Icon)

	if (READ_PROPERTY)
		GB.ReturnObject(THIS->icon);
	else
	{
		SET_PIXMAP(WIDGET->setWindowIcon, &(THIS->icon), PROP(GB_OBJECT));
		GB.Raise(THIS, EVENT_Icon, 0);
	}

END_PROPERTY


BEGIN_PROPERTY(Window_Picture)

	if (READ_PROPERTY)
		GB.ReturnObject(THIS->picture);
	else
	{
		CPICTURE *new_pict = (CPICTURE *)VPROP(GB_OBJECT);

		if (new_pict != THIS->picture)
		{
			CPICTURE *old = THIS->picture;
			GB.Ref(new_pict);
			THIS->picture = new_pict;
			//CWINDOW_define_mask(THIS);
			CWIDGET_reset_color((CWIDGET *)THIS);
			GB.Unref(POINTER(&old));
		}
	}

END_PROPERTY


BEGIN_PROPERTY(Window_Mask)

	/*if (THIS->embedded)
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(FALSE);
	}
	else*/
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(THIS->masked);
		else
		{
			bool new_masked = VPROP(GB_BOOLEAN);

			if (new_masked != THIS->masked)
			{
				THIS->masked = new_masked;
				//CWINDOW_define_mask(THIS);
				CWIDGET_reset_color((CWIDGET *)THIS);
			}
		}
	}

END_PROPERTY


static void manage_window_state(void *_object, void *_param, Qt::WindowState state)
{
	if (!THIS->toplevel)
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(FALSE);
	}
	else
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(WINDOW->getState() & state);
		else
		{
			if (VPROP(GB_BOOLEAN))
				WINDOW->setState(WINDOW->getState() | state);
			else
				WINDOW->setState(WINDOW->getState() & ~state);
		}
	}
}

BEGIN_PROPERTY(Window_Minimized)

	manage_window_state(_object, _param, Qt::WindowMinimized);

END_PROPERTY


BEGIN_PROPERTY(Window_Maximized)

	manage_window_state(_object, _param, Qt::WindowMaximized);

END_PROPERTY


BEGIN_PROPERTY(Window_FullScreen)

	manage_window_state(_object, _param, Qt::WindowFullScreen);

END_PROPERTY


BEGIN_PROPERTY(Window_Stacking)

	int p;

	if (!THIS->toplevel)
	{
		if (READ_PROPERTY)
			GB.ReturnInteger(0);
		return;
	}

	if (READ_PROPERTY)
	{
		GB.ReturnInteger(THIS->stacking);
	}
	else
	{
		p = VPROP(GB_INTEGER);
		if (p >= 0 && p <= 2)
		{
			THIS->stacking = p;
			WINDOW->initProperties(PROP_STACKING);
		}
	}

END_PROPERTY


BEGIN_PROPERTY(Window_TopOnly)

	if (!THIS->toplevel)
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(FALSE);
		return;
	}

	if (READ_PROPERTY)
	{
		GB.ReturnBoolean(THIS->stacking == 1);
	}
	else
	{
		THIS->stacking = VPROP(GB_BOOLEAN) ? 1 : 0;
		WINDOW->initProperties(PROP_STACKING);
	}

END_PROPERTY


BEGIN_PROPERTY(Window_SkipTaskbar)

	if (!THIS->toplevel)
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(FALSE);
		return;
	}

	if (READ_PROPERTY)
	{
		GB.ReturnBoolean(THIS->skipTaskbar);
	}
	else
	{
		THIS->skipTaskbar = VPROP(GB_BOOLEAN);
		WINDOW->initProperties(PROP_SKIP_TASKBAR);
	}

END_PROPERTY


BEGIN_PROPERTY(Window_Sticky)

	if (!THIS->toplevel)
	{
		if (READ_PROPERTY)
			GB.ReturnBoolean(FALSE);
		return;
	}

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->sticky);
	else
	{
		THIS->sticky = VPROP(GB_BOOLEAN);
		WINDOW->initProperties(PROP_STICKY);
	}

END_PROPERTY

BEGIN_PROPERTY(Window_Utility)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WINDOW->isUtility()); //WINDOW->getType() == _NET_WM_WINDOW_TYPE_UTILITY);
	else
	{
		//WINDOW->setType(VPROP(GB_BOOLEAN) ? _NET_WM_WINDOW_TYPE_UTILITY : _NET_WM_WINDOW_TYPE_NORMAL);
		WINDOW->setUtility(VPROP(GB_BOOLEAN));
	}

END_PROPERTY


BEGIN_METHOD_VOID(Window_Center)

	if (!THIS->toplevel)
		return;

	WINDOW->center();

END_METHOD


BEGIN_METHOD_VOID(Window_Delete)

	//qDebug("Window_Delete %p", THIS);

	do_close(THIS, 0);

	if (THIS->toplevel)
		THIS->persistent = false;

	CWIDGET_destroy((CWIDGET *)THIS);

END_METHOD


BEGIN_PROPERTY(Window_Visible)

	if (READ_PROPERTY)
		GB.ReturnBoolean(!WINDOW->isHidden());
	else
	{
		bool show = !!VPROP(GB_BOOLEAN);
		
		THIS->hidden = !show;
		
		if (show == WINDOW->isHidden())
		{
			if (show)
				Window_Show(_object, _param);
			else
				Window_Hide(_object, _param);
		}
	}

END_PROPERTY


BEGIN_PROPERTY(Window_Controls_Count)

	QList<QWidget *> list = WINDOW->findChildren<QWidget *>();
	int i;
	int n = 0;
	CWIDGET *control;

	for (i = 0; i < list.count(); i++)
	{
		control = CWidget::getReal(list.at(i));
		if (control && !CWIDGET_check(control))
			n++;
	}

	GB.ReturnInteger(n);

END_PROPERTY


BEGIN_METHOD_VOID(Window_Controls_next)

	QList<QWidget *> children = WINDOW->findChildren<QWidget *>();
	CWIDGET *control;
	int index;

	index = ENUM(int);

	control = NULL;

	do
	{
		if (index >= children.count())
		{
			GB.StopEnum();
			return;
		}

		control = CWidget::getReal(children.at(index));
		index++;
	}
	while (!control || CWIDGET_check(control));

	ENUM(int) = index;
	GB.ReturnObject(control);

END_PROPERTY


BEGIN_METHOD(Window_Reparent, GB_OBJECT container; GB_INTEGER x; GB_INTEGER y)

	//qDebug("Window_Reparent");
	reparent_window(THIS, VARG(container), !MISSING(x) && !MISSING(y), VARG(x), VARG(y));

END_METHOD


BEGIN_METHOD(Window_Controls_get, GB_STRING name)

	CWIDGET *control = WINDOW->names[GB.ToZeroString(ARG(name))];

	if (!control || CWIDGET_check(control))
		GB.ReturnNull();
	else
		GB.ReturnObject(control);

END_METHOD


BEGIN_PROPERTY(Window_Closed)

	GB.ReturnBoolean(!THIS->opened);

END_PROPERTY

/***************************************************************************/

BEGIN_PROPERTY(CWINDOW_menu_count)

  if (THIS->menuBar)
    GB.ReturnInteger(THIS->menuBar->actions().count());
  else
    GB.ReturnInteger(0);

END_PROPERTY


BEGIN_METHOD_VOID(CWINDOW_menu_next)

  int index;

  if (!THIS->menuBar)
  {
    GB.StopEnum();
    return;
  }

  index = ENUM(int);

  if (index >= THIS->menuBar->actions().count())
  {
    GB.StopEnum();
    return;
  }

  GB.ReturnObject(CMenu::dict[THIS->menuBar->actions().at(index)]);

  ENUM(int) = index + 1;

END_PROPERTY


BEGIN_METHOD(CWINDOW_menu_get, GB_INTEGER index)

  int index = VARG(index);

  if (!THIS->menuBar || index < 0 || index >= THIS->menuBar->actions().count())
  {
    GB.Error(GB_ERR_BOUND);
    return;
  }

  GB.ReturnObject(CMenu::dict[THIS->menuBar->actions().at(index)]);

END_PROPERTY


static bool are_menus_visible(void *_object)
{
	return !THIS->hideMenuBar && THIS->showMenuBar;
}


static void set_menus_visible(void *_object, bool v)
{
	THIS->showMenuBar = v;
	WINDOW->configure();
}

BEGIN_PROPERTY(CWINDOW_menu_visible)

	if (READ_PROPERTY)
		GB.ReturnBoolean(are_menus_visible(THIS));
	else
		set_menus_visible(THIS, VPROP(GB_BOOLEAN));

END_PROPERTY

BEGIN_METHOD_VOID(CWINDOW_menu_show)

	set_menus_visible(THIS, true);

END_METHOD

BEGIN_METHOD_VOID(CWINDOW_menu_hide)

	set_menus_visible(THIS, false);

END_METHOD

BEGIN_PROPERTY(Window_Opacity)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->windowOpacity() * 100);
	else
	{
		double opacity = VPROP(GB_INTEGER) / 100.0;

		if (opacity < 0.0)
			opacity = 0.0;
		else if (opacity > 1.0)
			opacity = 1.0;

		WIDGET->setWindowOpacity(opacity);
	}

END_PROPERTY

BEGIN_PROPERTY(Window_Screen)

	GB.ReturnInteger(get_screen_number(WIDGET));

END_PROPERTY

BEGIN_PROPERTY(Window_Transparent)

	bool trans = WINDOW->testAttribute(Qt::WA_TranslucentBackground);

	if (READ_PROPERTY)
		GB.ReturnBoolean(trans);
	else
	{
		bool new_trans = VPROP(GB_BOOLEAN);
		if (trans == new_trans)
			return;

		if (!new_trans)
		{
			GB.Error("Transparent property cannot be reset");
			return;
		}

		WINDOW->setAttribute(Qt::WA_TranslucentBackground, true);
		THIS->container->setPaintBackgroundColor(true);
		THIS->widget.flag.noBackground = true;
		CWIDGET_reset_color((CWIDGET *)THIS);
	}

END_PROPERTY

BEGIN_PROPERTY(Window_TakeFocus)

	if (READ_PROPERTY)
		GB.ReturnBoolean(!THIS->noTakeFocus);
	else
		THIS->noTakeFocus = !VPROP(GB_BOOLEAN);

END_PROPERTY

BEGIN_METHOD_VOID(Window_Activate)

	if (THIS->toplevel && WINDOW->isVisible() && !WINDOW->isHidden())
		WINDOW->activate();

END_METHOD

BEGIN_PROPERTY(Window_MinWidth)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->minw);
	else
	{
		THIS->minw = Max(0, VPROP(GB_INTEGER));
		WINDOW->setGeometryHints();
	}

END_PROPERTY

BEGIN_PROPERTY(Window_MinHeight)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->minh);
	else
	{
		THIS->minh = Max(0, VPROP(GB_INTEGER));
		WINDOW->setGeometryHints();
	}

END_PROPERTY

BEGIN_PROPERTY(Window_Geometry)

	GEOM_RECT *rect = GEOM.CreateRect();

	if (THIS->toplevel)
	{
		rect->x = THIS->sx;
		rect->y = THIS->sy;
		rect->w = THIS->sw;
		rect->h = THIS->sh;
	}
	else
	{
		rect->x = THIS->x;
		rect->y = THIS->y;
		rect->w = THIS->w;
		rect->h = THIS->h;
	}

	GB.ReturnObject(rect);

END_PROPERTY


/***************************************************************************/

GB_DESC CWindowMenusDesc[] =
{
	GB_DECLARE(".Window.Menus", 0), GB_VIRTUAL_CLASS(),

	GB_METHOD("_next", "Menu", CWINDOW_menu_next, NULL),
	GB_METHOD("_get", "Menu", CWINDOW_menu_get, "(Index)i"),
	GB_PROPERTY_READ("Count", "i", CWINDOW_menu_count),
  GB_METHOD("Show", NULL, CWINDOW_menu_show, NULL),
  GB_METHOD("Hide", NULL, CWINDOW_menu_hide, NULL),
  GB_PROPERTY("Visible", "b", CWINDOW_menu_visible),

	GB_END_DECLARE
};

GB_DESC CWindowControlsDesc[] =
{
	GB_DECLARE(".Window.Controls", 0), GB_VIRTUAL_CLASS(),

	GB_METHOD("_next", "Control", Window_Controls_next, NULL),
	GB_METHOD("_get", "Control", Window_Controls_get, "(Name)s"),
	GB_PROPERTY_READ("Count", "i", Window_Controls_Count),

	GB_END_DECLARE
};

#if 0
GB_DESC CWindowTypeDesc[] =
{
  GB_DECLARE("WindowType", 0), GB_VIRTUAL_CLASS(),

  GB_CONSTANT("Normal", "i", _NET_WM_WINDOW_TYPE_NORMAL),
  GB_CONSTANT("Dock", "i", _NET_WM_WINDOW_TYPE_DOCK),
  GB_CONSTANT("Toolbar", "i", _NET_WM_WINDOW_TYPE_TOOLBAR),
  GB_CONSTANT("Menu", "i", _NET_WM_WINDOW_TYPE_MENU),
  GB_CONSTANT("Utility", "i", _NET_WM_WINDOW_TYPE_UTILITY),
  GB_CONSTANT("Splash", "i", _NET_WM_WINDOW_TYPE_SPLASH),
  GB_CONSTANT("Dialog", "i", _NET_WM_WINDOW_TYPE_DIALOG),
  GB_CONSTANT("DropDownMenu", "i", _NET_WM_WINDOW_TYPE_DROPDOWN_MENU),
  GB_CONSTANT("PopupMenu", "i", _NET_WM_WINDOW_TYPE_POPUP_MENU),
  GB_CONSTANT("Tooltip", "i", _NET_WM_WINDOW_TYPE_TOOLTIP),
  GB_CONSTANT("Notification", "i", _NET_WM_WINDOW_TYPE_NOTIFICATION),
  GB_CONSTANT("Combo", "i", _NET_WM_WINDOW_TYPE_COMBO),
  GB_CONSTANT("DragAndDrop", "i", _NET_WM_WINDOW_TYPE_DND),
  GB_CONSTANT("Desktop", "i", _NET_WM_WINDOW_TYPE_DESKTOP),

	GB_END_DECLARE
};
#endif

GB_DESC CWindowDesc[] =
{
	GB_DECLARE("Window", sizeof(CWINDOW)), GB_INHERITS("Container"),

	GB_CONSTANT("Normal", "i", 0),
	GB_CONSTANT("Above", "i", 1),
	GB_CONSTANT("Below", "i", 2),

	GB_METHOD("_new", NULL, Window_new, "[(Parent)Control;]"),
	GB_METHOD("_free", NULL, Window_free, NULL),
	GB_METHOD("_get", "Control", Window_Controls_get, "(Name)s"),

	GB_METHOD("Close", "b", Window_Close, "[(Return)i]"),
	GB_METHOD("Raise", NULL, Window_Raise, NULL),
	GB_METHOD("Show", NULL, Window_Show, NULL),
	GB_METHOD("Hide", NULL, Window_Hide, NULL),
	GB_METHOD("ShowModal", "i", Window_ShowModal, NULL),
	GB_METHOD("ShowDialog", "i", Window_ShowModal, NULL),
	GB_METHOD("ShowPopup", "i", Window_ShowPopup, "[(X)i(Y)i]"),
	//GB_METHOD("ShowPopupAt", "i", Window_ShowPopupAt, "(Control)Control;[(Position)i(Alignment)i]"),
	GB_METHOD("Center", NULL, Window_Center, NULL),
	GB_METHOD("Activate", NULL, Window_Activate, NULL),

	GB_PROPERTY_READ("Modal", "b", Window_Modal),
	GB_PROPERTY_READ("TopLevel", "b", Window_TopLevel),
	GB_PROPERTY_READ("Closed", "b", Window_Closed),

	GB_METHOD("Delete", NULL, Window_Delete, NULL),

	GB_METHOD("Reparent", NULL, Window_Reparent, "(Container)Container;[(X)i(Y)i]"),

	GB_PROPERTY("Persistent", "b", Window_Persistent),
	GB_PROPERTY("Text", "s", Window_Text),
	GB_PROPERTY("Title", "s", Window_Text),
	GB_PROPERTY("Caption", "s", Window_Text),
	GB_PROPERTY("Icon", "Picture", Window_Icon),
	GB_PROPERTY("Picture", "Picture", Window_Picture),
	GB_PROPERTY("Mask", "b", Window_Mask),
	GB_PROPERTY("Minimized", "b", Window_Minimized),
	GB_PROPERTY("Maximized", "b", Window_Maximized),
	GB_PROPERTY("FullScreen", "b", Window_FullScreen),
	GB_PROPERTY("TopOnly", "b", Window_TopOnly),
	GB_PROPERTY("Stacking", "i", Window_Stacking),
	GB_PROPERTY("Sticky", "b", Window_Sticky),
	GB_PROPERTY("SkipTaskbar", "b", Window_SkipTaskbar),
	GB_PROPERTY("Visible", "b", Window_Visible),
	GB_PROPERTY("Opacity", "i", Window_Opacity),
	GB_PROPERTY("Transparent", "b", Window_Transparent),
	GB_PROPERTY("TakeFocus", "b", Window_TakeFocus),

	GB_PROPERTY("MinWidth", "i", Window_MinWidth),
	GB_PROPERTY("MinHeight", "i", Window_MinHeight),
	GB_PROPERTY("MinW", "i", Window_MinWidth),
	GB_PROPERTY("MinH", "i", Window_MinHeight),

	ARRANGEMENT_PROPERTIES,

	//GB_PROPERTY("Type", "i", CWINDOW_type),
	GB_PROPERTY("Utility", "b", Window_Utility),
	GB_PROPERTY("Border", "b", Window_Border),
	GB_PROPERTY("Resizable", "b", Window_Resizable),

	GB_PROPERTY_READ("Screen", "i", Window_Screen),
	GB_PROPERTY_READ("Geometry", "Rect", Window_Geometry),

	GB_PROPERTY_SELF("Menus", ".Window.Menus"),
	GB_PROPERTY_SELF("Controls", ".Window.Controls"),

	WINDOW_DESCRIPTION,

	GB_EVENT("Close", NULL, NULL, &EVENT_Close),
	GB_EVENT("Open", NULL, NULL, &EVENT_Open),
	GB_EVENT("Activate", NULL, NULL, &EVENT_Activate),
	GB_EVENT("Deactivate", NULL, NULL, &EVENT_Deactivate),
	GB_EVENT("Move", NULL, NULL, &EVENT_Move),
	GB_EVENT("Resize", NULL, NULL, &EVENT_Resize),
	GB_EVENT("Show", NULL, NULL, &EVENT_Show),
	GB_EVENT("Hide", NULL, NULL, &EVENT_Hide),
	GB_EVENT("Title", NULL, NULL, &EVENT_Title),
	GB_EVENT("Icon", NULL, NULL, &EVENT_Icon),
	GB_EVENT("Font", NULL, NULL, &EVENT_Font),
	GB_EVENT("State", NULL, NULL, &EVENT_State),

	//GB_INTERFACE("Draw", &DRAW_Interface),

	GB_END_DECLARE
};


GB_DESC CWindowsDesc[] =
{
	GB_DECLARE("Windows", 0), GB_VIRTUAL_CLASS(),

	GB_STATIC_METHOD("_next", "Window", CWINDOW_next, NULL),
	GB_STATIC_METHOD("_get", "Window", CWINDOW_get_from_id, "(Id)i"),
	GB_STATIC_PROPERTY_READ("Count", "i", CWINDOW_count),

	GB_END_DECLARE
};


GB_DESC CFormDesc[] =
{
	GB_DECLARE("Form", sizeof(CFORM)), GB_INHERITS("Window"),
	GB_AUTO_CREATABLE(),

	GB_STATIC_METHOD("Main", NULL, Form_Main, NULL),
	GB_STATIC_METHOD("Load", NULL, Form_Load, "[(Parent)Control;]"),
	GB_METHOD("_new", NULL, CFORM_new, NULL),

	FORM_DESCRIPTION,

	GB_END_DECLARE
};


/***************************************************************************

	MyMainWindow

***************************************************************************/

MyMainWindow::MyMainWindow(QWidget *parent, const char *name, bool embedded) :
	QWidget::QWidget(parent, embedded ? Qt::Widget : Qt::Window)
{
	sg = 0;
	_border = true;
	_resizable = true;
	_deleted = false;
	//_type = _NET_WM_WINDOW_TYPE_NORMAL;
	_enterLoop = false;
	_utility = false;
	_state = windowState();
	_screen = -1;

	//setAttribute(Qt::WA_KeyCompression, true);
	//setAttribute(Qt::WA_InputMethodEnabled, true);
	setAttribute(Qt::WA_QuitOnClose, false);
	setAttribute(Qt::WA_StaticContents, true);
	setObjectName(name);
	setFocusPolicy(Qt::NoFocus);

	resize(1, 1);
	_activate = false;
}


MyMainWindow::~MyMainWindow()
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(this);

	#if DEBUG_WINDOW
	qDebug("~MyMainWindow: %s %s %p", GB.GetClassName(THIS), THIS->widget.name, THIS);
	#endif

	do_close(THIS, 0, true);

	if (CWINDOW_Active == THIS)
		CWINDOW_Active = 0;

	if (CWINDOW_LastActive == THIS)
		CWINDOW_LastActive = 0;

	if (sg)
		delete sg;

	GB.Detach(THIS);

	if (THIS->menuBar)
	{
		//CMenu::unrefChildren(THIS->menuBar);
		//qDebug("delete menuBar");
		QMenuBar *menuBar = THIS->menuBar;
		THIS->menuBar = 0;
		delete menuBar;
	}

	CWindow::removeTopLevel(THIS);

	_deleted = true;

	//qDebug("~MyMainWindow %p (end)", this);
}


void MyMainWindow::showEvent(QShowEvent *e)
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(this);

	//qDebug("showEvent: %s\n", GB.GetClassName(THIS));

	emit_open_event(THIS);

	//CWINDOW_fix_menubar(THIS);

	if (_activate)
	{
		//qDebug("showEvent: activate: %s", THIS->widget.name);
		raise();
		//setFocus();
		activate();
		_activate = false;
	}

	QWidget::showEvent(e);
}


void MyMainWindow::initProperties(int which, bool force)
{
	CWIDGET *_object = CWidget::get(this);

	if (!THIS->toplevel) // || effectiveWinId() == 0)
		return;

	if (!force && !isVisible())
		return;

	if (!THIS->title && _border)
		setWindowTitle(TO_QSTRING(GB.Application.Title()));

	if (effectiveWinId() == 0)
		createWinId();

	#ifdef QT5

		QT_WINDOW_PROP prop;
		
		prop.stacking = THIS->stacking;
		prop.skipTaskbar = THIS->skipTaskbar;
		prop.border = _border;
		prop.sticky = THIS->sticky;
		
		PLATFORM.Window.SetProperties(this, which, &prop);

	#else

		X11_flush();

		if (which & (PROP_STACKING | PROP_SKIP_TASKBAR))
		{
			X11_window_change_begin(effectiveWinId(), isVisible());

			if (which & PROP_STACKING)
			{
				X11_window_change_property(X11_atom_net_wm_state_above, THIS->stacking == 1);
				X11_window_change_property(X11_atom_net_wm_state_stays_on_top, THIS->stacking == 1);
				X11_window_change_property(X11_atom_net_wm_state_below, THIS->stacking == 2);
			}
			if (which & PROP_SKIP_TASKBAR)
				X11_window_change_property(X11_atom_net_wm_state_skip_taskbar, THIS->skipTaskbar);

			X11_window_change_end();
		}

		//if (which == PROP_ALL)
		//	X11_set_window_type(effectiveWinId(), _type);

		if (which & PROP_BORDER)
			X11_set_window_decorated(effectiveWinId(), _border);

		if (which & PROP_STICKY)
			X11_window_set_desktop(effectiveWinId(), isVisible(), THIS->sticky ? 0xFFFFFFFF : X11_get_current_desktop());

		X11_flush();

	#endif
}

void MyMainWindow::setEventLoop()
{
	if (!THIS->closed)
		THIS->loopLevel = CWINDOW_Current ? CWINDOW_Current->loopLevel : 0;
}

void MyMainWindow::setGeometryHints()
{
	CWIDGET *_object = CWidget::get(this);
	int minw, minh;
	
	if (!THIS->toplevel)
	{
		setMinimumSize(0, 0);
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
		return;
	}
	
	minw = THIS->minw;
	minh = THIS->minh;
	
	if (_resizable)
	{
		if (isModal() || isUtility())
		{
			if (!minw && !minh)
			{
				minw = THIS->default_minw;
				minh = THIS->default_minh;
			}
		}
		setMinimumSize(minw, minh);
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	}
	else
	{
		setMinimumSize(width(), height());
		setMaximumSize(width(), height());
	}
}

void MyMainWindow::present(QWidget *parent)
{
	/*CWIDGET *_object = CWidget::get(this);
	CWIDGET *_parent = parent ? CWidget::get(parent) : 0;
	
	qDebug("present: %p %s: parent = %p %s", THIS, _object->name, _parent, _parent ? _parent->name : "");*/

	if (parent)
		_screen = get_screen_number(parent);
	else
		_screen = -1;

	if (!isVisible())
	{
		//X11_window_startup(WINDOW->effectiveWinId(), THIS->x, THIS->y, THIS->w, THIS->h);

		setGeometryHints();

		setAttribute(Qt::WA_ShowWithoutActivating, THIS->noTakeFocus);

/*#ifdef QT5
		if (MAIN_platform_is_wayland)
			initProperties(PROP_ALL, true);
#else
		if (effectiveWinId() == 0)
		{
			createWinId();
		}
		initProperties(PROP_ALL, true);
#endif*/

#ifdef QT5
		if (THIS->noTakeFocus)
			PLATFORM.Window.SetUserTime(this, 0);
#endif
		initProperties(PROP_ALL, true);
#ifdef QT5
		if (THIS->noTakeFocus)
			PLATFORM.Window.SetUserTime(this, 0);
#endif

		if (getState() & Qt::WindowMinimized)
			showMinimized();
		else if (getState() & Qt::WindowFullScreen)
			showFullScreen();
		else if (getState() & Qt::WindowMaximized)
			showMaximized();
		else
			show();

/*#ifdef QT5
		if (!MAIN_platform_is_wayland)
		{
			//qDebug("createWinId: %p", (void *)effectiveWinId());
			if (THIS->noTakeFocus)
				PLATFORM.Window.SetUserTime(this, 0);
			initProperties(PROP_ALL, true);
			if (THIS->noTakeFocus)
				PLATFORM.Window.SetUserTime(this, 0);
		}
#else
		initProperties(PROP_SKIP_TASKBAR, true);
#endif*/

#ifdef QT5
#else
		initProperties(PROP_SKIP_TASKBAR, true);
#endif

		/*if (isUtility() && _resizable)
			setSizeGrip(true);
		else
			setSizeGrip(false);*/
	}
	else
	{
		//_activate = true;

		if (getState() & Qt::WindowMinimized)
		{
			setState(windowState() & ~Qt::WindowMinimized);
			//qDebug("_activate set #2");
		}
	}

	if (!THIS->noTakeFocus) // && (parent || hasBorder()))
		activate();

	if (parent)
	{
		#ifdef QT5
			PLATFORM.Window.SetTransientFor(this, parent);
		#else
			X11_set_transient_for(effectiveWinId(), parent->effectiveWinId());
		#endif
	}

	raise();
}

void MyMainWindow::showActivate(QWidget *transient)
{
	QWidget *newParentWidget = 0;

	//qDebug("showActivate: %s %d", THIS->widget.name, isToolbar());

	// Reparent the window if, for example, there is an already modal window displayed

	if (CWINDOW_Current && THIS != CWINDOW_Current)
	{
		newParentWidget = CWINDOW_Current->widget.widget;

		if (!isVisible())
		{
			if (newParentWidget && parentWidget() != newParentWidget)
			{
				doReparent(newParentWidget);
			}
		}
	}

	//qDebug("showActivate %p", _object);

	//CWIDGET_clear_flag(THIS, WF_CLOSED);

	if (isUtility())
	{
		if (!newParentWidget && CWINDOW_Main && THIS != CWINDOW_Main)
		{
			if (CWINDOW_Active)
				newParentWidget = CWidget::getTopLevel((CWIDGET *)CWINDOW_Active)->widget.widget;
			else if (CWINDOW_Main)
				newParentWidget = ((CWIDGET *)CWINDOW_Main)->widget;

			if (newParentWidget && parentWidget() != newParentWidget)
			{
				doReparent(newParentWidget);
			}
		}
	}

	present(newParentWidget);
	setEventLoop();
}

void on_error_show_modal(MODAL_INFO *info)
{
	#ifdef DEBUG_WINDOW
	qDebug("on_error_show_modal");
	#endif

	// info->that can be NULL if the dialog is destroyed during the event loop

	if (info->that)
		info->that->_enterLoop = false;

	MyApplication::eventLoop->exit();

	GB.Debug.LeaveEventLoop();

	MyApplication::eventLoop = info->old;
	CWINDOW_Current = info->save;

	if (info->that && info->that->isPersistent())
	{
		info->that->setSizeGrip(false);
		info->that->setWindowModality(Qt::NonModal);
		info->that->setWindowFlags(Qt::Window | info->flags);
	}
	
	CWIDGET_leave_popup(info->save_popup);
}

void MyMainWindow::doShowModal(bool popup, const QPoint *p)
{
	CWIDGET *_object = CWidget::get(this);
	CWINDOW *parent;
	bool persistent = THIS->persistent;
	//QPoint p = pos();
	QEventLoop eventLoop;
	GB_ERROR_HANDLER handler;
	MODAL_INFO info;

	CWIDGET_finish_focus();
	CMOUSE_finish_event();

	info.that = this;
	info.old = MyApplication::eventLoop;
	info.save = CWINDOW_Current;
	info.save_popup = popup ? CWIDGET_enter_popup() : NULL;
	info.flags = windowFlags() & ~Qt::WindowType_Mask;

	setWindowModality(Qt::ApplicationModal);

	_enterLoop = false; // Do not call exitLoop() if we do not entered the loop yet!

	parent = CWINDOW_Current;
	if (!parent)
		parent = CWINDOW_Active;

	if (popup)
	{
		if (parent)
			setParent(CWidget::getTopLevel((CWIDGET *)parent)->widget.widget, Qt::Popup | info.flags);

		move(0, 0);
		move(*p);
		THIS->x = p->x();
		THIS->y = p->y();
		//fprintf(stderr, "doShowModal: popup: %d %d\n", pos().x(), pos().y());
		setFocus();
		show();
		raise();
	}
	else
	{
		if (_resizable && _border)
			setSizeGrip(true);

		if (parent)
			setParent(CWidget::getTopLevel((CWIDGET *)parent)->widget.widget, Qt::Window | info.flags);
		
		present(parent ? CWidget::getTopLevel((CWIDGET *)parent)->widget.widget : 0);
	}
	
	//fprintf(stderr, "set event loop to %p\n", &eventLoop);
	MyApplication::eventLoop = &eventLoop;

	setEventLoop();

	THIS->loopLevel++;
	
	THIS->save_focus = CWIDGET_active_control;
	THIS->previous = CWINDOW_Current;
	CWINDOW_Current = THIS;

	_enterLoop = true;

	GB.Debug.EnterEventLoop();

	handler.handler = (GB_CALLBACK)on_error_show_modal;
	handler.arg1 = (intptr_t)&info;

	GB.OnErrorBegin(&handler);

	//fprintf(stderr, "event loop <---- %p\n",CWINDOW_Current);
	eventLoop.exec();
	//fprintf(stderr, "event loop ---->%p\n", info.save);

	GB.OnErrorEnd(&handler);

	GB.Debug.LeaveEventLoop();
	
	//eventLoop.processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::DeferredDeletion, 0);

	MyApplication::eventLoop = info.old;
	CWINDOW_Current = info.save;

	if (persistent)
	{
		setSizeGrip(false);
		setWindowModality(Qt::NonModal);
		setWindowFlags(Qt::Window | info.flags);
	}

	if (popup)
		CWIDGET_leave_popup(info.save_popup);
	
	CWINDOW_ensure_active_window();
	
	if (THIS->save_focus)
	{
		CWIDGET_set_focus(THIS->save_focus);
		THIS->save_focus = NULL;
	}
}

void MyMainWindow::moveSizeGrip()
{
	CWINDOW *window;
	QWidget *cont;

	if (sg == 0)
		return;

	window = (CWINDOW *)CWidget::get(this);
	cont = window->container;

	if (qApp->isRightToLeft())
		sg->move(cont->rect().bottomLeft() - sg->rect().bottomLeft());
	else
		sg->move(cont->rect().bottomRight() - sg->rect().bottomRight());
}

void MyMainWindow::setSizeGrip(bool on)
{
	if (on == (sg != 0))
		return;

	if (!on)
	{
		delete sg;
		sg = 0;
	}
	else //if (!parentWidget())
	{
		sg = new QSizeGrip(((CWINDOW *)CWidget::get(this))->container);
		sg->adjustSize();
		moveSizeGrip();
		sg->lower();
		//if (paletteBackgroundPixmap())
		//  sg->setBackgroundOrigin(QWidget::AncestorOrigin);
		sg->show();
	}
}

void MyMainWindow::setBorder(bool b)
{
	if (_border == b)
		return;

	_border = b;
	
	if (!isWindow())
		return;

#ifdef QT5
	
	bool visible = isVisible();
	void *_object = CWidget::get(this);;
	
	/*if (_border)
		setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
	else
		setWindowFlags(windowFlags() | Qt::FramelessWindowHint);*/
	
	/*if (visible)
	{
		show();
		move(THIS->x, THIS->y);
	}*/

	initProperties(PROP_ALL);

#else

	initProperties(PROP_BORDER);
	if (effectiveWinId())
		X11_window_remap(effectiveWinId());
	doReparent(parentWidget());
	
#endif
}

void MyMainWindow::setResizable(bool b)
{
	if (_resizable == b)
		return;

	_resizable = b;
	if (!isWindow())
		return;
	doReparent(parentWidget());
}

void MyMainWindow::setUtility(bool b)
{
	if (_utility == b)
		return;

	_utility = b;
	if (!isWindow())
		return;

	doReparent(parentWidget());
}

#if 0
#ifdef NO_X_WINDOW
#else
int MyMainWindow::getType()
{
	if (!isWindow())
		return 0;
	else
		return _type;
	//return X11_get_window_type(effectiveWinId());
}

void MyMainWindow::setType(int type)
{
	if (!isWindow())
		return;
	X11_set_window_type(effectiveWinId(), type);
	_type = type;
}
#endif
#endif

void MyMainWindow::moveEvent(QMoveEvent *e)
{
	CWIDGET *_object = CWidget::getReal(this);

	//qDebug("Move: (%s %p) %d %d", GB.GetClassName(THIS), THIS, e->pos().x(), e->pos().y());

	QWidget::moveEvent(e);

	//qDebug("Move (pos %d %d) (oldPos %d %d)", e->pos().x(), e->pos().y(), e->oldPos().x(), e->oldPos().y());
	//qDebug("     (geom %d %d) (fgeom %d %d)", geometry().x(), geometry().y(), frameGeometry().x(), frameGeometry().y());
	//qDebug("     Visible = %s Hidden = %s", (isVisible() ? "Yes" : "No"), (isHidden() ? "Yes" : "No"));
	//qDebug("     Flags = 0x%s State = 0x%s", QString::number(getWFlags(), 16).latin1(), QString::number(getWState(), 16).latin1());

	//if (CWIDGET_test_flag(ob, WF_IGNORE_MOVE))

	//if (THIS->embedded)
	//	return;

	if (THIS->toplevel)
	{
		if (hasBorder() && !THIS->reallyMasked)
			if (geometry().x() == frameGeometry().x() && geometry().y() == frameGeometry().y())
				return;

		if (!isHidden())
		{
			THIS->x = x();
			THIS->y = y();
			if ((_state & (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)) == 0)
			{
				THIS->sx = THIS->x;
				THIS->sy = THIS->y;
			}
			//qDebug("moveEvent: x= %d y = %d", x(), y());
		}
	}

	//qDebug("moveEvent %ld %ld isHidden:%s shown:%s ", THIS->x, THIS->y, isHidden() ? "1" : "0", shown ? "1" : "0");

	if (THIS->opened)
		GB.Raise(THIS, EVENT_Move, 0);
}



/*
static void post_resize_event(CWINDOW *_object)
{
	qDebug("post resize: %d %d", THIS->w, THIS->h);
	WINDOW->resize(THIS->w, THIS->h);
	GB.Unref(POINTER(&_object));
}
*/

void MyMainWindow::resizeEvent(QResizeEvent *e)
{
	CWINDOW *_object = (CWINDOW *)CWidget::getReal(this);
	//int w, h;

	//qDebug("Resize %p: %d %d <- %d %d", _object, e->size().width(), e->size().height(), e->oldSize().width(), e->oldSize().height());

	//QMainWindow::resizeEvent(e);

	configure();

	if (sg)
		moveSizeGrip();

	if (!isHidden())
	{
		THIS->w = THIS->container->width();
		THIS->h = THIS->container->height();
		if (isWindow())
		{
			if ((_state & (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)) == 0)
			{
				THIS->sw = THIS->w;
				THIS->sh = THIS->h;
			}
			CCONTAINER_arrange(THIS);
		}
	}

#ifndef NO_X_WINDOW
#ifndef QT5
	if (THIS->xembed)
		XEMBED->resize(THIS->w, THIS->h);
#endif
#endif

  	//qDebug("resizeEvent %ld %ld isHidden:%s shown:%s ", THIS->w, THIS->h, isHidden() ? "1" : "0", shown ? "1" : "0");
	//qDebug("THIS->h = %ld  THIS->container->height() = %ld  height() = %ld", THIS->h, THIS->container->height(), height());

	if (THIS->opened) // && (e->spontaneous() || parentWidget()))
		raise_resize_event(THIS);
}


void MyMainWindow::keyPressEvent(QKeyEvent *e)
{
	CWINDOW *_object = (CWINDOW *)CWidget::getReal(this);
	QPushButton *test = 0;
	CWIDGET *ob;

	e->ignore();

	//qDebug("MyMainWindow::keyPressEvent: (%p '%s' %s)", this, this ? this->caption().latin1() : 0, GB.GetClassName(CWidget::get(this)));

	if ((e->modifiers() == Qt::NoModifier || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter )))
	{
		switch (e->key())
		{
			case Qt::Key_Enter:
			case Qt::Key_Return:

				test = THIS->defaultButton;
				break;

			case Qt::Key_Escape:

				test = THIS->cancelButton;
				break;
		}

		if (!test)
			return;

		ob = CWidget::get(test);
		if (!ob)
			return;

		if (CWIDGET_is_design(ob))
			return;

		if (!test->isVisible() || !test->isEnabled())
			return;

		test->setFocus();
		test->animateClick();
		e->accept();
	}
}


bool CWINDOW_close_all(bool main)
{
	QList<CWINDOW *> list(CWindow::list);
	CWINDOW *win;
	int i;
	bool ret = false;

	#if DEBUG_WINDOW
	qDebug("<<< CLOSE ALL");
	#endif

	for (i = 0; i < list.count(); i++)
	{
		win = list.at(i);
		if (win != CWINDOW_Main && do_close(win, 0))
		{
			ret = true;
			break;
		}
	}

	if (main && CWINDOW_Main)
		ret = do_close(CWINDOW_Main, 0);

	#if DEBUG_WINDOW
	qDebug(">>> CLOSE ALL");
	#endif

	return ret;
}

void CWINDOW_delete_all(bool main)
{
	QList<CWINDOW *> list(CWindow::list);
	CWINDOW *win;
	int i;

	#if DEBUG_WINDOW
	qDebug("<<< DELETE ALL");
	#endif

	for (i = 0; i < list.count(); i++)
	{
		win = CWindow::list.at(i);
		if (win != CWINDOW_Main)
		{
			//qDebug("destroy window %s", GB.GetClassName(win));
			CWIDGET_destroy((CWIDGET *)win);
		}
	}

	if (main && CWINDOW_Main)
	{
		//qDebug("destroy main window %s", GB.GetClassName(CWINDOW_Main));
		CWIDGET_destroy((CWIDGET *)CWINDOW_Main);
	}

	#if DEBUG_WINDOW
	qDebug("DELETE ALL >>>");
	#endif

	//qApp->eventLoop()->processEvents(QEventLoop::AllEvents);
}

bool CWINDOW_must_quit()
{
	CWINDOW *win;
	int i;

	for (i = 0; i < CWindow::list.count(); i++)
	{
		win = CWindow::list.at(i);
		if (win->opened)
			return false;
	}

	return true;
}


void MyMainWindow::closeEvent(QCloseEvent *e)
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(this);
	bool cancel = false;
	//bool modal;

	//fprintf(stderr, "closeEvent(): THIS = %p loopLevel = %d CWINDOW_Current = %p\n", THIS, THIS->loopLevel, CWINDOW_Current);
	
	e->ignore();

	#if DEBUG_WINDOW
		qDebug("closeEvent: CWINDOW_Current = %p / %d <-> %p / %d", CWINDOW_Current, CWINDOW_Current ? CWINDOW_Current->loopLevel : -1, THIS, THIS->loopLevel);
	#endif

	if (THIS->opened)
	{
		// If a window is not opened, then it can be closed whatever the loop level is
		if (CWINDOW_Current && (THIS->loopLevel != CWINDOW_Current->loopLevel))
		{
			goto IGNORE;
		}

		//qDebug("THIS->opened = %d: %p: %s", THIS->opened, THIS, GB.GetClassName(THIS));
		THIS->closing = true;
		//qDebug("Close event: %s %p", GB.GetClassName(THIS), THIS);
		cancel = GB.Raise(THIS, EVENT_Close, 0);
		THIS->closing = false;
	}

	if (!cancel && THIS == CWINDOW_Main)
	{
		if (CWINDOW_close_all(false))
			cancel = true;
	}

	if (cancel)
		goto IGNORE;

	//modal = isModal(); //testWFlags(Qt::WShowModal); // && THIS->opened;

	THIS->closed = true;
	//qApp->sendEvent(WIDGET, new QEvent(EVENT_CLOSE));

	if (CWINDOW_LastActive == THIS)
	{
		//GB.Unref(POINTER(&CWINDOW_LastActive));
		CWINDOW_LastActive = NULL;
		//qDebug("CWINDOW_LastActive = 0");
	}

	if (THIS == CWINDOW_Active)
		CWINDOW_activate(NULL);

	if (!THIS->persistent)
	{
		if (CWINDOW_Main == THIS)
		{
			CWINDOW_delete_all(false);
			#if DEBUG_WINDOW
			qDebug("CWINDOW_Main -> NULL");
			#endif
			CWINDOW_Main = NULL;
		}

		CWIDGET_destroy((CWIDGET *)THIS);
	}

	e->accept();

	if (THIS->modal && _enterLoop)
	{
		_enterLoop = false;
		MyApplication::eventLoop->exit();
	}

	#if DEBUG_WINDOW
	qDebug("THIS->opened <- false: %p: %s", THIS, GB.GetClassName(THIS));
	#endif
	THIS->opened = false;
	MAIN_check_quit();

	return;

IGNORE:

	THIS->closed = false;
	e->ignore();
}


bool MyMainWindow::isPersistent(void)
{
	return !testAttribute(Qt::WA_DeleteOnClose); //testWFlags(WDestructiveClose);
}


void MyMainWindow::setPersistent(bool pers)
{
	setAttribute(Qt::WA_DeleteOnClose, !pers);
}

void MyMainWindow::doReparent(QWidget *parent, const QPoint *p)
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(this);
	QIcon icon;
	bool old_toplevel;
	bool hidden;
	bool reparented = false;
	Qt::WindowFlags f = windowFlags();
	#ifndef NO_X_WINDOW
	bool active = qApp->activeWindow() == this;
	#endif

	icon = windowIcon();

	old_toplevel = THIS->toplevel;
	THIS->toplevel = !parent || parent->isWindow();
	THIS->embedded = !THIS->toplevel;

	f &= ~Qt::WindowType_Mask;

	if (THIS->toplevel)
	{
		if (THIS->popup)
			f |= Qt::Popup;
		else if (_utility)
			f |= Qt::Dialog;
		else
			f |= Qt::Window;

		if (!old_toplevel)
			CWindow::insertTopLevel(THIS);
	}
	else
	{
		if (old_toplevel)
		{
			THIS->toplevel = true;
			CWindow::removeTopLevel(THIS);
			THIS->toplevel = false;
		}
	}

	//qDebug("doReparent: %s %p: visible = %d opened = %d hidden = %d isVisible = %d isHidden = %d shown = %d",
	//				THIS->widget.name, THIS, THIS->widget.flag.visible, THIS->opened, THIS->hidden, isVisible(), isHidden(), THIS->widget.flag.shown);

	//if (!THIS->hidden) showIt = true;
	//hide();

	hidden = THIS->hidden || !WIDGET->isVisible();
	if (parent != parentWidget() || f != windowFlags())
	{
		reparented = true;
		setParent(parent, f);
		//qDebug("setParent %d", f != windowFlags());
	}

	if (p)
		move(*p);
	else
		move(THIS->x, THIS->y);

	if (!THIS->embedded)
	{
		#ifndef NO_X_WINDOW
			initProperties(PROP_ALL);
			if (active && hasBorder())
				activate();
		#endif

		setWindowIcon(icon);
	}

	if (!_resizable && _border && isWindow())
	{
		setMinimumSize(width(), height());
		setMaximumSize(width(), height());
	}
	else
	{
		setMinimumSize(0, 0);
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	}

	/*if (parentWidget())
		qDebug("doReparent (%s %p): new parent = (%s %p)", THIS->widget.name, THIS, CWidget::get(parentWidget())->name, CWidget::get(parentWidget()));
	else
		qDebug("doReparent (%s %p): new parent = 0", THIS->widget.name, THIS);*/

	if (reparented)
	{
		if (!hidden)
			Window_Show(THIS, NULL);
	}
}

int MyMainWindow::currentScreen() const
{
#ifdef QT5
	QList<QScreen*> screenList;
	QScreen* primaryScreen;
#endif

	if (_screen >= 0)
		return _screen;

	if (CWINDOW_Active)
		return get_screen_number(CWINDOW_Active->widget.widget);
	else if (CWINDOW_Main)
		return get_screen_number(CWINDOW_Main->widget.widget);
	else
#ifndef QT5
		return QApplication::desktop()->primaryScreen();
#else
	{
		primaryScreen = QGuiApplication::primaryScreen();
		screenList = QGuiApplication::screens();
		return screenList.indexOf(primaryScreen);
	}
#endif
}

void MyMainWindow::center()
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(this);
	QPoint p;
	QRect r;

#ifdef QT5
	r = QGuiApplication::screens().at(currentScreen())->availableGeometry();
#else
	r = QApplication::desktop()->availableGeometry(currentScreen());
#endif

	CWIDGET_move(THIS, r.x() + (r.width() - width()) / 2, r.y() + (r.height() - height()) / 2);
}

#if 0
void MyMainWindow::paintEvent(QPaintEvent *e)
{
	if (CSTYLE_fix_breeze && isWindow())
	{
		QPainter p(this);
		p.fillRect(0, 0, width(), 1, CCOLOR_light_foreground());
	}
}
#endif

void MyMainWindow::configure()
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(this);
	QMenuBar *menuBar = THIS->menuBar;
	bool arrange = false;
	QRect geom;
	int d;

	//qDebug("THIS->menuBar = %p  menuBar() = %p", THIS->menuBar, menuBar());

	// sometimes configure() seems to be called when the window is being deleted
	if (CWIDGET_check(THIS))
		return;

	d = 0; //CSTYLE_fix_breeze && isWindow() ? 1 : 0;

	if (menuBar && THIS->showMenuBar && !THIS->hideMenuBar)
	{
		int h = menuBar->sizeHint().height();

		if (h == 0)
			h = menuBar->height();

		menuBar->show();
		geom = QRect(0, h + d, this->width(), this->height() - h - d);

		if (THIS->container->geometry() != geom)
		{
			arrange = true;
			THIS->container->setGeometry(geom);
		}
		menuBar->setGeometry(0, d, this->width(), h);
	}
	else
	{
		if (menuBar)
		{
			menuBar->move(0, -menuBar->height());
			menuBar->lower();
		}

		geom = QRect(0, d, this->width(), this->height() - d);

		if (THIS->container->geometry() != geom)
		{
			arrange = true;
			THIS->container->setGeometry(geom);
		}

		THIS->container->raise();
	}

	if (arrange)
	{
		CCONTAINER_arrange(THIS);
		CMENU_update_menubar(THIS);
	}

	//qDebug(">>> THIS->menuBar = %p  menuBar() = %p", THIS->menuBar, menuBar());

	//qDebug("configure: %p (%d %d %d %d)", THIS, ((QFrame *)(THIS->container))->contentsRect().x(), ((QFrame *)(THIS->container))->contentsRect().y(), ((QFrame *)(THIS->container))->contentsRect().width(), ((QFrame *)(THIS->container))->contentsRect().height());
}


void MyMainWindow::setName(const char *name, CWIDGET *control)
{
	if (_deleted)
		return;

	names.remove(name);
	if (control)
		names.insert(name, control);
}

void MyMainWindow::resize(int w, int h)
{
	bool save = _resizable;

	if (!_resizable && _border)
		setResizable(true);

	QWidget::resize(w, h);

	if (_resizable != save)
		setResizable(save);
}

void MyMainWindow::setGeometry(int x, int y, int w, int h)
{
	bool save = _resizable;

	if (!_resizable && _border)
		setResizable(true);

	QWidget::setGeometry(x, y, w, h);

	if (_resizable != save)
		setResizable(save);
}


void MyMainWindow::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);

	if (e->type() == QEvent::StyleChange || e->type() == QEvent::FontChange)
	{
		void *_object = CWidget::get(this);
		configure();
		GB.Raise(THIS, EVENT_Font, 0);
	}
	else if (e->type() == QEvent::WindowStateChange)
	{
		_state = windowState();
		GB.Raise(CWidget::get(this), EVENT_State, 0);
	}
}

Qt::WindowStates MyMainWindow::getState() const
{
	return _state;
}

void MyMainWindow::setState(Qt::WindowStates state)
{
	_state = state;
	if (isVisible())
		setWindowState(state);
}

void MyMainWindow::setVisible(bool visible)
{
	if (!visible)
		setAttribute(Qt::WA_NoMouseReplay);
	QWidget::setVisible(visible);
}

void MyMainWindow::setBetterMask(QPixmap &bg)
{
	if (!bg.hasAlphaChannel())
			return;

	static const uchar qt_pixmap_bit_mask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
	const QImage img = bg.toImage();
	const QImage image = (img.depth() < 32 ? img.convertToFormat(QImage::Format_ARGB32_Premultiplied) : img);
	const int w = image.width();
	const int h = image.height();

	QImage mask(w, h, QImage::Format_MonoLSB);
	if (mask.isNull()) // allocation failed
		return;

	mask.setColorCount(2);
	mask.setColor(0, QColor(Qt::color0).rgba());
	mask.setColor(1, QColor(Qt::color1).rgba());

	const int bpl = mask.bytesPerLine();

	for (int y = 0; y < h; ++y) {
		const QRgb *src = reinterpret_cast<const QRgb*>(image.scanLine(y));
		uchar *dest = mask.scanLine(y);
		memset(dest, 0, bpl);
		for (int x = 0; x < w; ++x) {
				if (qAlpha(*src) >= 128)
						dest[x >> 3] |= qt_pixmap_bit_mask[x & 7];
				++src;
		}
	}

	setMask(QBitmap::fromImage(mask));
}

void MyMainWindow::activate(void)
{
#ifdef QT5
	PLATFORM.Window.Activate(this);
#else
	activateWindow();
#endif
}

bool MyMainWindow::focusNextPrevChild(bool next)
{
	void *current, *initial;
	QWidget *w;
	
	current = CWidget::getRealExisting(focusWidget());
	if (!current)
		return QWidget::focusNextPrevChild(next);
	
	//uint focus_flag = QGuiApplication::styleHints()->tabFocusBehavior() == Qt::TabFocusAllControls ? Qt::TabFocus : Qt::StrongFocus;
	
	initial = current;
	
	for(;;)
	{
		current = next ? CWIDGET_get_next_focus(current) : CWIDGET_get_previous_focus(current);
		if (!current || current == initial)
			return QWidget::focusNextPrevChild(next);
		
		//fprintf(stderr, "focusNextPrevChild: %s / %d\n", ((CWIDGET *)current)->name, CWIDGET_has_no_tab_focus(current));
		
		if (CWIDGET_has_no_tab_focus(current))
			continue;
		
		w = QWIDGET(current);
		if (w->isVisible() && w->isEnabled() && (w->focusPolicy() & Qt::TabFocus))
		{
			CWIDGET_set_focus(current);
			return true;
		}
	}
	
}

/***************************************************************************

	CWindow

***************************************************************************/

CWindow CWindow::manager;
int CWindow::count = 0;
QList<CWINDOW *> CWindow::list;

/*static void post_activate_event(void *ob)
{
	GB.Raise(ob, EVENT_Activate, 0);
	GB.Unref(&ob);
}

static void post_deactivate_event(void *ob)
{
	GB.Raise(ob, EVENT_Deactivate, 0);
	GB.Unref(&ob);
}*/

void CWINDOW_activate(CWIDGET *ob)
{
	CWINDOW *active;

	/*if (ob)
	{
		fprintf(stderr, "CWINDOW_activate: %s %s\n", GB.GetClassName(ob), ob->name);
		
		CWIDGET *parent = ob;
		for(;;)
		{
			parent = (CWIDGET *)CWIDGET_get_parent(parent);
			if (!parent)
				break;
			fprintf(stderr, "  --> %s %s\n", GB.GetClassName(parent), parent->name);
		}
	}
	else
		fprintf(stderr, "CWINDOW_activate: null\n");*/
	
	if (ob)
	{
		active = CWidget::getWindow(ob);
		for(;;)
		{
			if (active->toplevel)
				break;
			if (GB.CanRaise(active, EVENT_Activate))
				break;
			active = CWidget::getWindow(CWidget::get(QWIDGET(active)->parentWidget()));
		}
	}
	else
		active = 0;

	if (active == CWINDOW_Active)
		return;

	//qDebug("CWINDOW_activate: (%s %p): (%s %p) -> (%s %p)", ob ? GB.GetClassName(ob) : "", ob, CWINDOW_Active ? GB.GetClassName(CWINDOW_Active) : "", CWINDOW_Active, active ? GB.GetClassName(active) : "", active);

	if (CWINDOW_Active)
	{
		GB.Raise(CWINDOW_Active, EVENT_Deactivate, 0);
		CWINDOW_Active = NULL;
	}

	if (active)
	{
		//fprintf(stderr, "Activate: %s\n", GB.GetClassName(active));
		GB.Raise(active, EVENT_Activate, 0);
	}

	CWINDOW_Active = active;
	
	CWIDGET_check_hovered();
}

void CWINDOW_set_default_button(CWINDOW *win, QPushButton *button, bool on)
{
	if (on)
	{
		if (win->defaultButton)
			win->defaultButton->setDefault(false);

		win->defaultButton = button;
		button->setDefault(true);
	}
	else
	{
		if (win->defaultButton == button)
		{
			button->setDefault(false);
			win->defaultButton = 0;
		}
	}
}

void CWINDOW_set_cancel_button(CWINDOW *win, QPushButton *button, bool on)
{
	//qDebug("CWINDOW_set_cancel_button: (%s %p) (%s %p) %d", GB.GetClassName(win), win, GB.GetClassName(CWidget::get(button)), CWidget::get(button), on);
	if (on)
	{
		win->cancelButton = button;
	}
	else
	{
		if (button == win->cancelButton)
			win->cancelButton = 0;
	}
}


bool CWindow::eventFilter(QObject *o, QEvent *e)
{
	CWINDOW *_object = (CWINDOW *)CWidget::get(o);

	if (THIS && !THIS->widget.flag.deleted)
	{
		if (e->type() == QEvent::Show) // && !e->spontaneous())
		{
			MyMainWindow *w = (MyMainWindow *)o;

			THIS->noHideEvent = false;
			
			if (THIS->toplevel && !THIS->popup && !THIS->moved)
				w->center();

			//handle_focus(THIS);
			emit_open_event(THIS);

			//qDebug("eventFilter: Show: %s %d (%d) focus = %p", GB.GetClassName(THIS), !WINDOW->isHidden(), e->spontaneous(), THIS->focus);

			post_show_event(THIS);
			//CWINDOW_define_mask(THIS);

			GB.Raise(THIS, EVENT_Show, 0);
			if (!e->spontaneous())
				CACTION_raise(THIS);
		}
		else if (e->type() == QEvent::Hide) // && !e->spontaneous())
		{
			//fprintf(stderr, "Hide: %p %s %d (%d)\n", o, GB.GetClassName(THIS), WINDOW->isHidden(), e->spontaneous());
			if (!THIS->noHideEvent)
			{
				THIS->noHideEvent = true;
				GB.Raise(THIS, EVENT_Hide, 0);
				if (!e->spontaneous())
					CACTION_raise(THIS);
			}
		}
	}

	return QObject::eventFilter(o, e);    // standard event processing
}

#ifdef QT5
void CWindow::error(void)
{
}

void CWindow::embedded(void)
{
}

void CWindow::closed(void)
{
}
#else
void CWindow::error(void)
{
	//CWINDOW *_object = (CWINDOW *)CWidget::getReal((QObject *)sender());
	//qDebug("XEMBED: CWindow::error %p -> %p", sender(), THIS);
	CWINDOW_EmbedState = EMBED_ERROR;
}

void CWindow::embedded(void)
{
	//CWINDOW *_object = (CWINDOW *)CWidget::getReal((QObject *)sender());
	//qDebug("XEMBED: CWindow::embedded %p -> %p", sender(), THIS);
	CWINDOW_EmbedState = EMBED_OK;
}

void CWindow::closed(void)
{
	//CWINDOW *_object = (CWINDOW *)CWidget::getReal((QObject *)sender());
	//qDebug("XEMBED: CWindow::closed %p -> %p", sender(), THIS);
	//CWIDGET_destroy(CWidget::getReal((QObject *)sender()));
	delete sender();
}
#endif

void CWindow::destroy(void)
{
	CWINDOW *_object = (CWINDOW *)CWidget::getReal((QObject *)sender());
	//qDebug("XEMBED: CWindow::destroy %p -> %p", sender(), THIS);

	if (THIS)
	{
		do_close(THIS, 0, true);
		CWindow::removeTopLevel(THIS);
	}

#ifndef QT5
	CWINDOW_EmbedState = EMBED_WAIT;
	CWINDOW_Embedded = false;
	CWINDOW_Embedder = 0;
#endif
}

void CWindow::insertTopLevel(CWINDOW *_object)
{
	if (!THIS->toplevel)
		return;

	list.append(THIS);
	count = list.count();

	#if DEBUG_WINDOW
	qDebug("insertTopLevel: count = %d (%p %s %s)", count, _object, THIS->widget.name, THIS->embedded ? "E" : "W");
	#endif
}

void CWindow::removeTopLevel(CWINDOW *_object)
{
	if (!THIS->toplevel)
		return;

	list.removeAll(THIS);
  count = list.count();

	#if DEBUG_WINDOW
	qDebug("removeTopLevel: count = %d (%p %s %s)", count, THIS, THIS->widget.name, THIS->embedded ? "E" : "W");
	#endif

	MAIN_check_quit();
}

CMENU *CWindow::findMenu(CWINDOW *_object, const char *name)
{
	int i;
	CMENU *menu;
	CWIDGET *parent;

	for(;;)
	{
		if (THIS->menuBar)
		{
			for (i = 0; i < THIS->menuBar->actions().count(); i++)
			{
				menu = CMenu::dict[THIS->menuBar->actions().at(i)];
				if (!menu)
					continue;
				if (!strcasecmp(menu->widget.name, name))
					return menu;
			}
		}
		
		parent = (CWIDGET *)CWIDGET_get_parent(THIS);
		if (!parent)
			break;
		_object = CWidget::getWindow(parent);
		if (!THIS)
			break;
	}

	return NULL;
}

