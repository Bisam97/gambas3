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
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QWebEngineView>

//#include "cwebdownload.h"
#include "main.h"

#ifndef __C_WEBVIEW_CPP

/*extern GB_DESC WebViewAuthDesc[];
extern GB_DESC WebViewDownloadsDesc[];*/
extern GB_DESC WebViewDesc[];
extern GB_DESC WebViewCookiesDesc[];
extern GB_DESC WebViewHistoryDesc[];
extern GB_DESC WebViewHistoryItemDesc[];

#else

#define THIS      ((CWEBVIEW *)_object)
#define WIDGET    ((MyWebEngineView *)((QT_WIDGET *)_object)->widget)

#endif

typedef
	struct 
	{
		QT_WIDGET widget;
		QT_PICTURE icon;
		void *new_view;
		char *link;
		int history;
		int progress;
		char *language;
		unsigned cancel : 1;
	}
	CWEBVIEW;

class MyWebPage : public QWebEnginePage
{
	Q_OBJECT

public:
	
	MyWebPage(QWebEngineProfile *profile, QObject *parent);

protected:
	
	virtual bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame);
};

class MyWebEngineView : public QWebEngineView
{
	Q_OBJECT

public:
	
	MyWebEngineView(QWidget *parent);
	~MyWebEngineView();
	void clearPage(bool destroy);

protected:
	
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual QWebEngineView *createWindow(QWebEnginePage::WebWindowType type);


private:

	QWebEngineProfile *profile;
};

class WebViewSignalManager : public QObject
{
  Q_OBJECT

public:

  static WebViewSignalManager manager;

public slots:

	void iconChanged();
	void titleChanged();
	void urlChanged();
	void linkHovered(const QString &link);
	void loadStarted();
	void loadProgress(int progress);
	void loadFinished(bool ok);
	
	/*
	void selectionChanged();
	void statusBarMessage(const QString &text);
	//void downloadRequested(const QNetworkRequest &);
	void frameCreated(QWebFrame *);
	void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void downloadRequested(const QNetworkRequest &);
	void handleUnsupportedContent(QNetworkReply*);*/
};

//QNetworkAccessManager *WEBVIEW_get_network_manager();

#endif
