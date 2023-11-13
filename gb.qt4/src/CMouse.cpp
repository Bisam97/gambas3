/***************************************************************************

	CMouse.cpp

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

#define __CMOUSE_CPP

#include <qapplication.h>
#include <qpixmap.h>
#include <qcursor.h>
#include <qnamespace.h>

#include "gambas.h"
#include "main.h"

#include "gb.form.const.h"
#include "CWidget.h"
#include "CPicture.h"
#include "CMouse.h"

int MOUSE_click_x = -1;
int MOUSE_click_y = -1;
int MOUSE_click_count = 0;
double MOUSE_timer = 0;

MOUSE_INFO MOUSE_info = { 0 };
POINTER_INFO POINTER_info = { 0 };

int MOUSE_delta_x = 0;
int MOUSE_delta_y = 0;
void *MOUSE_wheel_on_control = NULL;

static int _dx = 0;
static int _dy = 0;

static void *_control = NULL;

void CMOUSE_clear(int valid)
{
	if (valid)
		MOUSE_info.valid++;
	else
		MOUSE_info.valid--;

	if (MOUSE_info.valid == 0)
		CLEAR(&POINTER_info);
}

void CMOUSE_reset_translate()
{
	_dx = _dy = 0;
}

void CMOUSE_set_control(void *control)
{
	if (_control)
		GB.Unref(&_control);
	if (control)
		GB.Ref(control);
	
	_control = control;
}

void CMOUSE_finish_event(void)
{
	if (_control)
		GB.Raise(_control, EVENT_MouseUp, 0);
}


//int CMOUSE_last_state = 0;

//static CCURSOR PredefinedCursor[LastCursor + 1] = { { { 0, 0 }, NULL, NULL } };
//static int MouseClassID;

#if 0
static int translate_state(int s)
{
	int bst = 0;

	if (s & Button1Mask)
		bst |= Qt::LeftButton;
	if ( s & Button2Mask)
		bst |= Qt::MidButton;
	if ( s & Button3Mask)
		bst |= Qt::RightButton;
	if ( s & ShiftMask)
		bst |= Qt::ShiftModifier;
	if ( s & ControlMask)
		bst |= Qt::ControlModifier;
	if ( s & qt_alt_mask)
		bst |= Qt::AltModifier;
	if ( s & qt_meta_mask)
		bst |= Qt::MetaModifier;

	return bst;
}

static int get_state()
{
	Window root;
	Window child;
	int root_x, root_y, win_x, win_y;
	uint state;
	Display* dpy = QPaintDevice::x11AppDisplay();

	for (int i = 0; i < ScreenCount(dpy); i++)
	{
		if (XQueryPointer(dpy, RootWindow(dpy, i), &root, &child,
					&root_x, &root_y, &win_x, &win_y, &state))
			return translate_state(state);
	}

	return 0;
}
#endif

//-------------------------------------------------------------------------

BEGIN_METHOD(Cursor_new, GB_OBJECT picture; GB_INTEGER x; GB_INTEGER y)

	CPICTURE *pict = (CPICTURE *)VARG(picture);

	THIS->x = VARGOPT(x, -1);
	THIS->y = VARGOPT(y, -1);

	//GB.StoreObject(ARG(picture), POINTER(&THIS->picture));
	if (GB.CheckObject(pict))
		return;
	
	if (THIS->x < 0 || THIS->x >= pict->pixmap->width())
		THIS->x = -1;
		
	if (THIS->y < 0 || THIS->y >= pict->pixmap->height())
		THIS->y = -1;
		
	THIS->cursor = new QCursor(*(pict->pixmap), THIS->x, THIS->y);

END_METHOD


BEGIN_METHOD_VOID(Cursor_Delete)

	//GB.Unref(POINTER(&THIS->picture));
	delete THIS->cursor;

END_METHOD


/*BEGIN_PROPERTY(CCURSOR_picture)

	GB.ReturnObject(THIS->picture);

END_PROPERTY*/


BEGIN_PROPERTY(Cursor_X)

	GB.ReturnInteger(THIS->x);

