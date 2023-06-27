/***************************************************************************

  widgets.h

  (c) 2004-2006 - Daniel Campos Fernández <dcamposf@gmail.com>

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

#ifndef __WIDGETS_H
#define __WIDGETS_H

#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_26

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef GTK3
#include <gdk/gdkkeysyms-compat.h>
#else
#ifndef GAMBAS_DIRECTFB
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/X.h>
#endif
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#include "gb_common.h"

#include "gb.form.properties.h"
#include "gb.form.const.h"

#include "gpicture.h"
#include "gcursor.h"
#include "gfont.h"
#include "gcontrol.h"
#include "gcontainer.h"
#include "gtools.h"

#include "main.h"
//#define GTK_DEBUG_SIGNALS
//#define GTK_DEBUG_OBJECTS

enum
{
 gEvent_MousePress,
 gEvent_MouseRelease,
 gEvent_MouseMove,
 gEvent_MouseDrag,
 gEvent_MouseWheel,
 gEvent_MouseDblClick,
 gEvent_MouseMenu,
 gEvent_KeyPress,
 gEvent_KeyRelease,
 gEvent_FocusIn,
 gEvent_FocusOut,
 gEvent_Enter,
 gEvent_Leave,
 gEvent_DragMove,
 gEvent_Drop
};

typedef
	unsigned char uchar;

#ifdef GTK3
	#define ON_DRAW_BEFORE(_widget, _this, _gtk, _gtk3) g_signal_connect(G_OBJECT(_widget), "draw", G_CALLBACK(_gtk3), (gpointer)_this)
	#define ON_DRAW(_widget, _this, _gtk, _gtk3) g_signal_connect_after(G_OBJECT(_widget), "draw", G_CALLBACK(_gtk3), (gpointer)_this)
#else
	#define ON_DRAW_BEFORE(_widget, _this, _gtk, _gtk3) g_signal_connect(G_OBJECT(_widget), "expose-event", G_CALLBACK(_gtk), (gpointer)_this)
	#define ON_DRAW(_widget, _this, _gtk, _gtk3) g_signal_connect_after(G_OBJECT(_widget), "expose-event", G_CALLBACK(_gtk), (gpointer)_this)
#endif

#ifdef GTK3

#define STATE_T GtkStateFlags
#define STYLE_T GtkStyleContext

#define STATE_NORMAL GTK_STATE_FLAG_NORMAL
#define STATE_ACTIVE GTK_STATE_FLAG_ACTIVE
#define STATE_INSENSITIVE GTK_STATE_FLAG_INSENSITIVE
#define STATE_PRELIGHT GTK_STATE_FLAG_PRELIGHT
#define STATE_SELECTED GTK_STATE_FLAG_SELECTED
#define STATE_FOCUSED GTK_STATE_FLAG_FOCUSED
#define STATE_LINK GTK_STATE_FLAG_LINK
#define STATE_VISITED GTK_STATE_FLAG_VISITED

#if GTK_CHECK_VERSION(3, 14, 0)
#else
#define GTK_STATE_FLAG_CHECKED GTK_STATE_FLAG_ACTIVE
#endif


#define gtk_hbox_new(_homogeneous, _spacing) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, _spacing)
#define gtk_vbox_new(_homogeneous, _spacing) gtk_box_new(GTK_ORIENTATION_VERTICAL, _spacing)

#define gtk_hscale_new(_adj) gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, _adj)
#define gtk_vscale_new(_adj) gtk_scale_new(GTK_ORIENTATION_VERTICAL, _adj)

#define gtk_hscrollbar_new(_adj) gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, _adj)
#define gtk_vscrollbar_new(_adj) gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, _adj)

#define gtk_modify_font gtk_override_font

#else

#define STATE_T GtkStateType
#define STYLE_T GtkStyle

#define STATE_NORMAL GTK_STATE_NORMAL
#define STATE_ACTIVE GTK_STATE_ACTIVE
#define STATE_INSENSITIVE GTK_STATE_INSENSITIVE
#define STATE_PRELIGHT GTK_STATE_PRELIGHT
#define STATE_SELECTED GTK_STATE_SELECTED
#define STATE_LINK GTK_STATE_NORMAL
#define STATE_VISITED GTK_STATE_NORMAL

#endif

#endif
