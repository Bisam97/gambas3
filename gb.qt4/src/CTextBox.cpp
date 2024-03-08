/***************************************************************************

  CTextBox.cpp

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

#define __CTEXTBOX_CPP

#include <qapplication.h>
#include <qcursor.h>
#include <qsizepolicy.h>
#include <QLineEdit>
#include <QListView>

#include "gambas.h"

#include "CConst.h"
#include "CTextBox.h"

DECLARE_EVENT(EVENT_Change);
DECLARE_EVENT(EVENT_Activate);
DECLARE_EVENT(EVENT_Cursor);

#define MAX_LEN 32767


//-------------------------------------------------------------------------


#define GET_TEXT_BOX() QLineEdit *textbox = TEXTBOX;


BEGIN_METHOD(TextBox_new, GB_OBJECT parent)

	QLineEdit *wid = new QLineEdit(QCONTAINER(VARG(parent)));

	QObject::connect(wid, SIGNAL(textChanged(const QString &)), &CTextBox::manager, SLOT(onChange()));
	QObject::connect(wid, SIGNAL(returnPressed()), &CTextBox::manager, SLOT(onActivate()));
	QObject::connect(wid, SIGNAL(cursorPositionChanged(int, int)), &CTextBox::manager, SLOT(onCursor()));
	//QObject::connect(wid, SIGNAL(selectionChanged()), &CTextBox::manager, SLOT(onSelectionChanged()));

	wid->setAlignment(Qt::AlignLeft);

	CWIDGET_new(wid, (void *)_object);

END_METHOD


BEGIN_METHOD_VOID(TextBox_Clear)

	TEXTBOX->clear();

END_METHOD

BEGIN_METHOD(TextBox_Insert, GB_STRING text)

	GET_TEXT_BOX();

	//textbox->insert(QString(GB.ToZeroString(ARG(text))));
	textbox->insert(QSTRING_ARG(text));

END_METHOD


BEGIN_PROPERTY(TextBox_Text)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(TEXTBOX->text());
	else
	{
		TEXTBOX->deselect();
		TEXTBOX->setText(QSTRING_PROP());
	}

END_PROPERTY


BEGIN_PROPERTY(TextBox_Placeholder)

	GET_TEXT_BOX();

	if (READ_PROPERTY)
		RETURN_NEW_STRING(textbox->placeholderText());
	else
		textbox->setPlaceholderText(QSTRING_PROP());

END_PROPERTY


BEGIN_PROPERTY(TextBox_Length)

	GB.ReturnInteger(TEXTBOX->text().length());

END_PROPERTY


BEGIN_PROPERTY(TextBox_Alignment)

	if (READ_PROPERTY)
		GB.ReturnInteger(CCONST_alignment(TEXTBOX->alignment() | Qt::AlignVCenter, ALIGN_NORMAL, false));
	else
		TEXTBOX->setAlignment((Qt::Alignment)CCONST_alignment(VPROP(GB_INTEGER), ALIGN_NORMAL, true) & Qt::AlignHorizontal_Mask);

END_PROPERTY


BEGIN_PROPERTY(TextBox_Pos)

	GET_TEXT_BOX();

	if (READ_PROPERTY)
		GB.ReturnInteger(textbox->cursorPosition());
	else
		textbox->setCursorPosition(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_PROPERTY(TextBox_ReadOnly)

	if (READ_PROPERTY)
		GB.ReturnBoolean(TEXTBOX->isReadOnly());
	else
		TEXTBOX->setReadOnly(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Border)

	GET_TEXT_BOX();

	if (READ_PROPERTY)
		GB.ReturnBoolean(textbox->hasFrame());
	else
		textbox->setFrame(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(TextBox_Password)

	GET_TEXT_BOX();

	if (READ_PROPERTY)
		GB.ReturnBoolean(textbox->echoMode() != QLineEdit::Normal);
	else
		textbox->setEchoMode(VPROP(GB_BOOLEAN) ? QLineEdit::Password : QLineEdit::Normal);

END_PROPERTY


BEGIN_PROPERTY(TextBox_MaxLength)

	int max;

	GET_TEXT_BOX();

	if (READ_PROPERTY)
	{
		max = textbox->maxLength();
		GB.ReturnInteger(max >= MAX_LEN ? 0 : max);
	}
	else
	{
		max = VPROP(GB_INTEGER);
		if (max < 1 || max > MAX_LEN)
			max = MAX_LEN;

		textbox->setMaxLength(max);
	}

END_PROPERTY


BEGIN_METHOD(TextBox_CursorAt, GB_INTEGER pos)

	QRect rect;
	int save = -1;

	GET_TEXT_BOX();
	
	if (!MISSING(pos))
	{
		save = textbox->cursorPosition();
		textbox->setCursorPosition(VARG(pos));
	}
	
	// Hack to call cursorRect()
#ifdef QT5
	rect = textbox->inputMethodQuery(Qt::ImCursorRectangle).toRect();
#else
	rect = textbox->inputMethodQuery(Qt::ImMicroFocus).toRect();
#endif
	
	if (save >= 0)
		textbox->setCursorPosition(save);
	
	GB.ReturnObject(GEOM.CreatePoint((rect.left() + rect.right()) / 2 + 1, rect.bottom()));

END_PROPERTY

/***************************************************************************

	.TextBox.Selection

***************************************************************************/