END_PROPERTY


BEGIN_PROPERTY(Cursor_Y)

	GB.ReturnInteger(THIS->y);

END_PROPERTY


// BEGIN_METHOD(CCURSOR_get, int shape)
//
//   int shape = PARAM(shape);
//   CCURSOR *p;
//
//   if (shape < 0 || shape > LastCursor)
//     GB.ReturnObject(NULL);
//
//   p = &PredefinedCursor[shape];
//   if (p->ob.klass == 0)
//   {
//     p->ob.klass = MouseClassID;
//     p->cursor = new QCursor(shape);
//     GB.Ref(p);
//   }
//
//   GB.ReturnObject(p);
//
// END_METHOD


//-------------------------------------------------------------------------

BEGIN_PROPERTY(Mouse_ScreenX)

	GB.ReturnInteger(QCursor::pos().x());

END_PROPERTY


BEGIN_PROPERTY(Mouse_ScreenY)

	GB.ReturnInteger(QCursor::pos().y());

END_PROPERTY


BEGIN_METHOD(Mouse_Move, GB_INTEGER x; GB_INTEGER y)

	QCursor::setPos(VARG(x), VARG(y));

END_PROPERTY

#define CHECK_VALID() \
	if (MOUSE_info.valid == 0) \
	{ \
		GB.Error("No mouse event data"); \
		return; \
	}

BEGIN_PROPERTY(Mouse_X)

	CHECK_VALID();
	GB.ReturnInteger(MOUSE_info.x + _dx);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Y)

	CHECK_VALID();
	GB.ReturnInteger(MOUSE_info.y + _dy);

END_PROPERTY

BEGIN_PROPERTY(Mouse_StartX)

	CHECK_VALID();
	GB.ReturnInteger(MOUSE_info.sx + _dx);

END_PROPERTY

BEGIN_PROPERTY(Mouse_StartY)

	CHECK_VALID();
	GB.ReturnInteger(MOUSE_info.sy + _dy);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Button)

	int i;
	
	CHECK_VALID();
	
	for (i = 0; i < 5; i++)
	{
		if ((int)MOUSE_info.button & (1 << i))
		{
			GB.ReturnInteger(i + 1);
			return;
		}
	}
	
	GB.ReturnInteger(0);

END_PROPERTY

BEGIN_PROPERTY(Mouse_State)

	CHECK_VALID();
	int state = (int)MOUSE_info.state;
	if (MOUSE_info.modifier & Qt::ShiftModifier)
		state |= MOUSE_SHIFT;
	if (MOUSE_info.modifier & Qt::ControlModifier)
		state |= MOUSE_CTRL;
	if (MOUSE_info.modifier & Qt::AltModifier)
		state |= MOUSE_ALT;
	if (MOUSE_info.modifier & Qt::MetaModifier)
		state |= MOUSE_META;
	
	GB.ReturnInteger(state);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Left)

	CHECK_VALID();
	GB.ReturnBoolean((MOUSE_info.state | MOUSE_info.button) & Qt::LeftButton);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Right)

	CHECK_VALID();
	GB.ReturnBoolean((MOUSE_info.state | MOUSE_info.button) & Qt::RightButton);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Middle)

	CHECK_VALID();
	GB.ReturnBoolean((MOUSE_info.state | MOUSE_info.button) & Qt::MidButton);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Shift)

	//CHECK_VALID();
	GB.ReturnBoolean(MOUSE_info.modifier & Qt::ShiftModifier);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Control)

	//CHECK_VALID();
	GB.ReturnBoolean(MOUSE_info.modifier & Qt::ControlModifier);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Alt)

	//CHECK_VALID();
	GB.ReturnBoolean(MOUSE_info.modifier & Qt::AltModifier);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Meta)

	//CHECK_VALID();
	GB.ReturnBoolean(MOUSE_info.modifier & Qt::MetaModifier);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Normal)

	//CHECK_VALID();
	GB.ReturnBoolean(MOUSE_info.modifier == Qt::NoModifier);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Orientation)

	CHECK_VALID();
	GB.ReturnInteger(MOUSE_info.orientation);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Delta)

	CHECK_VALID();
	GB.ReturnFloat((double)MOUSE_info.delta / 120);

