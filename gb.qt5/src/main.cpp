/***************************************************************************

  main.cpp

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

#define __MAIN_CPP

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "gambas.h"

#include <QApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QString>
#include <QMap>
#include <QFileInfo>
#include <QBuffer>
#include <QWidget>
#include <QEvent>
#include <QTextCodec>
#include <QTimer>
#include <QTranslator>
#include <QTimerEvent>
#include <QKeyEvent>
#include <QPixmap>
#include <QImageReader>
#include <QEventLoop>
#include <QDesktopWidget>
#include <QPaintDevice>
#include <QLibraryInfo>
#include <QThreadPool>
#include <QScreen>

#include "gb.image.h"
#include "gb.form.font.h"

#include "CFont.h"
#include "CScreen.h"
#include "CStyle.h"
#include "CWidget.h"
#include "CWindow.h"
#include "CButton.h"
#include "CContainer.h"
#include "CTextBox.h"
#include "CTextArea.h"
#include "CMenu.h"
#include "CPanel.h"
#include "CMouse.h"
#include "CKey.h"
#include "CColor.h"
#include "CConst.h"
#include "CCheckBox.h"
#include "CRadioButton.h"
#include "CTabStrip.h"
#include "CDialog.h"
#include "CPicture.h"
#include "CImage.h"
#include "canimation.h"
#include "CClipboard.h"
#include "CDraw.h"
#include "CWatch.h"
#include "CDrawingArea.h"
#include "CSlider.h"
#include "CScrollBar.h"
#include "CWatcher.h"
#include "cprinter.h"
#include "csvgimage.h"
#include "cpaint_impl.h"
#include "ctrayicon.h"

#include "gb.qt.platform.h"

#include "fix_style.h"
#include "main.h"

/*#define DEBUG*/

extern "C" {
const GB_INTERFACE *GB_PTR EXPORT;
IMAGE_INTERFACE IMAGE EXPORT;
GEOM_INTERFACE GEOM EXPORT;
QT_PLATFORM_INTERFACE PLATFORM EXPORT;
}

int MAIN_in_wait = 0;
int MAIN_in_message_box = 0;
int MAIN_loop_level = 0;
int MAIN_scale = 6;
bool MAIN_debug_busy = false;
bool MAIN_init = false;
bool MAIN_key_debug = false;
bool MAIN_right_to_left = false;
const char *MAIN_platform = NULL;
bool MAIN_platform_is_wayland = false;

GB_CLASS CLASS_Control;
GB_CLASS CLASS_Container;
GB_CLASS CLASS_ContainerChildren;
GB_CLASS CLASS_UserControl;
GB_CLASS CLASS_UserContainer;
GB_CLASS CLASS_TabStrip;
GB_CLASS CLASS_Window;
GB_CLASS CLASS_Menu;
GB_CLASS CLASS_Picture;
GB_CLASS CLASS_Drawing;
GB_CLASS CLASS_DrawingArea;
GB_CLASS CLASS_Printer;
GB_CLASS CLASS_Image;
GB_CLASS CLASS_SvgImage;
GB_CLASS CLASS_TextArea;

static bool in_event_loop = false;
static int _no_destroy = 0;
static QTranslator *_translator = NULL;
static bool _application_keypress = false;
static GB_FUNCTION _application_keypress_func;
static bool _check_quit_posted = false;
static int _prevent_quit = 0;

static QHash<void *, void *> _link_map;

static QByteArray _utf8_buffer[UTF8_NBUF];
static int _utf8_count = 0;
static int _utf8_length = 0;

static void QT_Init(void);

static QtMessageHandler _previousMessageHandler;

static void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg )
{
	//fprintf(stderr, "---- `%s'\n", QT_ToUtf8(msg));
	
	if (msg == "QXcbClipboard: SelectionRequest too old")
		return;
	
	if (msg.startsWith("QXcbConnection: ") && msg.contains("(TranslateCoords)"))
		return;

	_previousMessageHandler(type, context, msg);
}

//static MyApplication *myApp;

/***************************************************************************

	MyMimeSourceFactory

	Create a QMimeSourceFactory to handle files stored in an archive

***************************************************************************/

#if 0
class MyMimeSourceFactory: public Q3MimeSourceFactory
{
public:

	MyMimeSourceFactory();

	virtual const QMimeSource* data(const QString& abs_name) const;

private:

	QMap<QString, QString> extensions;
};


MyMimeSourceFactory::MyMimeSourceFactory()
{
	extensions.replace("htm", "text/html;charset=UTF-8");
	extensions.replace("html", "text/html;charset=UTF-8");
	extensions.replace("txt", "text/plain");
	extensions.replace("xml", "text/xml;charset=UTF-8");
	extensions.replace("jpg", "image/jpeg");
	extensions.replace("png", "image/png");
	extensions.replace("gif", "image/gif");
}


