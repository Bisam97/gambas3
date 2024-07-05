/***************************************************************************

  CCurl.c

  (c) 2003-2008 Daniel Campos Fernández <dcamposf@gmail.com>
  (c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __CCURL_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>

#include "main.h"
#include "gambas.h"
#include "CCurl.h"
#include "CProxy.h"

DECLARE_EVENT(EVENT_Finished);
DECLARE_EVENT(EVENT_Error);
DECLARE_EVENT(EVENT_Connect);
DECLARE_EVENT(EVENT_Read);
DECLARE_EVENT(EVENT_Progress);
DECLARE_EVENT(EVENT_Cancel);

static CCURL *_async_list = NULL;

//-------------------------------------------------------------------------

static void add_to_async_list(CCURL *_object)
{
	if (THIS->in_list)
		return;
	
	#ifdef DEBUG
	fprintf(stderr, "add_to_async_list: %p\n", THIS);
	#endif

	GB.List.Add(&_async_list, THIS, &THIS->list);
	THIS->in_list = TRUE;
	GB.Ref(THIS);
}

static void remove_from_async_list(CCURL *_object)
{
	if (!THIS->in_list)
		return;

	#ifdef DEBUG
	fprintf(stderr, "remove_from_async_list: %p\n", THIS);
	#endif

	GB.List.Remove(&_async_list, THIS, &THIS->list);
	THIS->in_list = FALSE;
	GB.Unref(POINTER(&_object));
}

//-------------------------------------------------------------------------

/*****************************************************
CURLM : a pointer to use curl_multi interface,
allowing asynchrnous work without using threads
in this class.
******************************************************/
CURLM *CCURL_multicurl;
int CCURL_pipe[2] = {-1, -1};

/******************************************************
Events from this class
******************************************************/
GB_STREAM_DESC CurlStream =
{
	.open = CCURL_stream_open,
	.close = CCURL_stream_close,
	.read = CCURL_stream_read,
	.write = CCURL_stream_write,
	.seek = CCURL_stream_seek,
	.tell = CCURL_stream_tell,
	.flush = CCURL_stream_flush,
	.eof = CCURL_stream_eof,
	.lof = CCURL_stream_lof,
	.handle = CCURL_stream_handle,
};

////////////////////////////////////////////////////////////////////
//	STREAM							  //
////////////////////////////////////////////////////////////////////

/* not allowed stream methods */

int CCURL_stream_handle(GB_STREAM *stream) { return -1;}
int CCURL_stream_open(GB_STREAM *stream, const char *path, int mode, void *data){return -1;}
int CCURL_stream_seek(GB_STREAM *stream, int64_t pos, int whence){	return -1;}
int CCURL_stream_tell(GB_STREAM *stream, int64_t *pos){return -1; }
int CCURL_stream_flush(GB_STREAM *stream) {	return 0;}
int CCURL_stream_close(GB_STREAM *stream) { return 0;}
int CCURL_stream_write(GB_STREAM *stream, char *buffer, int len){return -1;}

int CCURL_stream_lof(GB_STREAM *stream, int64_t *len)
{
	void *_object = STREAM_TO_OBJECT(stream);
	
	*len = 0;

	if ((THIS_STATUS != NET_RECEIVING_DATA ) && (THIS_STATUS != NET_INACTIVE)) 
		return -1;
		
	*len = GB.StringLength(THIS->data);
	return 0;
}

int CCURL_stream_eof(GB_STREAM *stream)
{
	void *_object = STREAM_TO_OBJECT(stream);
	
	if ((THIS_STATUS != NET_RECEIVING_DATA ) && (THIS_STATUS != NET_INACTIVE)) return -1;
	if (!GB.StringLength(THIS->data)) return -1;
	return 0;
}

int CCURL_stream_read(GB_STREAM *stream, char *buffer, int len)
{
	void *_object = STREAM_TO_OBJECT(stream);
	int len_data;
	char *new_data;
	
	if ((THIS_STATUS != NET_RECEIVING_DATA ) && (THIS_STATUS != NET_INACTIVE)) return -1;
	
	len_data = GB.StringLength(THIS->data);
	
	if (len_data < len)
		len = len_data;

	memcpy(buffer, THIS->data, len);

	len_data -= len;
	
	if (len_data > 0)
		new_data = GB.NewString(THIS->data + len, len_data);
	else
		new_data = NULL;
	
	GB.FreeString(&THIS->data);
	THIS->data = new_data;

	return len;
}

