/***************************************************************************

	CTextBox.cpp

	(c) 2004-2005 - Daniel Campos Fernández <dcamposf@gmail.com>

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

#define __CTEXTBOX_CPP

#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "gambas.h"
#include "widgets.h"
#include "CTextBox.h"
#include "CWidget.h"
#include "CContainer.h"


DECLARE_EVENT(EVENT_Change);
DECLARE_EVENT(EVENT_Activate);
DECLARE_EVENT(EVENT_Cursor);


void CB_textbox_change(gTextBox *sender)
{
	CWIDGET *_object = GetObject((gControl*)sender);
	GB.Raise(THIS, EVENT_Change, 0);
}

void CB_textbox_activate(gTextBox *sender)
{
	CWIDGET *_object = GetObject((gControl*)sender);
	GB.Raise(THIS, EVENT_Activate, 0);
}

void CB_textbox_cursor(gTextBox *sender)
{
	CWIDGET *_object = GetObject((gControl*)sender);
	GB.Raise(THIS, EVENT_Cursor, 0);
}

/***************************************************************************

	TextBox

***************************************************************************/


BEGIN_METHOD(TextBox_new, GB_OBJECT parent)

	InitControl(new gTextBox(CONTAINER(VARG(parent))), (CWIDGET*)THIS);
	
END_METHOD


BEGIN_METHOD_VOID(TextBox_Clear)

	TEXTBOX->clear();

END_METHOD


BEGIN_METHOD(TextBox_Insert, GB_STRING text)

	TEXTBOX->insert(STRING(text),LENGTH(text));

END_METHOD


BEGIN_PROPERTY(TextBox_Text)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(TEXTBOX->text());
	else
		TEXTBOX->setText(GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Placeholder)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(TEXTBOX->placeholder());
	else
		TEXTBOX->setPlaceholder(GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Length)

	GB.ReturnInteger(TEXTBOX->length());

END_PROPERTY


BEGIN_PROPERTY(TextBox_Alignment)

	if (READ_PROPERTY) { GB.ReturnInteger(TEXTBOX->alignment()); return; }
	TEXTBOX->setAlignment(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Pos)

	if (READ_PROPERTY) { GB.ReturnInteger(TEXTBOX->position()); return; }
	TEXTBOX->setPosition(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(TextBox_ReadOnly)

	if (READ_PROPERTY) { GB.ReturnBoolean(TEXTBOX->isReadOnly()); return; }
	TEXTBOX->setReadOnly(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Border)

	if (READ_PROPERTY) { GB.ReturnBoolean(TEXTBOX->hasBorder()); return; }
	TEXTBOX->setBorder(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Password)

	if (READ_PROPERTY) { GB.ReturnBoolean(TEXTBOX->password()); return; }
	TEXTBOX->setPassword(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(TextBox_MaxLength)

	if (READ_PROPERTY) { GB.ReturnInteger(TEXTBOX->maxLength()); return; }
	TEXTBOX->setMaxLength(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_METHOD_VOID(TextBox_Selected)

	GB.ReturnBoolean(TEXTBOX->isSelected());

END_METHOD


BEGIN_METHOD(TextBox_CursorAt, GB_INTEGER pos)

	int x, y;
	
	TEXTBOX->getCursorPos(&x, &y, VARGOPT(pos, -1));
	GB.ReturnObject(GEOM.CreatePoint(x, y));
	
END_PROPERTY

/***************************************************************************

	.TextBox.Selection

***************************************************************************/

BEGIN_PROPERTY(TextBox_Selection_Text)

	char *buf;
	
	if (READ_PROPERTY)
	{
		buf=TEXTBOX->selText();
		GB.ReturnNewZeroString(buf);
		g_free(buf);
		return;
	}
	
	buf=GB.ToZeroString(PROP(GB_STRING));
	TEXTBOX->setSelText(buf,strlen(buf));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Selection_Length)

	GB.ReturnInteger(TEXTBOX->selLength());

END_PROPERTY


BEGIN_PROPERTY(TextBox_Selection_Start)

	GB.ReturnInteger(TEXTBOX->selStart());

END_PROPERTY


BEGIN_METHOD_VOID(TextBox_Unselect)

	TEXTBOX->selClear();

END_METHOD

BEGIN_METHOD_VOID(TextBox_SelectAll)

	TEXTBOX->selectAll();

END_METHOD

BEGIN_METHOD(TextBox_Select, GB_INTEGER start; GB_INTEGER length)

	TEXTBOX->select(VARG(start),VARG(length));

END_METHOD


//-------------------------------------------------------------------------

GB_DESC CTextBoxSelectionDesc[] =
{
	GB_DECLARE(".TextBox.Selection", 0), GB_VIRTUAL_CLASS(),

	GB_PROPERTY("Text", "s", TextBox_Selection_Text),
	GB_PROPERTY_READ("Length", "i", TextBox_Selection_Length),
	GB_PROPERTY_READ("Start", "i", TextBox_Selection_Start),
	GB_PROPERTY_READ("Pos", "i", TextBox_Selection_Start),

	GB_METHOD("Hide", 0, TextBox_Unselect, 0),

	GB_END_DECLARE
};

GB_DESC CTextBoxDesc[] =
{
	GB_DECLARE("TextBox", sizeof(CTEXTBOX)), GB_INHERITS("Control"),

	GB_METHOD("_new", 0, TextBox_new, "(Parent)Container;"),

	GB_PROPERTY("Text", "s", TextBox_Text),
	GB_PROPERTY("Alignment", "i", TextBox_Alignment),
	GB_PROPERTY_READ("Length", "i", TextBox_Length),
	GB_PROPERTY("Pos", "i", TextBox_Pos),
	GB_PROPERTY("ReadOnly", "b", TextBox_ReadOnly),
	GB_PROPERTY("Border", "b", TextBox_Border),
	GB_PROPERTY("Password", "b", TextBox_Password),
	GB_PROPERTY("MaxLength", "i", TextBox_MaxLength),
	GB_PROPERTY("Placeholder", "s", TextBox_Placeholder),

	GB_PROPERTY_SELF("Selection", ".TextBox.Selection"),
	GB_METHOD("Select", 0, TextBox_Select, "[(Start)i(Length)i]"),
	GB_METHOD("SelectAll", 0, TextBox_SelectAll, 0),
	GB_METHOD("Unselect", 0, TextBox_Unselect, 0),
	GB_PROPERTY_READ("Selected", "b", TextBox_Selected),

	GB_METHOD("Clear", 0, TextBox_Clear, 0),
	GB_METHOD("Insert", 0, TextBox_Insert, "(Text)s"),

	GB_METHOD("CursorAt", "Point", TextBox_CursorAt, "[(Pos)i]"),

	GB_EVENT("Change", 0, 0, &EVENT_Change),
	GB_EVENT("Activate", 0, 0, &EVENT_Activate),
	GB_EVENT("Cursor", 0, 0, &EVENT_Cursor),
	
	TEXTBOX_DESCRIPTION,

	GB_END_DECLARE
};


