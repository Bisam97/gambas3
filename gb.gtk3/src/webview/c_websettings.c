/***************************************************************************

  c_websettings.c

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

#define __C_WEBSETTINGS_C

#include <errno.h>
#include <unistd.h>

#include "c_webview.h"
#include "c_websettings.h"

static WebKitSettings *_default_settings = NULL;

static WebKitSettings *get_settings(void *_object)
{
	if (!_object || GB.Is(_object, GB.FindClass("WebSettings")))
	{
		if (!_default_settings)
		{
			GtkWidget *widget = webkit_web_view_new();
			_default_settings = g_object_ref(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(widget)));
			gtk_widget_destroy(widget);
		}
		return _default_settings;
	}
	else
	{
		return webkit_web_view_get_settings(WIDGET);
	}
}

//-------------------------------------------------------------------------

static bool get_flag(WebKitSettings *settings, int flag)
{
	switch(flag)
	{
		case 0: return webkit_settings_get_auto_load_images(settings);
		case 1: return webkit_settings_get_enable_javascript(settings);
		case 2: return webkit_settings_get_javascript_can_open_windows_automatically(settings);
		case 3: return webkit_settings_get_javascript_can_access_clipboard(settings); // + 27
		case 5: return webkit_settings_get_enable_html5_local_storage(settings);
		case 8: return webkit_settings_get_enable_spatial_navigation(settings);
		case 9: return webkit_settings_get_allow_file_access_from_file_urls(settings);
		case 10: return webkit_settings_get_enable_hyperlink_auditing(settings);
#if WEBKIT_CHECK_VERSION(2, 32, 0)
#else
		case 13: return webkit_settings_get_enable_plugins(settings);
#endif
		case 14: return webkit_settings_get_enable_fullscreen(settings);
		case 16: return webkit_settings_get_enable_webgl(settings);
#if WEBKIT_CHECK_VERSION(2, 32, 0)
#else
		case 17: return webkit_settings_get_enable_accelerated_2d_canvas(settings);
#endif
		case 21: return webkit_settings_get_print_backgrounds(settings);
		case 26: return webkit_settings_get_media_playback_requires_user_gesture(settings);
		case 29: return webkit_settings_get_enable_dns_prefetching(settings);
	}
	
	return FALSE;
}

static void set_flag(WebKitSettings *settings, int flag, bool value)
{
	switch(flag)
	{
		case 0: webkit_settings_set_auto_load_images(settings, value); break;
		case 1: webkit_settings_set_enable_javascript(settings, value); break;
		case 2: webkit_settings_set_javascript_can_open_windows_automatically(settings, value); break;
		case 3: webkit_settings_set_javascript_can_access_clipboard(settings, value); break; // + 27
		case 5: webkit_settings_set_enable_html5_local_storage(settings, value); break;
		case 8: webkit_settings_set_enable_spatial_navigation(settings, value); break;
		case 9: webkit_settings_set_allow_file_access_from_file_urls(settings, value); break;
		case 10: webkit_settings_set_enable_hyperlink_auditing(settings, value); break;
#if WEBKIT_CHECK_VERSION(2, 32, 0)
#else
		case 13: webkit_settings_set_enable_plugins(settings, value); break;
#endif
		case 14: webkit_settings_set_enable_fullscreen(settings, value); break;
		case 16: webkit_settings_set_enable_webgl(settings, value); break;
#if WEBKIT_CHECK_VERSION(2, 32, 0)
#else
		case 17: webkit_settings_set_enable_accelerated_2d_canvas(settings, value); break;
#endif
		case 21: webkit_settings_set_print_backgrounds(settings, value); break;
		case 26: webkit_settings_set_media_playback_requires_user_gesture(settings, value); break;
		case 29: webkit_settings_set_enable_dns_prefetching(settings, value); break;
	}
}

BEGIN_METHOD(WebSettings_get, GB_INTEGER flag)

	GB.ReturnBoolean(get_flag(get_settings(_object), VARG(flag)));

END_METHOD

BEGIN_METHOD(WebSettings_put, GB_BOOLEAN value; GB_INTEGER flag)

	int flag = VARG(flag);

	if (flag >= 0)
		set_flag(get_settings(_object), flag, VARG(value));

END_METHOD

BEGIN_METHOD_VOID(WebSettings_exit)

	if (_default_settings)
	{
		g_object_unref(_default_settings);
		_default_settings = NULL;
	}

END_METHOD


//-------------------------------------------------------------------------

#define IMPLEMENT_FONT_PROPERTY(_Font, _font) \
BEGIN_PROPERTY(WebSettingsFonts_##_Font) \
 \
	WebKitSettings *settings = get_settings(_object); \
 \
	if (READ_PROPERTY) \
		GB.ReturnNewZeroString(webkit_settings_get_##_font##_font_family(settings)); \
	else \
		webkit_settings_set_##_font##_font_family(settings, GB.ToZeroString(PROP(GB_STRING))); \
 \
END_PROPERTY

IMPLEMENT_FONT_PROPERTY(Default, default)
IMPLEMENT_FONT_PROPERTY(Fixed, monospace)
IMPLEMENT_FONT_PROPERTY(Serif, serif)
IMPLEMENT_FONT_PROPERTY(SansSerif, sans_serif)
IMPLEMENT_FONT_PROPERTY(Cursive, cursive)
IMPLEMENT_FONT_PROPERTY(Fantasy, fantasy)
IMPLEMENT_FONT_PROPERTY(Pictograph, pictograph)

#define IMPLEMENT_SIZE_PROPERTY(_Size, _size) \
BEGIN_PROPERTY(WebSettingsFonts_##_Size##Size) \
 \
	WebKitSettings *settings = get_settings(_object); \
 \
	if (READ_PROPERTY) \
		GB.ReturnInteger(webkit_settings_get_##_size##_font_size(settings) * 72 / 96); \
	else \
		webkit_settings_set_##_size##_font_size(settings, VPROP(GB_INTEGER) * 96 / 72); \
 \
END_PROPERTY

IMPLEMENT_SIZE_PROPERTY(Default, default)
IMPLEMENT_SIZE_PROPERTY(DefaultFixed, default_monospace)
IMPLEMENT_SIZE_PROPERTY(Minimum, minimum)

//-------------------------------------------------------------------------

void WEBVIEW_init_settings(void *_object)
{
	WebKitSettings *init = get_settings(NULL);
	WebKitSettings *settings = get_settings(THIS);
	
	for (int i = 0; i <= 29; i++)
		set_flag(settings, i, get_flag(init, i));
	
	webkit_settings_set_default_font_family(settings, webkit_settings_get_default_font_family(init));
	webkit_settings_set_monospace_font_family(settings, webkit_settings_get_monospace_font_family(init));
	webkit_settings_set_serif_font_family(settings, webkit_settings_get_serif_font_family(init));
	webkit_settings_set_sans_serif_font_family(settings, webkit_settings_get_sans_serif_font_family(init));
	webkit_settings_set_cursive_font_family(settings, webkit_settings_get_cursive_font_family(init));
	webkit_settings_set_fantasy_font_family(settings, webkit_settings_get_fantasy_font_family(init));
	webkit_settings_set_pictograph_font_family(settings, webkit_settings_get_pictograph_font_family(init));
	
	webkit_settings_set_default_font_size(settings, webkit_settings_get_default_font_size(init));
	webkit_settings_set_default_monospace_font_size(settings, webkit_settings_get_default_monospace_font_size(init));
	webkit_settings_set_minimum_font_size(settings, webkit_settings_get_minimum_font_size(init));

	//webkit_settings_set_enable_developer_extras(settings, TRUE);
}

#if 0
/***************************************************************************/

