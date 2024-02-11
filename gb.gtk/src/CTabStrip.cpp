/***************************************************************************

	CTabStrip.cpp

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

#define __CTABSTRIP_CPP

#include "CPicture.h"
#include "CContainer.h"
#include "CFont.h"
#include "CTabStrip.h"

DECLARE_EVENT(EVENT_Click);
DECLARE_EVENT(EVENT_Close);

static void raise_click(CTABSTRIP *_object)
{
	GB.Raise(THIS, EVENT_Click, 0);
	GB.Unref(POINTER(&_object));
}

void CB_tabstrip_click(gTabStrip *sender)
{
  CB_GET_OBJECT(sender);
	
	if (GB.IsRaiseLocked(_object))
		return;
	
	GB.Ref(THIS);
	GB.Post((GB_CALLBACK)raise_click, (long)THIS);
}

void CB_tabstrip_close(gTabStrip *sender, int index)
{
  CB_GET_OBJECT(sender);
	GB.Raise(THIS, EVENT_Close, 1, GB_T_INTEGER, index);
}

/***************************************************************************

	TabStrip

***************************************************************************/

BEGIN_METHOD(TabStrip_new, GB_OBJECT parent)

	InitControl(new gTabStrip(CONTAINER(VARG(parent))), (CWIDGET*)THIS);
	CB_tabstrip_click(TABSTRIP);

END_METHOD


BEGIN_METHOD_VOID(TabStrip_free)

	GB.Unref(POINTER(&THIS->textFont));

END_METHOD


BEGIN_PROPERTY(TabStrip_Count)

	if (READ_PROPERTY) { GB.ReturnInteger(TABSTRIP->count()); return; }
	
	if (VPROP(GB_INTEGER) < 1 || VPROP(GB_INTEGER) > 255)
	{
		GB.Error("Bad argument");
		return;
	}
	
	if (TABSTRIP->setCount(VPROP(GB_INTEGER))) 
		GB.Error("Tab is not empty"); 

END_PROPERTY



BEGIN_PROPERTY(TabStrip_Index)

	if (READ_PROPERTY) { GB.ReturnInteger(TABSTRIP->index()); return; }
	if ( (VPROP(GB_INTEGER)<0) || (VPROP(GB_INTEGER)>=TABSTRIP->count()) )
	{
		GB.Error("Bad index");
		return;
	}
	TABSTRIP->setIndex(VPROP(GB_INTEGER));
	

END_PROPERTY


BEGIN_PROPERTY(TabStrip_Current)

	THIS->index = TABSTRIP->index();
	RETURN_SELF();

END_PROPERTY


BEGIN_METHOD(TabStrip_get, GB_INTEGER index)

	if ( (VARG(index)<0) || (VARG(index)>=TABSTRIP->count()) )
	{
		GB.Error("Bad index");
		return;
	}
	
	THIS->index=VARG(index);
	RETURN_SELF();

END_METHOD


BEGIN_PROPERTY(TabStrip_Orientation)

	if (READ_PROPERTY)
		switch (TABSTRIP->orientation())
		{ 
			case GTK_POS_TOP: GB.ReturnInteger(ALIGN_TOP); break;
			case GTK_POS_BOTTOM: GB.ReturnInteger(ALIGN_BOTTOM); break;
			case GTK_POS_LEFT: GB.ReturnInteger(ALIGN_LEFT); break;
			case GTK_POS_RIGHT: GB.ReturnInteger(ALIGN_RIGHT); break;
			default: GB.ReturnInteger(ALIGN_NORMAL); break;
		}
		
	else 
				switch (VPROP(GB_INTEGER))
				{
		case ALIGN_TOP: TABSTRIP->setOrientation(GTK_POS_TOP); break;
		case ALIGN_BOTTOM: TABSTRIP->setOrientation(GTK_POS_BOTTOM); break;
		case ALIGN_LEFT: TABSTRIP->setOrientation(GTK_POS_LEFT); break;
		case ALIGN_RIGHT: TABSTRIP->setOrientation(GTK_POS_RIGHT); break;
		
				}
				
