/***************************************************************************

  CTextArea.cpp

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

#define __CTEXTAREA_CPP

#include <QPalette>
#include <QTextEdit>
#include <QTextBlock>
#include <QTextDocumentFragment>

#include "gambas.h"
#include "main.h"
#include "CConst.h"
#include "CFont.h"
#include "CTextArea.h"


DECLARE_EVENT(EVENT_Change);
DECLARE_EVENT(EVENT_Cursor);
DECLARE_EVENT(EVENT_Link);

//-------------------------------------------------------------------------

static int get_length(void *_object)
{
	if (THIS->length < 0)
	{
		QTextBlock block = WIDGET->document()->begin();
		int len = 0;
		
		while (block.isValid())
		{
			len += block.length();
			block = block.next();
		}

		THIS->length = len - 1;
	}
	
	return THIS->length;
}

static bool is_empty(void *_object)
{
	QTextBlock block = WIDGET->document()->begin();
	return !block.isValid();
}

static void to_pos(QTextEdit *wid, int par, int car, int *pos)
{
	QTextCursor cursor;
	QTextBlock block;
	int p = 0;

	cursor = wid->textCursor();
	cursor.movePosition(QTextCursor::Start);
	
	block = cursor.block();
	
	while (par)
	{
		if (!block.isValid())
			break;
		p += block.length();
		block = block.next();
		par--;
	}

	if (block.isValid())
		car = qMin(block.length() - 1, car);
	
	*pos = p + car;
}


static void from_pos(CTEXTAREA *_object, int pos, int *par, int *car)
{
	QTextCursor cursor = WIDGET->textCursor();
	
	if (pos >= get_length(THIS))
		cursor.movePosition(QTextCursor::End);
	else
		cursor.setPosition(pos);
	
	*par = cursor.blockNumber();
	*car = cursor.position() - cursor.block().position();
}


static void get_selection(QTextEdit *wid, int *start, int *length)
{
	QTextCursor cursor = wid->textCursor();
	
	*start = cursor.selectionStart();
	*length = cursor.selectionEnd() - *start;
}

static void update_alignment(void *_object)
{
	THIS->no_change = TRUE;
	
	QTextOption opt = WIDGET->document()->defaultTextOption();
	opt.setAlignment((Qt::Alignment)CCONST_horizontal_alignment(THIS->align, ALIGN_NORMAL, true));
	WIDGET->document()->setDefaultTextOption(opt);
	
	THIS->no_change = FALSE;
}

static void set_text_color(void *_object)
{
	QTextCharFormat fmt;
	QBrush col;
	GB_COLOR fg = CWIDGET_get_foreground((CWIDGET *)THIS);
	
	fmt = WIDGET->currentCharFormat();
	
	if (fg == GB_COLOR_DEFAULT)
	{
		fmt.clearForeground();
		//col = WIDGET->palette().text();
	}
	else
	{
		fmt.setForeground(TO_QCOLOR(fg));
	}
	
	//WIDGET->setTextColor(col);
	THIS->no_change = TRUE;
	WIDGET->setCurrentCharFormat(fmt);
	THIS->no_change = FALSE;
}

void CTEXTAREA_set_foreground(void *_object)
{
	THIS->no_change = TRUE;
	
	if (is_empty(THIS))
	{
		WIDGET->setPlainText(" ");
		WIDGET->selectAll();
		WIDGET->setTextColor(Qt::black);
		set_text_color(THIS);
		WIDGET->textCursor().insertText("");
	}
	else
	{
		QTextCursor oldCursor = WIDGET->textCursor();
		
		WIDGET->selectAll();
		
		WIDGET->setTextColor(Qt::black);
		set_text_color(THIS);
		
		WIDGET->setTextCursor(oldCursor);
		
		set_text_color(THIS);
	}
	
	THIS->no_change = FALSE;
}


//-------------------------------------------------------------------------

BEGIN_METHOD(TextArea_new, GB_OBJECT parent)

	QTextEdit *wid = new QTextEdit(QCONTAINER(VARG(parent)));

	QObject::connect(wid, SIGNAL(textChanged()), &CTextArea::manager, SLOT(changed()));
	QObject::connect(wid, SIGNAL(cursorPositionChanged()), &CTextArea::manager, SLOT(cursor()));

	wid->setLineWrapMode(QTextEdit::NoWrap);
	wid->setAcceptRichText(false);
	
	THIS->widget.flag.wheel = true;
	THIS->widget.flag.autoFillBackground = true;
	CWIDGET_new(wid, (void *)_object);
	
	THIS->length = -1;
	THIS->align = ALIGN_NORMAL;

	wid->document()->setDocumentMargin(MAIN_scale * 3 / 4);
	
END_METHOD


BEGIN_PROPERTY(TextArea_Text)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(WIDGET->toPlainText());
	else
	{
		WIDGET->document()->setPlainText(QSTRING_PROP());
		update_alignment(THIS);
		CTEXTAREA_set_foreground(THIS);
	}

END_PROPERTY


BEGIN_PROPERTY(TextArea_Length)

	GB.ReturnInteger(get_length(THIS));

END_PROPERTY


BEGIN_PROPERTY(TextArea_ReadOnly)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->isReadOnly());
	else
		WIDGET->setReadOnly(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(TextArea_Wrap)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->lineWrapMode() != QTextEdit::NoWrap);
	else
		WIDGET->setLineWrapMode(VPROP(GB_BOOLEAN) ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);

END_PROPERTY


/*
BEGIN_PROPERTY(CTEXTAREA_max_length)

	int max;

	if (READ_PROPERTY)
	{
		max = WIDGET->maxLength();
		GB.ReturnInteger((max < 0) ? 0 : max);
	}
	else
	{
		max = PROPERTY(int);
		if (max <= 0)
			max = -1;
		WIDGET->setMaxLength(max);
	}

END_PROPERTY
*/

