/***************************************************************************

  main.cpp

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

#define __MAIN_C

#include <stdio.h>

#include "main.h"
#include "gb.image.h"
#include "gb.gtk.h"
#include "watcher.h"
#include "gglarea.h"
#include "gkey.h"

#include "x11.h"
#include "CScreen.h"
#include "CStyle.h"
#include "CDraw.h"
#include "CConst.h"
#include "CColor.h"
#include "CFont.h"
#include "CKey.h"
#include "CPicture.h"
#include "CImage.h"
#include "CClipboard.h"
#include "CMouse.h"
#include "CDialog.h"
#include "CWatcher.h"
#include "CWidget.h"
#include "CDrawingArea.h"
#include "CContainer.h"
#include "CPanel.h"
#include "CMenu.h"
#include "CWindow.h"
#include "CButton.h"
#include "CTextBox.h"
#include "CTextArea.h"
#include "CSlider.h"
#include "CTabStrip.h"
#include "CTrayIcon.h"
#include "cprinter.h"
#include "csvgimage.h"
#include "canimation.h"

#include <gtk/gtk.h>
#include <string.h>

GB_CLASS CLASS_Control;
GB_CLASS CLASS_ContainerChildren;
GB_CLASS CLASS_UserControl;
GB_CLASS CLASS_UserContainer;
GB_CLASS CLASS_Picture;
GB_CLASS CLASS_Image;
GB_CLASS CLASS_DrawingArea;
GB_CLASS CLASS_Menu;
GB_CLASS CLASS_Window;
GB_CLASS CLASS_Printer;
GB_CLASS CLASS_SvgImage;

static void hook_lang(char *lang, int rtl1);
static bool hook_error(int code, char *error, char *where, bool can_ignore);
static void hook_quit(void);
static void hook_main(int *argc, char ***argv);
static void hook_timer(GB_TIMER *timer,bool on);
static void hook_wait(int duration);
static void hook_post(void);
static int hook_loop();
static void hook_watch(int fd, int type, void *callback, intptr_t param);

static bool _post_check = false;
static bool _must_check_quit = false;

static bool _application_keypress = false;
static GB_FUNCTION _application_keypress_func;

static void *_old_hook_main;

bool MAIN_display_x11 = FALSE;
int MAIN_scale = 0;
bool MAIN_debug_busy = false;
bool MAIN_rtl = false;

//-------------------------------------------------------------------------

static void GTK_CreateControl(CWIDGET *ob, void *parent, GtkWidget *widget, uint flags)
{
	gControl *ctrl;
	bool recreate;
	
	if (!parent)
	{
		recreate = true;
		ctrl = ob->widget;
		ctrl->parent()->remove(ctrl);
		ctrl->createBorder(widget);
	}
	else
	{
		recreate = false;
		ctrl = new gControl(CONTAINER(parent));
		ctrl->border = widget;
	}
	
	ctrl->widget = ctrl->border;
	InitControl(ctrl, ob);
	ctrl->realize();
	
	if (flags & CCF_HAS_INPUT_METHOD)
		ctrl->_has_input_method = TRUE;
	
	if (recreate)
		ctrl->updateGeometry(true);
}

static GtkWidget *GTK_CreateGLArea(void *_object, void *parent, void (*init)(GtkWidget *))
{
	gControl *ctrl = new gGLArea(CONTAINER(parent), init);
	InitControl(ctrl, (CWIDGET *)_object);
	//WIDGET->onExpose = Darea_Expose;
	return ctrl->widget;
}

static void *GTK_CreatePicture(cairo_surface_t *surf, int w, int h)
{
	gPicture *p = new gPicture(surf);
	
	if (w > 0 && h > 0)
	{
		gPicture *p2 = p->stretch(w, h, true);
		p->unref();
		p = p2;
	}
	
	return CPICTURE_create(p);
}

static int GTK_GetDesktopScale(void)
{
	return MAIN_scale;
}


//-------------------------------------------------------------------------

extern "C"
{

const GB_INTERFACE *GB_PTR EXPORT;
IMAGE_INTERFACE IMAGE EXPORT;
GEOM_INTERFACE GEOM EXPORT;

static void declare_tray_icon()
{
	GB.Component.Declare(TrayIconsDesc);
	GB.Component.Declare(TrayIconDesc);
}

GB_DESC *GB_CLASSES[] EXPORT =
{
	ScreenDesc,
	ScreensDesc,
	DesktopDesc,
	ApplicationDesc,
	StyleDesc,
	SelectDesc,
	AlignDesc,
	ArrangeDesc,
	BorderDesc,
	ScrollDesc,
	DirectionDesc,
	CColorDesc,
	CFontsDesc,
	CFontDesc,
	CKeyDesc,
	CImageDesc,
	CPictureDesc,
	AnimationDesc,
	CClipboardDesc,
	CDragDesc,
	CCursorDesc,
	CMouseDesc,
	CPointerDesc,
	CDialogDesc,
	CWatcherDesc,
	CWidgetDesc,
	ContainerChildrenDesc,
	ContainerDesc,
	CDrawingAreaDesc,
	UserControlDesc,
	UserContainerDesc,
	CPanelDesc,
	CHBoxDesc,
	CVBoxDesc,
	CHPanelDesc,
	CVPanelDesc,
	CMenuDesc,
	CMenuChildrenDesc,
	CWindowMenusDesc,
	CWindowControlsDesc,
	CWindowDesc,
	CWindowsDesc,
	CFormDesc,
	SliderDesc,
	ScrollBarDesc,
	CButtonDesc,
	CToggleButtonDesc,
	CCheckBoxDesc,
	CRadioButtonDesc,
	CToolButtonDesc,
	CTextBoxSelectionDesc,
	CTextBoxDesc,
	CTextAreaDesc,
	CTextAreaSelectionDesc,
	CTabStripDesc,
	CTabStripContainerDesc,
	CTabStripContainerChildrenDesc,
	CPluginDesc,
	PrinterDesc,
	SvgImageDesc,
	NULL
};

#ifdef GTK3
void *GB_GTK3_1[] EXPORT =
#else
void *GB_GTK_1[] EXPORT =
#endif
{
	(void *)GTK_INTERFACE_VERSION,
	(void *)GTK_CreateControl,
	(void *)GTK_CreateGLArea,
	(void *)GTK_CreatePicture,
	(void *)GTK_GetDesktopScale,
	NULL
};

const char *GB_INCLUDE EXPORT = "gb.draw,gb.gui.base";

int EXPORT GB_INIT(void)
{
	char *env;

	env = getenv("GB_GUI_BUSY");
	if (env && atoi(env))
		MAIN_debug_busy = true;

	putenv((char *)"GTK_OVERLAY_SCROLLING=0");
	
	GB.Hook(GB_HOOK_QUIT, (void *)hook_quit);
	_old_hook_main = GB.Hook(GB_HOOK_MAIN, (void *)hook_main);
	GB.Hook(GB_HOOK_WAIT, (void *)hook_wait);
	GB.Hook(GB_HOOK_TIMER,(void *)hook_timer);
	GB.Hook(GB_HOOK_WATCH,(void *)hook_watch);
	GB.Hook(GB_HOOK_POST,(void*)hook_post);
	GB.Hook(GB_HOOK_ERROR,(void*)hook_error);
	GB.Hook(GB_HOOK_LANG,(void*)hook_lang);
	GB.Hook(GB_HOOK_LOOP, (void *)hook_loop);

	GB.Component.Load("gb.draw");
	GB.Component.Load("gb.image");
	GB.Component.Load("gb.gui.base");

	GB.GetInterface("gb.image", IMAGE_INTERFACE_VERSION, &IMAGE);
	GB.GetInterface("gb.geom", GEOM_INTERFACE_VERSION, &GEOM);

	GB.Signal.MustCheck(SIGCHLD);
	
	IMAGE.SetDefaultFormat(GB_IMAGE_RGBA);
	DRAW_init();

	CWatcher::init();

	CLASS_Control = GB.FindClass("Control");
	CLASS_ContainerChildren = GB.FindClass("ContainerChildren");
	CLASS_UserControl = GB.FindClass("UserControl");
	CLASS_UserContainer = GB.FindClass("UserContainer");
	CLASS_Window = GB.FindClass("Window");
	CLASS_Menu = GB.FindClass("Menu");
	CLASS_Picture = GB.FindClass("Picture");
	//CLASS_Drawing = GB.FindClass("Drawing");
	CLASS_DrawingArea = GB.FindClass("DrawingArea");
	CLASS_Printer = GB.FindClass("Printer");
	CLASS_Image = GB.FindClass("Image");
	CLASS_SvgImage = GB.FindClass("SvgImage");

#if !defined(GLIB_VERSION_2_36)
	g_type_init();
#endif /* !defined(GLIB_VERSION_2_36) */

	hook_lang(GB.System.Language(), GB.System.IsRightToLeft());

	return -1;
}