//-------------------------------------------------------------------------

static void raise_event(void *_object, int event)
{
	GB.Raise(THIS, event, 0);
	GB.Unref(POINTER(&_object));
}

void CURL_raise_finished(void *_object)
{
	raise_event(THIS, EVENT_Finished);
}

void CURL_raise_error(void *_object)
{
	raise_event(THIS, EVENT_Error);
}

void CURL_raise_cancel(void *_object)
{
	raise_event(THIS, EVENT_Cancel);
}

void CURL_raise_connect(void *_object)
{
	raise_event(THIS, EVENT_Connect);
}

void CURL_raise_read(void *_object)
{
	if (GB.CanRaise(THIS, EVENT_Read))
	{
		GB.Raise(THIS, EVENT_Read, 0);
		
		if (!GB.Stream.Eof(&THIS->stream))
		{
			GB.Ref(THIS);
			GB.Post(CURL_raise_read, (intptr_t)THIS);
		}
	}
	
	GB.Unref(POINTER(&_object));
}

//-------------------------------------------------------------------------

bool CURL_manage_option(int err, const char *option)
{
	if (err != CURLE_OK)
	{
		GB.Error("Unable to set option '&1': &2", option, curl_easy_strerror(err));
		return TRUE;
	}
	else
		return FALSE;
}

void CURL_manage_error(void *_object, int error)
{
	if (THIS_FILE)
	{
		fclose(THIS_FILE);
		THIS_FILE = NULL;
	}
	
	if (THIS->async)
	{
		#if DEBUG
		fprintf(stderr, "-- [%p] curl_multi_remove_handle(%p)\n", THIS, THIS_CURL);
		#endif
		curl_multi_remove_handle(CCURL_multicurl,THIS_CURL);
	}

	GB.Ref(THIS);

	if (error == CURLE_OK)
		GB.Post(CURL_raise_finished, (intptr_t)THIS);
	else
		GB.Post(CURL_raise_error, (intptr_t)THIS);

	CURL_stop(THIS);

	if (error == CURLE_OK)
		THIS_STATUS = NET_INACTIVE;
	else
	{
		THIS_STATUS = (- (1000 + error));
		GB.Error("&1", curl_easy_strerror(error));
	}
}

void CURL_init_stream(void *_object)
{
	THIS->stream.desc = &CurlStream;
	THIS->stream.tag = THIS;
	GB.Stream.SetAvailableNow(&THIS->stream, TRUE);
}

bool CURL_init_options(void *_object)
{
	if (CURL_set_option(THIS_CURL, CURLOPT_NOSIGNAL, 1))
		return TRUE;
	
	if (CURL_set_option(THIS_CURL, CURLOPT_TIMEOUT, THIS->timeout))
		return TRUE;
	
	if (CURL_set_option(THIS_CURL, CURLOPT_VERBOSE, (bool)THIS->debug))
		return TRUE;
	
	if (CURL_set_option(THIS_CURL, CURLOPT_PRIVATE, (char*)_object))
		return TRUE;
	
	if (THIS->buffer_size)
	{
		if (CURL_set_option(THIS_CURL, CURLOPT_BUFFERSIZE, THIS->buffer_size))
			return TRUE;
	}
	
	if (CURL_set_option(THIS_CURL, CURLOPT_SSL_VERIFYPEER, THIS->ssl_verify_peer ? 1 : 0))
		return TRUE;
	
	if (CURL_set_option(THIS_CURL, CURLOPT_SSL_VERIFYHOST , THIS->ssl_verify_host ? 2 : 0))
		return TRUE;
	
	if (THIS->ssl_ca_path && CURL_set_option(THIS_CURL, CURLOPT_CAPATH, THIS->ssl_ca_path))
		return TRUE;

	if (THIS->ssl_ca_info && CURL_set_option(THIS_CURL, CURLOPT_CAINFO, THIS->ssl_ca_info))
		return TRUE;

	if (CURL_proxy_set(&THIS->proxy, THIS_CURL))
		return TRUE;
	
	if (CURL_user_set(&THIS->user, THIS_CURL))
		return TRUE;
	
	if (CURL_set_option(THIS_CURL, CURLOPT_URL, THIS_URL))
		return TRUE;
	
	return FALSE;
}

#define CHECK_PROGRESS_VAL(_var) if (THIS->_var != (int64_t)_var) { THIS->_var = (int64_t)_var; raise = TRUE; }

