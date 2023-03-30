/***************************************************************************

	CContainer.cpp

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

#define __CCONTAINER_CPP

#include "gambas.h"
#include "CContainer.h"
#include "gpanel.h"
#include "gmainwindow.h"
#include "gapplication.h"
#include "cpaint_impl.h"

#define CALL_FUNCTION(_this, _func) \
{ \
	if ((_this) && (_this)->func._func) \
	{ \
		GB_FUNCTION func; \
		func.object = (_this); \
		func.index = (_this)->func._func; \
		GB.Call(&func, 0, TRUE); \
	} \
}


/***************************************************************************

	Container

***************************************************************************/

DECLARE_EVENT(EVENT_BeforeArrange);
DECLARE_EVENT(EVENT_Arrange);
DECLARE_EVENT(EVENT_Insert);


void CB_container_before_arrange(gContainer *sender)
{
	GB.Raise(sender->hFree, EVENT_BeforeArrange, 0);
}

void CB_container_arrange(gContainer *sender)
{
	GB.Raise(sender->hFree, EVENT_Arrange, 0);
}

void CCONTAINER_raise_insert(CCONTAINER *_object, CWIDGET *child)
{
	GB.Raise(THIS, EVENT_Insert, 1, GB_T_OBJECT, child);
}

#ifdef GTK3

static void cleanup_drawing(intptr_t arg1, intptr_t arg2)
{
	PAINT_end();
}

void CUSERCONTROL_cb_draw(gContainer *sender, cairo_t *cr)
{
	CWIDGET *_object = GetObject(sender);
	GB_ERROR_HANDLER handler;
	
	if (THIS_USERCONTAINER->func.paint)
	{
		cairo_t *save = THIS_USERCONTROL->func.context;
		THIS_USERCONTROL->func.context = cr;

		PAINT_begin(THIS);

		handler.handler = (GB_CALLBACK)cleanup_drawing;
		GB.OnErrorBegin(&handler);
		CALL_FUNCTION(THIS_USERCONTROL, paint);
		GB.OnErrorEnd(&handler);

		PAINT_end();

		THIS_USERCONTROL->func.context = save;
	}
}

#else

static void cleanup_drawing(cairo_t *cr, intptr_t arg2)
{
	cairo_restore(cr);
	PAINT_end();
}

void CUSERCONTROL_cb_draw(gContainer *sender, GdkRegion *region, int dx, int dy)
{
	CWIDGET *_object = GetObject(sender);
	GB_ERROR_HANDLER handler;
	cairo_t *cr;
	
	if (THIS_USERCONTAINER->func.paint)
	{
		PAINT_begin(THIS);

		cr = PAINT_get_current_context();
		cairo_save(cr);
		PAINT_clip(0, 0, sender->width(), sender->height());

		handler.handler = (GB_CALLBACK)cleanup_drawing;
		handler.arg1 = (intptr_t)cr;
		GB.OnErrorBegin(&handler);
		CALL_FUNCTION(THIS_USERCONTROL, paint);
		GB.OnErrorEnd(&handler);

		cairo_restore(cr);
		PAINT_end();
	}
}

#endif

void CUSERCONTROL_cb_font(gContainer *sender)
{
	CWIDGET *_object = GetObject(sender);
	CALL_FUNCTION(THIS_USERCONTROL, font);
}

static bool cb_change_filter(gControl *control)
{
	return control->isContainer() && ((gContainer *)control)->isUser();
}

static void cb_change(gControl *control)
{
	CWIDGET *_object = GetObject(control);
	CALL_FUNCTION(THIS_USERCONTROL, change);
}

void CUSERCONTROL_send_change_event(void)
{
	gApplication::forEachControl(cb_change, cb_change_filter);
}

