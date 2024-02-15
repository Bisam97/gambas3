/***************************************************************************

  c_webview.c

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

#define __C_WEBVIEW_C

#include "c_websettings.h"
#include "c_webview.h"

#define HISTORY (webkit_web_view_get_back_forward_list(WIDGET))

static bool _init = FALSE;

static WebKitUserStyleSheet *_style_sheet = NULL;

//---------------------------------------------------------------------------

static void set_link(void *_object, const char *link, int len)
{
	GB.FreeString(&THIS->link);
	if (len == 0 || !link)
		return;
	if (len < 0) len = strlen(link);
	THIS->link = GB.NewString(link, len);
}

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

static void cb_title(WebKitWebView *widget, GParamSpec *pspec, CWEBVIEW *_object)
{
	GB.Raise(THIS, EVENT_TITLE, 0);
}

static void cb_url(WebKitWebView *widget, GParamSpec *pspec, CWEBVIEW *_object)
{
	//fprintf(stderr, "cb_url: %s\n", webkit_web_view_get_uri(WIDGET));
	GB.Raise(THIS, EVENT_URL, 0);
	/*if (!THIS->got_load_event)
		GB.Raise(THIS, EVENT_FINISH, 0);*/
}

static void cb_icon(WebKitWebView *widget, GParamSpec *pspec, CWEBVIEW *_object)
{
	GB.Unref(POINTER(&THIS->icon));
	THIS->icon = NULL;
	GB.Raise(THIS, EVENT_ICON, 0);
}

static bool send_start(void *_object)
{
	if (THIS->start_sent)
		return FALSE;

	THIS->start_sent = TRUE;
	return GB.Raise(THIS, EVENT_START, 0);
}

static void send_finish(void *_object)
{
	if (THIS->finish_sent)
		return;

	THIS->finish_sent = TRUE;
	GB.Raise(THIS, EVENT_FINISH, 0);
	THIS->start_sent = FALSE;
}

static void cb_load_changed(WebKitWebView *widget, WebKitLoadEvent load_event, CWEBVIEW *_object)
{
	//fprintf(stderr, "cb_load_changed: %d\n", load_event);
	
	switch (load_event)
	{
		case WEBKIT_LOAD_STARTED:
			THIS->got_load_event = TRUE;
			THIS->finish_sent = FALSE;
			send_start(THIS);
			break;
			
		case WEBKIT_LOAD_FINISHED:
			send_finish(THIS);
			GB.FreeString(&THIS->link);
			break;
			
		default:
			break;
	}
}

static gboolean cb_load_failed(WebKitWebView *widget, WebKitLoadEvent load_event, gchar *failing_uri, GError *error, CWEBVIEW *_object)
{
	//fprintf(stderr, "cb_load_failed\n");
	if (!THIS->error)
	{
		THIS->error = TRUE;
		GB.Raise(THIS, EVENT_ERROR, 0);
	}
	return FALSE;
}

static void cb_progress(WebKitWebView *widget, GParamSpec *pspec, CWEBVIEW *_object)
{
	//fprintf(stderr, "cb_progress: %f\n", webkit_web_view_get_estimated_load_progress(WIDGET));
	if (!THIS->error)
	{
		send_start(THIS);
		GB.Raise(THIS, EVENT_PROGRESS, 0);
		if (webkit_web_view_get_estimated_load_progress(WIDGET) == 1.0)
			send_finish(THIS);
	}
}

static void cb_link(WebKitWebView *widget, WebKitHitTestResult *hit_test_result, guint modifiers, CWEBVIEW *_object)
{
	const char *uri = webkit_hit_test_result_get_link_uri(hit_test_result);
	set_link(THIS, uri, -1);
	GB.Raise(THIS, EVENT_LINK, 0);
}

