/***************************************************************************

  c_webview.cpp

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

#define __C_WEBVIEW_CPP

/*#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QWebPage>
#include <QWebFrame>
#include <QWebHistory>*/
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QJsonDocument>
#include <QContextMenuEvent>
#include <QPointer>

/*#include "ccookiejar.h"
#include "cwebsettings.h"
#include "cwebelement.h"
#include "cwebframe.h"
#include "cwebhittest.h"*/
#include "c_webview.h"

#define HISTORY (WIDGET->history())

static bool _cb_exited = FALSE;
static volatile bool _cb_running = FALSE;
static bool _cb_error = FALSE;
static char *_cb_result = NULL;

/*typedef
	struct {
		const char *name;
		QWebPage::WebAction action;
	}
	WEBVIEW_ACTION;*/
	
//---------------------------------------------------------------------------

DECLARE_EVENT(EVENT_TITLE);
DECLARE_EVENT(EVENT_URL);
DECLARE_EVENT(EVENT_ICON);
DECLARE_EVENT(EVENT_START);
DECLARE_EVENT(EVENT_PROGRESS);
DECLARE_EVENT(EVENT_FINISH);
DECLARE_EVENT(EVENT_ERROR);
DECLARE_EVENT(EVENT_LINK);
DECLARE_EVENT(EVENT_NEW_VIEW);

static int EVENT_MENU = -1;
/*DECLARE_EVENT(EVENT_CLICK);
DECLARE_EVENT(EVENT_LINK);
DECLARE_EVENT(EVENT_NEW_FRAME);
DECLARE_EVENT(EVENT_AUTH);
DECLARE_EVENT(EVENT_DOWNLOAD);*/



/*
static QNetworkAccessManager *_network_access_manager = 0;
static CWEBVIEW *_network_access_manager_view = 0;
static QT_COLOR_FUNC _old_after_set_color;
static bool _ignore_png_warnings = false;*/

/*
static WEBVIEW_ACTION _actions[] = 
{
	{ "OpenLink", QWebPage::OpenLink },
	{ "OpenLinkInNewWindow", QWebPage::OpenLinkInNewWindow },
	{ "OpenFrameInNewWindow", QWebPage::OpenFrameInNewWindow },
	{ "DownloadLinkToDisk", QWebPage::DownloadLinkToDisk },
	{ "CopyLinkToClipboard", QWebPage::CopyLinkToClipboard },
	{ "OpenImageInNewWindow", QWebPage::OpenImageInNewWindow },
	{ "DownloadImageToDisk", QWebPage::DownloadImageToDisk },
	{ "CopyImageToClipboard", QWebPage::CopyImageToClipboard },
	{ "Back", QWebPage::Back },
	{ "Forward", QWebPage::Forward },
	{ "Stop", QWebPage::Stop },
	{ "StopScheduledPageRefresh", QWebPage::StopScheduledPageRefresh },
	{ "Reload", QWebPage::Reload },
	{ "ReloadAndBypassCache", QWebPage::ReloadAndBypassCache },
	{ "Cut", QWebPage::Cut },
	{ "Copy", QWebPage::Copy },
	{ "Paste", QWebPage::Paste },
	{ "Undo", QWebPage::Undo },
	{ "Redo", QWebPage::Redo },
	{ "MoveToNextChar", QWebPage::MoveToNextChar },
	{ "MoveToPreviousChar", QWebPage::MoveToPreviousChar },
	{ "MoveToNextWord", QWebPage::MoveToNextWord },
	{ "MoveToPreviousWord", QWebPage::MoveToPreviousWord },
	{ "MoveToNextLine", QWebPage::MoveToNextLine },
	{ "MoveToPreviousLine", QWebPage::MoveToPreviousLine },
	{ "MoveToStartOfLine", QWebPage::MoveToStartOfLine },
	{ "MoveToEndOfLine", QWebPage::MoveToEndOfLine },
	{ "MoveToStartOfBlock", QWebPage::MoveToStartOfBlock },
	{ "MoveToEndOfBlock", QWebPage::MoveToEndOfBlock },
	{ "MoveToStartOfDocument", QWebPage::MoveToStartOfDocument },
	{ "MoveToEndOfDocument", QWebPage::MoveToEndOfDocument },
	{ "SelectNextChar", QWebPage::SelectNextChar },
	{ "SelectPreviousChar", QWebPage::SelectPreviousChar },
	{ "SelectNextWord", QWebPage::SelectNextWord },
	{ "SelectPreviousWord", QWebPage::SelectPreviousWord },
	{ "SelectNextLine", QWebPage::SelectNextLine },
	{ "SelectPreviousLine", QWebPage::SelectPreviousLine },
	{ "SelectStartOfLine", QWebPage::SelectStartOfLine },
	{ "SelectEndOfLine", QWebPage::SelectEndOfLine },
	{ "SelectStartOfBlock", QWebPage::SelectStartOfBlock },
	{ "SelectEndOfBlock", QWebPage::SelectEndOfBlock },
	{ "SelectStartOfDocument", QWebPage::SelectStartOfDocument },
	{ "SelectEndOfDocument", QWebPage::SelectEndOfDocument },
	{ "DeleteStartOfWord", QWebPage::DeleteStartOfWord },
	{ "DeleteEndOfWord", QWebPage::DeleteEndOfWord },
	{ "SetTextDirectionDefault", QWebPage::SetTextDirectionDefault },
	{ "SetTextDirectionLeftToRight", QWebPage::SetTextDirectionLeftToRight },
	{ "SetTextDirectionRightToLeft", QWebPage::SetTextDirectionRightToLeft },
	{ "ToggleBold", QWebPage::ToggleBold },
	{ "ToggleItalic", QWebPage::ToggleItalic },
	{ "ToggleUnderline", QWebPage::ToggleUnderline },
	{ "InspectElement", QWebPage::InspectElement },
	{ "InsertParagraphSeparator", QWebPage::InsertParagraphSeparator },
	{ "InsertLineSeparator", QWebPage::InsertLineSeparator },
	{ "SelectAll", QWebPage::SelectAll },
	{ "PasteAndMatchStyle", QWebPage::PasteAndMatchStyle },
	{ "RemoveFormat", QWebPage::RemoveFormat },
	{ "ToggleStrikethrough", QWebPage::ToggleStrikethrough },
	{ "ToggleSubscript", QWebPage::ToggleSubscript },
	{ "ToggleSuperscript", QWebPage::ToggleSuperscript },
	{ "InsertUnorderedList", QWebPage::InsertUnorderedList },
	{ "InsertOrderedList", QWebPage::InsertOrderedList },
	{ "Indent", QWebPage::Indent },
	{ "Unindent", QWebPage::Outdent },
	{ "AlignCenter", QWebPage::AlignCenter },
	{ "AlignJustified", QWebPage::AlignJustified },
	{ "AlignLeft", QWebPage::AlignLeft },
	{ "AlignRight", QWebPage::AlignRight },
	{ NULL, QWebPage::NoWebAction }
};
*/