static void get_client_area(gContainer *cont, int *x, int *y, int *w, int *h)
{
	gContainer *proxy = cont->proxyContainer();
	
	if (x) *x = proxy->clientX();
	if (y) *y = proxy->clientY();
	if (w) *w = proxy->clientWidth();
	if (h) *h = proxy->clientHeight();
	
	if (x || y)
	{
		while (proxy && proxy != cont)
		{
			if (x) *x += proxy->x();
			if (y) *y += proxy->y();
			proxy = proxy->parent();
		}
	}
}

void CUSERCONTROL_cb_resize(gContainer *sender)
{
	CWIDGET *_object = GetObject(sender);
	CALL_FUNCTION(THIS_USERCONTROL, resize);
}

//-------------------------------------------------------------------------

BEGIN_PROPERTY(Container_ClientX)

	int x;
	get_client_area(WIDGET, &x, NULL, NULL, NULL);
	GB.ReturnInteger(x);

END_PROPERTY


BEGIN_PROPERTY(Container_ClientY)

	int y;
	get_client_area(WIDGET, NULL, &y, NULL, NULL);
	GB.ReturnInteger(y);

END_PROPERTY


BEGIN_PROPERTY(Container_ClientWidth)

	int w;
	get_client_area(WIDGET, NULL, NULL, &w, NULL);
	GB.ReturnInteger(w);


END_PROPERTY


BEGIN_PROPERTY(Container_ClientHeight)

	int h;
	get_client_area(WIDGET, NULL, NULL, NULL, &h);
	GB.ReturnInteger(h);
	
END_PROPERTY


