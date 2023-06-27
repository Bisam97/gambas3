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

#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QWebPage>
#include <QWebFrame>
#include <QWebHistory>
#include <QContextMenuEvent>

#include "jsonwriter.h"

/*#include "ccookiejar.h"
#include "cwebelement.h"
#include "cwebframe.h"
#include "cwebhittest.h"*/
#include "c_websettings.h"
#include "c_webview.h"

#define HISTORY (WIDGET->history())

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

//static QNetworkAccessManager *_network_access_manager = 0;
static CWEBVIEW *_network_access_manager_view = 0;
//static QT_COLOR_FUNC _old_after_set_color;
static bool _ignore_png_warnings = false;


/*QNetworkAccessManager *WEBVIEW_get_network_manager()
{
	if (!_network_access_manager)
	{
		_network_access_manager = new QNetworkAccessManager();
		_network_access_manager->setCookieJar(new MyCookieJar);
	}
	
	return _network_access_manager;
}*/

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

/*static void after_set_color(void *_object)
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
}*/

static void stop_view(void *_object)
{
	//fprintf(stderr, "stop_view\n");
	THIS->stopping = TRUE;
	WIDGET->stop();
	THIS->stopping = FALSE;
	THIS->cancel = FALSE;
}

static void set_link(void *_object, const QString &link)
{
	GB.FreeString(&THIS->link);
	THIS->link = QT.NewString(link);
}

static void update_language(void *_object)
{
	// TODO: it seems that Qt4 QWebView does not support setting the language.
}

//-------------------------------------------------------------------------

BEGIN_METHOD(WebView_new, GB_OBJECT parent)

	int fd_save = -1;
	
	if (!_ignore_png_warnings)
	{
		int fd = ::open("/dev/null", O_RDWR);
		fd_save = ::dup(2);
		::dup2(fd, 2);
		::close(fd);
	}

  MyWebView *wid = new MyWebView(QT.GetContainer(VARG(parent)));
	
	if (!_ignore_png_warnings)
	{
		::dup2(fd_save, 2);
		::close(fd_save);
		_ignore_png_warnings = true;
		
		//_old_after_set_color = QT.AfterSetColor(after_set_color);
		QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont, "monospace");
	}

  QT.InitWidget(wid, _object, false);
	QT.SetWheelFlag(_object);
	
	//WEBVIEW_get_network_manager();
	
	//wid->page()->setNetworkAccessManager(_network_access_manager);
	wid->page()->setForwardUnsupportedContent(true);

  QObject::connect(wid, SIGNAL(loadFinished(bool)), &CWebView::manager, SLOT(loadFinished(bool)));
  QObject::connect(wid, SIGNAL(loadProgress(int)), &CWebView::manager, SLOT(loadProgress(int)));
  QObject::connect(wid, SIGNAL(loadStarted()), &CWebView::manager, SLOT(loadStarted()));
  //QObject::connect(wid, SIGNAL(selectionChanged()), &CWebView::manager, SLOT(selectionChanged()));
  //QObject::connect(wid, SIGNAL(statusBarMessage(const QString &)), &CWebView::manager, SLOT(statusBarMessage(const QString &)));
  QObject::connect(wid, SIGNAL(titleChanged(const QString &)), &CWebView::manager, SLOT(titleChanged(const QString &)));
  
	//QObject::connect(wid->page(), SIGNAL(frameCreated(QWebFrame *)), &CWebView::manager, SLOT(frameCreated(QWebFrame *)));
	//QObject::connect(wid->page(), SIGNAL(downloadRequested(QNetworkRequest)), &CWebView::manager, SLOT(downloadRequested(QNetworkRequest)));
  //QObject::connect(wid->page(), SIGNAL(unsupportedContent(QNetworkReply*)), &CWebView::manager, SLOT(handleUnsupportedContent(QNetworkReply*)));
	
  QObject::connect(wid, SIGNAL(iconChanged()), &CWebView::manager, SLOT(iconChanged()));
	QObject::connect(wid->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString &)), &CWebView::manager, 
										SLOT(linkHovered(const QString &, const QString &, const QString &)));
	QObject::connect(wid->page()->mainFrame(), SIGNAL(urlChanged(const QUrl &)), &CWebView::manager, SLOT(urlChanged(const QUrl &)));

	/*QObject::connect(wid->page()->networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), &CWebView::manager,
										SLOT(authenticationRequired(QNetworkReply *, QAuthenticator *)));*/

	update_language(THIS);

END_METHOD

BEGIN_METHOD_VOID(WebView_free)

	if (_network_access_manager_view == THIS)
		_network_access_manager_view = 0;
	
	//GB.FreeString(&THIS->status);
	//GB.FreeString(&THIS->userAgent);
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
		stop_view(THIS);
		set_link(THIS, url);
		WIDGET->setUrl(url);
	}

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