static int curl_progress(void *_object, progress_size_t dltotal, progress_size_t dlnow, progress_size_t ultotal, progress_size_t ulnow)
{
	bool raise = FALSE;
	
	if (THIS->progresscb)
		(*THIS->progresscb)(THIS, &dltotal, &dlnow, &ultotal, &ulnow);

	CHECK_PROGRESS_VAL(dltotal);
	CHECK_PROGRESS_VAL(dlnow);
	CHECK_PROGRESS_VAL(ultotal);
	CHECK_PROGRESS_VAL(ulnow);
	
	if (raise)
		GB.RaiseLater(THIS, EVENT_Progress);
	
	return 0;
}


/***************************************************************
This CallBack is called each event loop by Gambas to test
the status of curl descriptors
***************************************************************/

static void stop_post()
{
	if (CCURL_pipe[0] < 0) return;
	
	GB.Watch (CCURL_pipe[0], GB_WATCH_NONE, NULL, 0);
	close(CCURL_pipe[0]);
	close(CCURL_pipe[1]);
	CCURL_pipe[0]=-1;
}

static void CCURL_post_curl(intptr_t data)
{
	CURLMsg *Msg;
	int nread;
	int post=1;
	void *_object;
	char *tmp;

	do
	{
		usleep(1000);
	}
	while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(CCURL_multicurl,&nread));
	
	if (!nread) post=0;

	do
	{
		Msg=curl_multi_info_read(CCURL_multicurl,&nread);
		if (!Msg) nread=0;
		if (Msg)
		{
			curl_easy_getinfo(Msg->easy_handle,CURLINFO_PRIVATE,&tmp);
			_object=(void*)tmp;
			CURL_manage_error(THIS,Msg->data.result);
		}
	} 
	while (nread);

	if (!post)
		stop_post();
}

void CURL_stop(void *_object)
{
	if (THIS_STATUS == NET_INACTIVE)
		return;
	
	if (THIS_FILE)
	{
		fclose(THIS_FILE);
		THIS_FILE = NULL;
	}
	
	THIS_STATUS = NET_INACTIVE;
	
	remove_from_async_list(THIS);
}

void CURL_clean(void *_object)
{
	if (THIS_CURL)
	{
		#if DEBUG
		fprintf(stderr, "-- CURL_clean: [%p] curl_multi_remove_handle(%p)\n", THIS, THIS_CURL);
		#endif
		curl_multi_remove_handle(CCURL_multicurl,THIS_CURL);
		#if DEBUG
		fprintf(stderr, "-- CURL_clean: [%p] curl_easy_cleanup(%p)\n", THIS, THIS_CURL);
		#endif
		curl_easy_cleanup(THIS_CURL);
		THIS_CURL = NULL;
	}
}

static void init_post(void)
{
	if (CCURL_pipe[0]!=-1) return;
	
	if (pipe(CCURL_pipe))
	{
		fprintf(stderr, "gb.net.curl: warning: unable to create the client watching pipe: %s\n", strerror(errno));
		return;
	}
	
	GB.Watch (CCURL_pipe[0], GB_WATCH_READ, CCURL_post_curl, 0);
	if (write(CCURL_pipe[1], "1", sizeof(char)) != 1)
		fprintf(stderr, "gb.net.curl: warning: unable to write to the client watching pipe: %s\n", strerror(errno));
}

void CURL_start_post(void *_object)
{
	init_post();
	curl_multi_add_handle(CCURL_multicurl, THIS_CURL);
	add_to_async_list(THIS);
}

bool CURL_check_active(void *_object)
{
	if (THIS_STATUS > 0)
	{
		GB.Error("Property is read-only while client is active");
		return TRUE;
	}
	else
		return FALSE;
}

bool CURL_set_progress(void *_object, bool progress, CURL_FIX_PROGRESS_CB cb)
{
	#ifdef DEBUG
	fprintf(stderr, "CURL_set_progress: %p %d\n", _object, progress);
	#endif
	
	if (CURL_set_option(THIS_CURL, CURLOPT_NOPROGRESS, progress ? 0 : 1))
		return TRUE;

	if (progress)
	{
		#if LIBCURL_VERSION_NUM < 0x073200

		if (CURL_set_option(THIS_CURL, CURLOPT_PROGRESSFUNCTION , curl_progress)
				|| CURL_set_option(THIS_CURL, CURLOPT_PROGRESSDATA , _object))
			return TRUE;

		#else

		if (CURL_set_option(THIS_CURL, CURLOPT_XFERINFOFUNCTION , curl_progress)
				|| CURL_set_option(THIS_CURL, CURLOPT_XFERINFODATA , _object))
			return TRUE;

		#endif
	}

	THIS->progresscb = cb;
	return FALSE;
}