BEGIN_METHOD_VOID(WebSettings_exit)

	GB.FreeString(&_cache_path);

END_METHOD

/***************************************************************************/

BEGIN_PROPERTY(WebSettingsProxy_Host)

	QNetworkAccessManager *nam = WEBVIEW_get_network_manager();
	QNetworkProxy proxy = nam->proxy();
	
	if (READ_PROPERTY)
		RETURN_NEW_STRING(proxy.hostName());
	else
	{
		proxy.setHostName(QSTRING_PROP());
		nam->setProxy(proxy);
	}

END_PROPERTY

BEGIN_PROPERTY(WebSettingsProxy_User)

	QNetworkAccessManager *nam = WEBVIEW_get_network_manager();
	QNetworkProxy proxy = nam->proxy();
	
	if (READ_PROPERTY)
		RETURN_NEW_STRING(proxy.user());
	else
	{
		proxy.setUser(QSTRING_PROP());
		nam->setProxy(proxy);
	}

END_PROPERTY

BEGIN_PROPERTY(WebSettingsProxy_Password)

	QNetworkAccessManager *nam = WEBVIEW_get_network_manager();
	QNetworkProxy proxy = nam->proxy();
	
	if (READ_PROPERTY)
		RETURN_NEW_STRING(proxy.password());
	else
	{
		proxy.setPassword(QSTRING_PROP());
		nam->setProxy(proxy);
	}