static int get_column(CTEXTAREA *_object)
{
	QTextCursor cursor = WIDGET->textCursor();
	return cursor.position() - cursor.block().position();
}

BEGIN_PROPERTY(TextArea_Column)

	QTextCursor cursor = WIDGET->textCursor();
	
	if (READ_PROPERTY)
		//GB.ReturnInteger(WIDGET->textCursor().columnNumber());
		GB.ReturnInteger(get_column(THIS));
	else
	{
		int col = VPROP(GB_INTEGER);
		
		if (col <= 0)
			cursor.movePosition(QTextCursor::QTextCursor::StartOfBlock);
		else if (col >= cursor.block().length())
			cursor.movePosition(QTextCursor::QTextCursor::EndOfBlock);
		else
			cursor.setPosition(cursor.block().position() + col);
		
		WIDGET->setTextCursor(cursor);
	}

END_PROPERTY

BEGIN_PROPERTY(TextArea_Line)

	QTextCursor cursor = WIDGET->textCursor();
	
	if (READ_PROPERTY)
		GB.ReturnInteger(cursor.blockNumber());
	else
	{
		int col = get_column(THIS);
		int line = VPROP(GB_INTEGER);
		
		if (line < 0)
			cursor.movePosition(QTextCursor::Start);
		else if (line >= WIDGET->document()->blockCount())
			cursor.movePosition(QTextCursor::End);
		else
		{
			cursor.setPosition(WIDGET->document()->findBlockByNumber(line).position());
			if (col > 0)
			{
				if (col >= cursor.block().length())
					cursor.movePosition(QTextCursor::QTextCursor::EndOfBlock);
				else
					cursor.setPosition(cursor.block().position() + col);
			}
		}
		
		WIDGET->setTextCursor(cursor);
	}

END_PROPERTY

BEGIN_PROPERTY(TextArea_Pos)

	if (READ_PROPERTY)
	{
		GB.ReturnInteger(WIDGET->textCursor().position());
	}
	else
	{
		int pos = VPROP(GB_INTEGER);
		QTextCursor cursor = WIDGET->textCursor();
		
		if (pos >= get_length(THIS))
			cursor.movePosition(QTextCursor::End);
		else
			cursor.setPosition(pos);
		
		WIDGET->setTextCursor(cursor);
	}

END_PROPERTY


/*
BEGIN_METHOD(CTEXTAREA_select, int line; int col; int selline; int selcol)

	MyMultiLineEdit *wid = QMULTILINEEDIT(_object);

	int line = PARAM(line);
	int col = PARAM(col);

	look_pos(wid, &line, &col);
	wid->setCursorPosition(line, col);

	line = PARAM(selline);
	col = PARAM(selcol);

	look_pos(wid, &line, &col);
	wid->setCursorPosition(line, col, TRUE);

END_METHOD
*/

BEGIN_METHOD_VOID(TextArea_Clear)

	WIDGET->clear();

END_METHOD


BEGIN_METHOD(TextArea_Insert, GB_STRING text)

	WIDGET->textCursor().insertText(QSTRING_ARG(text));

END_METHOD

//-------------------------------------------------------------------------

BEGIN_PROPERTY(TextArea_Selection_Text)

	if (READ_PROPERTY)
		RETURN_NEW_STRING(WIDGET->textCursor().selection().toPlainText());
	else
		WIDGET->textCursor().insertText(QSTRING_PROP());

