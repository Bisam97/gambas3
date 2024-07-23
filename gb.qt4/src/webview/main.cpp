/***************************************************************************

	main.cpp

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

#define __MAIN_CPP

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <QDateTime>

/*#include "ccookiejar.h"
#include "cwebhittest.h"
#include "cwebelement.h"
#include "cwebframe.h"
#include "cwebdownload.h"*/
#include "c_websettings.h"
#include "c_webview.h"
#include "main.h"

GB_CLASS CLASS_WebView;

extern "C" {

GB_INTERFACE GB EXPORT;
QT_INTERFACE QT;

GB_DESC *GB_CLASSES[] EXPORT =
{
	/*WebDownloadDesc,
	WebDownloadsDesc,
	WebHitTestDesc,
	CookieDesc,
	WebSettingsIconDatabaseDesc,
	WebSettingsCacheDesc,
	WebSettingsProxyDesc,
	WebSettingsDesc,
	WebElementStyleDesc,
	WebElementDesc,
	WebFrameChildrenDesc,
	WebFrameDesc,
	WebViewAuthDesc,*/
	WebSettingsFontsDesc,
	WebSettingsDesc,
	WebViewSettingsDesc,
	WebViewHistoryItemDesc,
	WebViewHistoryDesc,
	WebViewCookiesDesc,
	WebViewDesc,
	NULL
};

int EXPORT GB_INIT(void)
{
	GB.GetInterface("gb.qt4", QT_INTERFACE_VERSION, &QT);
	CLASS_WebView = GB.FindClass("WebView");
	return 0;
}

void EXPORT GB_EXIT()
{
}

}

void MAIN_return_qvariant(const QVariant &result)
{
	GB_DATE date;
	GB_DATE_SERIAL ds;
	QDateTime qdate;

	switch (result.type())
	{
		case QVariant::Bool:
			GB.ReturnBoolean(result.toBool());
			break;
			
		case QVariant::Date:
		case QVariant::DateTime:
			qdate = result.toDateTime();
			ds.year = qdate.date().year();
			ds.month = qdate.date().month();
			ds.day = qdate.date().day();
			ds.hour = qdate.time().hour();
			ds.min = qdate.time().minute();
			ds.sec = qdate.time().second();
			ds.msec = qdate.time().msec();
			GB.MakeDate(&ds, &date, TRUE);
			GB.ReturnDate(&date);
			break;
			
		case QVariant::Double:
			GB.ReturnFloat(result.toDouble());
			break;
			
		case QVariant::Int:
		case QVariant::UInt:
			GB.ReturnInteger(result.toInt());
			break;
			
		case QVariant::LongLong:
		case QVariant::ULongLong:
			GB.ReturnLong(result.toLongLong());
			break;
			
		case QVariant::String:
			RETURN_NEW_STRING(result.toString());
			break;
		
		// TODO: Handle these three datatypes
		case QVariant::Hash:
		case QVariant::List:
		case QVariant::RegExp:
		default:
			GB.ReturnNull();
			break;
	}
	
	GB.ReturnConvVariant();
}