END_PROPERTY

BEGIN_PROPERTY(WebSettingsProxy_Port)

	QNetworkAccessManager *nam = WEBVIEW_get_network_manager();
	QNetworkProxy proxy = nam->proxy();
	
	if (READ_PROPERTY)
		GB.ReturnInteger(proxy.port());
	else
	{
		proxy.setPort(VPROP(GB_INTEGER));
		nam->setProxy(proxy);
	}

END_PROPERTY

BEGIN_PROPERTY(WebSettingsProxy_Type)

	QNetworkAccessManager *nam = WEBVIEW_get_network_manager();
	QNetworkProxy proxy = nam->proxy();
	
	if (READ_PROPERTY)
		GB.ReturnInteger(proxy.type());
	else
	{
		int type = VPROP(GB_INTEGER);
		if (type == QNetworkProxy::DefaultProxy || type == QNetworkProxy::NoProxy || type == QNetworkProxy::Socks5Proxy || type == QNetworkProxy::HttpProxy)
		{
			proxy.setType((QNetworkProxy::ProxyType)type);
			nam->setProxy(proxy);
		}
	}

END_PROPERTY
#endif

/***************************************************************************/

#if 0
GB_DESC WebSettingsIconDatabaseDesc[] =
{
  GB_DECLARE(".WebSettings.IconDatabase", 0), GB_VIRTUAL_CLASS(),
	
	GB_STATIC_METHOD("Clear", NULL, WebSettingsIconDatabase_Clear, NULL),
	GB_STATIC_PROPERTY("Path", "s", WebSettingsIconDatabase_Path),
	GB_STATIC_METHOD("_get", "Picture", WebSettingsIconDatabase_get, "(Url)s"),
	
	GB_END_DECLARE
};

GB_DESC WebSettingsCacheDesc[] =
{
  GB_DECLARE(".WebSettings.Cache", 0), GB_VIRTUAL_CLASS(),
	
	GB_STATIC_PROPERTY("Enabled", "b", WebSettingsCache_Enabled),
	GB_STATIC_PROPERTY("Path", "s", WebSettingsCache_Path),
	GB_STATIC_METHOD("Clear", NULL, WebSettingsCache_Clear, NULL),
	
	GB_END_DECLARE
};

GB_DESC WebSettingsProxyDesc[] =
{
  GB_DECLARE(".WebSettings.Proxy", 0), GB_VIRTUAL_CLASS(),
	
	GB_STATIC_PROPERTY("Type", "i", WebSettingsProxy_Type),
	GB_STATIC_PROPERTY("Host", "s", WebSettingsProxy_Host),
	GB_STATIC_PROPERTY("Port", "i", WebSettingsProxy_Port),
	GB_STATIC_PROPERTY("User", "s", WebSettingsProxy_User),
	GB_STATIC_PROPERTY("Password", "s", WebSettingsProxy_Password),
	
	GB_END_DECLARE
};
#endif