END_PROPERTY

BEGIN_PROPERTY(Mouse_Forward)

	CHECK_VALID();
	GB.ReturnBoolean(MOUSE_info.delta > 0);

END_PROPERTY

BEGIN_METHOD(Mouse_Inside, GB_OBJECT control)
	
	CWIDGET *control = (CWIDGET *)VARG(control);
	QPoint pos;
	
	if (GB.CheckObject(control))
		return;
	
	if (!CWIDGET_is_visible(control))
	{
		GB.ReturnBoolean(false);
		return;
	}
		
	pos = QCursor::pos() - QWIDGET(control)->mapToGlobal(QPoint(0, 0));
	GB.ReturnBoolean(pos.x() >= 0 && pos.x() < QWIDGET(control)->width() && pos.y() >= 0 && pos.y() < QWIDGET(control)->height());

END_METHOD

BEGIN_METHOD(Mouse_Translate, GB_INTEGER dx; GB_INTEGER dy)

	CHECK_VALID();

	_dx = VARG(dx);
	_dy = VARG(dy);

END_METHOD

BEGIN_PROPERTY(Mouse_Click)

	GB.ReturnInteger(MOUSE_info.valid ? MOUSE_click_count : 0);

END_PROPERTY

#if 0
BEGIN_METHOD(Mouse_Begin, GB_OBJECT control; GB_INTEGER x; GB_INTEGER y; GB_INTEGER state)

	void *control = VARG(control);
	int state = VARGOPT(state, 0);
	int x = VARG(x);
	int y = VARG(y);

	if (GB.CheckObject(control))
		return;
	
	CMOUSE_clear(true);
	MOUSE_info.x = x;
	MOUSE_info.y = y;
	QPoint p = QWIDGET(control)->mapToGlobal(QPoint(x, y));
	MOUSE_info.sx = p.x();
	MOUSE_info.sy = p.y();
	
	MOUSE_info.state = (Qt::MouseButtons)(state & 0xFF);
	if (state & MOUSE_LEFT)
		MOUSE_info.button = Qt::LeftButton;
	else if (state & MOUSE_MIDDLE)
		MOUSE_info.button = Qt::MiddleButton;
	else if (state & MOUSE_RIGHT)
		MOUSE_info.button = Qt::RightButton;
	
	MOUSE_info.modifier = (Qt::KeyboardModifiers)0;
	if (state & MOUSE_SHIFT)
		MOUSE_info.modifier |= Qt::ShiftModifier;
	if (state & MOUSE_CTRL)
		MOUSE_info.modifier |= Qt::ShiftModifier;
	if (state & MOUSE_ALT)
		MOUSE_info.modifier |= Qt::ShiftModifier;

END_METHOD

BEGIN_METHOD_VOID(Mouse_End)

	CMOUSE_clear(false);

END_METHOD
#endif

//-------------------------------------------------------------------------

BEGIN_PROPERTY(Pointer_X)

	CHECK_VALID();
	GB.ReturnFloat((double)(MOUSE_info.x + _dx) + (POINTER_info.tx - (int)POINTER_info.tx));

END_PROPERTY

BEGIN_PROPERTY(Pointer_Y)

	CHECK_VALID();
	GB.ReturnFloat((double)(MOUSE_info.y + _dy) + (POINTER_info.ty - (int)POINTER_info.ty));

END_PROPERTY

BEGIN_PROPERTY(Pointer_ScreenX)

	CHECK_VALID();
	GB.ReturnFloat(POINTER_info.tx);

END_PROPERTY

BEGIN_PROPERTY(Pointer_ScreenY)

	CHECK_VALID();
	GB.ReturnFloat(POINTER_info.ty);

END_PROPERTY

BEGIN_PROPERTY(Pointer_XTilt)

	CHECK_VALID();
	GB.ReturnFloat(POINTER_info.xtilt);