#if 0
QNetworkAccessManager *WEBVIEW_get_network_manager()
{
	if (!_network_access_manager)
	{
		_network_access_manager = new QNetworkAccessManager();
		_network_access_manager->setCookieJar(new MyCookieJar);
	}
	
	return _network_access_manager;
}

/*
static QWebPage::WebAction get_action(const char *name)
{
	WEBVIEW_ACTION *p;
	
	for (p = _actions; p->name; p++)
	{
		if (!strcasecmp(p->name, name))
			return p->action;
	}
	
	return QWebPage::NoWebAction;
}
*/

static void after_set_color(void *_object)
{
	if (!GB.Is(THIS, CLASS_WebView))
	{
		if (_old_after_set_color)
			(*_old_after_set_color)(THIS);
		return;
	}

	if (QT.GetBackgroundColor(THIS) == GB_COLOR_DEFAULT)
	{
		QPalette palette = WIDGET->palette();
		WIDGET->page()->setPalette(palette);
		WIDGET->setAttribute(Qt::WA_OpaquePaintEvent, true);
	}
	else
	{
		qDebug("after_set_color");
		QPalette palette = WIDGET->palette();
		palette.setBrush(QPalette::Base, Qt::transparent);
		WIDGET->page()->setPalette(palette);
		WIDGET->setAttribute(Qt::WA_OpaquePaintEvent, false);
	}
}

static void stop_view(void *_object)
{
	//fprintf(stderr, "stop_view\n");
	THIS->stopping = TRUE;
	WIDGET->stop();
	THIS->stopping = FALSE;
}
#endif


static void set_link(void *_object, const QString &link)
{
	GB.FreeString(&THIS->link);
	THIS->link = QT.NewString(link);
}

static bool start_callback()
{
	if (_cb_running)
	{
		GB.Error("Pending asynchronous method");
		return true;
	}

	_cb_running = true;
	return false;
}

static void run_callback(const char *error)
{
	while(_cb_running)
		GB.Wait(-1);

	if (_cb_error)
	{
		GB.Error(error);
		GB.FreeString(&_cb_result);
	}
	else
	{
		GB.ReturnString(GB.FreeStringLater(_cb_result));
		_cb_result = NULL;
	}

	_cb_error = FALSE;
}

static void cb_javascript_finished(const QVariant &result)
{
	if (_cb_exited)
		return;
	
	QVariantList value;
	value.append(result);
	
	QByteArray array = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
	
	if (array.size() > 2)
		_cb_result = GB.NewString(array.constData() + 1, array.size() - 2);
	
	_cb_running = FALSE;
}

static void cb_html_finished(const QString &result)
{
	if (_cb_exited)
		return;

	_cb_result = QT.NewString(result);

	_cb_running = FALSE;
}

static void update_language(void *_object)
{
	QString lang;

	if (THIS->language && *THIS->language)
		lang = TO_QSTRING(THIS->language);
	else
		lang = TO_QSTRING(GB.System.Language()).replace("_", "-");

	WIDGET->page()->profile()->setHttpAcceptLanguage(lang);
}

//-------------------------------------------------------------------------

BEGIN_METHOD_VOID(WebView_exit)

	_cb_exited = TRUE;
	GB.FreeString(&_cb_result);

END_METHOD

