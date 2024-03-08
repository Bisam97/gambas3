/***************************************************************************

  c_websettings.cpp

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

#define __C_WEBSETTINGS_CPP

#include <errno.h>
#include <unistd.h>

#include "c_webview.h"
#include "c_websettings.h"

//static QNetworkDiskCache *_cache = 0;
/*static bool _cache_enabled = false;
static char *_cache_path = 0;

static void set_cache(bool on)
{
	QNetworkDiskCache *cache;
	
	if (!_cache_path)
		return;
	
	_cache_enabled = on;
	
	if (on)
	{
		cache = new QNetworkDiskCache(0);
		cache->setCacheDirectory(TO_QSTRING(_cache_path));
		WEBVIEW_get_network_manager()->setCache(cache);
	}
	else
		WEBVIEW_get_network_manager()->setCache(0);
}*/


static QWebEngineSettings *get_settings(void *_object)
{
	if (GB.Is(_object, GB.FindClass("WebSettings")))
		return QWebEngineProfile::defaultProfile()->settings();
	else
		return WEBVIEW->page()->profile()->settings();
}

//-------------------------------------------------------------------------

static void handle_font_family(QWebEngineSettings::FontFamily font, void *_object, void *_param)
{
	if (READ_PROPERTY)
		RETURN_NEW_STRING(get_settings(_object)->fontFamily(font));
	else
		get_settings(_object)->setFontFamily(font, QSTRING_PROP());
}

BEGIN_PROPERTY(WebSettingsFonts_Default)

	handle_font_family(QWebEngineSettings::StandardFont, _object, _param);

END_METHOD

BEGIN_PROPERTY(WebSettingsFonts_Fixed)

	handle_font_family(QWebEngineSettings::FixedFont, _object, _param);

END_METHOD

BEGIN_PROPERTY(WebSettingsFonts_Serif)

	handle_font_family(QWebEngineSettings::SerifFont, _object, _param);

END_METHOD

BEGIN_PROPERTY(WebSettingsFonts_SansSerif)

	handle_font_family(QWebEngineSettings::SansSerifFont, _object, _param);

END_METHOD

BEGIN_PROPERTY(WebSettingsFonts_Cursive)

	handle_font_family(QWebEngineSettings::CursiveFont, _object, _param);

END_METHOD

BEGIN_PROPERTY(WebSettingsFonts_Fantasy)

	handle_font_family(QWebEngineSettings::FantasyFont, _object, _param);

END_METHOD

BEGIN_PROPERTY(WebSettingsFonts_Pictograph)

	handle_font_family(QWebEngineSettings::PictographFont, _object, _param);

END_METHOD

static void handle_font_size(QWebEngineSettings::FontSize size, void *_object, void *_param)
{
	if (READ_PROPERTY)
		GB.ReturnInteger(get_settings(_object)->fontSize(size) * 72 / 96);
	else
		get_settings(_object)->setFontSize(size, VPROP(GB_INTEGER) * 96 / 72);
}

BEGIN_PROPERTY(WebSettingsFonts_DefaultSize)

	handle_font_size(QWebEngineSettings::DefaultFontSize, _object, _param);

END_PROPERTY

BEGIN_PROPERTY(WebSettingsFonts_DefaultFixedSize)

	handle_font_size(QWebEngineSettings::DefaultFixedFontSize, _object, _param);

END_PROPERTY

BEGIN_PROPERTY(WebSettingsFonts_MinimumSize)

	handle_font_size(QWebEngineSettings::MinimumFontSize, _object, _param);

END_PROPERTY

/*BEGIN_PROPERTY(WebSettingsFonts_MinimumLogicalFontSize)

	handle_font_size(QWebEngineSettings::MinimumLogicalFontSize, _object, _param);

END_PROPERTY*/

#if 0
/***************************************************************************/

BEGIN_METHOD_VOID(WebSettingsIconDatabase_Clear)

	QWebEngineSettings::clearIconDatabase();

END_METHOD

BEGIN_PROPERTY(WebSettingsIconDatabase_Path)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(QWebEngineSettings::iconDatabasePath());
	else
		QWebEngineSettings::setIconDatabasePath(QSTRING_PROP());

END_PROPERTY

BEGIN_METHOD(WebSettingsIconDatabase_get, GB_STRING url)

	QIcon icon;
	QSize size;
	QSize max_size;
	
	icon = QWebEngineSettings::iconForUrl(QSTRING_ARG(url));
	if (icon.isNull())
	{
		GB.ReturnNull();
		return;
	}
	
	foreach(size, icon.availableSizes())
	{
		if ((size.width() * size.height()) > (max_size.width() * max_size.height()))
			max_size = size;
	}
	
	GB.ReturnObject(QT.CreatePicture(icon.pixmap(max_size)));