static GtkWidget *cb_create(WebKitWebView *widget, WebKitNavigationAction *navigation_action, CWEBVIEW *_object)
{
	GtkWidget *new_view;
	
	if (GB.Raise(THIS, EVENT_NEW_VIEW, 0))
		return NULL;
	
	if (!THIS->new_view)
		return NULL;
	
	new_view = ((CWEBVIEW *)THIS->new_view)->widget;
	GB.Unref(POINTER(&THIS->new_view));
	THIS->new_view = NULL;
	return new_view;
}

static gboolean cb_decide_policy(WebKitWebView *widget, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, CWEBVIEW *_object)
{
	if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
	{
		/*const char *uri = webkit_uri_request_get_uri(
			webkit_navigation_action_get_request(
				webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision))));

		fprintf(stderr, "cb_decide_policy: %s\n", uri);*/
		
		if (THIS->accept_next)
		{
			THIS->accept_next = FALSE;
			return FALSE;
		}
		
		THIS->error = FALSE;
		THIS->got_load_event = FALSE;

		if (send_start(THIS))
		{
			//fprintf(stderr, "cancel !\n");
			webkit_policy_decision_ignore(decision);
			GB.RaiseLater(THIS, EVENT_ERROR);
		}
	}
	
	return FALSE;
}

static bool start_callback(void *_object)
{
	if (THIS->cb_running)
	{
		GB.Error("Pending asynchronous method");
		return TRUE;
	}

	THIS->cb_running = TRUE;
	GB.Ref(THIS);
	return FALSE;
}

static void run_callback(void *_object, const char *error)
{
	while(THIS->cb_running)
		GB.Wait(-1);

	if (THIS->cb_error)
	{
		GB.Error(error, THIS->cb_result);
		GB.FreeString(&THIS->cb_result);
	}
	else
	{
		GB.ReturnString(GB.FreeStringLater(THIS->cb_result));
		THIS->cb_result = NULL;
	}

	THIS->cb_error = FALSE;
	GB.Unref(POINTER(&_object));
}

#if WEBKIT_CHECK_VERSION(2, 40, 0)

static void cb_javascript_finished(WebKitWebView *widget, GAsyncResult *result, void *_object)
{
	JSCValue *value;
	GError *error = NULL;
	JSCException *exception;
	char *json;

	value = webkit_web_view_evaluate_javascript_finish(widget, result, &error);

	if (!value)
	{
		THIS->cb_result = GB.NewZeroString(error->message);
		g_error_free(error);
		THIS->cb_error = TRUE;
	}
	else
	{
		json = jsc_value_to_json(value, 0);

		exception = jsc_context_get_exception(jsc_value_get_context (value));
		if (exception)
		{
			THIS->cb_result = GB.NewZeroString(jsc_exception_get_message(exception));
			THIS->cb_error = TRUE;
		}
		else
		{
			THIS->cb_result = GB.NewZeroString(json);
		}

		g_free(json);
		g_object_unref(value);
	}

	THIS->cb_running = FALSE;
}

#else

static void cb_javascript_finished(WebKitWebView *widget, GAsyncResult *result, void *_object)
{
	WebKitJavascriptResult *js_result;
	JSCValue *value;
	GError *error = NULL;
	JSCException *exception;
	char *json;

	js_result = webkit_web_view_run_javascript_finish(widget, result, &error);

	if (!js_result)
	{
		THIS->cb_result = GB.NewZeroString(error->message);
		g_error_free(error);
		THIS->cb_error = TRUE;
	}
	else
	{
		value = webkit_javascript_result_get_js_value(js_result);
		json = jsc_value_to_json(value, 0);
		
		exception = jsc_context_get_exception(jsc_value_get_context (value));
		if (exception)
		{
			THIS->cb_result = GB.NewZeroString(jsc_exception_get_message(exception));
			THIS->cb_error = TRUE;
		}
		else
		{
			THIS->cb_result = GB.NewZeroString(json);
		}
		
		g_free(json);
		webkit_javascript_result_unref(js_result);
	}
	
	THIS->cb_running = FALSE;
}

#endif