BEGIN_METHOD(WebView_new, GB_OBJECT parent)

	static bool force_init = false;

	/*int fd_save = -1;
	
	if (!_ignore_png_warnings)
	{
		int fd = ::open("/dev/null", O_RDWR);
		fd_save = ::dup(2);
		::dup2(fd, 2);
		::close(fd);
	}*/

  MyWebEngineView *wid = new MyWebEngineView(QT.GetContainer(VARG(parent)));

	//wid->setAttribute(Qt::WA_NativeWindow, true);
	
	/*if (!_ignore_png_warnings)
	{
		::dup2(fd_save, 2);
		::close(fd_save);
		_ignore_png_warnings = true;
	}*/

  QT.InitWidget(wid, _object, false);
	QT.SetWheelFlag(_object);
	
	//WEBVIEW_get_network_manager();
	
	//wid->page()->setNetworkAccessManager(_network_access_manager);
	//wid->page()->setForwardUnsupportedContent(true);

  QObject::connect(wid, SIGNAL(iconChanged(const QIcon &)), &WebViewSignalManager::manager, SLOT(iconChanged()));
  QObject::connect(wid, SIGNAL(titleChanged(const QString &)), &WebViewSignalManager::manager, SLOT(titleChanged()));
  QObject::connect(wid, SIGNAL(urlChanged(const QUrl &)), &WebViewSignalManager::manager, SLOT(urlChanged()));
  QObject::connect(wid, SIGNAL(loadStarted()), &WebViewSignalManager::manager, SLOT(loadStarted()));
  QObject::connect(wid, SIGNAL(loadProgress(int)), &WebViewSignalManager::manager, SLOT(loadProgress(int)));
  QObject::connect(wid, SIGNAL(loadFinished(bool)), &WebViewSignalManager::manager, SLOT(loadFinished(bool)));
	//QObject::connect(wid, SIGNAL(destroyed()), &WebViewSignalManager::manager, SLOT(destroy()));

	wid->clearPage(false);
	
	if (!force_init)
	{
		wid->setHtml("");
		force_init = true;
	}
	
	update_language(THIS);

	//QObject::connect(wid, SIGNAL(linkClicked(const QUrl &)), &CWebView::manager, SLOT(linkClicked(const QUrl &)));
#if 0
  QObject::connect(wid, SIGNAL(selectionChanged()), &CWebView::manager, SLOT(selectionChanged()));
  QObject::connect(wid, SIGNAL(statusBarMessage(const QString &)), &CWebView::manager, SLOT(statusBarMessage(const QString &)));
  
	QObject::connect(wid->page(), SIGNAL(frameCreated(QWebFrame *)), &CWebView::manager, SLOT(frameCreated(QWebFrame *)));
	QObject::connect(wid->page(), SIGNAL(downloadRequested(QNetworkRequest)), &CWebView::manager, SLOT(downloadRequested(QNetworkRequest)));
  QObject::connect(wid->page(), SIGNAL(unsupportedContent(QNetworkReply*)), &CWebView::manager, SLOT(handleUnsupportedContent(QNetworkReply*)));
	
	QObject::connect(wid->page()->mainFrame(), SIGNAL(urlChanged(const QUrl &)), &CWebView::manager, SLOT(urlChanged(const QUrl &)));

	QObject::connect(wid->page()->networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), &CWebView::manager,
										SLOT(authenticationRequired(QNetworkReply *, QAuthenticator *)));
#endif

END_METHOD

BEGIN_METHOD_VOID(WebView_free)

	GB.FreeString(&THIS->link);
	GB.FreeString(&THIS->language);

	GB.Unref(POINTER(&THIS->icon));
	GB.Unref(POINTER(&THIS->new_view));

END_METHOD


/*BEGIN_METHOD_VOID(WebView_exit)

	delete _network_access_manager;

END_METHOD*/

BEGIN_PROPERTY(WebView_Url)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(WIDGET->url().toString());
	else
	{
		QString url = QSTRING_PROP();
		set_link(THIS, url);
		THIS->cancel = false;
		WIDGET->setUrl(url);
	}

END_PROPERTY

BEGIN_PROPERTY(WebView_Title)

	RETURN_NEW_STRING(WIDGET->title());

END_PROPERTY

BEGIN_METHOD(WebView_SetHtml, GB_STRING html; GB_STRING root)

	if (!MISSING(root))
	{
		QUrl url(QSTRING_ARG(root));
		WIDGET->setHtml(QSTRING_ARG(html), url);
	}
	else
		WIDGET->setHtml(QSTRING_ARG(html));

END_METHOD

BEGIN_METHOD_VOID(WebView_Back)

	WIDGET->back();

END_METHOD

BEGIN_METHOD_VOID(WebView_Forward)

	WIDGET->forward();

END_METHOD

BEGIN_METHOD(WebView_Reload, GB_BOOLEAN bypass)

	bool bypass = VARGOPT(bypass, false);
	//stop_view(THIS);
	if (bypass)
		WIDGET->page()->triggerAction(QWebEnginePage::ReloadAndBypassCache);
	else
		WIDGET->reload();

END_METHOD

BEGIN_METHOD_VOID(WebView_Stop)

	WIDGET->stop();

END_METHOD

BEGIN_PROPERTY(WebView_Zoom)

	if (READ_PROPERTY)
		GB.ReturnFloat(WIDGET->zoomFactor());
	else
		WIDGET->setZoomFactor(VPROP(GB_FLOAT));