END_PROPERTY

BEGIN_PROPERTY(Pointer_YTilt)

	CHECK_VALID();
	GB.ReturnFloat(POINTER_info.ytilt);

END_PROPERTY

BEGIN_PROPERTY(Pointer_Pressure)

	CHECK_VALID();
	GB.ReturnFloat(POINTER_info.pressure);

END_PROPERTY

BEGIN_PROPERTY(Pointer_Rotation)

	CHECK_VALID();
	GB.ReturnFloat(POINTER_info.rotation);

END_PROPERTY

BEGIN_PROPERTY(Pointer_Type)

	CHECK_VALID();
	GB.ReturnInteger(POINTER_info.type);

END_PROPERTY


//-------------------------------------------------------------------------

GB_DESC CCursorDesc[] =
{
	GB_DECLARE("Cursor", sizeof(CCURSOR)),

	GB_METHOD("_new", NULL, Cursor_new, "(Picture)Picture;[(X)i(Y)i]"),
	GB_METHOD("_free", NULL, Cursor_Delete, NULL),

	GB_CONSTANT("Custom", "i", CURSOR_CUSTOM),
	GB_CONSTANT("Default", "i", CURSOR_DEFAULT),

	GB_CONSTANT("Blank", "i", CURSOR_NONE),
	GB_CONSTANT("Arrow", "i", CURSOR_ARROW),
	GB_CONSTANT("Cross", "i", CURSOR_CROSSHAIR),
	GB_CONSTANT("Wait", "i", CURSOR_WAIT),
	GB_CONSTANT("Text", "i", CURSOR_TEXT),
	GB_CONSTANT("SizeAll", "i", CURSOR_MOVE),
	GB_CONSTANT("SizeH", "i", CURSOR_EW_RESIZE),
	GB_CONSTANT("SizeV", "i", CURSOR_NS_RESIZE),
	GB_CONSTANT("SizeN", "i", CURSOR_N_RESIZE),
	GB_CONSTANT("SizeS", "i", CURSOR_S_RESIZE),
	GB_CONSTANT("SizeW", "i", CURSOR_W_RESIZE),
	GB_CONSTANT("SizeE", "i", CURSOR_E_RESIZE),
	GB_CONSTANT("SizeNW", "i", CURSOR_NW_RESIZE), //FDiag
	GB_CONSTANT("SizeSE", "i", CURSOR_SE_RESIZE),
	GB_CONSTANT("SizeNE", "i", CURSOR_NE_RESIZE), //BDiag
	GB_CONSTANT("SizeSW", "i", CURSOR_SW_RESIZE),
	GB_CONSTANT("SizeNWSE", "i", CURSOR_NWSE_RESIZE),
	GB_CONSTANT("SizeNESW", "i", CURSOR_NESW_RESIZE),
	GB_CONSTANT("SplitH", "i", CURSOR_COL_RESIZE), // SplitH
	GB_CONSTANT("SplitV", "i", CURSOR_ROW_RESIZE), // SplitV
	GB_CONSTANT("Pointing", "i", CURSOR_POINTER),

	GB_CONSTANT("None", "i", CURSOR_NONE),
	GB_CONSTANT("Help", "i", CURSOR_HELP),
	GB_CONSTANT("Pointer", "i", CURSOR_POINTER),
	GB_CONSTANT("ContextMenu", "i", CURSOR_CONTEXT_MENU),
	GB_CONSTANT("Progress", "i", CURSOR_PROGRESS),
	GB_CONSTANT("Cell", "i", CURSOR_CELL),
	GB_CONSTANT("CrossHair", "i", CURSOR_CROSSHAIR),
	GB_CONSTANT("VerticalText", "i", CURSOR_VERTICAL_TEXT),
	GB_CONSTANT("Alias", "i", CURSOR_ALIAS),
	GB_CONSTANT("Copy", "i", CURSOR_COPY),
	GB_CONSTANT("NoDrop", "i", CURSOR_NO_DROP),
	GB_CONSTANT("Move", "i", CURSOR_MOVE),
	GB_CONSTANT("NotAllowed", "i", CURSOR_NOT_ALLOWED),
	GB_CONSTANT("Grab", "i", CURSOR_GRAB),
	GB_CONSTANT("Grabbing", "i", CURSOR_GRABBING),
	GB_CONSTANT("AllScroll", "i", CURSOR_ALL_SCROLL),
	GB_CONSTANT("ColResize", "i", CURSOR_COL_RESIZE),
	GB_CONSTANT("RowResize", "i", CURSOR_ROW_RESIZE),
	GB_CONSTANT("NResize", "i", CURSOR_N_RESIZE),
	GB_CONSTANT("EResize", "i", CURSOR_E_RESIZE),
	GB_CONSTANT("SResize", "i", CURSOR_S_RESIZE),
	GB_CONSTANT("WResize", "i", CURSOR_W_RESIZE),
	GB_CONSTANT("NEResize", "i", CURSOR_NE_RESIZE),
	GB_CONSTANT("NWResize", "i", CURSOR_NW_RESIZE),
	GB_CONSTANT("SWResize", "i", CURSOR_SW_RESIZE),
	GB_CONSTANT("SEResize", "i", CURSOR_SE_RESIZE),
	GB_CONSTANT("EWResize", "i", CURSOR_EW_RESIZE),
	GB_CONSTANT("NSResize", "i", CURSOR_NS_RESIZE),
	GB_CONSTANT("NESWResize", "i", CURSOR_NESW_RESIZE),
	GB_CONSTANT("NWSEResize", "i", CURSOR_NWSE_RESIZE),
	GB_CONSTANT("ZoomIn", "i", CURSOR_ZOOM_IN),
	GB_CONSTANT("ZoomOut", "i", CURSOR_ZOOM_OUT),

	GB_PROPERTY_READ("X", "i", Cursor_X),
	GB_PROPERTY_READ("Y", "i", Cursor_Y),
	//GB_PROPERTY_READ("Picture", "Picture", CCURSOR_picture),

	GB_END_DECLARE
};