#define COPY_STRING(_field) \
{ \
	GB.FreeString(&dest->_field); \
	dest->_field = src->_field; \
	if (dest->_field) dest->_field = GB.NewString(dest->_field, GB.StringLength(dest->_field)); \
}

bool CURL_copy_from(CCURL *dest, CCURL *src)
{
	if (CURL_check_active(dest))
		return TRUE;

	dest->async = src->async;
	dest->timeout = src->timeout;
	dest->debug = src->debug;
	dest->buffer_size = src->buffer_size;
	COPY_STRING(url);

	dest->user.auth = src->user.auth;
	COPY_STRING(user.user);
	COPY_STRING(user.userpwd);
	COPY_STRING(user.pwd);

	dest->proxy.type = src->proxy.type;
	dest->proxy.auth = src->proxy.auth;
	COPY_STRING(proxy.host);
	COPY_STRING(proxy.user);
	COPY_STRING(proxy.pwd);
	COPY_STRING(proxy.userpwd);

	return FALSE;
}

static int curl_write_cb(void *buffer, size_t size, size_t nmemb, void *_object)
{
	THIS_STATUS = NET_RECEIVING_DATA;
	nmemb *= size;

	if (THIS_FILE)
	{
		return fwrite(buffer,size,nmemb,THIS_FILE);
	}
	else
	{
		THIS->data = GB.AddString(THIS->data, buffer, nmemb);
	}

	if (THIS->async)
	{
		GB.Ref(THIS);
		GB.Post(CURL_raise_read, (intptr_t)THIS);
	}

	return nmemb;
}


void CURL_reset(void *_object)
{
	GB.FreeString(&THIS->data);
}


static bool CURL_init_handle(void *_object)
{
	if (THIS_CURL)
	{
		if (CURL_check_userpwd(&THIS->user))
		{
			CURL_stop(_object);
			CURL_clean(_object);
			CURL_reset(_object);
			THIS_CURL = curl_easy_init();
		}
	}
	else
	{
		THIS_CURL = curl_easy_init();
	}

	if (CURL_init_options(THIS))
		return TRUE;

	if (CURL_set_option(THIS_CURL, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_write_cb))
		return TRUE;
	
	if (CURL_set_option(THIS_CURL, CURLOPT_WRITEDATA, THIS))
		return TRUE;

	CURL_reset(THIS);
	THIS_STATUS = NET_CONNECTING;

	CURL_init_stream(THIS);
	return FALSE;
}


static void CURL_get(void *_object, char *target)
{
	if (!target)
		target = THIS->target;

	if (target && *target)
	{
		target = GB.FileName(target, 0);
		THIS_FILE = fopen(target, "w");
		if (!THIS_FILE)
		{
			GB.Error("Unable to open file for writing: &1", target);
			return;
		}
	}

	if (CURL_init_handle(_object))
		return;
	
	CURL_set_progress(THIS, TRUE, NULL);

	if (THIS->async)
	{
		CURL_start_post(THIS);
		return;
	}

	CURL_manage_error(THIS, curl_easy_perform(THIS_CURL));
}

//-------------------------------------------------------------------------

BEGIN_PROPERTY(Curl_User)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->user.user);
	else
	{
		if (CURL_check_active(THIS))
			return;

		GB.StoreString(PROP(GB_STRING), &(THIS->user.user));
	}

END_PROPERTY


BEGIN_PROPERTY(Curl_Async)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->async);
	else
	{
		if (CURL_check_active(THIS))
			return;
	
		THIS->async = VPROP(GB_BOOLEAN);
	}

END_PROPERTY


BEGIN_PROPERTY(Curl_Timeout)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->timeout);
	else
	{
		int timeout;
		
		if (CURL_check_active(THIS))
			return;
	
		timeout = VPROP(GB_INTEGER);
		if (timeout < 0)
			timeout = 0;
		
		THIS->timeout = timeout;
	}

END_PROPERTY


BEGIN_PROPERTY(Curl_BufferSize)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->buffer_size);
	else
	{
		int buffer_size;
		
		if (CURL_check_active(THIS))
			return;
	
		buffer_size = VPROP(GB_INTEGER);
		if (buffer_size <= 0)
			buffer_size = 0;
		else if (buffer_size < 1024)
			buffer_size = 1024;
		else if (buffer_size > CURL_MAX_READ_SIZE)
			buffer_size = CURL_MAX_READ_SIZE;
		
		THIS->buffer_size = buffer_size;
	}