END_PROPERTY

BEGIN_PROPERTY(WebView_Icon)

	if (!THIS->icon)
	{
		QIcon icon = WIDGET->icon();
		
		if (!icon.isNull())
		{
			int size = QT.GetDesktopScale() * 2;
			THIS->icon = QT.CreatePicture(icon.pixmap(size, size));
			GB.Ref(THIS->icon);
		}
	}

	GB.ReturnObject(THIS->icon);

END_PROPERTY

BEGIN_PROPERTY(WebView_Progress)

	GB.ReturnFloat(THIS->progress / 100.0);

END_PROPERTY

BEGIN_PROPERTY(WebView_NewView)

	if (READ_PROPERTY)
		GB.ReturnObject(THIS->new_view);
	else
		GB.StoreObject(PROP(GB_OBJECT), &THIS->new_view);

END_PROPERTY

BEGIN_PROPERTY(WebView_Link)

	GB.ReturnString(THIS->link);

END_PROPERTY

BEGIN_METHOD_VOID(WebView_Clear)

	WIDGET->clearPage(true);

END_METHOD

BEGIN_METHOD(WebView_ExecJavascript, GB_STRING script)

	if (LENGTH(script) == 0)
		return;
	
	if (start_callback())
		return;

	WIDGET->page()->runJavaScript(QSTRING_ARG(script), cb_javascript_finished);
	
	run_callback("Javascript error");

END_METHOD

BEGIN_METHOD_VOID(WebView_GetHtml)

	if (start_callback())
		return;

	WIDGET->page()->toHtml(cb_html_finished);

	run_callback("Unable to retrieve HTML contents");

END_METHOD

BEGIN_PROPERTY(WebView_Language)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->language);
	else
	{
		GB.StoreString(PROP(GB_STRING), &THIS->language);
		update_language(THIS);
	}

END_PROPERTY

#if 0
BEGIN_PROPERTY(WebView_HTML)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(WIDGET->page()->mainFrame()->toHtml());
	else
		WIDGET->setHtml(QSTRING_PROP());

END_PROPERTY

BEGIN_PROPERTY(WebView_Text)

	RETURN_NEW_STRING(WIDGET->page()->mainFrame()->toPlainText());

END_PROPERTY

BEGIN_PROPERTY(WebView_SelectedText)

	RETURN_NEW_STRING(WIDGET->selectedText());

END_PROPERTY

#if QT_VERSION >= QT_VERSION_CHECK(4, 5, 0)
BEGIN_PROPERTY(WebView_Zoom)

	if (READ_PROPERTY)
		GB.ReturnFloat(WIDGET->zoomFactor());
	else
		WIDGET->setZoomFactor(VPROP(GB_FLOAT));

END_PROPERTY
#endif

BEGIN_PROPERTY(WebView_TextZoom)

	if (READ_PROPERTY)
		GB.ReturnFloat(WIDGET->textSizeMultiplier());
	else
		WIDGET->setTextSizeMultiplier(VPROP(GB_FLOAT));

END_PROPERTY

BEGIN_PROPERTY(WebView_Status)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->status);
	else
		GB.StoreString(PROP(GB_STRING), &THIS->status);

END_PROPERTY

BEGIN_PROPERTY(WebView_Frame)

	GB.ReturnObject(CWEBFRAME_get(WIDGET->page()->mainFrame()));

END_PROPERTY

BEGIN_PROPERTY(WebView_Current)

	GB.ReturnObject(CWEBFRAME_get(WIDGET->page()->currentFrame()));

END_PROPERTY


BEGIN_PROPERTY(WebViewAuth_Url)

	if (THIS->reply)
		RETURN_NEW_STRING(THIS->reply->url().toString());
	else
		GB.ReturnVoidString();

END_PROPERTY

BEGIN_PROPERTY(WebViewAuth_Realm)

	if (THIS->authenticator)
		RETURN_NEW_STRING(THIS->authenticator->realm());
	else
		GB.ReturnVoidString();

END_PROPERTY

BEGIN_PROPERTY(WebViewAuth_User)

	if (READ_PROPERTY)
	{
		if (THIS->authenticator)
			RETURN_NEW_STRING(THIS->authenticator->user());
		else
			GB.ReturnVoidString();
	}
	else
	{
		if (THIS->authenticator)
			THIS->authenticator->setUser(QSTRING_PROP());
		else
			GB.Error("No authentication required");
	}

END_PROPERTY

BEGIN_PROPERTY(WebViewAuth_Password)

	if (READ_PROPERTY)
	{
		if (THIS->authenticator)
			RETURN_NEW_STRING(THIS->authenticator->password());
		else
			GB.ReturnVoidString();
	}
	else
	{
		if (THIS->authenticator)
			THIS->authenticator->setPassword(QSTRING_PROP());
		else
			GB.Error("No authentication required");
	}

END_PROPERTY