static char *get_encoding(const char *mimetype)
{
	char *p = strstr(mimetype, ";charset=");

	if (!p)
		return NULL;

	p += 9;
	if (!strcasecmp(p, "utf-8") || !strcasecmp(p, "utf8"))
		return NULL;

	return p;
}

static void cb_html_finished(WebKitWebResource *resource, GAsyncResult *result, void *_object)
{
	GError *error = NULL;
	gsize length = 0;
	guchar *html;
	char *encoding;

	html = webkit_web_resource_get_data_finish(resource, result, &length, &error);

	if (!html)
	{
		THIS->cb_result = GB.NewZeroString(error->message);
		g_error_free(error);
		THIS->cb_error = TRUE;
		return;
	}

	encoding = get_encoding(webkit_uri_response_get_mime_type(webkit_web_resource_get_response(resource)));

	if (encoding)
	{
		if (GB.ConvString(&THIS->cb_result, (char *)html, length, encoding, "UTF-8"))
		{
			THIS->cb_result = GB.NewZeroString(GB.GetErrorMessage());
			THIS->cb_error = TRUE;
		}
	}
	else
	{
		THIS->cb_result = GB.NewString((char *)html, length);
	}

	g_free(html);
	THIS->cb_running = FALSE;
}

static gboolean cb_context_menu(WebKitWebView *web_view, WebKitContextMenu *context_menu, GdkEvent *event, WebKitHitTestResult *hit_test_result, void *_object)
{
	if (EVENT_MENU < 0)
		EVENT_MENU = GB.GetEvent(GB.GetClass(THIS), "Menu");
	
	return GB.CanRaise(THIS, EVENT_MENU);
}

static void update_language(void *_object)
{
	if (THIS->language && *THIS->language)
	{
		gchar **languages = g_strsplit(THIS->language, ",", -1);
		webkit_web_context_set_preferred_languages(THIS->context, (const gchar **)languages);
		g_strfreev(languages);
	}
	else
	{
		const gchar *languages[2] = { NULL, NULL };
		char *lang, *p;

		lang = g_strdup(GB.System.Language());
		p = index(lang, '_');
		if (p) *p = '-';
		languages[0] = lang;
		webkit_web_context_set_preferred_languages(THIS->context, languages);
		g_free(lang);
	}
}

//---------------------------------------------------------------------------

#define must_patch(_widget) (true)
#include "../gb.gtk.patch.h"

PATCH_DECLARE(WEBKIT_TYPE_WEB_VIEW)

static void create_widget(void *_object, void *parent)
{
	if (!_style_sheet)
	{
		_style_sheet = webkit_user_style_sheet_new (
			"::-webkit-scrollbar { width: 1rem; height: 1rem; background-color: Canvas;}\n"
			"::-webkit-scrollbar-thumb { background-color: ButtonFace; border: solid 0.25rem Canvas; border-radius: 2rem; opacity: 0.5;}",
			WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
			WEBKIT_USER_STYLE_LEVEL_USER,
			NULL,
			NULL);
	}

	THIS->context = webkit_web_context_new_ephemeral();

	THIS->manager = webkit_user_content_manager_new();

	webkit_user_content_manager_add_style_sheet(THIS->manager, _style_sheet);

	THIS->widget = GTK_WIDGET(g_object_new(WEBKIT_TYPE_WEB_VIEW,
		"is-ephemeral", webkit_web_context_is_ephemeral(THIS->context),
		"web-context", THIS->context,
		"user-content-manager", THIS->manager,
		NULL));

	GTK.CreateControl(THIS, parent, THIS->widget, CCF_HAS_INPUT_METHOD);
	
	PATCH_CLASS(THIS->widget, WEBKIT_TYPE_WEB_VIEW)
	
	if (!_init)
	{
		//webkit_settings_set_load_icons_ignoring_image_load_setting(WEBVIEW_default_settings, TRUE);
		//webkit_web_context_set_use_system_appearance_for_scrollbars(webkit_web_context_get_default(), TRUE);
		webkit_web_context_set_favicon_database_directory(webkit_web_context_get_default(), NULL);
		_init = TRUE;
	}
	
	g_signal_connect(G_OBJECT(WIDGET), "notify::title", G_CALLBACK(cb_title), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "notify::uri", G_CALLBACK(cb_url), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "notify::favicon", G_CALLBACK(cb_icon), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "notify::estimated-load-progress", G_CALLBACK(cb_progress), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "load-changed", G_CALLBACK(cb_load_changed), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "load-failed", G_CALLBACK(cb_load_failed), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "mouse-target-changed", G_CALLBACK(cb_link), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "create", G_CALLBACK(cb_create), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "decide-policy", G_CALLBACK(cb_decide_policy), (gpointer)THIS);
	g_signal_connect(G_OBJECT(WIDGET), "context-menu", G_CALLBACK(cb_context_menu), (gpointer)THIS);
	
	WEBVIEW_init_settings(THIS);
}

