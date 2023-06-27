/***************************************************************************

  gbutton.h

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

#ifndef __GBUTTON_H
#define __GBUTTON_H

class gButton : public gControl
{
public:

  enum Type
  {
    Button, Toggle, Check, Radio, Tool
  };

	gButton(gContainer *parent, Type type);
  ~gButton();
  
	bool getBorder() const;
	bool isCancel() const;
	bool isDefault() const;
	const char *text() const { return bufText; }
	bool hasText() const { return bufText && *bufText; }
	gPicture *picture() const;
	bool value() const;
	bool isToggle() const;
	bool isRadio() const;
	bool inconsistent() const;
	bool isStretch() { return _stretch; }
	bool isTristate() const { return _tristate; }
	bool isAutoResize() const { return _autoresize; }

	//void setEnabled(bool vl);
	void setBorder(bool vl);
	void setCancel(bool vl);
	void setDefault(bool vl);
	void setText(const char *st);
	void setPicture(gPicture *pic);
	void setValue(bool vl);
	void setToggle(bool vl);
	void setRadio(bool vl);
	void setInconsistent(bool vl);
	void setStretch(bool vl);
	void setTristate(bool vl);
	void setAutoResize(bool vl);
	
	virtual void setRealForeground(gColor color);
	gColor defaultBackground() const;
	
	virtual void updateDirection();
	
	//virtual void setRealBackground(gColor color);

//"Method"
	void animateClick(bool on);

//"Private"
	char type;
	char *bufText;
	GtkWidget *_label;
	GtkCellRenderer *rendtxt;
	GdkPixbuf *rendpix,*rendinc;
	gPicture *pic;
	int shortcut;
	unsigned disable : 1;
	unsigned _toggle : 1;
	unsigned _animated : 1;
	unsigned _radio : 1;
	unsigned _stretch : 1;
	unsigned _tristate : 1;
	unsigned _autoresize : 1;
	
	bool hasShortcut() const;
	void unsetOtherRadioButtons();
	int autoHeight() const;
	virtual void updateSize();
};

// Callbacks
void CB_button_click(gControl *sender);


#endif