BEGIN_PROPERTY(WebView_Cookies)

	MyCookieJar *cookieJar = static_cast<MyCookieJar *>(_network_access_manager->cookieJar());
	QList<QNetworkCookie> list;
	GB_ARRAY cookies;
	int i;

	if (READ_PROPERTY)
	{
		list = cookieJar->allCookies();

		GB.Array.New(POINTER(&cookies), GB.FindClass("Cookie"), list.count());

		for (i = 0; i < list.count(); i++)
		{
			CCOOKIE *cookie = WEB_create_cookie(list.at(i));
			*((CCOOKIE **)(GB.Array.Get(cookies, i))) = cookie;
			GB.Ref(cookie);
		}

		GB.ReturnObject(cookies);
	}
	else
	{
		// TODO what todo?
		cookies = VPROP(GB_OBJECT);
		if (GB.CheckObject(cookies))
			return;

		for (i = 0; i < GB.Array.Count(cookies); i++)
		{
			CCOOKIE *cookie = *((CCOOKIE **)GB.Array.Get(cookies, i));
			if (GB.CheckObject(cookie))
				continue;
			list.append(*(cookie->cookie));
		}

		cookieJar->setAllCookies(list);
	}

END_PROPERTY

BEGIN_METHOD(WebView_HitTest, GB_INTEGER X; GB_INTEGER Y)

	GB.ReturnObject(WEB_create_hit_test(WIDGET->page()->mainFrame()->hitTestContent(QPoint(VARG(X), VARG(Y)))));

END_METHOD

BEGIN_METHOD(WebView_FindText, GB_STRING text; GB_BOOLEAN backward; GB_BOOLEAN case_sensitive; GB_BOOLEAN wrap)

	QString text;
	QWebPage::FindFlags flags = 0;
	
	if (!MISSING(text)) 
		text = QSTRING_ARG(text);
	
	if (VARGOPT(backward, false)) flags |= QWebPage::FindBackward;
	if (VARGOPT(case_sensitive, false)) flags |= QWebPage::FindCaseSensitively;
	if (VARGOPT(wrap, false)) flags |= QWebPage::FindWrapsAroundDocument;
	//if (VARGOPT(highlight, false)) flags |= QWebPage::HighlightAllOccurrences;
	
	GB.ReturnBoolean(!WIDGET->findText(text, flags));

END_METHOD

BEGIN_PROPERTY(WebView_Editable)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->page()->isContentEditable());
	else
		WIDGET->page()->setContentEditable(VPROP(GB_BOOLEAN));

END_PROPERTY

#if 0
BEGIN_METHOD(WebView_Exec, GB_STRING action; GB_VARIANT arg)

	QWebPage::WebAction action = get_action(GB.ToZeroString(ARG(action)));
	
	if (action == QWebPage::NoWebAction)
	{
		GB.Error("Unknown action");
		return;
	}
	
	if (MISSING(arg))
	{
		WIDGET->page()->triggerAction(action);
	}
	else
	{
		if (GB.Conv((GB_VALUE *)ARG(arg), GB_T_BOOLEAN))
			return;
		
		WIDGET->page()->triggerAction(action, ((GB_BOOLEAN *)ARG(arg))->value);
	}

END_METHOD
#endif

/*
BEGIN_METHOD(WebView_CanExec, GB_STRING action)

	QWebPage::Action action = get_action(GB.ToZeroString(ARG(action)));
	
	if (action == QWebPage::NoWebAction)
	{
		GB.Error("Unknown action");
		return;
	}
	
END_METHOD
*/

BEGIN_METHOD(WebView_Eval, GB_STRING javascript)

	CWEBFRAME_eval(WIDGET->page()->currentFrame(), QSTRING_ARG(javascript));

END_METHOD

BEGIN_PROPERTY(WebView_UserAgent)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->userAgent);
	else
		GB.StoreString(PROP(GB_STRING), &THIS->userAgent);

END_PROPERTY

BEGIN_PROPERTY(WebView_Document)

	GB.ReturnObject(CWEBELEMENT_create(WIDGET->page()->mainFrame()->documentElement()));

END_PROPERTY
#endif

//-------------------------------------------------------------------------

static QWebEngineHistoryItem get_item(QWebEngineHistory *history, int index)
{
	return history->itemAt(history->currentItemIndex() + index);
}

BEGIN_PROPERTY(WebView_History_Item_Title)

	QWebEngineHistoryItem item = get_item(HISTORY, THIS->history);
	
	if (item.isValid())
		RETURN_NEW_STRING(item.title());
	else
		GB.ReturnNull();

END_PROPERTY

BEGIN_PROPERTY(WebView_History_Item_Url)

	QWebEngineHistoryItem item = get_item(HISTORY, THIS->history);
	
	if (item.isValid())
		RETURN_NEW_STRING(item.url().toString());
	else
		GB.ReturnNull();

END_PROPERTY

BEGIN_METHOD_VOID(WebView_History_Item_GoTo)

	QWebEngineHistoryItem item = get_item(HISTORY, THIS->history);
	
	if (item.isValid())
		HISTORY->goToItem(item);

END_METHOD

BEGIN_METHOD_VOID(WebView_History_Clear)

	HISTORY->clear();

END_METHOD

BEGIN_METHOD(WebView_History_get, GB_INTEGER index)

	int index = VARG(index);
	
	index += HISTORY->currentItemIndex();
	if (index < 0 || index >= HISTORY->count())
		GB.ReturnNull();
	else
	{
		THIS->history = index;
		RETURN_SELF();
	}

END_METHOD