BEGIN_PROPERTY(Container_Arrangement)

	if (READ_PROPERTY) { GB.ReturnInteger(WIDGET->arrange()); return; }
	WIDGET->setArrange(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Container_AutoResize)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->autoResize());
	else
		WIDGET->setAutoResize(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Container_Padding)

	if (READ_PROPERTY) { GB.ReturnInteger(WIDGET->padding()); return; }
	WIDGET->setPadding(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(Container_Spacing)

	if (READ_PROPERTY) { GB.ReturnBoolean(WIDGET->spacing()); return; }
	WIDGET->setSpacing(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Container_Margin)

	if (READ_PROPERTY) { GB.ReturnBoolean(WIDGET->margin()); return; }
	WIDGET->setMargin(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Container_Indent)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->indent());
	else
		WIDGET->setIndent(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Container_Centered)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->centered());
	else
		WIDGET->setCentered(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Container_Invert)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->invert());
	else
		WIDGET->setInvert(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_METHOD(Container_FindChild, GB_INTEGER x; GB_INTEGER y)

	gControl *child = WIDGET->proxyContainer()->find(VARG(x), VARG(y));
	
	if (child)
		GB.ReturnObject(child->hFree);
	else
		GB.ReturnNull();

END_METHOD


BEGIN_METHOD(Container_unknown, GB_VALUE x; GB_VALUE y)

	char *name = GB.GetUnknown();
	int nparam = GB.NParam();
	
	if (strcasecmp(name, "Find"))
	{
		GB.Error(GB_ERR_NSYMBOL, GB.GetClassName(NULL), name);
		return;
	}
	
	if (nparam < 2)
	{
		GB.Error("Not enough argument");
		return;
	}
	else if (nparam > 2)
	{
		GB.Error("Too many argument");
		return;
	}
	
	GB.Deprecated(GTK_NAME, "Container.Find", "Container.FindChild");
	
	if (GB.Conv(ARG(x), GB_T_INTEGER) || GB.Conv(ARG(y), GB_T_INTEGER))
		return;
	
	Container_FindChild(_object, _param);
	
	GB.ReturnConvVariant();

END_METHOD


/*BEGIN_PROPERTY(Container_Dirty)

	GB.ReturnBoolean(WIDGET->isDirty());

END_PROPERTY*/

//---------------------------------------------------------------------------


BEGIN_PROPERTY(Container_Children)

	CCONTAINERCHILDREN *children = (CCONTAINERCHILDREN *)GB.New(CLASS_ContainerChildren, NULL, NULL);
	gContainer *cont = WIDGET->proxyContainer();
	gControl *child;
	int i;
	
	children->container = THIS;
	GB.Ref(THIS);
	
	GB.NewArray(POINTER(&children->children), sizeof(void *), 0);
	
	for (i = 0; i < cont->childCount(); i++)
	{
		child = cont->child(i);
		if (!child->hFree || child->isDestroyed())
			continue;
		GB.Ref(child->hFree);
		*(void **)GB.Add(&children->children) = child->hFree;
	}
	
	GB.ReturnObject(children);

END_PROPERTY


BEGIN_METHOD_VOID(ContainerChildren_free)

	int i;
	CWIDGET **array = THIS_CHILDREN->children;

	for (i = 0; i < GB.Count(array); i++)
		GB.Unref(POINTER(&array[i]));
	
	GB.FreeArray(&THIS_CHILDREN->children);
	GB.Unref(POINTER(&THIS_CHILDREN->container));

END_METHOD


BEGIN_PROPERTY(ContainerChildren_Count)

	GB.ReturnInteger(GB.Count(THIS_CHILDREN->children));

END_PROPERTY


BEGIN_PROPERTY(ContainerChildren_Max)

	GB.ReturnInteger(GB.Count(THIS_CHILDREN->children) - 1);

END_PROPERTY


BEGIN_METHOD(ContainerChildren_get, GB_INTEGER index)

	CWIDGET **array = THIS_CHILDREN->children;
	int index = VARG(index);

	if (index < 0 || index >= GB.Count(array))
	{
		GB.Error(GB_ERR_BOUND);
		return;
	}
	
	GB.ReturnObject(array[index]);

END_METHOD


BEGIN_METHOD_VOID(ContainerChildren_next)

	CWIDGET **array = THIS_CHILDREN->children;
	int index;

	index = ENUM(int);

	if (index >= GB.Count(array))
		GB.StopEnum();
	else
	{
		ENUM(int) = index + 1;
		GB.ReturnObject(array[index]);
	}

END_METHOD


BEGIN_METHOD_VOID(ContainerChildren_Clear)

	((gContainer *)(THIS_CHILDREN->container->ob.widget))->clear();
	
END_METHOD


//---------------------------------------------------------------------------


GB_DESC ContainerChildrenDesc[] =
{
	GB_DECLARE("ContainerChildren", sizeof(CCONTAINER)), GB_NOT_CREATABLE(),

	GB_METHOD("_free", NULL, ContainerChildren_free, NULL),
	GB_METHOD("_next", "Control", ContainerChildren_next, NULL),
	GB_METHOD("_get", "Control", ContainerChildren_get, "(Index)i"),
	GB_PROPERTY_READ("Count", "i", ContainerChildren_Count),
	GB_PROPERTY_READ("Max", "i", ContainerChildren_Max),
	GB_METHOD("Clear", NULL, ContainerChildren_Clear, NULL),

	GB_END_DECLARE
};


GB_DESC ContainerDesc[] =
{
	GB_DECLARE("Container", sizeof(CCONTAINER)), GB_INHERITS("Control"),

	GB_NOT_CREATABLE(),

	GB_PROPERTY_READ("Children", "ContainerChildren", Container_Children),

	//GB_PROPERTY_READ("Dirty", "b", Container_Dirty),

	GB_PROPERTY_READ("ClientX", "i", Container_ClientX),
	GB_PROPERTY_READ("ClientY", "i", Container_ClientY),
	GB_PROPERTY_READ("ClientW", "i", Container_ClientWidth),
	GB_PROPERTY_READ("ClientWidth", "i", Container_ClientWidth),
	GB_PROPERTY_READ("ClientH", "i", Container_ClientHeight),
	GB_PROPERTY_READ("ClientHeight", "i", Container_ClientHeight),

	GB_METHOD("FindChild", "Control", Container_FindChild, "(X)i(Y)i"),
	GB_METHOD("_unknown", "v", Container_unknown, "."),

	CONTAINER_DESCRIPTION,

	GB_EVENT("BeforeArrange", NULL, NULL, &EVENT_BeforeArrange),
	GB_EVENT("Arrange", NULL, NULL, &EVENT_Arrange),
	GB_EVENT("NewChild", NULL, "(Child)Control", &EVENT_Insert),
	
	GB_END_DECLARE
};

/****************************************************************************

	UserControl & UserContainer

****************************************************************************/

static void declare_special_event_handler(void *_object, ushort *index, const char *suffix)
{
	GB_FUNCTION func;
	char name[128];

	snprintf(name, sizeof(name), "%s_%s", GB.Is(THIS, CLASS_UserContainer) ? "UserContainer" : "UserControl", suffix);
	if (!GB.GetFunction(&func, THIS, name, NULL, NULL))
		*index = func.index;
}

BEGIN_METHOD(UserControl_new, GB_OBJECT parent)

	InitControl(new gPanel(CONTAINER(VARG(parent))), (CWIDGET*)THIS);
	
	PANEL->setArrange(ARRANGE_FILL);
	PANEL->setUser();
	
	if (GB.Is(THIS, CLASS_UserContainer))
		PANEL->setUserContainer();

	declare_special_event_handler(THIS, &THIS_USERCONTROL->func.paint, "Draw");
	declare_special_event_handler(THIS, &THIS_USERCONTROL->func.font, "Font");
	declare_special_event_handler(THIS, &THIS_USERCONTROL->func.change, "Change");
	declare_special_event_handler(THIS, &THIS_USERCONTROL->func.resize, "Resize");

	GB.Error(NULL);

END_METHOD

	
BEGIN_PROPERTY(UserControl_Container)

	CCONTAINER *cont;
	gControl *p;
	gContainer *w;
	
	if (READ_PROPERTY)
	{
		GB.ReturnObject(GetObject(WIDGET->proxyContainer()));
		return;
	}
	
	cont = (CCONTAINER*)VPROP(GB_OBJECT);
	
	if (!cont)
	{
		if (WIDGET->proxyContainer() != WIDGET)
			WIDGET->proxyContainer()->setProxyContainerFor(NULL);
		WIDGET->setProxyContainer(NULL);
		
		WIDGET->setProxy(NULL);
		return;
	}
	
	if (GB.CheckObject(cont)) 
		return;
	
	w = ((gContainer *)cont->ob.widget)->proxyContainer();
	/*while (w->proxyContainer() != w)
		w = w->proxyContainer();*/
	
	if (w == WIDGET->proxyContainer())
		return;
	
	for (p = cont->ob.widget; p; p = p->parent())
	{
		if (p == WIDGET)
			break;
	}
	
	if (!p)
	{
		GB.Error("Container must be a child control");
		return;
	}

	gColor bg = WIDGET->proxyContainer()->background();
	gColor fg = WIDGET->proxyContainer()->foreground();

	if (WIDGET->proxyContainer() != WIDGET)
		WIDGET->proxyContainer()->setProxyContainerFor(NULL);

	WIDGET->setProxyContainer(w);
	w->setProxyContainerFor(WIDGET);
	
	w->setBackground(bg);
	w->setForeground(fg);
	
	WIDGET->performArrange();
	WIDGET->setProxy((gContainer *)cont->ob.widget);
	
END_PROPERTY


BEGIN_PROPERTY(UserControl_Focus)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->canFocus());
	else
		WIDGET->setCanFocus(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(UserContainer_Container)

	if (READ_PROPERTY)
		UserControl_Container(_object, _param);
	else
	{
		UserControl_Container(_object, _param);

		WIDGET->proxyContainer()->setFullArrangement(&THIS_USERCONTAINER->save);
	}

END_PROPERTY


BEGIN_PROPERTY(UserContainer_Arrangement)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->proxyContainer()->arrange());
	else
	{
		WIDGET->proxyContainer()->setArrange(VPROP(GB_INTEGER));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}

