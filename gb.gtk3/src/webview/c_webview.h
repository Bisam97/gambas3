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

#include <webkit2/webkit2.h>
#include "main.h"

typedef
  struct {
    GTK_CONTROL control;
		GtkWidget *widget;
    WebKitWebContext *context;
    WebKitUserContentManager *manager;
		WebKitBackForwardListItem *item;
		GTK_PICTURE icon;
		void *new_view;
		char *link;
		char *cb_result;
    char *language;
		unsigned error : 1;
		unsigned accept_next : 1;
		unsigned got_load_event : 1;
		unsigned cb_running : 1;
		unsigned cb_error : 1;
   }
  CWEBVIEW;

#ifndef __C_WEBVIEW_C
extern GB_DESC WebViewDesc[];
extern GB_DESC WebViewHistoryDesc[];
extern GB_DESC WebViewHistoryItemDesc[];
extern GB_DESC WebViewCookiesDesc[];
#else

#define THIS    ((CWEBVIEW *)_object)
#define WIDGET  (WEBKIT_WEB_VIEW(THIS->widget))

//#define CGLAREA_PROPERTIES QT_WIDGET_PROPERTIES

#endif /* __CGLAREA_CPP */

#endif
