/***************************************************************************

  gtextarea.h

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

#ifndef __GTEXTAREA_H
#define __GTEXTAREA_H

class gTextAreaAction;

class gTextArea : public gControl
{
public:
	gTextArea(gContainer *parent);
	~gTextArea();

//"Properties"
	int column() const;
	int length() const;
	int line() const;
	int position() const;
	bool readOnly() const;
	char* text() const;
	bool wrap() const;
	bool isSelected() const;

	void setColumn(int vl);
	void setLine(int vl);
	void setPosition(int vl);
	void setReadOnly(bool vl);
	void setText(const char *txt, int len = -1);
	void setWrap(bool vl);
	//int textWidth();
	//int textHeight();
	
	int alignment() const;
	void setAlignment(int vl);

//"Methods"
	void copy();
	void cut();
	void ensureVisible();
	void paste();
	void insert(const char *txt);
	int toLine(int pos) const;
	int toColumn(int pos) const;
	int toPosition(int line, int col) const;

//"Selection properties"
	int selStart() const;
	int selEnd() const;
	char* selText() const;

	void setSelText(const char *vl);

//"Selection methods"
	void selDelete();
	void selSelect(int pos, int length);
	void selectAll() { selSelect(0, length()); }

	bool canUndo() const { return _undo_stack != 0; }
	bool canRedo() const { return _redo_stack != 0; }
	void begin() { _not_undoable_action++; }
	void end() { _not_undoable_action--; }
	void undo();
	void redo();
	void clear();
	
	void getCursorPos(int *x, int *y, int pos) const;
	
	void emitCursor();
	
//"Private"
  virtual void updateCursor(GdkCursor *cursor);
	virtual void updateScrollBar();
	virtual void setMinimumSize();
	virtual void setFont(gFont *ft);
	virtual void setBorder(bool b);
	virtual gColor defaultBackground() const;
#ifdef GTK3
	virtual GtkWidget *getStyleSheetWidget();
	virtual const char *getStyleSheetColorNode();
	virtual void customStyleSheet(GString *css);
	virtual void onEnterEvent();
	virtual void onLeaveEvent();
#endif
	void updateFixSpacing();
	virtual GtkIMContext *getInputMethod();
  //void waitForLayout(int *tw, int *th);
	void clearUndoStack();
	void clearRedoStack();
	
	gTextAreaAction *_undo_stack;
	gTextAreaAction *_redo_stack;
	int _not_undoable_action;
	unsigned _undo_in_progress : 1;

private:
	GtkWidget *textview;
	GtkTextBuffer *_buffer;
	unsigned _align_normal : 1;
	unsigned _text_area_visible : 1;
	int _last_pos;
	GtkTextTag *_fix_spacing_tag;

	GtkTextIter *getIterAt(int pos = -1) const;
};

// Callbacks
void CB_textarea_change(gTextArea *sender);
void CB_textarea_cursor(gTextArea *sender);

#endif