const QMimeSource* MyMimeSourceFactory::data(const QString& abs_name) const
{
	char *addr;
	int len;
	Q3StoredDrag* sr = 0;
	char *path;

	//qDebug("MyMimeSourceFactory::data: %s", (char *)abs_name.latin1());

	path = (char *)abs_name.latin1();

	if (true) //abs_name[0] != '/')
	{
		if (GB.LoadFile(path, 0, &addr, &len))
			GB.Error(NULL);
		else
		{
			QByteArray ba;
			ba.setRawData((const char *)addr, len);

			QFileInfo fi(abs_name);
			QString e = fi.extension(FALSE);
			Q3CString mimetype = "text/html"; //"application/octet-stream";

			const char* imgfmt;

			if ( extensions.contains(e) )
				mimetype = extensions[e].latin1();
			else
			{
				QBuffer buffer(&ba);

				buffer.open(QIODevice::ReadOnly);
				if (( imgfmt = QImageReader::imageFormat( &buffer ) ) )
					mimetype = Q3CString("image/")+Q3CString(imgfmt).lower();
				buffer.close();
			}

			sr = new Q3StoredDrag( mimetype );
			sr->setEncodedData( ba );

			ba.resetRawData((const char*)addr, len);

			//qDebug("MimeSource: %s %s", abs_name.latin1(), (const char *)mimetype);

			GB.ReleaseFile(addr, len);
		}
	}

	return sr;
}

static MyMimeSourceFactory myMimeSourceFactory;
#endif

#if 0
/***************************************************************************

	MyAbstractEventDispatcher

	Manage window deletion

***************************************************************************/

class MyAbstractEventDispatcher : public QAbstractEventDispatcher
{
public:
	MyAbstractEventDispatcher();
	virtual bool processEvents(QEventLoop::ProcessEventsFlags flags);
};

MyAbstractEventDispatcher::MyAbstractEventDispatcher()
: QAbstractEventDispatcher()
{
}

bool MyAbstractEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
	bool ret;
	CWIDGET **ptr;
	CWIDGET *ob;

	MAIN_loop_level++;
	ret = QAbstractEventDispatcher::processEvents(flags);
	MAIN_loop_level--;

	for(;;)
	{
		ptr = &CWIDGET_destroy_list;

		for(;;)
		{
			ob = *ptr;
			if (!ob)
				return ret;

			//if (MAIN_loop_level <= ob->level && !ob->flag.notified)
			if (!ob->flag.notified)
			{
				//qDebug("delete: %s %p", GB.GetClassName(ob), ob);
				//qDebug(">> delete %p (%p) :%p:%ld", ob, ob->widget, ob->ob.klass, ob->ob.ref);
				//*ptr = ob->next;
				delete ob->widget;
				break;
				//GB.Unref(POINTER(&ob));
				//qDebug("   delete %p (%p) :%p:%ld #2", ob, ob->widget, ob->ob.klass, ob->ob.ref);
				//qDebug("<< delete %p (%p)", ob, ob->widget);
			}
			else
			{
				//qDebug("cannot delete: %s %p", GB.GetClassName(ob), ob);
				ptr = &ob->next;
			}
		}
	}
	//return ret;
}
#endif

void MAIN_process_events(void)
{
	_no_destroy++;
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 0);
	_no_destroy--;
}

void MAIN_init_error()
{
	GB.Error("GUI is not initialized");
}

/** MyApplication **********************************************************/

bool MyApplication::_tooltip_disable = false;
int MyApplication::_event_filter = 0;
QEventLoop *MyApplication::eventLoop = 0;

MyApplication::MyApplication(int &argc, char **argv)
: QApplication(argc, argv)
{
	if (isSessionRestored())
	{
		bool ok;
		int desktop;

		if (argc >= 2 && ::strcmp(argv[argc - 2], "-session-desktop") == 0)
		{
			desktop = QString(argv[argc - 1]).toInt(&ok);
			if (ok)
				CWINDOW_MainDesktop = desktop;

			//qDebug("session desktop: %d", CWINDOW_MainDesktop);

			argc -= 2;
		}
	}

	QObject::connect(this, SIGNAL(commitDataRequest(QSessionManager &)), SLOT(commitDataRequested(QSessionManager &)));
	QObject::connect(this, SIGNAL(paletteChanged(const QPalette &)), SLOT(paletteHasChanged(const QPalette &)));
}

