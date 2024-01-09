/***************************************************************************

  gb.gtk.platform.h

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

#ifndef __GB_GTK_PLATFORM_H
#define __GB_GTK_PLATFORM_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

// Gambas GTK+ platform component interface

#define GTK_PLATFORM_INTERFACE_VERSION 1

typedef
	struct {
		intptr_t version;

		void (*Init)(void);
		void (*Exit)(void);
		
		GtkWidget *(*CreatePlug)(long wid);
		
		struct {
			uintptr_t (*GetId)(GdkWindow *);
		} Window;
		
		struct {
			bool (*HasSystemTray)();
			void (*ShowTrayIcon)(GtkStatusIcon *icon, int width, int height);
		} Desktop;
		
		/*void (*ReleaseGrab)(void);
		void (*UnreleaseGrab)(void);
		int (*GetLastKeyCode)(void);

		struct {
			int (*GetResolutionX)(void);
			int (*GetResolutionY)(void);
			void (*Screenshot)(QPixmap *pixmap, int x, int y, int w, int h);
			} Desktop;

		struct {
			int (*GetVirtualDesktop)(QWidget *window);
			void (*SetVirtualDesktop)(QWidget *window, bool visible, int desktop);
			void (*Remap)(QWidget *window);
			void (*SetProperties)(QWidget *window, int which, QT_WINDOW_PROP *prop);
			void (*SetUserTime)(QWidget *window, int timestamp);
			void (*SetTransientFor)(QWidget *window, QWidget *parent);

			}	Window;*/
		}
	GTK_PLATFORM_INTERFACE;

#endif
