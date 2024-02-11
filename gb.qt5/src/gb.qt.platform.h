/***************************************************************************

  gb.qt.platform.h

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

#ifndef __GB_QT_PLATFORM_H
#define __GB_QT_PLATFORM_H

#include <QObject>
#include <QWidget>
#include <QPixmap>

// Gambas QT platform component interface

#define QT_PLATFORM_INTERFACE_VERSION 1

typedef 
	struct {
		unsigned stacking : 2;
		unsigned skipTaskbar : 1;
		unsigned border: 1;
		unsigned sticky: 1;
	} QT_WINDOW_PROP;
	
enum {
	PROP_ALL = -1,
	PROP_STACKING = 1,
	PROP_SKIP_TASKBAR = 2,
	PROP_BORDER = 4,
	PROP_STICKY = 8
};

typedef
	struct {
		intptr_t version;
		
		void (*Init)(void);
		void (*Exit)(void);
		
		void (*ReleaseGrab)(void);
		void (*UnreleaseGrab)(void);
		int (*GetLastKeyCode)(void);
		
		struct {
			int (*GetResolutionX)(void);
			int (*GetResolutionY)(void);
			void (*Screenshot)(QPixmap *pixmap, int x, int y, int w, int h);
			} Desktop;
		
		struct {
			uintptr_t (*GetId)(QWidget *window);
			int (*GetVirtualDesktop)(QWidget *window);
			void (*SetVirtualDesktop)(QWidget *window, bool visible, int desktop);
			void (*Remap)(QWidget *window);
			void (*SetProperties)(QWidget *window, int which, QT_WINDOW_PROP *prop);
			void (*SetUserTime)(QWidget *window, int timestamp);
			void (*SetTransientFor)(QWidget *window, QWidget *parent);
			void (*Activate)(QWidget *window);
			}	Window;
		}
	QT_PLATFORM_INTERFACE;

#endif
