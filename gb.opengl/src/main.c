/***************************************************************************

  main.c

  The Gambas openGL component

  (c) 2005-2007 Laurent Carlier <lordheavy@users.sourceforge.net>
                Benoît Minisini <gambas@users.sourceforge.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/

#define __MAIN_C

#include "gambas.h"
#include "gb_common.h"
#include "main.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include "GL.h"
#include "GLU.h"

GB_INTERFACE GB EXPORT;

GB_DESC *GB_CLASSES[] EXPORT =
{
/* GL */
	Cgl,
/* GLU */
	Cglu,

	NULL
};

int IMAGE_get_pixel_format(GB_IMG *image)
{
	if ((image->format == GB_IMAGE_RGBA) || (image->format == GB_IMAGE_RGBX))
		return GL_RGBA;
	// is it good ?
	else if ((image->format == GB_IMAGE_BGRA) || (image->format == GB_IMAGE_BGRX))
		return GL_BGRA;
	else // BM: what to do then ??
		return GL_RGBA;
}

int EXPORT GB_INIT(void)
{
	//GB.GetInterface("gb.image", IMAGE_INTERFACE_VERSION, &IMAGE);
	
  return FALSE;
}

void EXPORT GB_EXIT()
{
}