void MyApplication::initClipboard()
{
  QObject::connect(clipboard(), SIGNAL(changed(QClipboard::Mode)), qApp, SLOT(clipboardHasChanged(QClipboard::Mode)));
}

void MyApplication::clipboardHasChanged(QClipboard::Mode m)
{
	CLIPBOARD_has_changed(m);
}

void MyApplication::paletteHasChanged(const QPalette &)
{
	if (COLOR_update_palette())
	{
		//fprintf(stderr, "updatePalette\n");
		APPLICATION_update_dark_theme();
		APPLICATION_send_change_event(CHANGE_COLOR);
	}
}

static bool QT_EventFilter(QEvent *e)
{
	bool cancel;

	if (!_application_keypress)
		return false;

	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent *kevent = (QKeyEvent *)e;

		CKEY_clear(true);

		GB.FreeString(&CKEY_info.text);
		CKEY_info.text = GB.NewZeroString(QT_ToUtf8(kevent->text()));
		CKEY_info.state = kevent->modifiers();
		CKEY_info.code = kevent->key();

	}
	else if (e->type() == QEvent::InputMethod)
	{
		QInputMethodEvent *imevent = (QInputMethodEvent *)e;

		if (!imevent->commitString().isEmpty())
		{
			CKEY_clear(true);

			GB.FreeString(&CKEY_info.text);
			//qDebug("IMEnd: %s", imevent->text().latin1());
			CKEY_info.text = GB.NewZeroString(QT_ToUtf8(imevent->commitString()));
			CKEY_info.state = Qt::KeyboardModifiers();
			CKEY_info.code = 0;
		}
	}

	GB.Call(&_application_keypress_func, 0, FALSE);
	cancel = GB.Stopped();

	CKEY_clear(false);

	return cancel;
}

static bool QT_Notify(CWIDGET *object, bool value)
{
	bool old = object->flag.notified;
	//qDebug("QT_Notify: %s %p %d", GB.GetClassName(object), object, value);
	object->flag.notified = value;
	return old;
}

bool MyApplication::eventFilter(QObject *o, QEvent *e)
{
	if (o->isWidgetType())
	{
		if ((e->spontaneous() && e->type() == QEvent::KeyPress) || e->type() == QEvent::InputMethod)
		{
			if (QT_EventFilter(e))
				return true;
		}
		else if (e->type() == QEvent::ToolTip)
		{
			if (_tooltip_disable)
				return true;
		}
		else
		{
			QWidget *widget = (QWidget *)o;
			CWIDGET *control;

			if (widget->isWindow())
			{
				if (e->type() == QEvent::WindowActivate)
				{
					control = CWidget::getReal(widget);
					//qDebug("WindowActivate: %p %s", widget, control ? control->name : "NULL");
					if (control)
						CWIDGET_handle_focus(control, true);
					else
						CWINDOW_activate(NULL);
				}
				else if (e->type() == QEvent::WindowDeactivate)
				{
					control = CWidget::getReal(widget);
					//qDebug("WindowDeactivate: %p %s", widget, control ? control->name : "NULL");
					if (control)
						CWIDGET_handle_focus(control, false);
				}
			}
		}
	}

	return QApplication::eventFilter(o, e);
}

/*bool MyApplication::notify(QObject *o, QEvent *e)
{
	if (o->isWidgetType())
	{
		CWIDGET *ob = CWidget::get(o);
		bool old, res;

		if (ob)
		{
			old = QT_Notify(ob, true);
			res = QApplication::notify(o, e);
			QT_Notify(ob, old);
			return res;
		}
	}

	return QApplication::notify(o, e);
}*/

void MyApplication::setEventFilter(bool set)
{
	if (set)
	{
		_event_filter++;
		if (_event_filter == 1)
			qApp->installEventFilter(qApp);
	}
	else
	{
		_event_filter--;
		if (_event_filter == 0)
			qApp->removeEventFilter(qApp);
	}
}

void MyApplication::setTooltipEnabled(bool b)
{
	b = !b;
	if (b == _tooltip_disable)
		return;

	_tooltip_disable = b;
	setEventFilter(b);
}