void EXPORT GB_EXIT()
{
	CWatcher::exit();
}

int EXPORT GB_INFO(const char *key, void **value)
{
	if (MAIN_display_x11)
	{
		if (!strcasecmp(key, "DISPLAY"))
		{
			*value = (void *)gdk_x11_display_get_xdisplay(gdk_display_get_default());
			return TRUE;
		}
		else if (!strcasecmp(key, "ROOT_WINDOW"))
		{
			*value = (void *)gdk_x11_get_default_root_xwindow();
			return TRUE;
		}
	}

	if (!strcasecmp(key, "GET_HANDLE"))
	{
		*value = (void *)CWIDGET_get_handle;
		return TRUE;
	}
	else if (!strcasecmp(key, "SET_EVENT_FILTER"))
	{
		*value = (void *)gApplication::setEventFilter;
		return TRUE;
	}
	else if (!strcasecmp(key, "TIME"))
	{
		*value = (void *)(intptr_t)gtk_get_current_event_time(); //gdk_x11_display_get_user_time(gdk_display_get_default());
		return TRUE;
	}
	else if (!strcasecmp(key, "DECLARE_TRAYICON"))
	{
		*value = (void *)declare_tray_icon;
		return TRUE;
	}
	else
		return FALSE;
}

static void activate_main_window(intptr_t)
{
	if (gMainWindow::_active)
		gtk_window_present(GTK_WINDOW(gMainWindow::_active->topLevel()->border));
}