//---------------------------------------------------------------------------

BEGIN_METHOD(WebView_new, GB_OBJECT parent)

	create_widget(THIS, VARG(parent));
	update_language(THIS);

END_METHOD

BEGIN_METHOD_VOID(WebView_free)

	GB.FreeString(&THIS->link);
	GB.FreeString(&THIS->language);

	GB.Unref(POINTER(&THIS->icon));
	GB.Unref(POINTER(&THIS->new_view));

	g_object_unref(THIS->manager);
	g_object_unref(THIS->context);

END_METHOD

static void remove_file(const char *path)
{
	if (unlink(path))
		rmdir(path);
}

BEGIN_METHOD_VOID(WebView_exit)

	// Remove the hsts cache directory always created, whereas every widget and context are ephemeral!

	WebKitWebContext *context = webkit_web_context_new();
	gchar *dir;

	g_object_get(webkit_web_context_get_website_data_manager(context), "hsts-cache-directory", &dir, NULL);

	if (dir)
	{
		GB.BrowseDirectory(dir, NULL, remove_file);
		rmdir(dir);
	}

	g_object_unref(context);

	webkit_user_style_sheet_unref(_style_sheet);
	_style_sheet = NULL;

END_METHOD

BEGIN_PROPERTY(WebView_Url)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(webkit_web_view_get_uri(WIDGET));
	else
	{
		set_link(THIS, PSTRING(), PLENGTH());
		webkit_web_view_load_uri(WIDGET, THIS->link);
	}

END_PROPERTY

BEGIN_PROPERTY(WebView_Title)

	GB.ReturnNewZeroString(webkit_web_view_get_title(WIDGET));

END_PROPERTY

BEGIN_METHOD(WebView_SetHtml, GB_STRING html; GB_STRING root)

	THIS->accept_next = TRUE;
	webkit_web_view_load_html(WIDGET, GB.ToZeroString(ARG(html)), MISSING(root) ? NULL : GB.ToZeroString(ARG(root)));

END_METHOD

BEGIN_METHOD_VOID(WebView_Back)

	webkit_web_view_go_back(WIDGET);

END_METHOD

BEGIN_METHOD_VOID(WebView_Forward)

	webkit_web_view_go_forward(WIDGET);

END_METHOD

BEGIN_METHOD(WebView_Reload, GB_BOOLEAN bypass)

	if (VARGOPT(bypass, FALSE))
		webkit_web_view_reload_bypass_cache(WIDGET);
	else
		webkit_web_view_reload(WIDGET);

END_METHOD

BEGIN_METHOD_VOID(WebView_Stop)

	webkit_web_view_stop_loading(WIDGET);

END_METHOD

BEGIN_PROPERTY(WebView_Zoom)

	if (READ_PROPERTY)
		GB.ReturnFloat(webkit_web_view_get_zoom_level(WIDGET));
	else
		webkit_web_view_set_zoom_level(WIDGET, VPROP(GB_FLOAT));

END_PROPERTY