BEGIN_PROPERTY(TextBox_Selection_Text)

	GET_TEXT_BOX();

	if (READ_PROPERTY)
		RETURN_NEW_STRING(textbox->selectedText());
	else
		textbox->insert(QSTRING_PROP());

END_PROPERTY


static void set_selection(QLineEdit *textbox, int start, int length)
{
	int len = (int)textbox->text().length();

	if (start < 0 || start >= len)
	{
		start = textbox->cursorPosition();
		length = 0;
	}

	textbox->setCursorPosition(start);

	if (length <= 0)
		textbox->deselect();
	else
	{
		if ((start + length) >= len)
			length = len - start;
		textbox->setSelection(start, length);
	}
}

static void get_selection(QLineEdit *textbox, int *start, int *length)
{
	*start = textbox->selectionStart();
	if (*start < 0)
		*start = textbox->cursorPosition();
	if (!textbox->hasSelectedText())
		*length = 0;
	else
		*length = textbox->selectedText().length();
}


BEGIN_PROPERTY(TextBox_Selection_Length)

	int start, length;

	GET_TEXT_BOX();

	get_selection(textbox, &start, &length);

	GB.ReturnInteger(length);

END_PROPERTY


BEGIN_PROPERTY(TextBox_Selection_Start)

	int start, length;

	GET_TEXT_BOX();

	get_selection(textbox, &start, &length);
	GB.ReturnInteger(start);

END_PROPERTY


BEGIN_METHOD_VOID(TextBox_Unselect)

	GET_TEXT_BOX();

	textbox->deselect();

END_METHOD

BEGIN_METHOD_VOID(TextBox_Selected)

	GET_TEXT_BOX();

	GB.ReturnBoolean(textbox->hasSelectedText());

END_METHOD


BEGIN_METHOD(TextBox_Select, GB_INTEGER start; GB_INTEGER length)

	GET_TEXT_BOX();

	if (MISSING(start) && MISSING(length))
		textbox->selectAll();
	else if (!MISSING(start) && !MISSING(length))
		set_selection(textbox, VARG(start), VARG(length));

END_METHOD

BEGIN_METHOD_VOID(TextBox_SelectAll)

	GET_TEXT_BOX();

	textbox->selectAll();

END_METHOD



//-------------------------------------------------------------------------

CTextBox CTextBox::manager;

void CTextBox::onChange(void)
{
	RAISE_EVENT(EVENT_Change);
}


void CTextBox::onActivate(void)
{
	RAISE_EVENT(EVENT_Activate);
}


void CTextBox::onCursor()
{
	RAISE_EVENT(EVENT_Cursor);
}


//-------------------------------------------------------------------------

GB_DESC CTextBoxSelectionDesc[] =
{
	GB_DECLARE(".TextBox.Selection", 0), GB_VIRTUAL_CLASS(),

	GB_PROPERTY("Text", "s", TextBox_Selection_Text),
	GB_PROPERTY_READ("Length", "i", TextBox_Selection_Length),
	GB_PROPERTY_READ("Start", "i", TextBox_Selection_Start),
	GB_PROPERTY_READ("Pos", "i", TextBox_Selection_Start),

	GB_METHOD("Hide", NULL, TextBox_Unselect, NULL),

	GB_END_DECLARE
};


GB_DESC CTextBoxDesc[] =
{
	GB_DECLARE("TextBox", sizeof(CTEXTBOX)), GB_INHERITS("Control"),

	GB_METHOD("_new", NULL, TextBox_new, "(Parent)Container;"),

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
	GB_METHOD("Select", NULL, TextBox_Select, "[(Start)i(Length)i]"),
	GB_METHOD("SelectAll", NULL, TextBox_SelectAll, NULL),
	GB_METHOD("Unselect", NULL, TextBox_Unselect, NULL),
	GB_PROPERTY_READ("Selected", "b", TextBox_Selected),

	GB_METHOD("Clear", NULL, TextBox_Clear, NULL),
	GB_METHOD("Insert", NULL, TextBox_Insert, "(Text)s"),
	
	GB_METHOD("CursorAt", "Point", TextBox_CursorAt, "[(Pos)i]"),

	GB_EVENT("Change", NULL, NULL, &EVENT_Change),
	GB_EVENT("Activate", NULL, NULL, &EVENT_Activate),
	GB_EVENT("Cursor", NULL, NULL, &EVENT_Cursor),

	TEXTBOX_DESCRIPTION,

	GB_END_DECLARE
};