void EXPORT GB_SIGNAL(int signal, void *param)
{
	static GtkWidget *save_popup_grab = NULL;

	switch(signal)
	{
		case GB_SIGNAL_DEBUG_BREAK:
			if (gApplication::_popup_grab)
			{
				save_popup_grab = gApplication::_popup_grab;
				gApplication::ungrabPopup();
			}
			break;

		case GB_SIGNAL_DEBUG_FORWARD:
			//while (qApp->activePopupWidget())
			//	delete qApp->activePopupWidget();
			if (gdk_display_get_default())
			gdk_display_sync(gdk_display_get_default());
			break;

		case GB_SIGNAL_DEBUG_CONTINUE:
			GB.Post((GB_CALLBACK)activate_main_window, 0);
			if (save_popup_grab)
			{
				gApplication::_popup_grab = save_popup_grab;
				save_popup_grab = NULL;
				gApplication::grabPopup();
			}
			break;
	}
}

} // extern "C"

void hook_quit (void)
{
	GB_FUNCTION func;

	while (!gApplication::processInputEvent());
	while (gApplication::eventsPending())
		gtk_main_iteration();

	if (GB.ExistClass("TrayIcons"))
	{
		if (!GB.GetFunction(&func, (void *)GB.FindClass("TrayIcons"), "DeleteAll", NULL, NULL))
			GB.Call(&func, 0, FALSE);
	}

	if (!GB.GetFunction(&func, (void *)GB.FindClass("_Gui"), "_Quit", NULL, NULL))
		GB.Call(&func, 0, FALSE);

	CWINDOW_delete_all(true);
	gControl::postDelete();

	//CWatcher::Clear();
	gApplication::exit();

	#ifdef GDK_WINDOWING_X11
		if (MAIN_display_x11)
			X11_exit();
  #endif
}

static bool global_key_event_handler(int type)
{
	GB.Call(&_application_keypress_func, 0, FALSE);
	return GB.Stopped();
}

static void hook_main(int *argc, char ***argv)
{
	static bool init = false;
	char *env;

	if (init)
		return;

	env = getenv("GB_X11_INIT_THREADS");
	if (env && atoi(env))
		XInitThreads();

	gtk_init(argc, argv);
	gApplication::init(argc, argv);
	gApplication::setDefaultTitle(GB.Application.Title());
	CAPPLICATION_MiddleClickPaste = gApplication::hasMiddleClickPaste();

	gApplication::onEnterEventLoop = GB.Debug.EnterEventLoop;
	gApplication::onLeaveEventLoop = GB.Debug.LeaveEventLoop;

	MAIN_scale = gDesktop::scale();
	#ifdef GDK_WINDOWING_X11
		#ifdef GTK3
		if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
		#endif
		{
			X11_init(gdk_x11_display_get_xdisplay(gdk_display_get_default()), gdk_x11_get_default_root_xwindow());
			MAIN_display_x11 = TRUE;
		}
  #endif

	if (GB.GetFunction(&_application_keypress_func, (void *)GB.Application.StartupClass(), "Application_KeyPress", "", "") == 0)
	{
		_application_keypress = true;
		gApplication::onKeyEvent = global_key_event_handler;
	}

	init = true;
	
	CALL_HOOK_MAIN(_old_hook_main, argc, argv);
}

/*static void raise_timer(GB_TIMER *timer)
{
	GB.RaiseTimer(timer);
	GB.Unref(POINTER(&timer));
}*/

typedef
	struct {
		int source;
		GTimer *timer;
		int timeout;
		}
	MyTimerId;

