/***************************************************************************

	CMouse.h

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

#ifndef __CMOUSE_H
#define __CMOUSE_H

#include <QCursor>
#include <QMouseEvent>

#include "gambas.h"

#include "CPicture.h"

typedef
	struct {
		int valid;
		int x;
		int y;
		int sx;
		int sy;
		Qt::MouseButton button;
		Qt::MouseButtons state;
		Qt::KeyboardModifiers modifier;
		int orientation;
		int delta;
		int screenX;
		int screenY;
		int dx;
		int dy;
		}
	MOUSE_INFO;
	
typedef
	struct {
		double tx;
		double ty;
		int xtilt;
		int ytilt;
		int type;
		double pressure;
		double rotation;
	}
	POINTER_INFO;

#ifndef __CMOUSE_CPP
	
extern GB_DESC CMouseDesc[];
extern GB_DESC CCursorDesc[];
extern GB_DESC CPointerDesc[];

extern int MOUSE_click_x;
extern int MOUSE_click_y;
extern int MOUSE_click_count;
extern double MOUSE_timer;

extern MOUSE_INFO MOUSE_info;
extern POINTER_INFO POINTER_info;

#else

#define THIS ((CCURSOR *)_object)

#endif

typedef
	struct _CCURSOR {
		GB_BASE ob;
		int x;
		int y;
		QCursor *cursor;
		}
	CCURSOR;

void CMOUSE_clear(int valid);
void CMOUSE_reset_translate(void);
void CMOUSE_set_control(void *control);
void CMOUSE_finish_event(void);

#define CMOUSE_is_valid() (MOUSE_info.valid != 0)

#endif
