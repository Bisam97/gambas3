/***************************************************************************

	CWidget.cpp

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

#define __CWIDGET_CPP

#include "widgets.h"
#include "gapplication.h"

#include "CWidget.h"
#include "CWindow.h"
#include "CMenu.h"
#include "CFont.h"
#include "CMouse.h"
#include "CPicture.h"
#include "CContainer.h"
#include "CClipboard.h"
#include "CPanel.h"

extern int MAIN_scale;

DECLARE_EVENT(EVENT_Enter);
DECLARE_EVENT(EVENT_GotFocus);
DECLARE_EVENT(EVENT_LostFocus);
DECLARE_EVENT(EVENT_KeyPress);
DECLARE_EVENT(EVENT_KeyRelease);
DECLARE_EVENT(EVENT_Leave);
DECLARE_EVENT(EVENT_MouseDown);
DECLARE_EVENT(EVENT_MouseMove);
DECLARE_EVENT(EVENT_MouseDrag);
DECLARE_EVENT(EVENT_MouseUp);
DECLARE_EVENT(EVENT_MouseWheel);
DECLARE_EVENT(EVENT_DblClick);
DECLARE_EVENT(EVENT_Menu);
DECLARE_EVENT(EVENT_Drag);
DECLARE_EVENT(EVENT_DragMove);
DECLARE_EVENT(EVENT_Drop);
DECLARE_EVENT(EVENT_DragLeave);

//Plug
DECLARE_EVENT(EVENT_Plugged);
DECLARE_EVENT(EVENT_Unplugged);
DECLARE_EVENT(EVENT_PlugError);

//static void *CLASS_Image = NULL;

/** Action *****************************************************************/

static bool has_action(void *control)
{
	if (GB.Is(control, GB.FindClass("Menu")))
	{
		gMenu *menu = ((CMENU *)(control))->widget;
		return menu ? menu->action() : false;
	}
	else
	{
		gControl *ctrl = ((CWIDGET *)(control))->widget;
		return ctrl ? ctrl->action() : false;
	}
}

static void set_action(void *control, bool v)
{
	if (GB.Is(control, GB.FindClass("Menu")))
	{
		gMenu *menu = ((CMENU *)(control))->widget;
		if (menu)
			menu->setAction(v);
	}
	else
	{
		gControl *ctrl = ((CWIDGET *)(control))->widget;
		if (ctrl)
			ctrl->setAction(v);
	}
}

#define HAS_ACTION(_control) has_action(_control)
#define SET_ACTION(_control, _flag) set_action(_control, _flag)

#include "gb.form.action.h"


/** Control ****************************************************************/

void gb_plug_raise_plugged(gControl *sender)
{
	CWIDGET *_ob=GetObject(sender);

	if (!_ob) return;
	GB.Raise((void*)_ob,EVENT_Plugged,0);
}

void gb_plug_raise_unplugged(gControl *sender)
{
	CWIDGET *_ob=GetObject(sender);

	if (!_ob) return;
	GB.Raise((void*)_ob,EVENT_Unplugged,0);
}

void gb_plug_raise_error(gControl *sender)
{
	CWIDGET *_ob=GetObject(sender);

	if (!_ob) return;
	GB.Raise((void*)_ob,EVENT_PlugError,0);
}

// static void raise_menu(void *_object)
// {
// 	GB.Raise(THIS, EVENT_Menu, 0);
// 	GB.Unref(POINTER(&_object));
// }

static int to_gambas_event(int type)
{
	switch (type)
	{
		case gEvent_MousePress: return EVENT_MouseDown;
		case gEvent_MouseRelease: return EVENT_MouseUp;
		case gEvent_MouseMove: return EVENT_MouseMove;
		case gEvent_MouseDrag: return EVENT_MouseDrag;
		case gEvent_MouseWheel: return EVENT_MouseWheel;
		case gEvent_MouseMenu: return EVENT_Menu;
		case gEvent_MouseDblClick: return EVENT_DblClick;
		case gEvent_KeyPress: return EVENT_KeyPress;
		case gEvent_KeyRelease: return EVENT_KeyRelease;
		case gEvent_FocusIn: return EVENT_GotFocus;
		case gEvent_FocusOut: return EVENT_LostFocus;
		case gEvent_Enter: return EVENT_Enter;
		case gEvent_Leave: return EVENT_Leave;
		case gEvent_DragMove: return EVENT_DragMove;
		case gEvent_Drop: return EVENT_Drop;
		default: fprintf(stderr, "warning: to_gambas_event: unhandled event: %d\n", type); return -1;
	}
}