void MyApplication::commitDataRequested(QSessionManager &session)
{
	QStringList cmd;

	if (CAPPLICATION_Restart)
	{
		int i;
		char **str;

		str = (char **)GB.Array.Get(CAPPLICATION_Restart, 0);
		for (i = 0; i < GB.Array.Count(CAPPLICATION_Restart); i++)
		{
			if (str[i])
				cmd += str[i];
			else
				cmd += "";
		}
	}
	else
		cmd += arguments().at(0);

	cmd += "-session";
	cmd += sessionId();

	if (CWINDOW_Main)
	{
		cmd += "-session-desktop";
		cmd += QString::number(PLATFORM.Window.GetVirtualDesktop(CWINDOW_Main->widget.widget));
		/*cmd += "-session-data";
		cmd += QString::number(CWINDOW_Main->x) + ","
		       + QString::number(CWINDOW_Main->y) + ","
					 + QString::number(CWINDOW_Main->w) + ","
					 + QString::number(CWINDOW_Main->h) + ","
					 + QString::number(QApplication::desktop()->screenNumber(CWINDOW_Main->widget.widget));*/
	}

	session.setRestartCommand(cmd);
}


//---------------------------------------------------------------------------

MyTimer::MyTimer(GB_TIMER *t) : QObject(0)
{
	timer = t;
	id = startTimer(t->delay);
}

MyTimer::~MyTimer()
{
	killTimer(id);
}

void MyTimer::timerEvent(QTimerEvent *e)
{
	if (timer)
		GB.RaiseTimer(timer);
}

//---------------------------------------------------------------------------

static bool must_quit(void)
{
	#if DEBUG_WINDOW
	qDebug("must_quit: Window = %d Watch = %d in_event_loop = %d MAIN_in_message_box = %d _prevent_quit = %d", CWindow::count, CWatch::count, in_event_loop, MAIN_in_message_box, _prevent_quit);
	#endif
	return CWINDOW_must_quit() && CWatch::count == 0 && in_event_loop && MAIN_in_message_box == 0 && _prevent_quit == 0 && !GB.HasActiveTimer();
}

static void check_quit_now(intptr_t param)
{
	static bool exit_called = false;

	if (must_quit() && !exit_called)
	{
		if (QApplication::instance())
		{
			GB_FUNCTION func;

			if (GB.ExistClass("TrayIcons"))
			{
				if (!GB.GetFunction(&func, (void *)GB.FindClass("TrayIcons"), "DeleteAll", NULL, NULL))
					GB.Call(&func, 0, FALSE);
			}

			qApp->exit();
			exit_called = true;
		}
	}
	else
		_check_quit_posted = false;
}

void MAIN_check_quit(void)
{
	if (_check_quit_posted)
		return;

	GB.Post((GB_CALLBACK)check_quit_now, 0);
	_check_quit_posted = true;
}

void MAIN_update_scale(const QFont &font)
{
	MAIN_scale = GET_DESKTOP_SCALE(font.pointSize(), QGuiApplication::primaryScreen()->logicalDotsPerInch());
}

static void QT_InitEventLoop(void)
{
}

//extern void qt_x11_set_global_double_buffer(bool);


static bool try_to_load_translation(QString &locale)
{
	// QLocale::system().name()
	return _translator->load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
/*#ifdef QT5
	return (!_translator->load(QString("qt_") + locale, QString(getenv("QTDIR")) + "/translations")
		  && !_translator->load(QString("qt_") + locale, QString("/usr/lib/qt5/translations"))
		  && !_translator->load(QString("qt_") + locale, QString("/usr/share/qt5/translations")));
#else
	return (!_translator->load(QString("qt_") + locale, QString(getenv("QTDIR")) + "/translations")
		  && !_translator->load(QString("qt_") + locale, QString("/usr/lib/qt4/translations"))
		  && !_translator->load(QString("qt_") + locale, QString("/usr/share/qt4/translations")));
#endif*/
}

static void init_lang(char *lang, bool rtl)
{
	int pos;
	QString locale(lang);

	MAIN_right_to_left = rtl;
	
	pos = locale.lastIndexOf(".");
	if (pos >= 0) locale = locale.left(pos);

	if (_translator)
	{
		qApp->removeTranslator(_translator);
		delete _translator;
		_translator = NULL;
	}

	_translator = new QTranslator();

	if (!try_to_load_translation(locale))
		goto __INSTALL_TRANSLATOR;

	pos = locale.lastIndexOf("_");
	if (pos >= 0)
	{
		locale = locale.left(pos);
		if (!try_to_load_translation(locale))
			goto __INSTALL_TRANSLATOR;
	}

	delete _translator;
	_translator = NULL;

	//if (strcmp(lang, "C"))
	//	qDebug("gb.qt4: warning: unable to load Qt translation: %s", lang);

	goto __SET_DIRECTION;

__INSTALL_TRANSLATOR:
	qApp->installTranslator(_translator);

__SET_DIRECTION:
	qApp->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
}

static void hook_lang(char *lang, int rtl)
{
	if (!qApp)
		return;

	init_lang(lang, rtl);

	//locale = QTextCodec::locale();
}

