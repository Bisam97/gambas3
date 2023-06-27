/***************************************************************************

  main.h

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

#ifndef __MAIN_H
#define __MAIN_H

#include "gb_common.h"
#include "gambas.h"
#include "../gb.gtk.platform.h"
#include <gdk/gdkx.h>
#include <gtk/gtkx.h>

typedef
	void (*X11_EVENT_FILTER)(XEvent *);

#ifndef __MAIN_C
extern "C" const GB_INTERFACE *GB_PTR;
#endif
#define GB (*GB_PTR)

#endif