BEGIN_PROPERTY(WebView_History_CanGoBack)

	GB.ReturnBoolean(HISTORY->canGoBack());

END_PROPERTY

BEGIN_PROPERTY(WebView_History_CanGoForward)

	GB.ReturnBoolean(HISTORY->canGoForward());

END_PROPERTY

//-------------------------------------------------------------------------

BEGIN_METHOD_VOID(WebView_Cookies_Clear)

	WIDGET->page()->profile()->cookieStore()->deleteAllCookies();

END_METHOD

//-------------------------------------------------------------------------

#if 0
GB_DESC WebViewAuthDesc[] =
{
  GB_DECLARE_VIRTUAL(".WebView.Auth"),
	
	GB_PROPERTY_READ("Url", "s", WebViewAuth_Url),
	GB_PROPERTY_READ("Realm", "s", WebViewAuth_Realm),
	GB_PROPERTY("User", "s", WebViewAuth_User),
	GB_PROPERTY("Password", "s", WebViewAuth_Password),
	
	GB_END_DECLARE
};

GB_DESC WebViewDesc[] =
{
  GB_DECLARE("WebView", sizeof(CWEBVIEW)), GB_INHERITS("Control"),
	
  GB_METHOD("_new", NULL, WebView_new, "(Parent)Container;"),
  GB_METHOD("_free", NULL, WebView_free, NULL),
  GB_METHOD("_init", NULL, WebView_init, NULL),
  GB_METHOD("_exit", NULL, WebView_exit, NULL),

	GB_PROPERTY("Status", "s", WebView_Status),

	GB_PROPERTY_READ("SelectedText", "s", WebView_SelectedText),
	#if QT_VERSION >= QT_VERSION_CHECK(4, 5, 0)
	GB_PROPERTY("Zoom", "f", WebView_Zoom),
	#else
	GB_PROPERTY("Zoom", "f", WebView_TextZoom),
	#endif
	GB_PROPERTY("TextZoom", "f", WebView_TextZoom),
	GB_PROPERTY_READ("Title", "s", WebView_Title),
	
	GB_PROPERTY_READ("Frame", "WebFrame", WebView_Frame),
	GB_PROPERTY_READ("Current", "WebFrame", WebView_Current),
	GB_PROPERTY_READ("Document", "WebElement", WebView_Document),
	
	GB_PROPERTY_SELF("Auth", ".WebView.Auth"),
	GB_PROPERTY_SELF("History", ".WebView.History"),

	GB_PROPERTY("NewView", "WebView", WebView_NewView),

	GB_PROPERTY("Cookies", "Cookie[]", WebView_Cookies),
	
	GB_METHOD("HitTest", "WebHitTest", WebView_HitTest, "(X)i(Y)i"),
	GB_METHOD("FindText", "b", WebView_FindText, "[(Text)s(Backward)b(CaseSensitive)b(Wrap)b]"),

	GB_PROPERTY("Editable", "b", WebView_Editable),
	//GB_METHOD("Exec", NULL, WebView_Exec, "(Action)s[(Argument)v]"),
	//GB_METHOD("CanExec", "b", WebView_CanExec, "(Action)s"),
	GB_METHOD("Eval", "v", WebView_Eval, "(JavaScript)s"),

	GB_PROPERTY("UserAgent", "s", WebView_UserAgent),

	GB_CONSTANT("_Properties", "s", "*,Url,Editable"),
	GB_CONSTANT("_Group", "s", "View"),
	
	GB_EVENT("Click", NULL, "(Frame)WebFrame", &EVENT_CLICK),
	GB_EVENT("Link", NULL, "(Url)s", &EVENT_LINK),
	GB_EVENT("Progress", NULL, NULL, &EVENT_PROGRESS),
	GB_EVENT("Load", NULL, NULL, &EVENT_LOAD),
	GB_EVENT("Error", NULL, NULL, &EVENT_ERROR),
	GB_EVENT("Icon", NULL, NULL, &EVENT_ICON),
	GB_EVENT("Title", NULL, NULL, &EVENT_TITLE),
	GB_EVENT("Select", NULL, NULL, &EVENT_SELECT),
	GB_EVENT("Status", NULL, NULL, &EVENT_STATUS),
	GB_EVENT("NewWindow", NULL, "(Modal)b", &EVENT_NEW_WINDOW),
	GB_EVENT("NewFrame", NULL, "(Frame)WebFrame", &EVENT_NEW_FRAME),
	GB_EVENT("Auth", NULL, NULL, &EVENT_AUTH),
	GB_EVENT("Download", NULL, "(Download)WebDownload", &EVENT_DOWNLOAD),

	GB_END_DECLARE
};
#endif

GB_DESC WebViewHistoryItemDesc[] =
{
	GB_DECLARE_VIRTUAL(".WebView.History.Item"),

	GB_PROPERTY_READ("Title", "s", WebView_History_Item_Title),
	GB_PROPERTY_READ("Url", "s", WebView_History_Item_Url),
	GB_METHOD("GoTo", NULL, WebView_History_Item_GoTo, NULL),

	GB_END_DECLARE
};