#if 0
static int (*_old_handler)(Display *d, XErrorEvent *e) = NULL;

static int X11_error_handler(Display *d, XErrorEvent *e)
{
	qDebug("X11 ERROR");
	//BREAKPOINT();

	if (_old_handler)
		return (*_old_handler)(d, e);
	else
		return 0;
}
#endif

static void *_old_hook_main;

static void hook_main(int *argc, char ***argv)
{
	QString platform;
	const char *comp;
	char *env;
	
	env = getenv("GB_GUI_PLATFORM");
	if (env && *env)
	{
		if (!strcasecmp(env, "X11"))
			putenv((char *)"QT_QPA_PLATFORM=xcb");
		else if (!strcasecmp(env, "WAYLAND"))
			putenv((char *)"QT_QPA_PLATFORM=wayland");
		else
			fprintf(stderr, QT_NAME ": warning: unknown platform: %s\n", env);
	}

	new MyApplication(*argc, *argv);
	
	platform = qApp->platformName();

	if (platform == "wayland")
	{
		comp = "gb.qt5.wayland";
		MAIN_platform = "wayland";
		MAIN_platform_is_wayland = true;
	}
	else if (platform == "xcb")
	{
		comp = "gb.qt5.x11";
		MAIN_platform = "x11";
	}
	else
	{
		fprintf(stderr, QT_NAME ": error: unsupported platform: %s\n", TO_UTF8(qApp->platformName()));
		::abort();
	}
	
	GB.Component.Load(comp);
	GB.GetInterface(comp, QT_PLATFORM_INTERFACE_VERSION, &PLATFORM);
	
	QT_Init();
	init_lang(GB.System.Language(), GB.System.IsRightToLeft());

	MAIN_init = true;

	//_old_handler = XSetErrorHandler(X11_error_handler);

	CALL_HOOK_MAIN(_old_hook_main, argc, argv);
}


static void hook_quit()
{
	CWINDOW_close_all(true);
	CWINDOW_delete_all(true);
	CMOUSE_set_control(NULL);

	qApp->sendPostedEvents(); //processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::DeferredDeletion, 0);
	qApp->sendPostedEvents(0, QEvent::DeferredDelete);

	CALL_GUI("_Quit", NULL, NULL, 0, FALSE);
}


static void hook_loop()
{
	//qDebug("**** ENTERING EVENT LOOP");

	qApp->sendPostedEvents();
	//qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::DeferredDeletion, 0);

	in_event_loop = true;

	if (!must_quit())
		qApp->exec();
	else
		MAIN_check_quit();

	hook_quit();
}


static void hook_wait(int duration)
{
	static bool _warning = FALSE;
	
	if (MyDrawingArea::inAnyDrawEvent())
	{
		GB.Error("Wait is forbidden during a repaint event");
		return;
	}

	if (CKEY_is_valid() && duration != -1)
	{
		if (!_warning)
		{
			fprintf(stderr, QT_NAME ": warning: calling the event loop during a keyboard event handler is ignored\n");
			_warning = TRUE;
		}
		return;
	}
	
	MAIN_in_wait++;
	
	if (duration >= 0)
		qApp->processEvents(QEventLoop::AllEvents, duration);
	else if (duration == -1)
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 0);
	else if (duration == -2)
		qApp->processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
	
	MAIN_in_wait--;
}


static void hook_timer(GB_TIMER *timer, bool on)
{
	if (timer->id)
	{
		MyTimer *t = (MyTimer *)(timer->id);
		t->clearTimer();
		t->deleteLater();
		timer->id = 0;
	}

	if (on)
		timer->id = (intptr_t)(new MyTimer(timer));
	else
		MAIN_check_quit();
}


static void hook_watch(int fd, int type, void *callback, intptr_t param)
{
	CWatch::watch(fd, type, (GB_WATCH_CALLBACK)callback, param);
}


static void hook_post(void)
{
	static MyPostCheck check;

	//qDebug("hook_post ?");

	if (MyPostCheck::in_check)
		return;

	//qDebug("hook_post !");

	MyPostCheck::in_check = true;
	QTimer::singleShot(0, &check, SLOT(check()));
}