GB_DESC CMouseDesc[] =
{
	GB_DECLARE("Mouse", 0), GB_VIRTUAL_CLASS(),

	GB_STATIC_PROPERTY_READ("ScreenX", "i", Mouse_ScreenX),
	GB_STATIC_PROPERTY_READ("ScreenY", "i", Mouse_ScreenY),
	GB_STATIC_METHOD("Move", NULL, Mouse_Move, "(X)i(Y)i"),
	GB_STATIC_METHOD("Inside", "b", Mouse_Inside, "(Control)Control"),

	GB_CONSTANT("Custom", "i", CURSOR_CUSTOM),
	GB_CONSTANT("Default", "i", CURSOR_DEFAULT),

	GB_CONSTANT("Blank", "i", CURSOR_NONE),
	GB_CONSTANT("Arrow", "i", CURSOR_ARROW),
	GB_CONSTANT("Cross", "i", CURSOR_CROSSHAIR),
	GB_CONSTANT("Wait", "i", CURSOR_WAIT),
	GB_CONSTANT("Text", "i", CURSOR_TEXT),
	GB_CONSTANT("SizeAll", "i", CURSOR_MOVE),
	GB_CONSTANT("SizeH", "i", CURSOR_EW_RESIZE),
	GB_CONSTANT("SizeV", "i", CURSOR_NS_RESIZE),
	GB_CONSTANT("SizeN", "i", CURSOR_N_RESIZE),
	GB_CONSTANT("SizeS", "i", CURSOR_S_RESIZE),
	GB_CONSTANT("SizeW", "i", CURSOR_W_RESIZE),
	GB_CONSTANT("SizeE", "i", CURSOR_E_RESIZE),
	GB_CONSTANT("SizeNW", "i", CURSOR_NW_RESIZE), //FDiag
	GB_CONSTANT("SizeSE", "i", CURSOR_SE_RESIZE),
	GB_CONSTANT("SizeNE", "i", CURSOR_NE_RESIZE), //BDiag
	GB_CONSTANT("SizeSW", "i", CURSOR_SW_RESIZE),
	GB_CONSTANT("SizeNWSE", "i", CURSOR_NWSE_RESIZE),
	GB_CONSTANT("SizeNESW", "i", CURSOR_NESW_RESIZE),
	GB_CONSTANT("SplitH", "i", CURSOR_COL_RESIZE), // SplitH
	GB_CONSTANT("SplitV", "i", CURSOR_ROW_RESIZE), // SplitV
	GB_CONSTANT("Pointing", "i", CURSOR_POINTER),

	GB_CONSTANT("Horizontal", "i", Qt::Horizontal),
	GB_CONSTANT("Vertical", "i", Qt::Vertical),

	GB_STATIC_PROPERTY_READ("X", "i", Mouse_X),
	GB_STATIC_PROPERTY_READ("Y", "i", Mouse_Y),
	GB_STATIC_PROPERTY_READ("StartX", "i", Mouse_StartX),
	GB_STATIC_PROPERTY_READ("StartY", "i", Mouse_StartY),
	GB_STATIC_PROPERTY_READ("Left", "b", Mouse_Left),
	GB_STATIC_PROPERTY_READ("Right", "b", Mouse_Right),
	GB_STATIC_PROPERTY_READ("Middle", "b", Mouse_Middle),
	GB_STATIC_PROPERTY_READ("State", "i", Mouse_State),
	GB_STATIC_PROPERTY_READ("Button", "i", Mouse_Button),
	GB_STATIC_PROPERTY_READ("Shift", "b", Mouse_Shift),
	GB_STATIC_PROPERTY_READ("Control", "b", Mouse_Control),
	GB_STATIC_PROPERTY_READ("Alt", "b", Mouse_Alt),
	GB_STATIC_PROPERTY_READ("Meta", "b", Mouse_Meta),
	GB_STATIC_PROPERTY_READ("Normal", "b", Mouse_Normal),
	GB_STATIC_PROPERTY_READ("Orientation", "i", Mouse_Orientation),
	GB_STATIC_PROPERTY_READ("Delta", "f", Mouse_Delta),
	GB_STATIC_PROPERTY_READ("Forward", "b", Mouse_Forward),
	GB_STATIC_PROPERTY_READ("Click", "i", Mouse_Click),

	GB_STATIC_METHOD("Translate", NULL, Mouse_Translate, "(DX)i(DY)i"),
	//GB_STATIC_METHOD("Begin", NULL, Mouse_Begin, "(Control)Control;(X)i(Y)i[(State)i]"),
	//GB_STATIC_METHOD("End", NULL, Mouse_End, NULL),

	GB_END_DECLARE
};

