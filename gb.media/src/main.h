/***************************************************************************

  main.h

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

#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

#include "gb_common.h"
#include "gambas.h"
#include "gb.image.h"

#ifndef __MAIN_C
extern bool MAIN_debug;
extern GB_INTERFACE GB;
extern IMAGE_INTERFACE IMAGE;
#endif

uintptr_t MAIN_get_window_handle(void *control);

#endif /* __MAIN_H */