END_PROPERTY


BEGIN_PROPERTY(UserContainer_AutoResize)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->proxyContainer()->autoResize());
	else
	{
		WIDGET->proxyContainer()->setAutoResize(VPROP(GB_BOOLEAN));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY


BEGIN_PROPERTY(UserContainer_Padding)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->proxyContainer()->padding());
	else
	{
		WIDGET->proxyContainer()->setPadding(VPROP(GB_INTEGER));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY


BEGIN_PROPERTY(UserContainer_Spacing)
	
	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->proxyContainer()->spacing());
	else
	{
		WIDGET->proxyContainer()->setSpacing(VPROP(GB_BOOLEAN));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY


BEGIN_PROPERTY(UserContainer_Margin)
	
	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->proxyContainer()->margin());
	else
	{
		WIDGET->proxyContainer()->setMargin(VPROP(GB_BOOLEAN));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY


BEGIN_PROPERTY(UserContainer_Indent)
	
	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->proxyContainer()->indent());
	else
	{
		WIDGET->proxyContainer()->setIndent(VPROP(GB_BOOLEAN));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY


BEGIN_PROPERTY(UserContainer_Centered)
	
	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->proxyContainer()->centered());
	else
	{
		WIDGET->proxyContainer()->setCentered(VPROP(GB_BOOLEAN));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY


BEGIN_PROPERTY(UserContainer_Invert)
	
	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->proxyContainer()->invert());
	else
	{
		WIDGET->proxyContainer()->setInvert(VPROP(GB_BOOLEAN));
		THIS_USERCONTAINER->save = WIDGET->proxyContainer()->fullArrangement();
	}
	