BEGIN_PROPERTY(WebView_Icon)

	if (!THIS->icon)
	{
		QIcon icon = WIDGET->icon();
		
		if (icon.isNull()) 
			icon = QWebSettings::iconForUrl(WIDGET->url());
		
		if (!icon.isNull())
		{
			THIS->icon = QT.CreatePicture(icon.pixmap(16, 16));
			GB.Ref(THIS->icon);
		}
	}

	GB.ReturnObject(THIS->icon);

END_PROPERTY

BEGIN_PROPERTY(WebView_Zoom)

	if (READ_PROPERTY)
		GB.ReturnFloat(WIDGET->zoomFactor());
	else
		WIDGET->setZoomFactor(VPROP(GB_FLOAT));

END_PROPERTY

BEGIN_PROPERTY(WebView_Title)

	RETURN_NEW_STRING(WIDGET->title());

END_PROPERTY

BEGIN_METHOD_VOID(WebView_Back)

	WIDGET->back();

END_METHOD

BEGIN_METHOD_VOID(WebView_Forward)

	WIDGET->forward();

END_METHOD

BEGIN_METHOD(WebView_Reload, GB_BOOLEAN bypass)

	bool bypass = VARGOPT(bypass, false);
	stop_view(THIS);
	if (bypass)
		WIDGET->page()->triggerAction(QWebPage::ReloadAndBypassCache);
	else
		WIDGET->reload();

END_METHOD

BEGIN_METHOD_VOID(WebView_Stop)

	stop_view(THIS);

END_METHOD

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

	delete WIDGET->page();
	WIDGET->setPage(new QWebPage(WIDGET));
	QObject::connect(WIDGET->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString &)), &CWebView::manager, 
										SLOT(linkHovered(const QString &, const QString &, const QString &)));
	QObject::connect(WIDGET->page()->mainFrame(), SIGNAL(urlChanged(const QUrl &)), &CWebView::manager, SLOT(urlChanged(const QUrl &)));

END_METHOD

BEGIN_METHOD(WebView_ExecJavascript, GB_STRING script)

	if (LENGTH(script) == 0)
		return;
	
	QVariant result = WIDGET->page()->currentFrame()->evaluateJavaScript(QSTRING_ARG(script));
	QByteArray json;
	
	JSONWRITER_valueToJson(result, json);
	
	GB.ReturnNewString(json.constData(), json.size());
	
END_METHOD

BEGIN_METHOD_VOID(WebView_GetHtml)

	RETURN_NEW_STRING(WIDGET->page()->mainFrame()->toHtml());

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

//-------------------------------------------------------------------------

static QWebHistoryItem get_item(QWebHistory *history, int index)
{
	if (index == 0)
		return history->currentItem();
	
	QList<QWebHistoryItem> list;
	
	if (index > 0)
		list = history->forwardItems(history->count());
	else
	{
		list = history->backItems(history->count());
		index = (-index);
	}
	
	return list.at(index);
}

BEGIN_PROPERTY(WebViewHistoryItem_Title)

	QWebHistoryItem item = get_item(HISTORY, THIS->history);
	
	if (item.isValid())
		RETURN_NEW_STRING(item.title());
	else
		GB.ReturnNull();

END_PROPERTY

BEGIN_PROPERTY(WebViewHistoryItem_Url)

	QWebHistoryItem item = get_item(HISTORY, THIS->history);
	
	if (item.isValid())
		RETURN_NEW_STRING(item.url().toString());
	else
		GB.ReturnNull();

END_PROPERTY

BEGIN_METHOD_VOID(WebViewHistoryItem_GoTo)

	QWebHistoryItem item = get_item(HISTORY, THIS->history);
	
	if (item.isValid())
		HISTORY->goToItem(item);

END_METHOD

BEGIN_METHOD_VOID(WebViewHistory_Clear)

	HISTORY->clear();

END_METHOD

BEGIN_METHOD(WebViewHistory_get, GB_INTEGER index)

	int index = VARG(index);
	
	if (index > 0 && index > HISTORY->forwardItems(HISTORY->count()).count())
		GB.ReturnNull();
	else if (index < 0 && (-index) > HISTORY->backItems(HISTORY->count()).count())
		GB.ReturnNull();
	else
	{
		THIS->history = index;
		RETURN_SELF();
	}

END_METHOD

BEGIN_PROPERTY(WebViewHistory_CanGoBack)

	GB.ReturnBoolean(HISTORY->canGoBack());

END_PROPERTY

BEGIN_PROPERTY(WebViewHistory_CanGoForward)

	GB.ReturnBoolean(HISTORY->canGoForward());

END_PROPERTY

//-------------------------------------------------------------------------

class MyCookieJar : public QNetworkCookieJar
{
	Q_OBJECT

public:

	MyCookieJar(QObject *parent = 0);

	QList<QNetworkCookie> allCookies () const { return QNetworkCookieJar::allCookies(); }
	void setAllCookies(const QList<QNetworkCookie> &cookieList) { QNetworkCookieJar::setAllCookies(cookieList); }

	//virtual QList<QNetworkCookie> cookiesForUrl(const QUrl & url) const;
	//virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);
};

