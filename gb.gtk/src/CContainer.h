/***************************************************************************

	CContainer.h

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

#ifndef __CCONTAINER_H
#define __CCONTAINER_H


#include "main.h"
#include "gambas.h"
#include "widgets.h"
#include "CWidget.h"

#ifndef __CCONTAINER_CPP

extern GB_DESC ContainerChildrenDesc[];
extern GB_DESC ContainerDesc[];
extern GB_DESC UserControlDesc[];
extern GB_DESC UserContainerDesc[];

#else

#define THIS ((CCONTAINER *)_object)
#define THIS_CHILDREN ((CCONTAINERCHILDREN *)_object)
#define THIS_USERCONTAINER ((CUSERCONTAINER *)_object)
#define THIS_USERCONTROL ((CUSERCONTROL *)_object)
#define WIDGET ((gContainer*)THIS->ob.widget)
#define PANEL ((gPanel *)(THIS->ob.widget))

#endif

#define ARRANGEMENT_FLAG_PROPERTIES \
	GB_PROPERTY("AutoResize", "b", Container_AutoResize), \
	GB_PROPERTY("Padding", "i", Container_Padding), \
	GB_PROPERTY("Spacing", "b", Container_Spacing), \
	GB_PROPERTY("Margin", "b", Container_Margin), \
	GB_PROPERTY("Indent", "b", Container_Indent), \
	GB_PROPERTY("Invert", "b", Container_Invert), \
	GB_PROPERTY("Centered", "b", Container_Centered)

#define ARRANGEMENT_PROPERTIES \
	GB_PROPERTY("Arrangement", "i", Container_Arrangement), \
	ARRANGEMENT_FLAG_PROPERTIES

typedef 
	struct
	{
		CWIDGET ob;
	}  
	CCONTAINER;

typedef 
	struct
	{
		GB_BASE ob;
		CCONTAINER *container;
		CWIDGET **children;
	}  
	CCONTAINERCHILDREN;

typedef
	struct {
	#ifdef GTK3
		cairo_t *context;
	#endif
		ushort paint;
		ushort font;
		ushort change;
		ushort resize;
	}
	CUSERCONTROL_FUNC;

typedef
	struct
	{
		CWIDGET widget;
		CUSERCONTROL_FUNC func;
	}
	CUSERCONTROL;

typedef
	struct
	{
		CWIDGET widget;
		CUSERCONTROL_FUNC func;
		gContainerArrangement save;
	}
	CUSERCONTAINER;

DECLARE_PROPERTY(Container_ClientX);
DECLARE_PROPERTY(Container_ClientY);
DECLARE_PROPERTY(Container_ClientWidth);
DECLARE_PROPERTY(Container_ClientHeight);
DECLARE_PROPERTY(Container_Arrangement);
DECLARE_PROPERTY(Container_AutoResize);
DECLARE_PROPERTY(Container_Padding);
DECLARE_PROPERTY(Container_Spacing);
DECLARE_PROPERTY(Container_Margin);
DECLARE_PROPERTY(Container_Indent);
DECLARE_PROPERTY(Container_Invert);
DECLARE_PROPERTY(Container_Centered);

void CCONTAINER_cb_arrange(gContainer *sender);
void CCONTAINER_cb_before_arrange(gContainer *sender);
void CCONTAINER_raise_insert(CCONTAINER *_object, CWIDGET *child);
void CUSERCONTROL_send_change_event();


#endif
