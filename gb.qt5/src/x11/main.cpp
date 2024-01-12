/***************************************************************************

  main.cpp

  (c) Benoît Minisini <gambas@users.sourceforge.net>

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

#define __MAIN_C

#include "main.h"

#include <QAbstractNativeEventFilter>
#include <QPointer>
#include <QApplication>
#include <QScreen>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "x11.h"

static int _x11_last_key_code = 0;
static int (*_x11_event_filter)(XEvent *) = 0;
static QPointer<QWidget> _mouseGrabber = 0;
static QPointer<QWidget> _keyboardGrabber = 0;

//-------------------------------------------------------------------------

class MyNativeEventFilter: public QAbstractNativeEventFilter
{
public:

	static MyNativeEventFilter manager;

	virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *)
	{
		xcb_generic_event_t *ev = static_cast<xcb_generic_event_t *>(message);
		int type = ev->response_type & ~0x80;

		switch(type)
		{
			case XCB_KEY_PRESS:
			case XCB_KEY_RELEASE:
				_x11_last_key_code = ((xcb_key_press_event_t *)ev)->detail;
				break;
		}

		if (_x11_event_filter)
		{
			XEvent xev;

			CLEAR(&xev);
			xev.xany.type = type;
			xev.xany.display = QX11Info::display();
			xev.xany.send_event = ev->response_type & 0x80 ? 1 : 0;

			switch (type)
			{
				//case XCB_KEY_PRESS:
				//case XCB_KEY_RELEASE:
				case XCB_EXPOSE:
				{
					xcb_expose_event_t *e = (xcb_expose_event_t *)ev;
					xev.xexpose.window = e->window;
					xev.xexpose.x = e->x;
					xev.xexpose.y = e->y;
					xev.xexpose.width = e->width;
					xev.xexpose.height = e->height;
					xev.xexpose.count = e->count;
					break;
				}

				case XCB_VISIBILITY_NOTIFY:
				{
					xcb_visibility_notify_event_t *e = (xcb_visibility_notify_event_t *)ev;
					xev.xvisibility.window = e->window;
					xev.xvisibility.state = e->state;
					break;
				}

				case XCB_DESTROY_NOTIFY:
				{
					xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *)ev;
					xev.xdestroywindow.event = e->event;
					xev.xdestroywindow.window = e->window;
					break;
				}

				case XCB_MAP_NOTIFY:
				{
					xcb_map_notify_event_t *e = (xcb_map_notify_event_t *)ev;
					xev.xmap.event = e->event;
					xev.xmap.window = e->window;
					xev.xmap.override_redirect = e->override_redirect;
					break;
				}

				case XCB_UNMAP_NOTIFY:
				{
					xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *)ev;
					xev.xunmap.event = e->event;
					xev.xunmap.window = e->window;
					xev.xunmap.from_configure = e->from_configure;
					break;
				}

				case XCB_REPARENT_NOTIFY:
				{
					xcb_reparent_notify_event_t *e = (xcb_reparent_notify_event_t *)ev;
					xev.xreparent.event = e->event;
					xev.xreparent.window = e->window;
					xev.xreparent.parent = e->parent;
					xev.xreparent.x = e->x;
					xev.xreparent.y = e->y;
					xev.xreparent.override_redirect = e->override_redirect;
					break;
				}

				case XCB_CONFIGURE_NOTIFY:
				{
					xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t *)ev;
					xev.xconfigure.event = e->event;
					xev.xconfigure.window = e->window;
					xev.xconfigure.x = e->x;
					xev.xconfigure.y = e->y;
					xev.xconfigure.width = e->width;
					xev.xconfigure.height = e->height;
					xev.xconfigure.border_width = e->border_width;
					xev.xconfigure.override_redirect = e->override_redirect;
					break;
				}
				
				case XCB_PROPERTY_NOTIFY:
				{
					xcb_property_notify_event_t *e = (xcb_property_notify_event_t *)ev;
					xev.xproperty.window = e->window;
					xev.xproperty.atom = e->atom;
					xev.xproperty.time = e->time;
					xev.xproperty.state = e->state;
					break;
				}

				case XCB_SELECTION_CLEAR:
				{
					xcb_selection_clear_event_t *e = (xcb_selection_clear_event_t *)ev;
					xev.xselectionclear.window = e->owner;
					xev.xselectionclear.selection = e->selection;
					xev.xselectionclear.time = e->time;
					break;
				}

				case XCB_SELECTION_REQUEST:
				{
					xcb_selection_request_event_t *e = (xcb_selection_request_event_t *)ev;
					xev.xselectionrequest.owner = e->owner;
					xev.xselectionrequest.requestor = e->requestor;
					xev.xselectionrequest.selection = e->selection;
					xev.xselectionrequest.target = e->target;
					xev.xselectionrequest.property = e->property;
					xev.xselectionrequest.time = e->time;
					break;
				}

				case XCB_SELECTION_NOTIFY:
				{
					xcb_selection_notify_event_t *e = (xcb_selection_notify_event_t *)ev;
					xev.xselection.requestor = e->requestor;
					xev.xselection.selection = e->selection;
					xev.xselection.target = e->target;
					xev.xselection.property = e->property;
					xev.xselection.time = e->time;
					break;
				}

				case XCB_CLIENT_MESSAGE:
				{
					xcb_client_message_event_t *e = (xcb_client_message_event_t *)ev;
					xev.xclient.window = e->window;
					xev.xclient.message_type = e->type;
					xev.xclient.format = e->format;
					xev.xclient.data.l[0] = e->data.data32[0];
					xev.xclient.data.l[1] = e->data.data32[1];
					xev.xclient.data.l[2] = e->data.data32[2];
					xev.xclient.data.l[3] = e->data.data32[3];
					xev.xclient.data.l[4] = e->data.data32[4];
					break;
				}

				default:
					qDebug("gb.qt5: warning: unhandled xcb event: %d", type);
					return false;
			}

			return (*_x11_event_filter)(&xev) != 0;
		}

		return false;
	}
};

MyNativeEventFilter MyNativeEventFilter::manager;

//-------------------------------------------------------------------------

static void platform_init(void)
{
	char *env;

	env = getenv("GB_X11_INIT_THREADS");
	if (env && atoi(env))
		XInitThreads();

	X11_init(QX11Info::display(), QX11Info::appRootWindow());
	
	qApp->installNativeEventFilter(&MyNativeEventFilter::manager);
}

static void platform_exit(void)
{
	X11_exit();
}

//-------------------------------------------------------------------------

static void release_grab()
{
	_mouseGrabber = QWidget::mouseGrabber();
	_keyboardGrabber = QWidget::keyboardGrabber();

	if (_mouseGrabber)
	{
		//qDebug("releaseMouse");
		_mouseGrabber->releaseMouse();
	}
	if (_keyboardGrabber)
	{
		//qDebug("releaseKeyboard");
		_keyboardGrabber->releaseKeyboard();
	}

	#ifndef NO_X_WINDOW
	if (qApp->activePopupWidget())
	{
		XUngrabPointer(QX11Info::display(), CurrentTime);
		XFlush(QX11Info::display());
	}
	#endif
}

static void unrelease_grab()
{
	if (_mouseGrabber)
	{
		//qDebug("grabMouse");
		_mouseGrabber->grabMouse();
		_mouseGrabber = 0;
	}

	if (_keyboardGrabber)
	{
		//qDebug("grabKeyboard");
		_keyboardGrabber->grabKeyboard();
		_keyboardGrabber = 0;
	}
}

static int get_last_key_code(void)
{
	return _x11_last_key_code;
}

//-------------------------------------------------------------------------

static int desktop_get_resolution_x(void)
{
	return QX11Info::appDpiX();
}

static int desktop_get_resolution_y(void)
{
	return QX11Info::appDpiY();
}

static void desktop_screenshot(QPixmap *pixmap, int x, int y, int w, int h)
{
	*pixmap = qApp->primaryScreen()->grabWindow(QX11Info::appRootWindow(), x, y, w, h);
}

//-------------------------------------------------------------------------

static uintptr_t window_get_id(QWidget *window)
{
	return (uintptr_t)window->winId();
}

static int window_get_virtual_desktop(QWidget *window)
{
	return X11_window_get_desktop(window->winId());
}

static void window_set_virtual_desktop(QWidget *window, bool visible, int desktop)
{
	X11_window_set_desktop(window->winId(), window->isVisible(), desktop);
}

static void window_remap(QWidget *window)
{
	X11_window_remap(window->effectiveWinId());
}

static void window_set_properties(QWidget *window, int which, QT_WINDOW_PROP *prop)
{
	X11_flush();

	if (which & (PROP_STACKING | PROP_SKIP_TASKBAR))
	{
		X11_window_change_begin(window->effectiveWinId(), window->isVisible());

		if (which & PROP_STACKING)
		{
			X11_window_change_property(X11_atom_net_wm_state_above, prop->stacking == 1);
			X11_window_change_property(X11_atom_net_wm_state_stays_on_top, prop->stacking == 1);
			X11_window_change_property(X11_atom_net_wm_state_below, prop->stacking == 2);
		}
		if (which & PROP_SKIP_TASKBAR)
			X11_window_change_property(X11_atom_net_wm_state_skip_taskbar, prop->skipTaskbar);

		X11_window_change_end();
	}

	if (which & PROP_BORDER)
		X11_set_window_decorated(window->effectiveWinId(), prop->border);

	if (which & PROP_STICKY)
		X11_window_set_desktop(window->effectiveWinId(), window->isVisible(), prop->sticky ? 0xFFFFFFFF : X11_get_current_desktop());

	X11_flush();
}

static void window_set_user_time(QWidget *window, int timestamp)
{
	X11_window_set_user_time(window->effectiveWinId(), 0);
}

static void window_set_transient_for(QWidget *window, QWidget *parent)
{
	X11_set_transient_for(window->effectiveWinId(), parent->effectiveWinId());
}

static void window_activate(QWidget *window)
{
	window->activateWindow();
}

//-------------------------------------------------------------------------

static void x11_set_event_filter(int (*filter)(XEvent *))
{
	_x11_event_filter = filter;
}

//-------------------------------------------------------------------------

extern "C" {

const GB_INTERFACE *GB_PTR EXPORT;

void *GB_QT5_X11_1[] EXPORT = {

  (void *)QT_PLATFORM_INTERFACE_VERSION,
  
  (void *)platform_init,
  (void *)platform_exit,
  
  (void *)release_grab,
  (void *)unrelease_grab,
  (void *)get_last_key_code,
  
  (void *)desktop_get_resolution_x,
  (void *)desktop_get_resolution_y,
  (void *)desktop_screenshot,
  
  (void *)window_get_id,
  (void *)window_get_virtual_desktop,
  (void *)window_set_virtual_desktop,
  (void *)window_remap,
  (void *)window_set_properties,
  (void *)window_set_user_time,
  (void *)window_set_transient_for,
  (void *)window_activate,

  NULL
  };


int EXPORT GB_INFO(const char *key, void **value)
{
	if (!strcasecmp(key, "DISPLAY"))
	{
		*value = (void *)QX11Info::display();
		return TRUE;
	}
	else if (!strcasecmp(key, "ROOT_WINDOW"))
	{
		*value = (void *)QX11Info::appRootWindow();
		return TRUE;
	}
	else if (!strcasecmp(key, "SET_EVENT_FILTER"))
	{
		*value = (void *)x11_set_event_filter;
		return TRUE;
	}
	else if (!strcasecmp(key, "TIME"))
	{
		*value = (void *)QX11Info::appTime();
		return TRUE;
	}
	else
		return FALSE;
}

int EXPORT GB_INIT(void)
{
	return 0;
}

void EXPORT GB_EXIT()
{
}

}

