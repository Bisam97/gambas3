/***************************************************************************

  c_webview.h

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

#ifndef __C_WEBVIEW_H
#define __C_WEBVIEW_H

#include <QUrl>
#include <QAuthenticator>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>

//#include "cwebdownload.h"
#include "main.h"

#ifndef __C_WEBVIEW_CPP

extern GB_DESC WebViewDesc[];
extern GB_DESC WebViewHistoryDesc[];
extern GB_DESC WebViewHistoryItemDesc[];
extern GB_DESC WebViewCookiesDesc[];

#else

#define THIS      ((CWEBVIEW *)_object)
#define WIDGET    ((MyWebView *)((QT_WIDGET *)_object)->widget)

#endif

class MyWebPage : public QWebPage
{
	Q_OBJECT

public:
	
	MyWebPage(QObject *parent);

/*protected:
	
	virtual QString userAgentForUrl(const QUrl& url) const;*/
};

class MyWebView : public QWebView
{
	Q_OBJECT

public:
	
	MyWebView(QWidget *parent);

protected:
		
	virtual QWebView *createWindow(QWebPage::WebWindowType type);
	virtual void contextMenuEvent(QContextMenuEvent *event);
};

typedef
	struct 
	{
		QT_WIDGET widget;
		QT_PICTURE icon;
		void *new_view;
		char *link;
		char *language;
		int history;
		int progress;
		unsigned cancel : 1;
		unsigned stopping : 1;
	}
	CWEBVIEW;

class CWebView : public QObject
{
  Q_OBJECT

public:

  static CWebView manager;

public slots:

	void iconChanged();
	void loadFinished(bool ok);
	void loadProgress(int progress);
	void loadStarted();
	//void selectionChanged();
	//void statusBarMessage(const QString &text);
	void titleChanged(const QString &title);
	void linkHovered(const QString &link, const QString &title, const QString &textContent);
	//void frameCreated(QWebFrame *);
	//void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void urlChanged(const QUrl &);
	//void downloadRequested(const QNetworkRequest &);
	//void handleUnsupportedContent(QNetworkReply*);
};

//QNetworkAccessManager *WEBVIEW_get_network_manager();

#endif