END_PROPERTY


BEGIN_PROPERTY(Curl_Password)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->user.pwd);
	else
	{
		if (CURL_check_active(THIS))
			return;
	
		GB.StoreString(PROP(GB_STRING), &(THIS->user.pwd));
	}

END_PROPERTY


BEGIN_PROPERTY(Curl_Status)

	GB.ReturnInteger(THIS_STATUS);

END_PROPERTY


BEGIN_PROPERTY(Curl_ErrorText)

	if (THIS_STATUS >= 0)
		GB.ReturnVoidString();
	else
		GB.ReturnConstZeroString(curl_easy_strerror((-THIS_STATUS) - 1000));

END_PROPERTY


BEGIN_PROPERTY(Curl_URL)

	if (READ_PROPERTY)
	{
		GB.ReturnString(THIS_URL);
		return;
	}
	
	if (CURL_check_active(THIS))
		return;

	CURL_set_url(THIS, PSTRING(), PLENGTH());
	
END_PROPERTY

BEGIN_METHOD_VOID(Curl_new)

	#if DEBUG
	fprintf(stderr, "Curl_new: %p\n", THIS);
	#endif

	CURL_user_init(&THIS->user);
	CURL_proxy_init(&THIS->proxy);

	THIS->ssl_verify_peer = TRUE;
	THIS->ssl_verify_host = TRUE;

END_METHOD

BEGIN_METHOD_VOID(Curl_free)
	
	#if DEBUG
	fprintf(stderr, "Curl_free: %p\n", THIS);
	#endif
	
	CURL_stop(THIS);
	CURL_clean(THIS);
	CURL_reset(THIS);
	
	GB.FreeString(&THIS_URL);
	GB.FreeString(&THIS->target);
	
	CURL_user_clear(&THIS->user);
	CURL_proxy_clear(&THIS->proxy);
	
	GB.FreeString(&THIS->ssl_ca_path);
	GB.FreeString(&THIS->ssl_ca_info);
	
END_METHOD

BEGIN_METHOD_VOID(Curl_init)

	#if DEBUG
	fprintf(stderr, "-- curl_multi_init()\n");
	#endif
	CCURL_multicurl = curl_multi_init();

END_METHOD

BEGIN_METHOD_VOID(Curl_exit)

	CCURL *curl, *next;

	#if DEBUG
	fprintf(stderr, "-- CURL_exit: clear async list\n");
	#endif
	
	curl = _async_list;
	while (curl)
	{
		next = curl->list.next;
		remove_from_async_list(curl);
		curl = next;
	}
	
	#if DEBUG
	fprintf(stderr, "-- curl_multi_cleanup()\n");
	#endif
	
	curl_multi_cleanup(CCURL_multicurl);
	
	CURL_default_proxy_clear();

END_METHOD

BEGIN_METHOD_VOID(Curl_Peek)

	GB.ReturnString(THIS->data);

END_METHOD

BEGIN_PROPERTY(Curl_Debug)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->debug);
	else
		THIS->debug = VPROP(GB_BOOLEAN);

END_PROPERTY

BEGIN_PROPERTY(Curl_Downloaded)

	GB.ReturnLong(THIS->dlnow);

END_PROPERTY

BEGIN_PROPERTY(Curl_Uploaded)

	GB.ReturnLong(THIS->ulnow);

END_PROPERTY

BEGIN_PROPERTY(Curl_TotalDownloaded)

	GB.ReturnLong(THIS->dltotal);

END_PROPERTY

BEGIN_PROPERTY(Curl_TotalUploaded)

	GB.ReturnLong(THIS->ultotal);

END_PROPERTY

BEGIN_PROPERTY(Curl_TargetFile)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->target);
	else
		GB.StoreString(PROP(GB_STRING), &THIS->target);

END_PROPERTY


BEGIN_METHOD_VOID(Curl_Stop)

	CURL_stop(THIS);
	GB.Ref(THIS);
	CURL_raise_cancel(THIS);

END_METHOD

BEGIN_METHOD(Curl_Get, GB_STRING target)

	char *target = NULL;

	if (MISSING(target))
		target = THIS->target;
	else
		target = GB.FileName(STRING(target), LENGTH(target));

	if (target && *target)
	{
		THIS_FILE = fopen(target, "w");

		if (!THIS_FILE)
		{
			GB.Error("Unable to open file for writing");
			return;
		}
	}

	CURL_get(THIS, target);