END_PROPERTY



/***************************************************************************

	.Tab

***************************************************************************/

BEGIN_PROPERTY(TabStripContainer_Text)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(TABSTRIP->tabText(THIS->index));
	else
		TABSTRIP->setTabText(THIS->index, GB.ToZeroString(PROP(GB_STRING)));
		
END_PROPERTY


BEGIN_PROPERTY(TabStrip_TextFont)

	if (READ_PROPERTY)
		GB.ReturnObject(THIS->textFont);
	else
	{
		GB.StoreObject(PROP(GB_OBJECT), POINTER(&THIS->textFont));
		CFONT *font = (CFONT *)THIS->textFont;
		if (font)
			TABSTRIP->setTextFont(font->font);
		else
			TABSTRIP->setTextFont(0);
	}

END_PROPERTY


BEGIN_PROPERTY(TabStripContainer_Picture)

	if (READ_PROPERTY)
	{
		gPicture *pic = TABSTRIP->tabPicture(THIS->index);;
		GB.ReturnObject(pic ? pic->getTagValue() : 0);
	}
	else
	{
		CPICTURE *pic = (CPICTURE *)VPROP(GB_OBJECT);
		TABSTRIP->setTabPicture(THIS->index, pic ? pic->picture : 0);
	}

END_PROPERTY


BEGIN_PROPERTY(TabStripContainer_Enabled)

	if (READ_PROPERTY)
		GB.ReturnBoolean(TABSTRIP->tabEnabled(THIS->index));
	else
		TABSTRIP->setTabEnabled(THIS->index,VPROP(GB_BOOLEAN));		

END_PROPERTY


BEGIN_METHOD_VOID(TabStripContainer_next)

	int *ct = (int *)GB.GetEnum();
	
	if (*ct >= TABSTRIP->tabCount(THIS->index)) 
	{ 
		GB.StopEnum(); 
		return; 
	}
	
	GB.ReturnObject(GetObject(TABSTRIP->tabChild(THIS->index, *ct)));
	(*ct)++;

END_METHOD

BEGIN_METHOD(TabStripContainer_get, GB_INTEGER index)

	int index = VARG(index);
	
	if (index < 0 || index >= TABSTRIP->tabCount(THIS->index))
	{
		GB.Error(GB_ERR_BOUND);
		return;
	}

	GB.ReturnObject(GetObject(TABSTRIP->tabChild(THIS->index, index)));

END_METHOD

BEGIN_PROPERTY(TabStripContainer_Count)

	GB.ReturnInteger(TABSTRIP->tabCount(THIS->index));

END_PROPERTY


