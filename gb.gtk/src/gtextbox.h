/***************************************************************************

  gtextbox.h

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

#ifndef __GTEXTBOX_H
#define __GTEXTBOX_H

class gTextBox : public gControl
{
public:
	gTextBox(gContainer *parent);
	~gTextBox();

//"Properties"
	int alignment() const;
	bool hasBorder() const { return _has_border; }
	virtual int length();
	int maxLength() const;
	bool password() const;
	int position() const;
	virtual char *text();
	virtual char *placeholder() const;
	virtual bool isReadOnly() const;
	int selLength() const;
	int selStart() const;
	char* selText() const;
	bool isSelected() const;

	void setAlignment(int vl);
	void setBorder(bool vl);
	void setMaxLength(int len);
	void setPassword(bool vl);
	void setPosition(int pos);
	virtual void setReadOnly(bool vl);
	virtual void setText(const char *vl);
	virtual void setPlaceholder(const char *vl);
	void setSelText(char *txt, int len);

//"Methods"
	virtual void clear();
	virtual void setFocus();
	void insert(char* txt,int len);
	void selClear();
	void select(int start,int len);
	void selectAll();
	bool hasEntry() const { return entry != 0; }

	void getCursorPos(int *x, int *y, int pos);
	
#ifdef GTK3
	virtual void customStyleSheet(GString *css);
#endif
	
//"Private"
  virtual void updateCursor(GdkCursor *cursor);
  void initEntry();
	virtual GtkIMContext *getInputMethod();
	
	virtual void setMinimumSize();
	virtual void setFont(gFont *ft);
	
	virtual gColor defaultBackground() const;

#ifdef GTK3
	virtual void onEnterEvent();
	virtual void onLeaveEvent();
#endif

	GtkWidget *entry;

	unsigned _changed : 1;
	unsigned _has_border : 1;
	unsigned _text_area_visible : 1;
	
	int _last_position;

#ifndef GTK3
	char *_placeholder;
#endif
};

// Callbacks
void CB_textbox_change(gTextBox *sender);
void CB_textbox_activate(gTextBox *sender);
void CB_textbox_cursor(gTextBox *sender);

#endif