GB_DESC CPointerDesc[] =
{
	GB_DECLARE_VIRTUAL("Pointer"),
	
	GB_CONSTANT("Mouse", "i", POINTER_MOUSE),
	GB_CONSTANT("Pen", "i", POINTER_PEN),
	GB_CONSTANT("Eraser", "i", POINTER_ERASER),
	GB_CONSTANT("Cursor", "i", POINTER_CURSOR),
	
	GB_STATIC_PROPERTY_READ("Type", "i", Pointer_Type),
	GB_STATIC_PROPERTY_READ("X", "f", Pointer_X),
	GB_STATIC_PROPERTY_READ("Y", "f", Pointer_Y),
	GB_STATIC_PROPERTY_READ("ScreenX", "f", Pointer_ScreenX),
	GB_STATIC_PROPERTY_READ("ScreenY", "f", Pointer_ScreenY),
	GB_STATIC_PROPERTY_READ("XTilt", "f", Pointer_XTilt),
	GB_STATIC_PROPERTY_READ("YTilt", "f", Pointer_YTilt),
	GB_STATIC_PROPERTY_READ("Pressure", "f", Pointer_Pressure),
	GB_STATIC_PROPERTY_READ("Rotation", "f", Pointer_Rotation),
	
	GB_END_DECLARE
};