END_PROPERTY

//---------------------------------------------------------------------------

GB_DESC UserControlDesc[] =
{
	GB_DECLARE("UserControl", sizeof(CUSERCONTROL)), GB_INHERITS("Container"),
	GB_NOT_CREATABLE(),

	GB_METHOD("_new", NULL, UserControl_new, "(Parent)Container;"),
	GB_PROPERTY("_Container", "Container", UserControl_Container),
	GB_PROPERTY("_AutoResize", "b", Container_AutoResize),
	GB_PROPERTY("_Arrangement", "i", Container_Arrangement),
	GB_PROPERTY("_Padding", "i", Container_Padding),
	GB_PROPERTY("_Spacing", "b", Container_Spacing),
	GB_PROPERTY("_Margin", "b", Container_Margin),
	GB_PROPERTY("_Indent", "b", Container_Indent),
	GB_PROPERTY("_Invert", "b", Container_Invert),
	GB_PROPERTY("_Centered", "b", Container_Centered),
	GB_PROPERTY("_Focus", "b", UserControl_Focus),

	USERCONTROL_DESCRIPTION,

	GB_INTERFACE("Paint", &PAINT_Interface),

	GB_END_DECLARE
};

GB_DESC UserContainerDesc[] =
{
	GB_DECLARE("UserContainer", sizeof(CUSERCONTAINER)), GB_INHERITS("Container"),
	GB_NOT_CREATABLE(),

	GB_METHOD("_new", NULL, UserControl_new, "(Parent)Container;"),

	GB_PROPERTY("_Container", "Container", UserContainer_Container),
	GB_PROPERTY("_Arrangement", "i", Container_Arrangement),

	GB_PROPERTY("Arrangement", "i", UserContainer_Arrangement),
	GB_PROPERTY("AutoResize", "b", UserContainer_AutoResize),
	GB_PROPERTY("Padding", "i", UserContainer_Padding),
	GB_PROPERTY("Spacing", "b", UserContainer_Spacing),
	GB_PROPERTY("Margin", "b", UserContainer_Margin),
	GB_PROPERTY("Indent", "b", UserContainer_Indent),
	GB_PROPERTY("Invert", "b", UserContainer_Invert),
	GB_PROPERTY("Centered", "b", UserContainer_Centered),
	
	USERCONTAINER_DESCRIPTION,

	GB_INTERFACE("Paint", &PAINT_Interface),

	GB_END_DECLARE
};