BEGIN_METHOD_VOID(WebView_Cookies_Clear)

	QList<QNetworkCookie> list;
	static_cast<MyCookieJar *>(WIDGET->page()->networkAccessManager()->cookieJar())->setAllCookies(list);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC WebViewHistoryItemDesc[] = 
{
	GB_DECLARE_VIRTUAL(".WebView.History.Item"),
	
	GB_PROPERTY_READ("Title", "s", WebViewHistoryItem_Title),
	GB_PROPERTY_READ("Url", "s", WebViewHistoryItem_Url),
	GB_METHOD("GoTo", NULL, WebViewHistoryItem_GoTo, NULL),
	
	GB_END_DECLARE
};

GB_DESC WebViewHistoryDesc[] =
{
	GB_DECLARE_VIRTUAL(".WebView.History"),

	GB_METHOD("Clear", NULL, WebViewHistory_Clear, NULL),
	GB_METHOD("_get", ".WebView.History.Item", WebViewHistory_get, "(Index)i"),
	GB_PROPERTY_READ("CanGoBack", "b", WebViewHistory_CanGoBack),
	GB_PROPERTY_READ("CanGoForward", "b", WebViewHistory_CanGoForward),

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

MyWebPage::MyWebPage(QObject *parent) : QWebPage(parent)
{
}

/*QString MyWebPage::userAgentForUrl(const QUrl& url) const 
{
	MyWebView *view = (MyWebView *)parent();
	void *_object = QT.GetObject(view);
	
	if (THIS->userAgent)
		return (const char *)THIS->userAgent;
	else
		return QWebPage::userAgentForUrl(url);
};*/


MyWebView::MyWebView(QWidget *parent) : QWebView(parent)
{
	//settings()->setFontFamily(QWebSettings::FixedFont, "monospace");
	setPage(new MyWebPage(this));
}

QWebView *MyWebView::createWindow(QWebPage::WebWindowType type)
{
	void *_object = QT.GetObject(this);
	QWebView *new_view;
	
	if (GB.Raise(THIS, EVENT_NEW_VIEW, 0))
		return NULL;
	
	if (!THIS->new_view)
		return NULL;
	
	new_view = (QWebView *)(((CWEBVIEW *)THIS->new_view)->widget.widget);
	GB.Unref(POINTER(&THIS->new_view));
	THIS->new_view = NULL;
	return new_view;
}

void MyWebView::contextMenuEvent(QContextMenuEvent *event)
{
	void *_object = QT.GetObject(this);
	
	if (EVENT_MENU < 0)
		EVENT_MENU = GB.GetEvent(GB.GetClass(THIS), "Menu");
	
	if (!GB.CanRaise(THIS, EVENT_MENU))
		QWebView::contextMenuEvent(event);
	
	event->accept();
}

/***************************************************************************/

CWebView CWebView::manager;

void CWebView::loadFinished(bool ok)
{
	GET_SENDER();

	//fprintf(stderr, "loadFinished %d (%d)\n", ok, THIS->stopping);

	THIS->progress = 100;

	if (ok)
		GB.Raise(THIS, EVENT_FINISH, 0);
	else if (!THIS->stopping)
		GB.Raise(THIS, EVENT_ERROR, 0);

	GB.FreeString(&THIS->link);
}

void CWebView::loadProgress(int progress)
{
	GET_SENDER();

	if (THIS->cancel || THIS->progress == progress)
		return;

	THIS->progress = progress;
	GB.Raise(THIS, EVENT_PROGRESS, 0);
}

void CWebView::loadStarted()
{
	GET_SENDER();

	THIS->progress = 0;
	_network_access_manager_view = THIS;
	THIS->cancel = GB.Raise(THIS, EVENT_START, 0);
	if (THIS->cancel)
		stop_view(THIS);
	else
		GB.Raise(THIS, EVENT_PROGRESS, 0);
}

/*void CWebView::selectionChanged()
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
}*/
	
void CWebView::titleChanged(const QString &title)
{
	GET_SENDER();
	GB.Raise(THIS, EVENT_TITLE, 0);
}

void CWebView::linkHovered(const QString &link, const QString &title, const QString &textContent)
{
	void *_object = QT.GetObject(((QWebPage*)sender())->view());
	
	if (THIS)
	{
		set_link(THIS, link);
		GB.Raise(THIS, EVENT_LINK, 0);
	}
}

/*void CWebView::frameCreated(QWebFrame *frame)
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
}*/

void CWebView::iconChanged()
{
	GET_SENDER();
	GB.Unref(POINTER(&THIS->icon));
	THIS->icon = NULL;
	GB.RaiseLater(THIS, EVENT_ICON);
}

void CWebView::urlChanged(const QUrl &)
{
	QWebFrame *frame = (QWebFrame *)sender();
	void *_object = QT.GetObject(frame->page()->view());
	//GB.Raise(THIS, EVENT_CLICK, 1, GB_T_OBJECT, CWEBFRAME_get(frame));
	GB.Raise(THIS, EVENT_URL, 0);
}

/*void CWebView::downloadRequested(const QNetworkRequest &request)
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
}*/
