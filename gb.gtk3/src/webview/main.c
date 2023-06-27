/***************************************************************************

	main.c

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

#define __MAIN_C

#include "main.h"

#include "c_websettings.h"
#include "c_webview.h"

GB_INTERFACE GB EXPORT;
GTK_INTERFACE GTK;

GB_DESC *GB_CLASSES[] EXPORT =
{
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
	GB.GetInterface("gb.gtk3", GTK_INTERFACE_VERSION, &GTK);
	return 0;
}

void EXPORT GB_EXIT()
{
}