BEGIN_PROPERTY(WebView_Icon)

	cairo_surface_t *icon;
	
	if (!THIS->icon)
	{
		icon = webkit_web_view_get_favicon(WIDGET);
		
		if (icon)
		{
			int size = GTK.GetDesktopScale() * 2;
			cairo_surface_reference(icon);
			THIS->icon = GTK.CreatePicture(icon, size, size);
			GB.Ref(THIS->icon);
		}
	}

	GB.ReturnObject(THIS->icon);

END_PROPERTY

BEGIN_PROPERTY(WebView_Progress)

	GB.ReturnFloat(webkit_web_view_get_estimated_load_progress(WIDGET));

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

	THIS->accept_next = TRUE;
	webkit_web_view_load_html(WIDGET, "", NULL);

END_METHOD

BEGIN_METHOD(WebView_ExecJavascript, GB_STRING script)

	if (LENGTH(script) == 0)
		return;
	
	if (start_callback(THIS))
		return;

#if WEBKIT_CHECK_VERSION(2, 40, 0)
	webkit_web_view_evaluate_javascript(WIDGET, STRING(script), LENGTH(script), NULL, NULL, NULL, (GAsyncReadyCallback)cb_javascript_finished, (gpointer)THIS);
#else
	char *script = GB.ToZeroString(ARG(script));
	webkit_web_view_run_javascript(WIDGET, script, NULL, (GAsyncReadyCallback)cb_javascript_finished, (gpointer)THIS);
#endif
	
	run_callback(THIS, "Javascript error: &1");

END_METHOD

BEGIN_METHOD_VOID(WebView_GetHtml)

	if (start_callback(THIS))
		return;

	webkit_web_resource_get_data(webkit_web_view_get_main_resource(WIDGET), NULL, (GAsyncReadyCallback)cb_html_finished, (gpointer)THIS);

	run_callback(THIS, "Unable to retrieve HTML contents: &1");

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

//---------------------------------------------------------------------------

BEGIN_PROPERTY(WebViewHistoryItem_Title)

	GB.ReturnNewZeroString(webkit_back_forward_list_item_get_title(THIS->item));

END_PROPERTY

BEGIN_PROPERTY(WebViewHistoryItem_Url)

	GB.ReturnNewZeroString(webkit_back_forward_list_item_get_uri(THIS->item));

END_PROPERTY

BEGIN_METHOD_VOID(WebViewHistoryItem_GoTo)

	webkit_web_view_go_to_back_forward_list_item(WIDGET, THIS->item);

END_METHOD

BEGIN_METHOD_VOID(WebViewHistory_Clear)

	fprintf(stderr, "gb.gtk3.webview: warning: WebKitGTK does not know how to clear its history at the moment.\n");

END_METHOD

BEGIN_METHOD(WebViewHistory_get, GB_INTEGER index)

	WebKitBackForwardListItem *item = webkit_back_forward_list_get_nth_item(HISTORY, VARG(index));
	
	if (!item)
		GB.ReturnNull();
	else
	{
		THIS->item = item;
		RETURN_SELF();
	}

END_METHOD

BEGIN_PROPERTY(WebViewHistory_CanGoBack)

	GB.ReturnBoolean(webkit_web_view_can_go_back(WIDGET));

END_PROPERTY

BEGIN_PROPERTY(WebViewHistory_CanGoForward)

	GB.ReturnBoolean(webkit_web_view_can_go_forward(WIDGET));

END_PROPERTY

//-------------------------------------------------------------------------

static void cb_cookies_clear_finished(WebKitWebView *widget, GAsyncResult *result, void *_object)
{
}

BEGIN_METHOD_VOID(WebView_Cookies_Clear)

	WebKitWebsiteDataManager *manager = webkit_web_view_get_website_data_manager(WIDGET);

	webkit_website_data_manager_clear(manager, WEBKIT_WEBSITE_DATA_COOKIES, 0, NULL, (GAsyncReadyCallback)cb_cookies_clear_finished, (gpointer)THIS);

END_METHOD

//---------------------------------------------------------------------------

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
  GB_STATIC_METHOD("_exit", NULL, WebView_exit, NULL),
  
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
