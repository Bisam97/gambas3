/***************************************************************************

  CStyle.h

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

#ifndef __CSTYLE_H
#define __CSTYLE_H

#include "main.h"
#include "gdesktop.h"
#include "gapplication.h"

#ifndef __CSTYLE_CPP
extern GB_DESC StyleDesc[];
#endif

#ifdef GTK3
void CSTYLE_paint_check(cairo_t *cr, int x, int y, int w, int h, int value, int state);
void CSTYLE_paint_option(cairo_t *cr, int x, int y, int w, int h, int value, int state);
#else
void CSTYLE_paint_check(GdkDrawable *dr, int x, int y, int w, int h, int value, int state);
void CSTYLE_paint_option(GdkDrawable *dr, int x, int y, int w, int h, int value, int state);
#endif

#endif
