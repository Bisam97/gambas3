/***************************************************************************

  gapplication.h

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

#ifndef __GAPPLICATION_H
#define __GAPPLICATION_H

#ifndef GTK3
#include <X11/Xlib.h>
typedef
	void (*X11_EVENT_FILTER)(XEvent *);
#endif

class gControl;
class gMainWindow;

class gApplication
{
public:
	static void init(int *argc, char ***argv);
	static bool isInit() { return _init; }
	static void quit();
	static void exit();
	static bool mustQuit() { return _must_quit; }

	static gControl* controlItem(GtkWidget *wid);

  static void setBusy(bool b);
  static bool isBusy() { return _busy; }

	static gControl* activeControl() { return _active_control; }
	static gControl* previousControl() { return _previous_control; }
	static gControl *grabControl() { return _control_grab; }
	static void setActiveControl(gControl *control, bool on);
	static void finishFocus();

	static bool areTooltipsEnabled();
	static void enableTooltips(bool vl);

	static bool hasMiddleClickPaste();

	static int dblClickTime();

	static void setDefaultTitle(const char *title);
	static char *defaultTitle() { return _title; }

	//static void setDirty();
	static int loopLevel() { return _loopLevel; }
	static void enterLoop(void *owner, bool showIt = false, GtkWindow *modal = NULL);
	static void enterPopup(gMainWindow *owner);
	static void exitLoop(void *owner);
	static bool hasLoop(void *owner) { return _loop_owner == owner; }
	static GtkWindowGroup *enterGroup();
	static void exitGroup(GtkWindowGroup *oldGroup);
	static guint32 lastEventTime() { return _event_time; }
	static GdkEvent *lastEvent() { return _event; }
	static void updateLastEvent(GdkEvent *e);
	static void updateLastEventTime();

	static bool (*onKeyEvent)(int type);

	static int getScrollbarSize();
	static int getScrollbarBigSize();
	static int getScrollbarSpacing();
	static int getFrameWidth();
	static int getInnerWidth();
	static void getBoxFrame(int *w, int *h);
	static char *getStyleName();

	static void grabPopup();
	static void ungrabPopup();

	static void setMainWindow(gMainWindow *win);
	static gMainWindow *mainWindow() { return _main_window; }

	static void checkHoveredControl(gControl *control);

	#ifndef GTK3
	static void setEventFilter(X11_EVENT_FILTER filter);
	#endif

	static void setButtonGrab(gControl *grab) { _button_grab = grab; }
	
	static void onThemeChange();
	
	static void forEachControl(void (*cb)(gControl *), bool (*filter)(gControl *) = NULL);
	
	static bool disableInputEvents(bool disable);
	static bool areInputEventsEnabled() { return !_disable_input_events; }
	static bool eventsPending();
	static bool processInputEvent() { return true; }

	static void updateDarkTheme();
	static bool isDarkTheme() { return _dark_theme; }
	static void setDarkTheme(bool v);
#if 0
	static void pushInputEvent(GdkEvent *);
#endif
	
	static bool _fix_breeze;
	static bool _fix_oxygen;
	static int _scrollbar_size;
	static int _scrollbar_big_size;

	//"Private"
	static bool _init;
	static bool _busy;
	static bool _must_quit;
	static char *_title;
	static char *_theme;
	static int _loopLevel;
	static int _in_popup;
	static GtkWidget *_popup_grab;
	static void *_loop_owner;
	static GtkWindowGroup *_group;
	static GtkWindowGroup *currentGroup();
	static gControl *_enter;
	static gControl *_leave;
	static gControl *_ignore_until_next_enter;
	static gControl *_active_control;
	static gControl *_previous_control;
	static gControl *_old_active_control;
	static gControl *_button_grab;
	static gControl *_enter_after_button_grab;
	static gControl *_control_grab;
	static guint32 _event_time;
	static GdkEvent *_event;
	static gMainWindow *_main_window;
	static bool _close_next_window;
	static bool _fix_printer_dialog;
	static void (*onEnterEventLoop)();
	static void (*onLeaveEventLoop)();
	static bool _keep_focus;
	static bool _disable_mapping_events;
	static bool _disable_input_events;
	static GQueue *_input_events;
	static bool _dark_theme;
	static bool _dark_theme_set;
};

#endif