END_PROPERTY


BEGIN_PROPERTY(TextArea_Selection_Length)

	int start, length;

	get_selection(WIDGET, &start, &length);
	GB.ReturnInteger(length);

END_PROPERTY


BEGIN_PROPERTY(TextArea_Selection_Start)

	int start, length;

	get_selection(WIDGET, &start, &length);
	GB.ReturnInteger(start);

END_PROPERTY


BEGIN_METHOD_VOID(TextArea_Selection_Clear)

	QTextCursor cursor = WIDGET->textCursor();
	cursor.clearSelection();	
	WIDGET->setTextCursor(cursor);

END_METHOD

//-------------------------------------------------------------------------

BEGIN_PROPERTY(TextArea_Selected)

	GB.ReturnBoolean(WIDGET->textCursor().hasSelection());

END_PROPERTY


BEGIN_METHOD(TextArea_Select, GB_INTEGER start; GB_INTEGER length)

	if (MISSING(start) && MISSING(length))
		WIDGET->textCursor().select(QTextCursor::Document);
	else if (!MISSING(start) && !MISSING(length))
	{
		QTextCursor cursor = WIDGET->textCursor();
		
		cursor.setPosition(VARG(start));
		cursor.setPosition(VARG(start) + VARG(length), QTextCursor::KeepAnchor);
		
		WIDGET->setTextCursor(cursor);
	}

END_METHOD


BEGIN_METHOD_VOID(TextArea_SelectAll) //, GB_BOOLEAN sel)

	QTextCursor cursor = WIDGET->textCursor();
	cursor.select(QTextCursor::Document);
	WIDGET->setTextCursor(cursor);

END_METHOD


BEGIN_METHOD(TextArea_ToPos, GB_INTEGER line; GB_INTEGER col)

	int pos;

	to_pos(WIDGET, VARG(line), VARG(col), &pos);

	GB.ReturnInteger(pos);

END_METHOD


BEGIN_METHOD(TextArea_ToLine, GB_INTEGER pos)

	int line, col;

	from_pos(THIS, VARG(pos), &line, &col);

	GB.ReturnInteger(line);

END_METHOD


BEGIN_METHOD(TextArea_ToColumn, GB_INTEGER pos)

	int line, col;

	from_pos(THIS, VARG(pos), &line, &col);

	GB.ReturnInteger(col);

END_METHOD


BEGIN_METHOD_VOID(TextArea_Copy)

	WIDGET->copy();

END_METHOD


BEGIN_METHOD_VOID(TextArea_Cut)

	WIDGET->cut();

END_METHOD


BEGIN_METHOD_VOID(TextArea_Paste)

	WIDGET->paste();

END_METHOD


BEGIN_METHOD_VOID(TextArea_Undo)

	WIDGET->undo();

END_METHOD


BEGIN_METHOD_VOID(TextArea_Redo)

	WIDGET->redo();

END_METHOD


/*BEGIN_PROPERTY(CTEXTAREA_scrollbar)

	int scroll;

	if (READ_PROPERTY)
	{
		scroll = 0;
		if (WIDGET->hScrollBarMode() == QScrollView::Auto)
			scroll += 1;
		if (WIDGET->vScrollBarMode() == QScrollView::Auto)
			scroll += 2;

		GB.ReturnInteger(scroll);
	}
	else
	{
		scroll = VPROP(GB_INTEGER) & 3;
		WIDGET->setHScrollBarMode( (scroll & 1) ? QScrollView::Auto : QScrollView::AlwaysOff);
		WIDGET->setVScrollBarMode( (scroll & 2) ? QScrollView::Auto : QScrollView::AlwaysOff);
	}

END_PROPERTY*/


BEGIN_METHOD_VOID(TextArea_EnsureVisible)

	WIDGET->ensureCursorVisible();

END_METHOD


BEGIN_PROPERTY(TextArea_Alignment)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->align);
	else
	{
		THIS->align = VPROP(GB_INTEGER);
		update_alignment(THIS);
	}

END_PROPERTY

BEGIN_PROPERTY(TextArea_Border)

	CWIDGET_border_simple(_object, _param);

	if (!READ_PROPERTY)
		WIDGET->document()->setDocumentMargin(VPROP(GB_BOOLEAN) ? (MAIN_scale * 3 / 4) : 0);
	
END_PROPERTY