gboolean hook_timer_function(GB_TIMER *timer)
{
	intptr_t old_id = timer->id;

	if (!old_id)
		return false;

	GB.RaiseTimer(timer);

	if (timer->id == old_id) // If the event handler has removed the timer or restarted it, do nothing, just let the current source be removed.
	{
		MyTimerId *id = (MyTimerId *)timer->id;
		GTimer *t = id->timer;
		int elapsed = (int)(g_timer_elapsed(t, NULL) * 1000) - id->timeout;
		int next = timer->delay - elapsed;
		if (next < 10)
			next = 10;
		id->timeout = next;
		g_timer_start(t);
		id->source = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, next, (GSourceFunc)hook_timer_function, (gpointer)timer, NULL);
		//fprintf(stderr, "elapsed = %d  delay = %d  next = %d\n", elapsed, timer->delay, next);
	}

	return false;
}

static void hook_timer(GB_TIMER *timer, bool on)
{
	if (timer->id)
	{
		MyTimerId *id = (MyTimerId *)timer->id;
		g_source_remove(id->source);
		g_timer_destroy(id->timer);
		g_free(id);
		timer->id = 0;
	}

	if (on)
	{
		MyTimerId *id = g_new(MyTimerId, 1);
		id->timer = g_timer_new();
		id->timeout = timer->delay;
		id->source = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, timer->delay, (GSourceFunc)hook_timer_function, (gpointer)timer, NULL);
		timer->id = (intptr_t)id;
	}
	else
		MAIN_check_quit();
}

static void hook_post(void)
{
	_post_check = true;
}

void MAIN_check_quit()
{
	_must_check_quit = true;
}

static int hook_loop()
{
	gControl::postDelete();
	_must_check_quit = true;

	for(;;)
	{
		if (_must_check_quit)
		{
			if (gApplication::mustQuit())
				break;
			if (CWINDOW_must_quit() && CWatcher::count() == 0 && gTrayIcon::visibleCount() == 0 && !GB.HasActiveTimer())
				break;
			_must_check_quit = false;
		}
		MAIN_do_iteration(false);
	}

	hook_quit();

  return 0;
}

static void hook_wait(int duration)
{
	static bool _warning = FALSE;
	
	if (gDrawingArea::inAnyDrawEvent())
	{
		GB.Error("Wait is forbidden during a repaint event");
		return;
	}

	if (duration != -1 && gKey::isValid())
	{
		if (!_warning)
		{
			fprintf(stderr, "gb.gtk3: warning: calling the event loop during a keyboard event handler is ignored\n");
			_warning = TRUE;
		}
		return;
	}

	if (duration >= 0)
	{
		MAIN_do_iteration(true);
	}
	else if (duration == -1)
	{
		bool d = gApplication::disableInputEvents(true);
		MAIN_do_iteration(true); // To process deferred routines
		while (gApplication::eventsPending())
			MAIN_do_iteration(false);
		gApplication::disableInputEvents(d);
	}
	else if (duration == -2)
		MAIN_do_iteration(false);
}

static void hook_watch(int fd, int type, void *callback, intptr_t param)
{
	CWatcher::Add(fd,type,callback,param);
}

static bool hook_error(int code, char *error, char *where, bool can_ignore)
{
	gMainWindow *active;
	GtkWidget *dialog;
	char *msg;
	char scode[16];
	gint res;

	if (code > 0)
		sprintf(scode, " (#%d)", code);
	else
		*scode = 0;
	
	msg = g_strconcat("<b>This application has raised an unexpected error and must abort.</b>\n\n", error, scode, ".\n\n<tt>", where, "</tt>", NULL);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, NULL);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);
	if (can_ignore)
		gtk_dialog_add_button(GTK_DIALOG(dialog), GB.Translate("Ignore"), 2);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GB.Translate("Close"), 1);
	
	active = gDesktop::activeWindow();
	if (active)
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(active->border));
	
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	
	gtk_widget_destroy(dialog);
	
	g_free(msg);
	return (res == 2);
}

static void cb_update_lang(gControl *control)
{
	if (control->isVisible() && control->isContainer())
		((gContainer*)control)->performArrange();
}

static void hook_lang(char *lang, int rtl)
{
	MAIN_rtl = rtl;

	if (rtl)
		gtk_widget_set_default_direction(GTK_TEXT_DIR_RTL);
	else
		gtk_widget_set_default_direction(GTK_TEXT_DIR_LTR);
	
	gApplication::forEachControl(cb_update_lang);
}

void MAIN_do_iteration_just_events()
{
	if (gApplication::eventsPending())
	{
		while (!gApplication::processInputEvent());
		gtk_main_iteration_do(false);
	}
}

void MAIN_do_iteration(bool do_not_block)
{
	gApplication::_loopLevel++;

	while (!gApplication::processInputEvent());
	
	if (do_not_block)
	{
		if (gApplication::eventsPending())
			gtk_main_iteration();
	}
	else
		gtk_main_iteration_do(true);

	gApplication::_loopLevel--;

	gContainer::postArrange();

	if (_post_check)
	{
		_post_check = false;
		GB.CheckPost();
	}

	gControl::postDelete();
}