BEGIN_PROPERTY(TabStrip_Text)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(TABSTRIP->tabText(TABSTRIP->index()));
	else
		TABSTRIP->setTabText(TABSTRIP->index(), GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(TabStrip_Picture)

	int index = TABSTRIP->index();

	if (READ_PROPERTY)
	{
		gPicture *pic = TABSTRIP->tabPicture(index);
		GB.ReturnObject(pic ? pic->getTagValue() : 0);
	}
	else
	{
		CPICTURE *pic = (CPICTURE *)VPROP(GB_OBJECT);
		TABSTRIP->setTabPicture(index, pic ? pic->picture : 0);
	}

END_PROPERTY


BEGIN_PROPERTY(TabStrip_Closable)

	if (READ_PROPERTY)
		GB.ReturnBoolean(TABSTRIP->isClosable());
	else
		TABSTRIP->setClosable(VPROP(GB_BOOLEAN));
	
END_PROPERTY


BEGIN_METHOD_VOID(TabStripContainer_Delete)

	if (TABSTRIP->removeTab(THIS->index))
		GB.Error("Tab is not empty");

END_METHOD

BEGIN_PROPERTY(TabStripContainer_Visible)

	if (READ_PROPERTY)
		GB.ReturnBoolean(TABSTRIP->tabVisible(THIS->index));
	else
		TABSTRIP->setTabVisible(THIS->index, VPROP(GB_BOOLEAN));		

END_PROPERTY

BEGIN_METHOD(TabStrip_FindIndex, GB_OBJECT child)

	CWIDGET *child = (CWIDGET *)VARG(child);
	
	if (GB.CheckObject(child))
		return;
	
	GB.ReturnInteger(TABSTRIP->findIndex(child->widget));
	
END_METHOD


/***************************************************************************

	Descriptions

***************************************************************************/

GB_DESC CTabStripContainerChildrenDesc[] =
{
	GB_DECLARE_VIRTUAL(".TabStripContainer.Children"),

	GB_METHOD("_next", "Control", TabStripContainer_next, NULL),
	GB_PROPERTY_READ("Count", "i", TabStripContainer_Count),
	GB_METHOD("_get", "Control", TabStripContainer_get, "(Index)i"),

	GB_END_DECLARE
};


GB_DESC CTabStripContainerDesc[] =
{
	GB_DECLARE_VIRTUAL(".TabStripContainer"),

	GB_PROPERTY("Text", "s", TabStripContainer_Text),
	GB_PROPERTY("Picture", "Picture", TabStripContainer_Picture),
	GB_PROPERTY("Caption", "s", TabStripContainer_Text),
	GB_PROPERTY("Enabled", "b", TabStripContainer_Enabled),
	GB_PROPERTY("Visible", "b", TabStripContainer_Visible),
	GB_PROPERTY_SELF("Children", ".TabStripContainer.Children"),
	GB_METHOD("Delete", NULL, TabStripContainer_Delete, 0),

	GB_END_DECLARE
};


GB_DESC CTabStripDesc[] =
{
	GB_DECLARE("TabStrip", sizeof(CTABSTRIP)), GB_INHERITS("Container"),

	GB_METHOD("_new", NULL, TabStrip_new, "(Parent)Container;"),
	GB_METHOD("_free", NULL, TabStrip_free, NULL),

	GB_PROPERTY("Count", "i", TabStrip_Count),
	GB_PROPERTY("Text", "s", TabStrip_Text),
	GB_PROPERTY("TextFont", "Font", TabStrip_TextFont),
	GB_PROPERTY("Picture", "Picture", TabStrip_Picture),
	GB_PROPERTY("Closable", "b", TabStrip_Closable),
	GB_PROPERTY("Caption", "s", TabStrip_Text),
	GB_PROPERTY_READ("Current", ".TabStripContainer", TabStrip_Current),
	GB_PROPERTY("Index", "i", TabStrip_Index),
	GB_PROPERTY("Orientation", "i", TabStrip_Orientation),
	
	GB_PROPERTY_READ("ClientX", "i", Container_ClientX),
	GB_PROPERTY_READ("ClientY", "i", Container_ClientY),
	GB_PROPERTY_READ("ClientW", "i", Container_ClientWidth),
	GB_PROPERTY_READ("ClientWidth", "i", Container_ClientWidth),
	GB_PROPERTY_READ("ClientH", "i", Container_ClientHeight),
	GB_PROPERTY_READ("ClientHeight", "i", Container_ClientHeight),

	ARRANGEMENT_PROPERTIES,
	
	GB_METHOD("_get", ".TabStripContainer", TabStrip_get, "(Index)i"),
	GB_METHOD("FindIndex", "i", TabStrip_FindIndex, "(Child)Control;"),

	GB_EVENT("Click", NULL, NULL, &EVENT_Click),
	GB_EVENT("Close", NULL, "(Index)i", &EVENT_Close),
	
	TABSTRIP_DESCRIPTION,

	GB_END_DECLARE
};