bool CB_control_can_raise(gControl *sender, int type)
{
	CWIDGET *ob = GetObject(sender);
	if (!ob)
		return false;

	type = to_gambas_event(type);
	if (type < 0)
		return false;

	return GB.CanRaise(ob, type);
}

bool CB_control_mouse(gControl *sender, int type)
{
  CB_GET_OBJECT_RETURN(sender, false);
	bool ret = false;

	if (!THIS) return false; // possible, for MouseDrag for example

	switch(type)
	{
		case gEvent_MouseDrag:
			ret = GB.Raise(THIS, EVENT_MouseDrag, 0);
			break;

		case gEvent_MouseMenu:

			for(;;)
			{
				if (GB.CanRaise(THIS, EVENT_Menu))
				{
					int old = gMenu::popupCount();
					if (GB.Raise(THIS, EVENT_Menu, 0) || old != gMenu::popupCount())
						return true;
				}

				if (THIS->popup)
				{
					gMainWindow *window = sender->window();
					gMenu *menu = gMenu::findFromName(window, THIS->popup);
					if (menu)
					{
						menu->popup();
						CMENU_check_popup_click();
					}
					return true;
				}
				
				if (sender->hasNativePopup())
					return false;

				if (sender->isTopLevel())
					break;

				sender = sender->parent();
				_object = GetObject(sender);
			}

			break;

		default:
			ret = GB.Raise(THIS, to_gambas_event(type), 0);
	}

	return ret;
}

bool CB_control_key(gControl *sender, int type)
{
	CB_GET_OBJECT_RETURN(sender, false);
	return GB.Raise(THIS, to_gambas_event(type), 0);
}

void CB_control_enter_leave(gControl *sender, int type)
{
	CB_GET_OBJECT(sender);
	GB.Raise(THIS, to_gambas_event(type), 0);
}

void CB_control_focus(gControl *sender, int type)
{
	CB_GET_OBJECT(sender);
	GB.Raise(THIS, to_gambas_event(type), 0);
}

bool CB_control_drag(gControl *sender)
{
	CB_GET_OBJECT_RETURN(sender, true);

	if (!GB.CanRaise(THIS, EVENT_Drag))
	{
		if (GB.CanRaise(THIS, EVENT_DragMove) || GB.CanRaise(THIS, EVENT_Drop))
			return false;
		else
			return true;
	}

	return GB.Raise(THIS, EVENT_Drag, 0);
}

void CB_control_drag_leave(gControl *sender)
{
	CB_GET_OBJECT(sender);
	GB.Raise(THIS, EVENT_DragLeave, 0);
}

bool CB_control_drag_move(gControl *sender)
{
	CB_GET_OBJECT_RETURN(sender, true);
	if (!THIS)
		return true;

	if (!GB.CanRaise(THIS, EVENT_DragMove))
		return !GB.CanRaise(THIS, EVENT_Drag);

	return GB.Raise(THIS, EVENT_DragMove, 0);
}

bool CB_control_drop(gControl *sender)
{
	CB_GET_OBJECT_RETURN(sender, false);
	if (!THIS)
		return false;

	if (!GB.CanRaise(THIS, EVENT_Drop))
		return false;

	GB.Raise(THIS, EVENT_Drop, 0);
	return true;
}

void CB_control_finish(gControl *control)
{
	CB_GET_OBJECT(control);
	if (!THIS)
		return;

	GB.Detach(THIS);

	GB.StoreVariant(NULL, POINTER(&THIS->tag));
	GB.StoreObject(NULL, POINTER(&THIS->cursor));

	CACTION_register(THIS, THIS->action, NULL);
	GB.FreeString(&THIS->action);

	if (control->isTopLevel())
		CWINDOW_check_main_window((CWINDOW *)THIS);

	GB.Unref(POINTER(&THIS->font));
	GB.FreeString(&THIS->popup);

	THIS->font = NULL;
	THIS->widget = NULL;
	GB.Unref(POINTER(&_object));

	control->hFree = NULL;
}


