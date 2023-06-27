/***************************************************************************

  canimation.cpp

  (c) 2004-2006 - Daniel Campos Fernández <dcamposf@gmail.com>
  (c) 2018 Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __CANIMATION_CPP

#include "CImage.h"
#include "canimation.h"

DECLARE_EVENT(EVENT_CHANGE);

static void raise_change(void *_object)
{
	GB.Raise(THIS, EVENT_CHANGE, 0);
}

static gboolean advance_movie(void *_object)
{
	if (gdk_pixbuf_animation_iter_advance(THIS->iter, NULL))
		raise_change(THIS);
	return true;
}

static void pause_movie(void *_object)
{
	if (!THIS->animation || !THIS->playing)
		return;

	if (THIS->timeout)
	{
		g_source_remove(THIS->timeout);
		THIS->timeout = 0;
	}
	
	THIS->playing = FALSE;
}

static void stop_movie(void *_object)
{
	pause_movie(THIS);
	
	if (THIS->iter)
	{
		g_object_unref(THIS->iter);
		THIS->iter = NULL;
	}
}

static void play_movie(void *_object)
{
	GTimeVal now;
	int interval;
	
	if (!THIS->animation || THIS->playing)
		return;
	
	if (!THIS->iter)
	{
		g_get_current_time(&now);
		THIS->iter = gdk_pixbuf_animation_get_iter(THIS->animation, &now);
		raise_change(THIS);
	}
	
	if (!THIS->timeout)
	{
		interval = gdk_pixbuf_animation_iter_get_delay_time(THIS->iter);
		if (interval > 0)
		{
			THIS->timeout = g_timeout_add(interval, (GSourceFunc)advance_movie, THIS);
			THIS->playing = TRUE;
		}
	}
	
	/*buf = gdk_pixbuf_animation_iter_get_pixbuf(iter);
	gtk_image_set_from_pixbuf(GTK_IMAGE(widget),buf);*/
}

static void free_movie(void *_object)
{
	if (!THIS->animation)
		return;
	
	stop_movie(THIS);
	
	g_object_unref(G_OBJECT(THIS->animation));
	THIS->animation = NULL;
  
  GB.ReleaseFile(THIS->addr, THIS->len);
}

static void *load_movie(char *path, int len_path)
{
	void *_object;
	char *addr;
	int len;
	GdkPixbufLoader *loader;

	if (GB.LoadFile(path, len_path, &addr, &len))
		return NULL;

	loader = gdk_pixbuf_loader_new();
	if (!gdk_pixbuf_loader_write(loader, (guchar*)addr, (gsize)len, NULL))
	{
		g_object_unref(G_OBJECT(loader));
		GB.Error("Unable to load animation");
		return NULL;
	}
	
	gdk_pixbuf_loader_close(loader, NULL);
	
	_object = GB.Create(GB.FindClass("Animation"), NULL, NULL);
	
	THIS->addr = addr;
	THIS->len = len;

	THIS->animation = gdk_pixbuf_loader_get_animation(loader);
	g_object_ref(G_OBJECT(THIS->animation));
	
	g_object_unref(G_OBJECT(loader));
	
	return THIS;
}


/*BEGIN_METHOD_(Animation_new, GB_STRING path)

	if (load_movie(THIS, STRING(path), LENGTH(path)))
		return;

	QObject::connect(MOVIE, SIGNAL(frameChanged(int)), &CAnimationManager::manager, SLOT(change()));
	
END_METHOD*/

BEGIN_METHOD_VOID(Animation_free)

  free_movie(THIS);

END_METHOD

BEGIN_METHOD(Animation_Load, GB_STRING path)

	GB.ReturnObject(load_movie(STRING(path), LENGTH(path)));
	
END_METHOD

BEGIN_PROPERTY(Animation_Playing)

  GB.ReturnBoolean(THIS->playing);
	
END_PROPERTY

BEGIN_METHOD_VOID(Animation_Play)

	play_movie(THIS);

END_METHOD

BEGIN_METHOD_VOID(Animation_Pause)

	pause_movie(THIS);

END_METHOD

BEGIN_METHOD_VOID(Animation_Stop)

  stop_movie(THIS);

END_METHOD

BEGIN_PROPERTY(Animation_Image)

	GdkPixbuf *buf;
	
	if (!THIS->animation || !THIS->iter)
	{
		GB.ReturnNull();
		return;
	}
	
	buf = gdk_pixbuf_copy(gdk_pixbuf_animation_iter_get_pixbuf(THIS->iter));
	GB.ReturnObject(CIMAGE_create(new gPicture(buf)));

END_PROPERTY

//-------------------------------------------------------------------------

GB_DESC AnimationDesc[] =
{
  GB_DECLARE("Animation", sizeof(CANIMATION)), GB_NOT_CREATABLE(),

  //GB_METHOD("_new", NULL, Animation_new, "(Path)s"),
  GB_METHOD("_free", NULL, Animation_free, NULL),
  
  GB_STATIC_METHOD("Load", "Animation", Animation_Load, "(Path)s"),

  GB_PROPERTY_READ("Playing", "b", Animation_Playing),
  GB_PROPERTY_READ("Image", "Image", Animation_Image),
  
  GB_METHOD("Play", NULL, Animation_Play, NULL),
  GB_METHOD("Stop", NULL, Animation_Stop, NULL),
  GB_METHOD("Pause", NULL, Animation_Pause, NULL),

	GB_EVENT("Change", NULL, EVENT_CHANGE, NULL),

  GB_END_DECLARE
};