static bool hook_error(int code, char *error, char *where, bool in_event_loop)
{
	QString msg;
	int ret;

	qApp->restoreOverrideCursor();
	while (qApp->activePopupWidget())
		delete qApp->activePopupWidget();
	CWatch::stop();

	msg = "<b>This application has raised an unexpected<br>error and must abort.</b><br><br>";

	if (code > 0)
	{
		msg = msg + "[%1] %2.<br><br><tt>%3</tt>";
		msg = msg.arg(code).arg(TO_QSTRING(error)).arg(where);
	}
	else
	{
		msg = msg + "%1.<br><br><tt>%2</tt>";
		msg = msg.arg(TO_QSTRING(error)).arg(where);
	}

	PLATFORM.ReleaseGrab();
	MAIN_in_message_box++;
	ret = QMessageBox::critical(0, TO_QSTRING(GB.Application.Name()), msg, in_event_loop ? QMessageBox::Close | QMessageBox::Ignore : QMessageBox::Ok);
	MAIN_in_message_box--;
	PLATFORM.UnreleaseGrab();
	MAIN_check_quit();

	return ret == QMessageBox::Ignore;
}

static void QT_Init(void)
{
	static bool init = false;
	QFont f;
	char *env;
	bool fix_style;

	if (init)
		return;

	//qApp->setAttribute(Qt::AA_ImmediateWidgetCreation);

	PLATFORM.Init();

	_previousMessageHandler = qInstallMessageHandler(myMessageHandler);
	
	/*QX11Info::setAppDpiX(0, 92);
	QX11Info::setAppDpiY(0, 92);*/

	/*fcntl(ConnectionNumber(qt_xdisplay()), F_SETFD, FD_CLOEXEC);*/

	#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)

	char *appid = GB.ToZeroString((GB_STRING *)CALL_GUI("_InitApp", NULL, "s", 0, FALSE));

	//fprintf(stderr, "appid = %s\n", appid);
	qApp->setDesktopFileName(TO_QSTRING(appid));

	#endif
	
	fix_style = false;
	
	if (::strcmp(qApp->style()->metaObject()->className(), "Breeze::Style") == 0)
	{
		env = getenv("GB_QT_NO_BREEZE_FIX");
		if (!env || atoi(env) == 0)
		{
			CSTYLE_fix_breeze = TRUE;
			qApp->setStyle(new FixBreezeStyle);
		fix_style = true;
		}
	}
	else if (::strcmp(qApp->style()->metaObject()->className(), "Oxygen::Style") == 0)
	{
		env = getenv("GB_QT_NO_OXYGEN_FIX");
		if (!env || atoi(env) == 0)
		{
			CSTYLE_fix_oxygen = TRUE;
			qApp->setStyle(new FixBreezeStyle);
			fix_style = true;
		}
	}
	
	if (!fix_style)
		qApp->setStyle(new FixStyle);

	MAIN_update_scale(qApp->desktop()->font());

	qApp->installEventFilter(&CWidget::manager);

	MyApplication::setEventFilter(true);

	if (GB.GetFunction(&_application_keypress_func, (void *)GB.Application.StartupClass(), "Application_KeyPress", "", "") == 0)
	{
		_application_keypress = true;
		MyApplication::setEventFilter(true);
	}

	//qt_x11_set_global_double_buffer(false);

	qApp->setQuitOnLastWindowClosed(false);

	MyApplication::initClipboard();

	env = getenv("GB_QT_KEY_DEBUG");
	if (env && atoi(env) != 0)
		MAIN_key_debug = TRUE;
	
	COLOR_update_palette();
	APPLICATION_update_dark_theme();

	GB.Hook(GB_HOOK_WAIT, (void *)hook_wait);
	GB.Hook(GB_HOOK_TIMER, (void *)hook_timer);
	GB.Hook(GB_HOOK_WATCH, (void *)hook_watch);
	GB.Hook(GB_HOOK_POST, (void *)hook_post);
	GB.Hook(GB_HOOK_LOOP, (void *)hook_loop);

	init = true;
}


static void QT_InitWidget(QWidget *widget, void *object, int fill_bg)
{
	((CWIDGET *)object)->flag.fillBackground = fill_bg;
	CWIDGET_new(widget, object);
}

static void QT_SetWheelFlag(void *object)
{
	((CWIDGET *)object)->flag.wheel = true;
}

void *QT_GetObject(QWidget *widget)
{
	return CWidget::get((QObject *)widget);
}

static QWidget *QT_GetContainer(void *object)
{
	return QCONTAINER(object);
}

/*static bool QT_IsDestroyed(void *object)
{
	return CWIDGET_test_flag(object, WF_DELETED);
}*/

static QPixmap *QT_GetPixmap(CPICTURE *pict)
{
	return pict->pixmap;
}

const char *QT_ToUtf8(const QString &str)
{
	const char *res;

	_utf8_buffer[_utf8_count] = str.toUtf8();
	res = _utf8_buffer[_utf8_count].data();
	_utf8_length = _utf8_buffer[_utf8_count].length();
	_utf8_count++;
	if (_utf8_count >= UTF8_NBUF)
		_utf8_count = 0;

	return res;
}