BEGIN_METHOD(TextArea_CursorAt, GB_INTEGER pos)

	QRect rect;
	QTextCursor cursor = WIDGET->textCursor();
	
	if (!MISSING(pos))
		cursor.setPosition(VARG(pos));
	
	rect = WIDGET->cursorRect(cursor);
	
	GB.ReturnObject(GEOM.CreatePoint(rect.x() + WIDGET->viewport()->x(), rect.bottom() + WIDGET->viewport()->y()));

END_PROPERTY

//-------------------------------------------------------------------------

GB_DESC CTextAreaSelectionDesc[] =
{
	GB_DECLARE(".TextArea.Selection", 0), GB_VIRTUAL_CLASS(),

	GB_PROPERTY("Text", "s", TextArea_Selection_Text),
	GB_PROPERTY_READ("Length", "i", TextArea_Selection_Length),
	GB_PROPERTY_READ("Start", "i", TextArea_Selection_Start),
	GB_PROPERTY_READ("Pos", "i", TextArea_Selection_Start),

	//GB_METHOD("Clear", NULL, TextArea_Selection_Clear, NULL),
	GB_METHOD("Hide", NULL, TextArea_Selection_Clear, NULL),

	GB_END_DECLARE
};

GB_DESC CTextAreaDesc[] =
{
	GB_DECLARE("TextArea", sizeof(CTEXTAREA)), GB_INHERITS("Control"),

	GB_METHOD("_new", NULL, TextArea_new, "(Parent)Container;"),

	GB_PROPERTY("Text", "s", TextArea_Text),
	GB_PROPERTY_READ("Length", "i", TextArea_Length),
	GB_PROPERTY("ReadOnly", "b", TextArea_ReadOnly),

	//GB_PROPERTY_READ("Lines", ".TextArea.Line", CTEXTAREA_line_or_selection),

	GB_PROPERTY("ScrollBar", "i", CWIDGET_scrollbar),
	GB_PROPERTY("Wrap", "b", TextArea_Wrap),
	GB_PROPERTY("Border", "b", TextArea_Border),
	GB_PROPERTY("Alignment", "i", TextArea_Alignment),

	GB_PROPERTY("Line", "i", TextArea_Line),
	GB_PROPERTY("Column", "i", TextArea_Column),
	GB_PROPERTY("Pos", "i", TextArea_Pos),

	GB_PROPERTY_SELF("Selection", ".TextArea.Selection"),
	GB_METHOD("Select", NULL, TextArea_Select, "[(Start)i(Length)i]"),
	GB_METHOD("SelectAll", NULL, TextArea_SelectAll, NULL),
	GB_METHOD("Unselect", NULL, TextArea_Selection_Clear, NULL),
	GB_PROPERTY_READ("Selected", "b", TextArea_Selected),
	
	GB_METHOD("Clear", NULL, TextArea_Clear, NULL),
	GB_METHOD("Insert", NULL, TextArea_Insert, "(Text)s"),

	GB_METHOD("Copy", NULL, TextArea_Copy, NULL),
	GB_METHOD("Cut", NULL, TextArea_Cut, NULL),
	GB_METHOD("Paste", NULL, TextArea_Paste, NULL),
	GB_METHOD("Undo", NULL, TextArea_Undo, NULL),
	GB_METHOD("Redo", NULL, TextArea_Redo, NULL),

	GB_METHOD("ToPos", "i", TextArea_ToPos, "(Line)i(Column)i"),
	GB_METHOD("ToLine", "i", TextArea_ToLine, "(Pos)i"),
	GB_METHOD("ToColumn", "i", TextArea_ToColumn, "(Pos)i"),

	GB_METHOD("CursorAt", "Point", TextArea_CursorAt, "[(Pos)i]"),

	GB_METHOD("EnsureVisible", NULL, TextArea_EnsureVisible, NULL),

	GB_EVENT("Change", NULL, NULL, &EVENT_Change),
	GB_EVENT("Cursor", NULL, NULL, &EVENT_Cursor),

	TEXTAREA_DESCRIPTION,

	GB_END_DECLARE
};


//-------------------------------------------------------------------------

CTextArea CTextArea::manager;

void CTextArea::changed(void)
{
	GET_SENDER();
	
	if (THIS->no_change)
		return;
	
	set_text_color(THIS);
	THIS->length = -1;
	GB.Raise(THIS, EVENT_Change, 0);
}

void CTextArea::cursor(void)
{
	GET_SENDER();
	//set_text_color(THIS);
	GB.Raise(THIS, EVENT_Cursor, 0);
}

void CTextArea::link(const QString &path)
{
	GET_SENDER();
	const char *str = TO_UTF8(path);
	GB.Raise(THIS, EVENT_Link, 1, GB_T_STRING, str, LAST_UTF8_LENGTH());
}