void InitControl(gControl *control, CWIDGET *widget)
{
	static int n = 0;
	char *name;
	
	if (control->hFree)
		return;

	GB.Ref((void*)widget);

	widget->widget = control;
	control->hFree = (void*)widget;
	//fprintf(stderr, "InitControl: %p %p\n", control, widget);

	name = GB.GetLastEventName();
	if (!name)
	{
		char buffer[16];
		n++;
		sprintf(buffer, "#%d", n);
		control->setName(buffer);
	}
	else
		control->setName(name);

	if (control->parent())
		CCONTAINER_raise_insert((CCONTAINER *)control->parent()->hFree, widget);
}

CWIDGET *GetContainer(CWIDGET *control)
{
	gContainer *cont;
	
	if (GB.CheckObject(control))
		GB.Propagate();
	
	cont = ((gContainer *)control->widget)->proxyContainer();
	//fprintf(stderr, "container: (%p %s)\n", control->widget, control->widget->name());
	
	return GetObject(cont);
}

int CWIDGET_get_handle(void *_object)
{
	return CONTROL->handle();
}

int CWIDGET_check(void *_object)
{
	return (!CONTROL || CONTROL->isDestroyed());
}

/*************************************************************************************

Embedder

**************************************************************************************/
#ifndef GTK3

BEGIN_METHOD(CPLUGIN_new, GB_OBJECT parent)

	InitControl(new gPlugin(CONTAINER(VARG(parent))), (CWIDGET*)_object);
	PLUGIN->onPlug = gb_plug_raise_plugged;
	PLUGIN->onUnplug = gb_plug_raise_unplugged;
	PLUGIN->onError = gb_plug_raise_error;

END_METHOD

BEGIN_PROPERTY(CPLUGIN_client)

	GB.ReturnInteger(PLUGIN->client());

END_PROPERTY

BEGIN_METHOD(CPLUGIN_embed, GB_INTEGER id)

	PLUGIN->plug(VARG(id));

END_METHOD

BEGIN_METHOD_VOID(CPLUGIN_discard)

	PLUGIN->discard();

END_METHOD

#endif

/**************************************************************************************

Control

**************************************************************************************/

BEGIN_PROPERTY(Control_X)

	if (READ_PROPERTY) {  GB.ReturnInteger(CONTROL->left()); return; }
	CONTROL->setLeft(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_ScreenX)

	GB.ReturnInteger(CONTROL->screenX());

END_PROPERTY


BEGIN_PROPERTY(Control_Y)

	if (READ_PROPERTY) {  GB.ReturnInteger(CONTROL->top()); return; }
	CONTROL->setTop(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_ScreenY)

	GB.ReturnInteger(CONTROL->screenY());

END_PROPERTY


BEGIN_PROPERTY(Control_Width)

	if (READ_PROPERTY)
		GB.ReturnInteger(CONTROL->width());
	else
		CONTROL->setWidth(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_Height)

	if (READ_PROPERTY)
		GB.ReturnInteger(CONTROL->height());
	else
		CONTROL->setHeight(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_Font)

	if (!THIS->font)
	{
		THIS->font = CFONT_create(new gFont(), 0, THIS);
		GB.Ref(THIS->font);
	}

	if (READ_PROPERTY)
	{
		CONTROL->actualFontTo(((CFONT *)THIS->font)->font);
		GB.ReturnObject(THIS->font);
	}
	else
	{
		CFONT *font = (CFONT *)VPROP(GB_OBJECT);
		if (font)
			CONTROL->setFont(font->font->copy());
		else
			CONTROL->setFont(NULL);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Design)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isDesign());
	else
	{
		bool v = VPROP(GB_BOOLEAN);
		if (v == CONTROL->isDesign())
			return;
		if (v)
			CONTROL->setDesign();
		else
			GB.Error("Design property cannot be reset");
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Visible)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isVisible());
	else
		CONTROL->setVisible(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Control_Enabled)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isEnabled());
	else
		CONTROL->setEnabled(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Control_HasFocus)

	GB.ReturnBoolean(CONTROL->hasFocus());

END_PROPERTY


BEGIN_PROPERTY(Control_Hovered)

	GB.ReturnBoolean(CONTROL->hovered());

