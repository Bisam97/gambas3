/***************************************************************************

  canimation.cpp

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

#define __CANIMATION_CPP

#include "main.h"

#include "CImage.h"
#include "canimation.h"

DECLARE_EVENT(EVENT_CHANGE);

static QHash<QObject *, void *> dict;

static void free_movie(void *_object)
{
  if (!MOVIE)
    return;
  
	dict.remove(MOVIE);
	
  delete MOVIE;
  THIS->movie = NULL;

	THIS->buffer->close();
	delete THIS->buffer;
	
	THIS->data->clear();
  delete THIS->data;
  
  GB.ReleaseFile(THIS->addr, THIS->len);
}

static void *load_movie(char *path, int len_path)
{
	void *_object;
	char *addr;
	int len;

	if (GB.LoadFile(path, len_path, &addr, &len))
		return NULL;

	_object = GB.Create(GB.FindClass("Animation"), NULL, NULL);
	
	THIS->addr = addr;
	THIS->len = len;
	THIS->data = new QByteArray();    
	*THIS->data = QByteArray::fromRawData((const char *)THIS->addr, THIS->len);
	THIS->buffer = new QBuffer(THIS->data);
	THIS->buffer->open(QIODevice::ReadOnly);
	THIS->movie = new QMovie(THIS->buffer);
	dict.insert(MOVIE, THIS);
  
	QObject::connect(THIS->movie, SIGNAL(frameChanged(int)), &CAnimationManager::manager, SLOT(change()));
	
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

/*BEGIN_PROPERTY(CMOVIEBOX_path)

  if (READ_PROPERTY)
    GB.ReturnString(THIS->path);
  else
  {
    bool playing = false;

    if (THIS->movie)
      playing = THIS->movie->state() == QMovie::Running;
    else
      playing = FALSE;

    if (load_movie(THIS, PSTRING(), PLENGTH()))
      return;
          
    if (!playing && THIS->movie)
      THIS->movie->setPaused(true);
  }

END_PROPERTY*/


BEGIN_PROPERTY(Animation_Playing)

  GB.ReturnBoolean(MOVIE->state() == QMovie::Running);
	
END_PROPERTY

BEGIN_METHOD_VOID(Animation_Play)

	MOVIE->setPaused(false);

END_METHOD

BEGIN_METHOD_VOID(Animation_Pause)

	MOVIE->setPaused(true);

END_METHOD

BEGIN_METHOD_VOID(Animation_Stop)

  MOVIE->stop();

END_METHOD

BEGIN_PROPERTY(Animation_Image)

	QImage *image = new QImage;
	
	*image = MOVIE->currentImage();
	GB.ReturnObject(CIMAGE_create(image));

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

//-------------------------------------------------------------------------

CAnimationManager CAnimationManager::manager;

void CAnimationManager::change(void)
{
	void *_object = dict[sender()];
	GB.Raise(THIS, EVENT_CHANGE, 0);
}
