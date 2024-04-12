/***************************************************************************

  main.cpp

  gb.form.htmlview component

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __MAIN_CPP

#include "c_htmldocument.h"
#include "main.h"

extern "C" {
	
const GB_INTERFACE *GB_PTR EXPORT;
DRAW_INTERFACE DRAW;
GEOM_INTERFACE GEOM;
IMAGE_INTERFACE IMAGE;

GB_DESC *GB_CLASSES[] EXPORT =
{
	HtmlDocumentDesc,
	NULL
};

int EXPORT GB_INIT(void)
{
  GB.GetInterface("gb.draw", DRAW_INTERFACE_VERSION, &DRAW);
  GB.GetInterface("gb.geom", GEOM_INTERFACE_VERSION, &GEOM);
  GB.GetInterface("gb.image", IMAGE_INTERFACE_VERSION, &IMAGE);
	return 0;
}

void EXPORT GB_EXIT()
{
}

}
