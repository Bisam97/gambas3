/***************************************************************************

  main.c

  gb.media component

  (c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#define __MAIN_C

#include "main.h"
#include "c_media.h"
#include "c_mediaplayer.h"

bool MAIN_debug = FALSE;

GB_INTERFACE GB EXPORT;
IMAGE_INTERFACE IMAGE EXPORT;

GB_DESC *GB_CLASSES[] EXPORT =
{
	//MediaSignalArgumentsDesc,
	MediaTagListDesc,
	MediaMessageDesc,
	MediaLinkDesc,
	MediaTypeDesc,
	MediaControlDesc,
	MediaFilterDesc,
	MediaContainerChildrenDesc,
	MediaContainerDesc,
	MediaPipelineDesc,
	MediaDesc,
	MediaPlayerAudioDesc,
	MediaPlayerVideoDesc,
	MediaPlayerSubtitlesDesc,
	MediaPlayerBalanceChannelDesc,
	MediaPlayerBalanceDesc,
	MediaPlayerDesc,
  NULL
};

uintptr_t MAIN_get_window_handle(void *control)
{
	uintptr_t (*get_handle)(void *) = NULL;

	if (!get_handle)
	{
		GB.Component.GetInfo("GET_HANDLE", (void **)&get_handle);
		if (!get_handle)
		{
			GB.Error("Unable to get window handle");
			return 0;
		}
	}

	return (*get_handle)(control);
}

uintptr_t MAIN_get_display()
{
	uintptr_t display = 0;

	GB.Component.GetInfo("DISPLAY", (void **)&display);

	if (!display)
		GB.Error("Unable to get display");

	return display;
}

int EXPORT GB_INIT()
{
	char *env;
	
	gst_init(NULL, NULL);
	
	env = getenv("GB_MEDIA_DEBUG");
	if (env && atoi(env))
		MAIN_debug = TRUE;
	
	GB.GetInterface("gb.image", IMAGE_INTERFACE_VERSION, &IMAGE);
	return 0;
}

void EXPORT GB_EXIT()
{
}