GB_DESC WebViewHistoryDesc[] =
{
	GB_DECLARE_VIRTUAL(".WebView.History"),

	GB_METHOD("Clear", NULL, WebView_History_Clear, NULL),
	GB_METHOD("_get", ".WebView.History.Item", WebView_History_get, "(Index)i"),
	GB_PROPERTY_READ("CanGoBack", "b", WebView_History_CanGoBack),
	GB_PROPERTY_READ("CanGoForward", "b", WebView_History_CanGoForward),

	GB_END_DECLARE
};

GB_DESC WebViewCookiesDesc[] =
{
	GB_DECLARE_VIRTUAL(".WebView.Cookies"),

	GB_METHOD("Clear", NULL, WebView_Cookies_Clear, NULL),

	GB_END_DECLARE
};

GB_DESC WebViewDesc[] =
{
  GB_DECLARE("WebView", sizeof(CWEBVIEW)), GB_INHERITS("Control"),
	
	GB_STATIC_METHOD("_exit", NULL, WebView_exit, NULL),
	
  GB_METHOD("_new", NULL, WebView_new, "(Parent)Container;"),
  GB_METHOD("_free", NULL, WebView_free, NULL),

	GB_PROPERTY("Url", "s", WebView_Url),
	GB_PROPERTY("Title", "s", WebView_Title),
	GB_PROPERTY("Zoom", "f", WebView_Zoom),
	GB_PROPERTY_READ("Icon", "Picture", WebView_Icon),
	GB_PROPERTY_READ("Progress", "f", WebView_Progress),
	GB_PROPERTY("NewView", "WebView", WebView_NewView),
	GB_PROPERTY_READ("Link", "s", WebView_Link),
	GB_PROPERTY("Language", "s", WebView_Language),
	
	GB_METHOD("SetHtml", NULL, WebView_SetHtml, "(Html)s[(Root)s]"),
	GB_METHOD("GetHtml", "s", WebView_GetHtml, NULL),

	GB_METHOD("Clear", NULL, WebView_Clear, NULL),
	
	GB_METHOD("Back", NULL, WebView_Back, NULL),
	GB_METHOD("Forward", NULL, WebView_Forward, NULL),
	GB_METHOD("Reload", NULL, WebView_Reload, "[(BypassCache)b]"),
	GB_METHOD("Stop", NULL, WebView_Stop, NULL),

	GB_METHOD("ExecJavascript", "s", WebView_ExecJavascript, "(Javascript)s"),
	
	GB_PROPERTY_SELF("History", ".WebView.History"),
	GB_PROPERTY_SELF("Settings", ".WebView.Settings"),
	GB_PROPERTY_SELF("Cookies", ".WebView.Cookies"),

	GB_CONSTANT("_Properties", "s", "*,Url,Zoom=1"),
	GB_CONSTANT("_Group", "s", "View"),
	
	GB_EVENT("Title", NULL, NULL, &EVENT_TITLE),
	GB_EVENT("Url", NULL, NULL, &EVENT_URL),
	GB_EVENT("Icon", NULL, NULL, &EVENT_ICON),
	GB_EVENT("Start", NULL, NULL, &EVENT_START),
	GB_EVENT("Progress", NULL, NULL, &EVENT_PROGRESS),
	GB_EVENT("Finish", NULL, NULL, &EVENT_FINISH),
	GB_EVENT("Error", NULL, NULL, &EVENT_ERROR),
	GB_EVENT("Link", NULL, NULL, &EVENT_LINK),
	GB_EVENT("NewView", NULL, NULL, &EVENT_NEW_VIEW),

	GB_END_DECLARE
};

//-------------------------------------------------------------------------

MyWebEngineView::MyWebEngineView(QWidget *parent) : QWebEngineView(parent)
{
	profile = new QWebEngineProfile(this);
	//settings()->setFontFamily(QWebSettings::FixedFont, "monospace");
	//setPage(new MyWebPage(profile, this));
}

MyWebEngineView::~MyWebEngineView()
{
	delete page();
	delete profile;
}

void MyWebEngineView::clearPage(bool destroy)
{
	QPointer<QWebEnginePage> p;
	if (destroy)
		p = page();
	
	setPage(new MyWebPage(profile, this));
	QObject::connect(page(), SIGNAL(linkHovered(const QString &)), &WebViewSignalManager::manager, SLOT(linkHovered(const QString &)));
	
	if (destroy && p)
		delete p;
}

QWebEngineView *MyWebEngineView::createWindow(QWebEnginePage::WebWindowType type)
{
	void *_object = QT.GetObject(this);
	QWebEngineView *new_view;
	
	if (GB.Raise(THIS, EVENT_NEW_VIEW, 0))
		return NULL;
	
	if (!THIS->new_view)
		return NULL;
	
	new_view = (QWebEngineView *)(((CWEBVIEW *)THIS->new_view)->widget.widget);
	GB.Unref(POINTER(&THIS->new_view));
	THIS->new_view = NULL;
	return new_view;
}

void MyWebEngineView::contextMenuEvent(QContextMenuEvent *event)
{
	void *_object = QT.GetObject(this);
	
	if (EVENT_MENU < 0)
		EVENT_MENU = GB.GetEvent(GB.GetClass(THIS), "Menu");
	
	if (!GB.CanRaise(THIS, EVENT_MENU))
		QWebEngineView::contextMenuEvent(event);
	
	event->accept();
}