END_METHOD


/***************************************************************************/

BEGIN_PROPERTY(WebSettingsCache_Path)

	if (READ_PROPERTY)
		GB.ReturnString(_cache_path);
	else
	{
		char *path = GB.FileName(PSTRING(), PLENGTH());
		QString qpath = QString(path);
		QString root = QString(GB.System.Home());

		if (root.at(root.length() - 1) != '/')
			root += '/';
		root += ".cache/";
		if (!qpath.startsWith(root))
		{
			GB.Error("Cache directory must be located inside ~/.cache");
			return;
		}

		GB.FreeString(&_cache_path);
		_cache_path = GB.NewZeroString(path);
		set_cache(_cache_enabled);
	}

END_PROPERTY

BEGIN_PROPERTY(WebSettingsCache_Enabled)

	if (READ_PROPERTY)
		GB.ReturnBoolean(_cache_enabled);
	else
		set_cache(VPROP(GB_BOOLEAN));

END_PROPERTY

static int _clear_error;
static char *_clear_path;

static void remove_file(const char *path)
{
	if (rmdir(path) == 0)
		return;

	if (errno == ENOTDIR)
	{
		if (unlink(path) == 0)
			return;
	}

	if (_clear_error == 0)
	{
		_clear_error = errno;
		_clear_path = GB.NewZeroString(path);
	}
}

BEGIN_METHOD_VOID(WebSettingsCache_Clear)

	if (!_cache_path || !*_cache_path)
		return;

	_clear_error = 0;
	GB.BrowseDirectory(_cache_path, NULL, remove_file);

	if (_clear_error)
	{
		GB.Error("Unable to remove '&1': &2", _clear_path, strerror(_clear_error));
		GB.FreeString(&_clear_path);
	}

	set_cache(_cache_enabled);

END_METHOD
#endif

//-------------------------------------------------------------------------

BEGIN_METHOD(WebSettings_get, GB_INTEGER flag)

	QWebEngineSettings *settings = get_settings(_object);
	int flag = VARG(flag);
	
	if (flag >= 0)
		GB.ReturnBoolean(settings->testAttribute((QWebEngineSettings::WebAttribute)flag));
	else
		GB.ReturnBoolean(FALSE);

END_METHOD

BEGIN_METHOD(WebSettings_put, GB_BOOLEAN value; GB_INTEGER flag)

	QWebEngineSettings *settings = get_settings(_object);
	int flag = VARG(flag);

	if (flag >= 0)
		settings->setAttribute((QWebEngineSettings::WebAttribute)flag, VARG(value));

END_METHOD

/*BEGIN_METHOD(WebSettings_Reset, GB_INTEGER flag)

	QWebEngineSettings *settings = get_settings(_object);
	int flag = VARG(flag);

	if (flag >= 0)
		settings->resetAttribute((QWebEngineSettings::WebAttribute)flag);

END_METHOD*/

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
  
	GB_CONSTANT("AutoLoadImages", "i", QWebEngineSettings::AutoLoadImages),
	GB_CONSTANT("Javascript", "i", QWebEngineSettings::JavascriptEnabled),
	GB_CONSTANT("JavascriptCanOpenWindows", "i", QWebEngineSettings::JavascriptCanOpenWindows),
	GB_CONSTANT("JavascriptCanAccessClipboard", "i", QWebEngineSettings::JavascriptCanAccessClipboard),
	//GB_CONSTANT("LinksIncludedInFocusChain", "i", QWebEngineSettings::LinksIncludedInFocusChain),
	GB_CONSTANT("LocalStorage", "i", QWebEngineSettings::LocalStorageEnabled),
	//GB_CONSTANT("LocalContentCanAccessRemoteUrls", "i", QWebEngineSettings::LocalContentCanAccessRemoteUrls),
	GB_CONSTANT("SpatialNavigation", "i", QWebEngineSettings::SpatialNavigationEnabled),
	GB_CONSTANT("LocalContentCanAccessFileUrls", "i", QWebEngineSettings::LocalContentCanAccessFileUrls),
	GB_CONSTANT("HyperlinkAuditing", "i", QWebEngineSettings::HyperlinkAuditingEnabled),
	//GB_CONSTANT("ScrollAnimatorEnabled", "i", QWebEngineSettings::ScrollAnimatorEnabled),
	//GB_CONSTANT("ErrorPageEnabled", "i", QWebEngineSettings::ErrorPageEnabled),
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	GB_CONSTANT("Plugins", "i", QWebEngineSettings::PluginsEnabled),
	GB_CONSTANT("FullScreenSupport", "i", QWebEngineSettings::FullScreenSupportEnabled),
