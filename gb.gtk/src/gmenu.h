/***************************************************************************

  gmenu.h

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

#ifndef __GMENU_H
#define __GMENU_H

class gMainWindow;
class gPicture;

class gMenu
{
public:
	gMenu(gMainWindow *par, bool hidden);
	gMenu(gMenu *par, bool hidden);
	~gMenu();

	void *hFree;

	static int   winChildCount(gMainWindow *win);
	static gMenu* winChildMenu(gMainWindow *par,int pos);
	static void updateFont(gMainWindow *win);
	static void updateColor(gMainWindow *win);
	static gMenu *findFromName(gMainWindow *win, const char *name);

	static int popupCount() { return _popup_count; }

//"Properties"
	bool checked() const { return _checked; }
	bool toggle() const { return _toggle; }
	bool radio() const { return _radio; }
	bool isEnabled() const { return !_disabled; }
	bool isFullyEnabled() const;
	gMenu *child(int index) const;
	int childCount() const;
	char* shortcut() const { return _shortcut; }
	char* text() const { return _text; }
	bool isVisible() const;
	gPicture* picture() const { return _picture; }
	gMainWindow* window();
	char *name() const { return _name; }
	bool isTopLevel() const { return _toplevel; }
	bool isSeparator() const { return _style == SEPARATOR; }
	void *parent() const { return pr; }
	gMenu *parentMenu() const { return _toplevel ? NULL : (gMenu *)pr; }
	bool isClosed() const { return !_opened; }
	bool isDestroyed() const { return _destroyed; }

	void setChecked(bool vl);
	void setToggle(bool vl);
	void setRadio(bool vl);
	void setEnabled(bool vl);
	void setShortcut(char *txt);
	void setText(const char *vl);
	void setVisible(bool vl);
	void show() { setVisible(true); }
	void hide() { setVisible(false); }
	void setPicture(gPicture *pic);
	void setName(char *name);
	bool action() const { return _action; }
	void setAction(bool v) { _action = v; }
	void setFont();
	//void setColor();
	
	bool setProxy(gMenu *menu);
	gMenu *proxy() const { return _proxy; }

//"Methods"
	void popup();
	void popup(int x, int y);
	void close();
	void destroy();
	static bool insidePopup() { return _in_popup > 0; }
	static gMenu *currentPopup() { return _current_popup; }

//"Private"
	enum gMenuStyle { NOTHING, SEPARATOR, CHECK, NORMAL };
	
	void *pr;

	GtkMenuItem *menu;
	GtkWidget *hbox;
	//GtkWidget *check;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *shlabel;

	GtkMenu *_popup;

	GtkSizeGroup *sizeGroup;
	GtkAccelGroup *accel;

	gMenu *_proxy;
	unsigned _opened : 1;
	unsigned _exec: 1;
	unsigned _disabled : 1;
	unsigned _mapping : 1;
	unsigned _proxy_for : 1;
	unsigned _ignore_destroy : 1;
	unsigned _ignore_activate : 1;

	void initialize();
	gMenuStyle style() const { return _style; }
  void hideSeparators();
	void willBeDeletedLater();
	void destroyNow() { _delete_later = false; destroy(); }
	void updateRadio();
	void updateChecked();
	void updatePicture();
	GtkMenu *getSubMenu();
	void ensureChildMenu();
	void updateShortcutRecursive();
	GtkSizeGroup *getSizeGroup();
	
	void insert(gMenu *child);
	void remove(gMenu *child);
	void removeParent();
	
	bool ignoreDestroy();
	bool ignoreActivate();
	
	static void cleanRemovedMenus();

private:

	gMenuStyle _style, _oldstyle;
	char *_name;
  gPicture *_picture;
	char *_text;
	
	char *_shortcut;
	guint _shortcut_key;
	GdkModifierType _shortcut_mods;

	GPtrArray *_children;
	
	unsigned _checked : 1;
	unsigned _toggle : 1;
	unsigned _radio : 1;
	unsigned _no_update : 1;
	unsigned _destroyed : 1;
	unsigned _delete_later : 1;
	unsigned _toplevel : 1;
	unsigned _action : 1;
	unsigned _visible : 1;

	static gMenu *_current_popup;
	static int _in_popup;
	static int _popup_count;
  
	void doPopup(bool move, int x = 0, int y = 0);
  void update();
  void updateVisible();
	void updateShortcut();
	void dispose();
};

// Callbacks
void CB_menu_finish(gMenu *sender);
void CB_menu_click(gMenu *sender);
void CB_menu_show(gMenu *sender);
void CB_menu_hide(gMenu *sender);



#endif