//-------------------------------------------------------------------------

MyWebPage::MyWebPage(QWebEngineProfile *profile, QObject *parent) : QWebEnginePage(profile, parent)
{
}

bool MyWebPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
	QWidget *wid = qobject_cast<QWidget *>(parent());
	void *_object;
	
	if (!wid)
		return true;
	
	_object = QT.GetObject(wid);
	
	//fprintf(stderr, "acceptNavigationRequest: cancel = %d / %s\n", THIS->cancel, TO_UTF8(url.toString()));
	
	if (THIS->cancel)
	{
		THIS->cancel = FALSE;
		return false;
	}
	
	return true;
}

//-------------------------------------------------------------------------

WebViewSignalManager WebViewSignalManager::manager;

void WebViewSignalManager::iconChanged()
{
	GET_SENDER();
	GB.Unref(POINTER(&THIS->icon));
	THIS->icon = NULL;
	GB.Raise(THIS, EVENT_ICON, 0);
}

void WebViewSignalManager::titleChanged()
{
	GET_SENDER();
	GB.Raise(THIS, EVENT_TITLE, 0);
}

void WebViewSignalManager::urlChanged()
{
	GET_SENDER();
	GB.Raise(THIS, EVENT_URL, 0);
}

void WebViewSignalManager::linkHovered(const QString &link)
{
	void *_object = QT.GetObject(qobject_cast<QWidget *>((QWebEnginePage*)sender()->parent()));
	
	if (THIS)
	{
		set_link(THIS, link);
		GB.Raise(THIS, EVENT_LINK, 0);
	}
}

void WebViewSignalManager::loadStarted()
{
	GET_SENDER();

	//fprintf(stderr, "loadStarted\n");
	THIS->progress = 0;
	THIS->cancel = GB.Raise(THIS, EVENT_START, 0);
	if (!THIS->cancel)
		GB.Raise(THIS, EVENT_PROGRESS, 0);
}

void WebViewSignalManager::loadProgress(int progress)
{
	GET_SENDER();

	if (THIS->cancel || THIS->progress == progress)
		return;

	THIS->progress = progress;
	GB.Raise(THIS, EVENT_PROGRESS, 0);
	if (progress == 100)
		GB.Raise(THIS, EVENT_FINISH, 0);
}

void WebViewSignalManager::loadFinished(bool ok)
{
	GET_SENDER();

	if (ok)
	{
		if (THIS->progress < 100)
		{
			THIS->progress = 100;
			GB.Raise(THIS, EVENT_FINISH, 0);
		}
	}
	else //if (!THIS->stopping)
		GB.Raise(THIS, EVENT_ERROR, 0);
	
	GB.FreeString(&THIS->link);
}


#if 0

// void CWebView::linkClicked(const QUrl &url)
// {
// 	GET_SENDER();
// 	WIDGET->page()->currentFrame()->setUrl(url);
// 	//WIDGET->setUrl(url);
// 	GB.Raise(THIS, EVENT_CLICK, 0);
// }

void CWebView::selectionChanged()
{
	GET_SENDER();
	GB.Raise(THIS, EVENT_SELECT, 0);
}

void CWebView::statusBarMessage(const QString &text)
{
	GET_SENDER();
	GB.FreeString(&THIS->status);
	THIS->status = NEW_STRING(text);
	GB.Raise(THIS, EVENT_STATUS, 0);
}
	
void CWebView::frameCreated(QWebFrame *frame)
{
	QObject::connect(frame, SIGNAL(urlChanged(const QUrl &)), &CWebView::manager, SLOT(urlChanged(const QUrl &)));
	void *_object = QT.GetObject(((QWebPage*)sender())->view());
	GB.Raise(THIS, EVENT_NEW_FRAME, 1, GB_T_OBJECT, CWEBFRAME_get(frame));
}

void CWebView::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
	//qDebug("CWebView::authenticationRequired: %p", _network_access_manager_view);

	void *_object = _network_access_manager_view; //QT.GetObject((QWidget *)((QNetworkAccessManager*)sender())->parent());
	if (!THIS)
		return;
	
	THIS->reply = reply;
	THIS->authenticator = authenticator;
	
	GB.Raise(THIS, EVENT_AUTH, 0);
	
	THIS->reply = 0;
	THIS->authenticator = 0;
}

void CWebView::downloadRequested(const QNetworkRequest &request)
{
	void *_object = QT.GetObject(((QWebPage*)sender())->view());
	CWEBDOWNLOAD *download;
	
	download = WEB_create_download(_network_access_manager->get(request));
	
	if (GB.Raise(THIS, EVENT_DOWNLOAD, 1, GB_T_OBJECT, download) || !download->path || !*download->path)
		WEB_remove_download(download);
}

void CWebView::handleUnsupportedContent(QNetworkReply *reply)
{
	void *_object = QT.GetObject(((QWebPage*)sender())->view());
	CWEBDOWNLOAD *download;
	
	if (reply->error() == QNetworkReply::NoError)
	{
		download = WEB_create_download(reply);
		
		if (GB.Raise(THIS, EVENT_DOWNLOAD, 1, GB_T_OBJECT, download) || !download->path || !*download->path)
			WEB_remove_download(download);
	}
	else
		delete reply;
}
#endif