END_PROPERTY


BEGIN_PROPERTY(Control_Expand)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isExpand());
	else
		CONTROL->setExpand(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Control_Ignore)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isIgnore());
	else
		CONTROL->setIgnore(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_METHOD(Control_Move, GB_INTEGER x; GB_INTEGER y; GB_INTEGER w; GB_INTEGER h)

	if (MISSING(w) && MISSING(h))
		CONTROL->move(VARG(x), VARG(y));
	else
		CONTROL->moveResize(VARG(x), VARG(y), VARGOPT(w, CONTROL->width()), VARGOPT(h, CONTROL->height()));

END_METHOD


BEGIN_METHOD(Control_Resize, GB_INTEGER w; GB_INTEGER h)

	CONTROL->resize(VARG(w),VARG(h));

END_METHOD

BEGIN_METHOD(Control_MoveScaled, GB_FLOAT x; GB_FLOAT y; GB_FLOAT w; GB_FLOAT h)

	int x, y, w, h;

	x = (int)(VARG(x) * MAIN_scale + 0.5);
	y = (int)(VARG(y) * MAIN_scale + 0.5);
	w = (MISSING(w) ? -1 : (int)(VARG(w) * MAIN_scale + 0.5));
	h = (MISSING(h) ? -1 : (int)(VARG(h) * MAIN_scale + 0.5));

	if (w == 0) w = 1;
	if (h == 0) h = 1;

	if (w > 0 && h > 0)
		CONTROL->moveResize(x, y, w, h);
	else
		CONTROL->move(x, y);

END_METHOD


BEGIN_METHOD(Control_ResizeScaled, GB_FLOAT w; GB_FLOAT h)

	int w, h;

	w = (int)(VARG(w) * MAIN_scale + 0.5);
	h = (int)(VARG(h) * MAIN_scale + 0.5);

	if (w == 0) w = 1;
	if (h == 0) h = 1;

	CONTROL->resize(w, h);

END_METHOD


BEGIN_METHOD_VOID(Control_Delete)

	if (CONTROL)
	{
		if (CONTROL->isDragging())
			GB.Error("Control is being dragged");
		else
			CONTROL->destroy();
	}

END_METHOD


BEGIN_METHOD_VOID(Control_Show)

	CONTROL->show();

END_METHOD


BEGIN_METHOD_VOID(Control_Hide)

	CONTROL->hide();

END_METHOD

BEGIN_METHOD(Control_Reparent, GB_OBJECT parent; GB_INTEGER x; GB_INTEGER y)

	CCONTAINER *parent = (CCONTAINER*)VARG(parent);
	int x, y;

	if (parent || !GB.Is(THIS, CLASS_Window))
	{
		if (GB.CheckObject(parent))
			return;
	}

	x = CONTROL->x();
	y = CONTROL->y();

	if (!MISSING(x) && !MISSING(y))
	{
		x = VARG(x);
		y = VARG(y);
	}

	CONTROL->reparent(parent ? CONTAINER(parent) : NULL, x, y);

END_METHOD


BEGIN_METHOD_VOID(Control_Raise)

	CONTROL->raise();

END_METHOD


BEGIN_METHOD_VOID(Control_Lower)

	CONTROL->lower();

END_METHOD


BEGIN_PROPERTY(Control_Next)

	if (READ_PROPERTY)
		GB.ReturnObject(GetObject(CONTROL->next()));
	else
	{
		CWIDGET *next = (CWIDGET *)VPROP(GB_OBJECT);
		CONTROL->setNext(next ? next->widget : NULL);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Previous)

	if (READ_PROPERTY)
		GB.ReturnObject(GetObject(CONTROL->previous()));
	else
	{
		CWIDGET *previous = (CWIDGET *)VPROP(GB_OBJECT);
		CONTROL->setPrevious(previous ? previous->widget : NULL);
	}

END_PROPERTY


BEGIN_METHOD_VOID(Control_Refresh) //, GB_INTEGER x; GB_INTEGER y; GB_INTEGER w; GB_INTEGER h)

	CONTROL->refresh();

END_METHOD


BEGIN_METHOD_VOID(Control_SetFocus)

	CONTROL->setFocus();

END_METHOD


BEGIN_PROPERTY(Control_Tag)

	if (READ_PROPERTY)
		GB.ReturnVariant(&THIS->tag);
	else
		GB.StoreVariant(PROP(GB_VARIANT), (void *)&THIS->tag);

END_METHOD


BEGIN_PROPERTY(Control_Mouse)

	if (READ_PROPERTY)
		GB.ReturnInteger(CONTROL->mouse());
	else
		CONTROL->setMouse(VPROP(GB_INTEGER));

END_METHOD


BEGIN_PROPERTY(Control_Cursor)

	if (READ_PROPERTY)
		GB.ReturnObject(THIS->cursor);
	else
	{
		GB.StoreObject(PROP(GB_OBJECT), &THIS->cursor);
		CCURSOR *c = (CCURSOR *)THIS->cursor;
		CONTROL->setCursor(c ? c->cur : NULL);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Background)

	if (CONTROL->proxy())
	{
		if (READ_PROPERTY)
			GB.GetProperty(GetObject(CONTROL->proxy()), "Background");
		else
		{
			GB_VALUE value;
			value.type = GB_T_INTEGER;
			value._integer.value = VPROP(GB_INTEGER);
			GB.SetProperty(GetObject(CONTROL->proxy()), "Background", &value);
		}

		return;
	}

	if (READ_PROPERTY)
		GB.ReturnInteger(CONTROL->background());
	else
		CONTROL->setBackground(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_Foreground)

	if (CONTROL->proxy())
	{
		if (READ_PROPERTY)
			GB.GetProperty(GetObject(CONTROL->proxy()), "Foreground");
		else
		{
			GB_VALUE value;
			value.type = GB_T_INTEGER;
			value._integer.value = VPROP(GB_INTEGER);
			GB.SetProperty(GetObject(CONTROL->proxy()), "Foreground", &value);
		}

		return;
	}

	if (READ_PROPERTY)
		GB.ReturnInteger(CONTROL->foreground());
	else
		CONTROL->setForeground(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_Parent)

	gContainer *parent = CONTROL->parent();

	if (parent)
	{
		while (parent->proxyContainerFor())
			parent = parent->proxyContainerFor();
	}

	GB.ReturnObject(GetObject(parent));

END_PROPERTY


BEGIN_PROPERTY(Control__Parent)

	GB.ReturnObject(GetObject(CONTROL->parent()));

END_PROPERTY


BEGIN_PROPERTY(Control_Window)

	GB.ReturnObject(GetObject(CONTROL->window()));

END_PROPERTY


BEGIN_PROPERTY(Control_Id)

	GB.ReturnInteger(CONTROL->handle());

END_PROPERTY


BEGIN_PROPERTY(Control_Tooltip)

	if (READ_PROPERTY)
	{
		GB.ReturnNewZeroString(CONTROL->tooltip());
		return;
	}

	CONTROL->setTooltip(GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


/*BEGIN_METHOD_VOID(CWIDGET_screenshot)

	CPICTURE *img;

	GB.New(POINTER(&img), GB.FindClass("Picture"), 0, 0);
	if (img->picture) delete img->picture;
	img->picture=CONTROL->screenshot();
	GB.ReturnObject((void*)img);

END_METHOD*/

BEGIN_METHOD_VOID(Control_Grab)

	CONTROL->grab();

END_METHOD


BEGIN_METHOD(Control_Drag, GB_VARIANT data; GB_STRING format)

	static GB_FUNCTION func;
	static bool init = FALSE;

	if (!init)
	{
		GB.GetFunction(&func, (void *)GB.FindClass("Drag"), "_call", NULL, NULL);
		init = TRUE;
	}

	GB.Push(2, GB_T_OBJECT, THIS, GB_T_VARIANT, &VARG(data));
	if (MISSING(format))
	{
		GB.Call(&func, 2, FALSE);
	}
	else
	{
		GB.Push(1, GB_T_STRING, STRING(format), LENGTH(format));
		GB.Call(&func, 3, FALSE);
	}

END_METHOD


BEGIN_PROPERTY(Control_Drop)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->acceptDrops());
	else
		CONTROL->setAcceptDrops(VPROP(GB_BOOLEAN));

END_PROPERTY

BEGIN_PROPERTY(Control_Tracking)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isTracking());
	else
		CONTROL->setTracking(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Control_Name)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(CONTROL->name());
	else
		CONTROL->setName(GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(Control_Action)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->action);
	else
	{
		CACTION_register(THIS, THIS->action, GB.ToZeroString(PROP(GB_STRING)));
		GB.StoreString(PROP(GB_STRING), &THIS->action);
	}