#else
	GB_CONSTANT("Plugins", "i", -1),
	GB_CONSTANT("FullScreenSupport", "i", -1),
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
	//GB_CONSTANT("ScreenCaptureEnabled", "i", QWebEngineSettings::ScreenCaptureEnabled),
	GB_CONSTANT("WebGL", "i", QWebEngineSettings::WebGLEnabled),
	GB_CONSTANT("Accelerated2dCanvas", "i", QWebEngineSettings::Accelerated2dCanvasEnabled),
	//GB_CONSTANT("AutoLoadIconsForPage", "i", QWebEngineSettings::AutoLoadIconsForPage),
	//GB_CONSTANT("TouchIconsEnabled", "i", QWebEngineSettings::TouchIconsEnabled),
#else
	//GB_CONSTANT("ScreenCaptureEnabled", "i", -1),
	GB_CONSTANT("WebGL", "i", -1),
	GB_CONSTANT("Accelerated2dCanvas", "i", -1),
	//GB_CONSTANT("AutoLoadIconsForPage", "i", -1),
	//GB_CONSTANT("TouchIconsEnabled", "i", -1),
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
	//GB_CONSTANT("FocusOnNavigationEnabled", "i", QWebEngineSettings::FocusOnNavigationEnabled),
	GB_CONSTANT("PrintElementBackgrounds", "i", QWebEngineSettings::PrintElementBackgrounds),
	//GB_CONSTANT("AllowRunningInsecureContent", "i", QWebEngineSettings::AllowRunningInsecureContent),
#else
	//GB_CONSTANT("FocusOnNavigationEnabled", "i", -1),
	GB_CONSTANT("PrintElementBackgrounds", "i", -1),
	//GB_CONSTANT("AllowRunningInsecureContent", "i", -1),
#endif
/*#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	GB_CONSTANT("AllowGeolocationOnInsecureOrigins", "i", QWebEngineSettings::AllowGeolocationOnInsecureOrigins),
#else
	GB_CONSTANT("AllowGeolocationOnInsecureOrigins", "i", -1),
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	GB_CONSTANT("AllowWindowActivationFromJavaScript", "i", QWebEngineSettings::AllowWindowActivationFromJavaScript),
	GB_CONSTANT("ShowScrollBars", "i", QWebEngineSettings::ShowScrollBars),
#else
	GB_CONSTANT("AllowWindowActivationFromJavaScript", "i", -1),
	GB_CONSTANT("ShowScrollBars", "i", -1),
#endif*/
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	GB_CONSTANT("PlaybackRequiresUserGesture", "i", QWebEngineSettings::PlaybackRequiresUserGesture),
	//GB_CONSTANT("JavascriptCanPaste", "i", QWebEngineSettings::JavascriptCanPaste),
	//GB_CONSTANT("WebRTCPublicInterfacesOnly", "i", QWebEngineSettings::WebRTCPublicInterfacesOnly),
#else
	GB_CONSTANT("PlaybackRequiresUserGesture", "i", -1),
	//GB_CONSTANT("JavascriptCanPaste", "i", -1),
	//GB_CONSTANT("WebRTCPublicInterfacesOnly", "i", -1),
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
	GB_CONSTANT("DnsPrefetch", "i", QWebEngineSettings::DnsPrefetchEnabled),
#else
	GB_CONSTANT("DnsPrefetch", "i", -1),
#endif
/*#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
	GB_CONSTANT("PdfViewerEnabled", "i", QWebEngineSettings::PdfViewerEnabled),
#else
	GB_CONSTANT("PdfViewerEnabled", "i", -1),
#endif*/

	/*GB_CONSTANT("DefaultProxy", "i", QNetworkProxy::DefaultProxy),
	GB_CONSTANT("NoProxy", "i", QNetworkProxy::NoProxy),
	GB_CONSTANT("Socks5Proxy", "i", QNetworkProxy::Socks5Proxy),
	GB_CONSTANT("HttpProxy", "i", QNetworkProxy::HttpProxy),*/
	
	GB_PROPERTY_SELF("Fonts", ".WebSettings.Fonts"),
	/*GB_STATIC_PROPERTY_SELF("IconDatabase", ".WebSettings.IconDatabase"),
	GB_STATIC_PROPERTY_SELF("Cache", ".WebSettings.Cache"),
	GB_STATIC_PROPERTY_SELF("Proxy", ".WebSettings.Proxy"),*/
	
	GB_METHOD("_get", "b", WebSettings_get, "(Flag)i"),
	GB_METHOD("_put", NULL, WebSettings_put, "(Value)b(Flag)i"),
	
	//GB_STATIC_METHOD("_exit", NULL, WebSettings_exit, NULL),

GB_END_DECLARE
};


