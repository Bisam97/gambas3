/***************************************************************************

  gmainwindow.h

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

#ifndef __GMAINWINDOW_H
#define __GMAINWINDOW_H

#include "gbutton.h"

class gMainWindow : public gContainer
{
public:
	gMainWindow();
	gMainWindow(gContainer *parent);
	gMainWindow(int plug);
	~gMainWindow();

//"Properties"
	bool hasBorder();
	bool isResizable();
	bool isUtility() const;
	bool isEmbedded() const { return _xembed; }
	gPicture *icon() { return _icon; }
	gPicture *picture() { return _picture; }
	bool mask() { return _mask; }
	int menuCount();
	bool isModal() const;
	const char *text();
	bool isTopOnly() const { return isTopLevel() && _top_only; }
	bool isSkipTaskBar() const { return isTopLevel() && _skip_taskbar; }
	bool minimized() const { return _minimized; }
	bool maximized() const { return _maximized; }
	bool fullscreen() const { return _fullscreen; }
	bool isSticky() const { return isTopLevel() && _sticky; };
	int  getStacking();
	bool isPersistent() const { return _persistent; }
	bool isOpened() const { return _opened; }
	bool isClosed() const { return _closed; }
	bool isHidden() const { return _hidden; }
	bool isPopup() const { return _popup; }
	bool isTransparent() const { return _transparent; }
	bool isNoTakeFocus() const { return _no_take_focus; }
	int screen();

	GPtrArray *getControlList();
	gControl *getControl(const char *name);

	void setBorder(bool b);
	void setResizable(bool b);
	void setUtility(bool v);
	void setIcon(gPicture *pic);
	void setMask(bool vl);
	void setPicture(gPicture *pic);
	void setText(const char *txt);
	void setTopOnly(bool vl);
	void setSkipTaskBar(bool b);
	void setMinimized(bool vl);
	void setMaximized(bool vl);
	void setFullscreen(bool vl);
	void setSticky(bool vl);
	void setStacking(int vl);
  void setPersistent(bool vl);
	void setTransparent(bool vl);
	void setNoTakeFocus(bool vl);
	
	void setCustomMinimumSize(int w, int h);
	void getCustomMinimumSize(int *w, int *h) const;

	virtual void setVisible(bool vl);
	virtual void setBackground(gColor vl);
	virtual void setRealBackground(gColor vl);
	virtual void setRealForeground(gColor vl);

	virtual int clientWidth();
	virtual int clientHeight();
	virtual int clientX();
	virtual int clientY();
	virtual int containerX();
	virtual int containerY();

	//virtual bool getScreenPos(int *x, int *y);

	bool spontaneous() { return !_not_spontaneous; }

	bool setMenuBarVisible(bool v);
	bool isMenuBarVisible();

	double opacity();
	void setOpacity(double v);

//"Methods"
	void center();
	void showActivate();
	void showModal();
	void showPopup();
	void showPopup(int x, int y);
	void activate();
	virtual void move(int x, int y);
	virtual bool resize(int w, int h, bool no_decide = false);
	bool close();
	virtual void reparent(gContainer *newpr, int x, int y);
	virtual void destroy();
	virtual void restack(bool raise);
	
//"Static"
	static GList *windows;
	static int count() { return g_list_length(windows); }
	static gMainWindow *get(int index) { return (gMainWindow *)g_list_nth_data(windows, index); }
	static gMainWindow *_active;
	static void setActiveWindow(gControl *control);
	static gMainWindow *_current;
	static bool closeAll();
	
//"Private"
  void initialize();
	void drawMask();
	void initWindow();
	bool emitOpen();
	void remap();
	bool doClose(bool destroying = false);
	void afterShow();
	void checkMenuBar();
	int menuBarHeight();
	void configure();
	void embedMenuBar(GtkWidget *border);
	void emitResize();
	void emitResizeLater();
	void setGeometryHints();
	virtual void updateFont();
	void present();
	
	void setTransientFor();
	void setType(GtkWindowType type);
	void calcCsdSize();
	void createWindow(GtkWidget *new_border);
	void updateSize();
	gControl *getInitialFocus();
#ifdef GTK3
	virtual GtkWidget *getStyleSheetWidget();
#endif

	GtkWindowGroup *group;
	GtkAccelGroup *accel;
	GtkMenuBar *menuBar;
	int stack;
	gPicture *_icon;
	gPicture *_picture;
	char *_title;
	gMainWindow *_previous;

	gControl *_initial_focus;
	gControl *_save_focus;
	gButton *_default;
	gButton *_cancel;

	int _resize_last_w;
	int _resize_last_h;

	int _min_w;
	int _min_h;
	int _default_min_w;
	int _default_min_h;
	
	int _csd_w;
	int _csd_h;

	int _sx, _sy, _sw, _sh;
	
	unsigned _mask : 1;
	unsigned _top_only : 1;
	unsigned _persistent : 1;
	unsigned _sticky : 1;
	unsigned _opened : 1;
	unsigned _closed : 1;
	unsigned _closing : 1;
	unsigned _not_spontaneous : 1;
	unsigned _skip_taskbar : 1;
	unsigned _masked : 1;
	unsigned _xembed : 1;
	unsigned _activate : 1;
	unsigned _hidden : 1;
	unsigned _hideMenuBar : 1;
	unsigned _showMenuBar : 1;
	unsigned _popup : 1;
	unsigned _maximized : 1;
	unsigned _minimized : 1;
	unsigned _fullscreen : 1;
	unsigned _utility : 1;
	unsigned _transparent : 1;
	unsigned _no_take_focus : 1;
	unsigned _moved : 1;
	unsigned _resized : 1;
	unsigned _resizable : 1;
	unsigned _unmap : 1;
	unsigned _initMenuBar : 1;
	unsigned _grab_on_show : 1;
	unsigned _frame_init : 1;
	unsigned _set_focus : 1;
};

// Callbacks

void CB_window_open(gMainWindow *sender);
void CB_window_show(gMainWindow *sender);
void CB_window_hide(gMainWindow *sender);
void CB_window_move(gMainWindow *sender);
void CB_window_resize(gMainWindow *sender);
bool CB_window_close(gMainWindow *sender);
gMainWindow *CB_window_activate(gControl *sender);
void CB_window_state(gMainWindow *sender);
void CB_window_font(gMainWindow *sender);

#endif