END_PROPERTY


BEGIN_PROPERTY(Control_Proxy)

	if (READ_PROPERTY)
		GB.ReturnObject(GetObject(CONTROL->proxy()));
	else
	{
		CWIDGET *proxy = (CWIDGET *)VPROP(GB_OBJECT);
		if (CONTROL->setProxy(proxy ? proxy->widget : NULL))
			GB.Error("Circular proxy chain");
	}

END_PROPERTY


BEGIN_PROPERTY(Control_PopupMenu)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->popup);
	else
		GB.StoreString(PROP(GB_STRING), &(THIS->popup));

END_PROPERTY


BEGIN_PROPERTY(Control_NoTabFocus)

	if (READ_PROPERTY)
		GB.ReturnBoolean(CONTROL->isNoTabFocus());
	else
		CONTROL->setNoTabFocus(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Control_Direction)

	if (READ_PROPERTY)
		GB.ReturnInteger(CONTROL->direction());
	else
		CONTROL->setDirection(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Control_RightToLeft)

	GB.ReturnBoolean(CONTROL->isRightToLeft());

END_PROPERTY

//---------------------------------------------------------------------------

GB_DESC CWidgetDesc[] =
{
	GB_DECLARE("Control", sizeof(CWIDGET)),

	GB_NOT_CREATABLE(),
	GB_HOOK_CHECK(CWIDGET_check),

	//GB_METHOD("_free", 0, Control_Delete, 0),

	GB_METHOD("Move", NULL, Control_Move, "(X)i(Y)i[(Width)i(Height)i]"),
	GB_METHOD("Resize", NULL, Control_Resize, "(Width)i(Height)i"),
	GB_METHOD("MoveScaled", NULL, Control_MoveScaled, "(X)f(Y)f[(Width)f(Height)f]"),
	GB_METHOD("ResizeScaled", NULL, Control_ResizeScaled, "(Width)f(Height)f"),
	GB_METHOD("Delete", NULL, Control_Delete, NULL),
	GB_METHOD("Show", NULL, Control_Show, NULL),
	GB_METHOD("Hide", NULL, Control_Hide, NULL),
	GB_METHOD("Reparent", NULL, Control_Reparent, "(Parent)Container;[(X)i(Y)i]"),

	GB_METHOD("Raise", NULL, Control_Raise, NULL),
	GB_METHOD("Lower", NULL, Control_Lower, NULL),

	GB_PROPERTY("Next", "Control", Control_Next),
	GB_PROPERTY("Previous", "Control", Control_Previous),

	GB_METHOD("SetFocus", NULL, Control_SetFocus, NULL),
	GB_METHOD("Refresh", NULL, Control_Refresh, NULL),
	//GB_METHOD("Screenshot", "Picture", CWIDGET_screenshot, 0),
	GB_METHOD("Grab", NULL, Control_Grab, NULL),
	GB_METHOD("Drag", "Control", Control_Drag, "(Data)v[(Format)s]"),

	GB_PROPERTY("X", "i", Control_X),
	GB_PROPERTY("Y", "i", Control_Y),
	GB_PROPERTY_READ("ScreenX", "i", Control_ScreenX),
	GB_PROPERTY_READ("ScreenY", "i", Control_ScreenY),
	GB_PROPERTY("W", "i", Control_Width),
	GB_PROPERTY("H", "i", Control_Height),
	GB_PROPERTY("Left", "i", Control_X),
	GB_PROPERTY("Top", "i", Control_Y),
	GB_PROPERTY("Width", "i", Control_Width),
	GB_PROPERTY("Height", "i", Control_Height),

	GB_PROPERTY("Visible", "b", Control_Visible),
	GB_PROPERTY("Enabled", "b", Control_Enabled),
	GB_PROPERTY_READ("HasFocus", "b", Control_HasFocus),
	GB_PROPERTY_READ("Hovered", "b", Control_Hovered),

	GB_PROPERTY("Expand", "b", Control_Expand),
	GB_PROPERTY("Ignore", "b", Control_Ignore),

	GB_PROPERTY("Font", "Font", Control_Font),
	GB_PROPERTY("Background", "i", Control_Background),
	GB_PROPERTY("Foreground", "i", Control_Foreground),

	GB_PROPERTY("Design", "b", Control_Design),
	GB_PROPERTY("Name", "s", Control_Name),
	GB_PROPERTY("Tag", "v", Control_Tag),
	GB_PROPERTY("Tracking", "b", Control_Tracking),
	GB_PROPERTY("Mouse", "i", Control_Mouse),
	GB_PROPERTY("Cursor", "Cursor", Control_Cursor),
	GB_PROPERTY("Tooltip", "s", Control_Tooltip),
	GB_PROPERTY("Drop", "b", Control_Drop),
	GB_PROPERTY("Action", "s", Control_Action),
	GB_PROPERTY("PopupMenu", "s", Control_PopupMenu),
	GB_PROPERTY("Proxy", "Control", Control_Proxy),
	GB_PROPERTY("NoTabFocus", "b", Control_NoTabFocus),
	GB_PROPERTY("Direction", "i", Control_Direction),
	GB_PROPERTY_READ("RightToLeft", "b", Control_RightToLeft),

	GB_PROPERTY_READ("Parent", "Container", Control_Parent),
	GB_PROPERTY_READ("_Parent", "Container", Control__Parent),
	GB_PROPERTY_READ("Window", "Window", Control_Window),
	GB_PROPERTY_READ("Id", "i", Control_Id),
	GB_PROPERTY_READ("Handle", "i", Control_Id),

	GB_EVENT("Enter", NULL, NULL, &EVENT_Enter),
	GB_EVENT("GotFocus", NULL, NULL, &EVENT_GotFocus),
	GB_EVENT("LostFocus", NULL, NULL, &EVENT_LostFocus),
	GB_EVENT("KeyPress", NULL, NULL, &EVENT_KeyPress),
	GB_EVENT("KeyRelease", NULL, NULL, &EVENT_KeyRelease),
	GB_EVENT("Leave", NULL, NULL, &EVENT_Leave),
	GB_EVENT("MouseDown", NULL, NULL, &EVENT_MouseDown),
	GB_EVENT("MouseMove", NULL, NULL, &EVENT_MouseMove),
	GB_EVENT("MouseDrag", NULL, NULL, &EVENT_MouseDrag),
	GB_EVENT("MouseUp", NULL, NULL, &EVENT_MouseUp),
	GB_EVENT("MouseWheel", NULL, NULL, &EVENT_MouseWheel),
	GB_EVENT("DblClick", NULL, NULL, &EVENT_DblClick),
	GB_EVENT("Menu", NULL, NULL, &EVENT_Menu),
	GB_EVENT("Drag", NULL, NULL, &EVENT_Drag),
	GB_EVENT("DragMove", NULL, NULL, &EVENT_DragMove),
	GB_EVENT("Drop", NULL, NULL, &EVENT_Drop),
	GB_EVENT("DragLeave", NULL, NULL, &EVENT_DragLeave),

	CONTROL_DESCRIPTION,

	GB_END_DECLARE
};

#ifndef GTK3
GB_DESC CPluginDesc[] =
{
	GB_DECLARE("Embedder", sizeof(CPLUGIN)), GB_INHERITS("Control"),

	GB_METHOD("_new", 0, CPLUGIN_new, "(Parent)Container;"),
	GB_METHOD("Embed", 0, CPLUGIN_embed, "(Client)i"),
	GB_METHOD("Discard", 0, CPLUGIN_discard, 0),

	GB_PROPERTY_READ("Client", "i", CPLUGIN_client),
	//GB_PROPERTY("Border", "i<Border>", CPLUGIN_border),

	GB_EVENT("Embed", NULL, NULL, &EVENT_Plugged),
	GB_EVENT("Close", NULL, NULL, &EVENT_Unplugged),
	GB_EVENT("Error", NULL, NULL, &EVENT_PlugError), /* TODO */

	EMBEDDER_DESCRIPTION,

	GB_END_DECLARE
};
#endif