int QT_GetLastUtf8Length()
{
	return _utf8_length;
}

char *QT_NewString(const QString &str)
{
	const char *res = QT_ToUtf8(str);
	return GB.NewString(res, _utf8_length);
}

void QT_ReturnNewString(const QString &str)
{
	const char *res = QT_ToUtf8(str);
	GB.ReturnNewString(res, _utf8_length);
}

static void *QT_CreatePicture(const QPixmap &p)
{
	return CPICTURE_create(&p);
}

void MyApplication::linkDestroyed(QObject *qobject)
{
	void *object = _link_map.value(qobject, 0);
	_link_map.remove(qobject);
	if (object)
		GB.Unref(POINTER(&object));
}

void QT_Link(QObject *qobject, void *object)
{
	_link_map.insert(qobject, object);
  QObject::connect(qobject, SIGNAL(destroyed(QObject *)), qApp, SLOT(linkDestroyed(QObject *)));
	GB.Ref(object);
}

void *QT_GetLink(QObject *qobject)
{
	return _link_map.value(qobject, 0);
}

void QT_PreventQuit(bool inc)
{
	if (inc)
		_prevent_quit++;
	else
	{
		_prevent_quit--;
		MAIN_check_quit();
	}
}

QMenu *QT_FindMenu(void *parent, const char *name)
{
	CMENU *menu = NULL;

	if (parent && GB.Is(parent, CLASS_Control))
	{
		CWINDOW *window = CWidget::getWindow((CWIDGET *)parent);
		menu = CWindow::findMenu(window, name);
	}

	return menu ? menu->menu : NULL;
}

static void declare_tray_icon()
{
	GB.Component.Declare(TrayIconDesc);
	GB.Component.Declare(TrayIconsDesc);
}

static int QT_GetDesktopScale(void)
{
	return MAIN_scale;
}