GB_DESC WebSettingsFontsDesc[] =
{
	GB_DECLARE_VIRTUAL(".WebSettings.Fonts"),

	GB_PROPERTY("Default", "s", WebSettingsFonts_Default),
	GB_PROPERTY("Fixed", "s", WebSettingsFonts_Fixed),
	GB_PROPERTY("Serif", "s", WebSettingsFonts_Serif),
	GB_PROPERTY("SansSerif", "s", WebSettingsFonts_SansSerif),
	GB_PROPERTY("Cursive", "s", WebSettingsFonts_Cursive),
	GB_PROPERTY("Fantasy", "s", WebSettingsFonts_Fantasy),
	GB_PROPERTY("Pictograph", "s", WebSettingsFonts_Pictograph),

	GB_PROPERTY("MinimumSize", "i", WebSettingsFonts_MinimumSize),
	GB_PROPERTY("DefaultSize", "i", WebSettingsFonts_DefaultSize),
	GB_PROPERTY("DefaultFixedSize", "i", WebSettingsFonts_DefaultFixedSize),
	
	GB_END_DECLARE
};

GB_DESC WebViewSettingsDesc[] =
{
  GB_DECLARE_VIRTUAL(".WebView.Settings"),
	
	GB_METHOD("_get", "b", WebSettings_get, "(Flag)i"),
	GB_METHOD("_put", NULL, WebSettings_put, "(Value)b(Flag)i"),
	
	GB_PROPERTY_SELF("Fonts", ".WebSettings.Fonts"),

	GB_END_DECLARE
};

GB_DESC WebSettingsDesc[] =
{
  GB_DECLARE("WebSettings", sizeof(CWEBSETTINGS)), GB_AUTO_CREATABLE(), GB_NOT_CREATABLE(),
	
	GB_CONSTANT("AutoLoadImages", "i", 0),
	GB_CONSTANT("Javascript", "i", 1),
	GB_CONSTANT("JavascriptCanOpenWindows", "i", 2),
	GB_CONSTANT("JavascriptCanAccessClipboard", "i", 3),
	GB_CONSTANT("LocalStorage", "i", 5),
	GB_CONSTANT("SpatialNavigation", "i", 8),
	GB_CONSTANT("LocalContentCanAccessFileUrls", "i", 9),
	GB_CONSTANT("HyperlinkAuditing", "i", 10),
	GB_CONSTANT("Plugins", "i", 13),
	GB_CONSTANT("FullScreenSupport", "i", 14),
	GB_CONSTANT("WebGL", "i", 16),
	GB_CONSTANT("Accelerated2dCanvas", "i", 17),
	GB_CONSTANT("PrintElementBackgrounds", "i", 21),
	GB_CONSTANT("PlaybackRequiresUserGesture", "i", 26),
	GB_CONSTANT("DnsPrefetch", "i", 29),

	/*GB_CONSTANT("DefaultProxy", "i", QNetworkProxy::DefaultProxy),
	GB_CONSTANT("NoProxy", "i", QNetworkProxy::NoProxy),
	GB_CONSTANT("Socks5Proxy", "i", QNetworkProxy::Socks5Proxy),
	GB_CONSTANT("HttpProxy", "i", QNetworkProxy::HttpProxy),*/
	
	/*GB_STATIC_PROPERTY_SELF("Fonts", ".WebSettings.Fonts"),
	GB_STATIC_PROPERTY_SELF("IconDatabase", ".WebSettings.IconDatabase"),
	GB_STATIC_PROPERTY_SELF("Cache", ".WebSettings.Cache"),
	GB_STATIC_PROPERTY_SELF("Proxy", ".WebSettings.Proxy"),*/
	
	GB_STATIC_METHOD("_exit", NULL, WebSettings_exit, NULL),
	
	GB_PROPERTY_SELF("Fonts", ".WebSettings.Fonts"),
	//GB_STATIC_METHOD("_exit", NULL, WebSettings_exit, NULL),

	GB_METHOD("_get", "b", WebSettings_get, "(Flag)i"),
	GB_METHOD("_put", NULL, WebSettings_put, "(Value)b(Flag)i"),
	
	GB_END_DECLARE
};