END_METHOD

//---------------------------------------------------------------------------

BEGIN_PROPERTY(Curl_SSL_VerifyPeer)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->ssl_verify_peer);
	else
		THIS->ssl_verify_peer = VPROP(GB_BOOLEAN);

END_PROPERTY

BEGIN_PROPERTY(Curl_SSL_VerifyHost)

	if (READ_PROPERTY)
		GB.ReturnBoolean(THIS->ssl_verify_host);
	else
		THIS->ssl_verify_host = VPROP(GB_BOOLEAN);

END_PROPERTY

BEGIN_PROPERTY(Curl_SSL_CAPath)
	
	if (READ_PROPERTY)
		GB.ReturnString(THIS->ssl_ca_path);
	else
		GB.StoreString(PROP(GB_STRING), &THIS->ssl_ca_path);

END_PROPERTY

BEGIN_PROPERTY(Curl_SSL_CAInfo)
	
	if (READ_PROPERTY)
		GB.ReturnString(THIS->ssl_ca_info);
	else
		GB.StoreString(PROP(GB_STRING), &THIS->ssl_ca_info);

END_PROPERTY

//---------------------------------------------------------------------------

GB_DESC CurlSSLDesc[] =
{
	GB_DECLARE_VIRTUAL(".Curl.SSL"),

	GB_PROPERTY("VerifyPeer", "b", Curl_SSL_VerifyPeer),
	GB_PROPERTY("VerifyHost", "b", Curl_SSL_VerifyHost),
	GB_PROPERTY("CAPath", "s", Curl_SSL_CAPath),
	GB_PROPERTY("CAInfo", "s", Curl_SSL_CAInfo),

	GB_END_DECLARE
};

GB_DESC CurlDesc[] =
{
	GB_DECLARE("Curl", sizeof(CCURL)), GB_NOT_CREATABLE(),

	GB_INHERITS("Stream"),

	GB_STATIC_METHOD("_init", NULL, Curl_init, NULL),
	GB_STATIC_METHOD("_exit", NULL, Curl_exit, NULL),

	GB_METHOD("_new", NULL, Curl_new, NULL),
	GB_METHOD("_free", NULL, Curl_free, NULL),
	
	GB_METHOD("Peek","s", Curl_Peek, NULL),
	
	GB_PROPERTY("URL", "s", Curl_URL),
	GB_PROPERTY("User", "s", Curl_User),
	GB_PROPERTY("Password", "s", Curl_Password),  
	GB_PROPERTY("Async", "b", Curl_Async),
	GB_PROPERTY("Timeout", "i", Curl_Timeout),
	GB_STATIC_PROPERTY_SELF("DefaultProxy", ".Curl.Proxy"),
	GB_PROPERTY_SELF("Proxy", ".Curl.Proxy"),
	GB_PROPERTY_SELF("SSL", ".Curl.SSL"),
	GB_PROPERTY_READ("Status", "i", Curl_Status),
	GB_PROPERTY_READ("ErrorText", "s", Curl_ErrorText),
	GB_PROPERTY("Debug", "b", Curl_Debug),
	GB_PROPERTY("BufferSize", "i", Curl_BufferSize),
	GB_PROPERTY("TargetFile", "s", Curl_TargetFile),

  GB_METHOD("Stop", NULL, Curl_Stop, NULL),
  GB_METHOD("Get", NULL, Curl_Get, "[(TargetFile)s]"),

	GB_PROPERTY_READ("Downloaded", "l", Curl_Downloaded),
	GB_PROPERTY_READ("Uploaded", "l", Curl_Uploaded),
	GB_PROPERTY_READ("TotalDownloaded", "l", Curl_TotalDownloaded),
	GB_PROPERTY_READ("TotalUploaded", "l", Curl_TotalUploaded),
	
	GB_EVENT("Finished", NULL, NULL, &EVENT_Finished),
	GB_EVENT("Connect", NULL, NULL, &EVENT_Connect),
	GB_EVENT("Read", NULL, NULL, &EVENT_Read),
	GB_EVENT("Error", NULL, NULL, &EVENT_Error),
	GB_EVENT("Progress", NULL, NULL, &EVENT_Progress),
	GB_EVENT("Cancel", NULL, NULL, &EVENT_Cancel),

	GB_END_DECLARE
};