extern "C" {

GB_DESC *GB_CLASSES[] EXPORT =
{
	BorderDesc, CColorDesc,
	AlignDesc, ArrangeDesc, ScrollDesc, CKeyDesc, SelectDesc, DirectionDesc,
	CImageDesc, CPictureDesc, AnimationDesc,
	CFontDesc, CFontsDesc,
	CMouseDesc, CCursorDesc, CPointerDesc,
	CClipboardDesc, CDragDesc,
	StyleDesc, ScreenDesc, ScreensDesc, DesktopDesc,
	ApplicationDesc,
	CControlDesc, ContainerChildrenDesc, ContainerDesc,
	UserControlDesc, UserContainerDesc,
	CMenuChildrenDesc, CMenuDesc,
	CButtonDesc, CToggleButtonDesc, CToolButtonDesc,
	CCheckBoxDesc, CRadioButtonDesc,
	CTextBoxSelectionDesc, CTextBoxDesc,
	CTextAreaSelectionDesc, CTextAreaDesc,
	CPanelDesc, CHBoxDesc, CVBoxDesc, CHPanelDesc, CVPanelDesc,
	CTabStripContainerChildrenDesc, CTabStripContainerDesc, CTabStripDesc,
	CDrawingAreaDesc,
	SliderDesc, ScrollBarDesc,
	CWindowMenusDesc, CWindowControlsDesc, CWindowDesc, CWindowsDesc, CFormDesc,
	CDialogDesc,
	CWatcherDesc,
	PrinterDesc,
	SvgImageDesc,
	NULL
};

void *GB_QT5_1[] EXPORT =
{
	(void *)1,

	(void *)QT_InitEventLoop,
	(void *)QT_Init,
	(void *)QT_InitWidget,
	(void *)QT_SetWheelFlag,
	(void *)QT_GetObject,
	(void *)QT_GetContainer,
	(void *)CWIDGET_border_simple,
	(void *)CWIDGET_border_full,
	(void *)CWIDGET_scrollbar,
	(void *)Control_Font,
	(void *)CFONT_create,
	(void *)CFONT_set,
	(void *)QT_CreatePicture,
	//(void *)QT_MimeSourceFactory,
	(void *)QT_GetPixmap,
	(void *)QT_ToUtf8,
	(void *)QT_GetLastUtf8Length,
	(void *)QT_NewString,
	(void *)QT_ReturnNewString,
	(void *)QT_EventFilter,
	(void *)QT_Notify,
	(void *)CCONST_alignment,
	(void *)QT_Link,
	(void *)QT_GetLink,
	(void *)PAINT_get_current,
	(void *)CWIDGET_get_background,
	(void *)Control_Mouse,
	(void *)CWIDGET_after_set_color,
	(void *)QT_GetDesktopScale,
	NULL
};

const char *GB_INCLUDE EXPORT = "gb.draw,gb.gui.base";

int EXPORT GB_INIT(void)
{
	char *env;

	// Do not disable GLib support

	/*env = getenv("KDE_FULL_SESSION");
	if (env && !strcasecmp(env, "true"))
		putenv((char *)"QT_NO_GLIB=1");*/

	env = getenv("GB_GUI_BUSY");
	if (env && atoi(env))
		MAIN_debug_busy = true;
	
	//putenv((char *)"QT_SLOW_TOPLEVEL_RESIZE=1");

	_old_hook_main = GB.Hook(GB_HOOK_MAIN, (void *)hook_main);
	GB.Hook(GB_HOOK_QUIT, (void *)hook_quit);
	GB.Hook(GB_HOOK_ERROR, (void *)hook_error);
	GB.Hook(GB_HOOK_LANG, (void *)hook_lang);

	GB.Component.Load("gb.draw");
	GB.Component.Load("gb.image");
	GB.Component.Load("gb.gui.base");
	
	GB.GetInterface("gb.geom", GEOM_INTERFACE_VERSION, &GEOM);

	GB.GetInterface("gb.image", IMAGE_INTERFACE_VERSION, &IMAGE);
	IMAGE.SetDefaultFormat(GB_IMAGE_BGRP);

	DRAW_init();

	CLASS_Control = GB.FindClass("Control");
	CLASS_Container = GB.FindClass("Container");
	CLASS_ContainerChildren = GB.FindClass("ContainerChildren");
	CLASS_UserControl = GB.FindClass("UserControl");
	CLASS_UserContainer = GB.FindClass("UserContainer");
	CLASS_TabStrip = GB.FindClass("TabStrip");
	CLASS_Window = GB.FindClass("Window");
	CLASS_Menu = GB.FindClass("Menu");
	CLASS_Picture = GB.FindClass("Picture");
	CLASS_Drawing = GB.FindClass("Drawing");
	CLASS_DrawingArea = GB.FindClass("DrawingArea");
	CLASS_Printer = GB.FindClass("Printer");
	CLASS_Image = GB.FindClass("Image");
	CLASS_SvgImage = GB.FindClass("SvgImage");
	CLASS_TextArea = GB.FindClass("TextArea");

	QT_InitEventLoop();

	#ifdef OS_CYGWIN
		return 1;
	#else
		return 0;
	#endif
}

void EXPORT GB_EXIT()
{
	if (qApp)
	{
		PLATFORM.Exit();
		delete qApp;
	}
}

#ifndef NO_X_WINDOW
int EXPORT GB_INFO(const char *key, void **value)
{
	if (!strcasecmp(key, "DECLARE_TRAYICON"))
	{
		*value = (void *)declare_tray_icon;
		return TRUE;
	}
	else if (!strcasecmp(key, "GET_HANDLE"))
	{
		*value = (void *)CWIDGET_get_handle;
		return TRUE;
	}
	else
		return FALSE;
}
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
void EXPORT GB_FORK(void)
{
	delete QThreadPool::globalInstance();
}
#endif

/*#ifndef NO_X_WINDOW
extern Time	qt_x_time;
#endif*/

static void activate_main_window(intptr_t value)
{
	CWINDOW *active;

	active = CWINDOW_Active;
	if (!active) active = CWINDOW_LastActive;

	if (!active)
		return;

	MyMainWindow *win = (MyMainWindow *)active->widget.widget;
	if (win && !win->isWindow())
		win = (MyMainWindow *)win->window();
	if (win)
	{
		/*#ifndef NO_X_WINDOW
		qt_x_time = CurrentTime;
		#endif*/
		win->raise();
		win->activateWindow();
	}
}

void EXPORT GB_SIGNAL(int signal, void *param)
{
	if (!qApp)
		return;

	switch(signal)
	{
		case GB_SIGNAL_DEBUG_BREAK:
			PLATFORM.ReleaseGrab();
			break;

		case GB_SIGNAL_DEBUG_FORWARD:
			//while (qApp->activePopupWidget())
			//	delete qApp->activePopupWidget();
			break;

		case GB_SIGNAL_DEBUG_CONTINUE:
			GB.Post((GB_CALLBACK)activate_main_window, 0);
			PLATFORM.UnreleaseGrab();
			break;
	}
}

} // extern "C"

/* class MyPostCheck */

bool MyPostCheck::in_check = false;

void MyPostCheck::check(void)
{
	//qDebug("MyPostCheck::check");
	in_check = false;
	GB.CheckPost();
}

