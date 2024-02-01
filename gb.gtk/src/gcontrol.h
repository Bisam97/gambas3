/***************************************************************************

  gcontrol.h

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#ifndef __GCONTROL_H
#define __GCONTROL_H

#include "gcolor.h"
#include "gdrag.h"

class gContainer;
class gMainWindow;

#ifdef GTK3
void gt_patch_control(GtkWidget *widget);
#endif

class gControl
{
public:
	gControl();
	gControl(gContainer *parent);
	virtual ~gControl();

	void *hFree;

	bool isContainer() const { return _is_container; }
	bool isWindow() const { return _is_window; }
	bool isButton() const { return _is_button; }
	bool isDrawingArea() const { return _is_drawingarea; }
	bool isTopLevel() const { return pr == NULL; }
	bool isDestroyed() const { return _destroyed; }
	
	gMainWindow *window() const;
	gMainWindow *topLevel() const;
	
	gContainer *parent() const { return pr; }
	bool isAncestorOf(gControl *child);

	bool isDesign() const { return _design && !_no_design; }
	bool isDesignIgnore() const { return _design_ignore; }

	bool hovered();
	virtual uintptr_t handle();
	
	int left() const { return bufX; }
	int x() const { return left(); }
	int top() const { return bufY; }
	int y() const { return top(); }
	int width() const { return bufW; }
	int height() const { return bufH; }
	void setLeft(int v) { move(v, y()); }
	void setTop(int v) { move(x(), v); }
	void setWidth(int w) { resize(w, height()); }
	void setHeight(int h) { resize(width(), h); }
	virtual void move(int x, int y);
	virtual bool resize(int w, int h, bool no_decide = false);
	void resize() { resize(width(), height()); }
	void moveResize(int x, int y, int w, int h, bool no_decide = false);
	void getGeometry(GdkRectangle *rect) const { rect->x = bufX; rect->y = bufY; rect->width = bufW; rect->height = bufH; }
	void setGeometry(GdkRectangle *rect) { moveResize(rect->x, rect->y, rect->width, rect->height); }

	bool isVisible() const { return _visible; }
	bool isReallyVisible();
	virtual void setVisible(bool v);
	void hide() { setVisible(false); }
	void show() { setVisible(true); }

	void setVisibility(bool v);
	void checkVisibility();
	
	virtual bool isEnabled() const;
	virtual void setEnabled(bool vl);

	int mouse();
	void setMouse(int m);
	gCursor* cursor();
	void setCursor(gCursor *vl);
	virtual void updateCursor(GdkCursor *cursor);
	void updateCurrentCursor();
	
	gControl *next();
	gControl *previous();
	gControl *nextFocus();
	gControl *previousFocus();
	void setPrevious(gControl *prev);
	void setNext(gControl *next);
	
	int screenX();
	int screenY();
	virtual bool getScreenPos(int *x, int *y);
	
	bool isExpand() const { return _expand; }
	bool isIgnore() const { return _ignore; }
	void setExpand (bool vl);
	void setIgnore (bool vl);

	bool acceptDrops() const;
	void setAcceptDrops(bool vl);
	void updateAcceptDrops();
	
	const char *name() const { return _name; }
	void setName(char *name);
	
	bool action() const { return _action; }
	void setAction(bool v) { _action = v; }

	virtual void setDesign(bool ignore = false);
	gControl *ignoreDesign();
	void updateDesign();
	
	char *tooltip() { return _tooltip; }
	void setTooltip(char *vl);

	bool isTracking() const;
	void setTracking(bool vl);
	
	bool isNoTabFocus() const;
	bool isNoTabFocusRec() const;
	void setNoTabFocus(bool v);
	
	void setDirection(int v);
	int direction() const { return _direction; }
	
	void setInverted(bool v);
	bool isInverted() const { return _inverted; }

	bool isRightToLeft() const;

	void setIgnoreMouse(bool v) { _ignore_mouse = v; }
	bool isIgnoreMouse() const { return _ignore_mouse; }
	
	gColor background() const { return _bg; }
	gColor foreground() const { return _fg; }
	virtual void setBackground(gColor color = COLOR_DEFAULT);
	virtual void setForeground(gColor color = COLOR_DEFAULT);
	virtual gColor defaultBackground() const;
	gColor realBackground(bool no_default = false);
	gColor realForeground(bool no_default = false);
	virtual void setRealBackground(gColor color);
	virtual void setRealForeground(gColor color);

	virtual gFont *font() const;
	void actualFontTo(gFont *ft);
	virtual void setFont(gFont *ft);
	bool ownFont() { return _font != NULL; }
	virtual void updateFont();
	virtual void updateSize();
	
	bool hasNativePopup() const { return _has_native_popup; }
	
#ifdef GTK3
	void setWidgetName();
	virtual GtkWidget *getStyleSheetWidget();
	virtual const char *getStyleSheetColorNode();
	virtual const char *getStyleSheetFontNode();
	void updateStyleSheet(bool dirty);
	virtual void customStyleSheet(GString *css);
	void setStyleSheetNode(GString *css, const char *node);
	virtual void updateColor();
#else
	void setColorNames(const char **, const char **) {}
#endif
	void setColorBase() { use_base = TRUE; }
	void setColorButton() { use_base = FALSE; }

	virtual bool canFocus() const;
	bool canFocusOnClick() const;
	void setCanFocus(bool vl);

	bool eatReturnKey() const { return _eat_return_key; }
	
	gControl *proxy() const { return _proxy; }
	bool setProxy(gControl *proxy);

	int scrollX();
	int scrollY();
	void scroll(int x, int y);
	void setScrollX(int vl);
	void setScrollY(int vl);

	int scrollBar() const { return _scrollbar; }
	void setScrollBar(int vl);
	virtual void updateScrollBar();
	
	bool isDragging() const { return _dragging; }
	
	void dragText(char *txt, char *format = NULL) { gDrag::dragText(this, txt, format); }
	void dragImage(gPicture *pic) { gDrag::dragImage(this, pic); }
	
	virtual void reparent(gContainer *newpr, int x, int y);

	void lower() { restack(false); }
	void raise() { restack(true); }
	virtual void restack(bool raise);

	virtual void setFocus();
	bool hasFocus() const;
#if GTK_CHECK_VERSION(3, 2, 0)
	bool hasVisibleFocus() const;
#else
	bool hasVisibleFocus() const { return hasFocus(); }
#endif

	void refresh();
	void refresh(int x, int y, int w, int h);
	virtual void afterRefresh();
	
	bool grab();
	
	virtual void destroy();
	void destroyNow() { destroy(); postDelete(); }
	
	void lock() { _locked++; }
	void unlock() { _locked--; }
	bool locked() const { return _locked; }
	
	void emit(void *signal);
	void emit(void *signal, intptr_t arg);
	void emit(void *signal, char *arg) { emit(signal, (intptr_t)arg); }

// "Private"
	gint bufW,bufH,bufX,bufY;
	int _min_w, _min_h;
	gCursor *curs;
	gFont *_font;
	gFont *_resolved_font;
	GtkWidget *widget;
	GtkWidget *border;
	GtkWidget *frame;
	GtkScrolledWindow *_scroll;
	short _mouse;
	gControl *_proxy, *_proxy_for;
	gColor _bg, _fg;
	char *_tooltip;
#ifdef GTK3
	GtkStyleProvider *_css;
	const char *_css_node;
#endif
	
	unsigned _destroyed : 1;               // If the control has already been added to the destroy list
	unsigned _design : 1;
	unsigned _design_ignore : 1;
	unsigned _no_design : 1;
	unsigned _expand : 1;
	unsigned _ignore : 1;
	unsigned _action : 1;                  // *reserved*
	unsigned _inverted : 1;                // if the widget direction has been inverted

	unsigned _accept_drops : 1;            // If the control accepts drops
	unsigned _dragging : 1;                // if the control is being dragged
	unsigned _tracking : 1;                // If we are tracking mouse move even if no mouse button is pressed
	unsigned _old_tracking : 1;            // real value when Tracking is false
	unsigned _bg_set : 1;                  // Have a private background
	unsigned _fg_set : 1;                  // Have a private foreground
	unsigned _have_cursor : 1;             // If gApplication::setBusy() must update the cursor
	unsigned use_base : 1;                 // Use base and text color for foreground and background

	unsigned _visible : 1;                 // A control can be hidden if its width or height is zero
	unsigned _no_delete : 1;               // Do not delete on destroy signal
	unsigned _scrollbar : 2;
	unsigned _dirty_pos : 1;               // If the position of the widget has changed
	unsigned _dirty_size : 1;              // If the size of the widget has changed
	unsigned _inside : 1;                  // if we got an enter event, but not a leave event yet.
	unsigned _has_border : 1;              // if the control has a border

	unsigned _locked : 4;                  // For locking events
	unsigned frame_border : 4;
	
	unsigned frame_padding : 8;

	unsigned _has_input_method : 1;        // Has its own input method management
	unsigned _no_default_mouse_event : 1;  // No default mouse events
	unsigned _grab : 1;                    // control is currently grabbing mouse and keyboard
	unsigned _no_tab_focus : 1;            // Don't put inside focus chain
	unsigned _no_auto_grab : 1;            // do not automatically grab widget on button press event
	unsigned _no_background : 1;           // Don't draw the background automatically
	unsigned _use_wheel : 1;               // Do not propagate the mouse wheel event
	unsigned _has_native_popup : 1;        // I have a native popup menu

	unsigned _is_container : 1;            // I am a container
	unsigned _is_window : 1;               // I am a window
	unsigned _is_button : 1;               // I am a button
	unsigned _is_drawingarea : 1;          // I am a drawing area
	unsigned _eat_return_key : 1;          // If the control eats the return key
	unsigned _minimum_size_set : 1;        // If minimum size has been computed
	unsigned _direction : 2;               // Text direction

	unsigned _allow_show : 1;              // Allowed to be visible (after the first resize)
	unsigned _ignore_mouse : 1;            // Ignore mouse events (for drawing area only)
	
#ifdef GTK3
	unsigned _style_dirty : 1;             // If the style must be refreshed
	unsigned _no_style_without_child : 1;  // For containers that do not need a css if they have no children
	unsigned _has_css_id : 1;              // If the widget has a css id
#endif
	
 	void removeParent() { pr = NULL; }
	void initSignals();
	virtual void borderSignals();
	void connectParent();
	void setParent(gContainer *parent) { pr = parent; }
	void initAll(gContainer *pr);
	void realize(bool draw_frame = false);
	void realizeScrolledWindow(GtkWidget *wid, bool doNotRealize = false);
	void registerControl();
	void updateGeometry(bool force = false);
	void updateEventMask();
	
	bool hasInputMethod() { return _has_input_method; }
	virtual GtkIMContext *getInputMethod();
	
	GdkCursor *getGdkCursor();
	virtual void updateBorder();
	int getFrameBorder() const { return frame_border; }
	void setFrameBorder(int border);
	virtual void setBorder(bool b);
	bool hasBorder() const;
	int getFramePadding() const { return frame_padding; }
	void setFramePadding(int padding);
	virtual int getFrameWidth() const;
	virtual gColor getFrameColor();
#ifdef GTK3
	void drawBorder(cairo_t *cr);
	void drawBackground(cairo_t *cr);
#else
	void drawBorder(GdkEventExpose *e);
	void drawBackground(GdkEventExpose *e);
#endif
	
	void createBorder(GtkWidget *new_border, bool keep_widget = false);
	void createWidget();
	
	int minimumWidth() const { return _min_w; }
	int minimumHeight() const { return _min_h; }
	virtual void setMinimumSize();

	void resolveFont();

	void emitEnterEvent(bool no_leave = false);
	void emitLeaveEvent();
#ifdef GTK3
	virtual void onEnterEvent();
	virtual void onLeaveEvent();
#endif
	
	void hideButKeepFocus();
	void showButKeepFocus();
	
	int actualDirection() const;
	virtual void updateDirection();
	
	static void postDelete();

private:
	
	gContainer *pr;
	char *_name;
	GtkIMContext *_input_method;
	
	void dispose();	
};

#define SIGNAL(_signal) ((void *)_signal)

// Callbacks

// "Signals"
bool CB_control_can_raise(gControl *sender, int type);
void CB_control_finish(gControl *sender);
void CB_control_focus(gControl *sender, int type);
bool CB_control_key(gControl *sender, int type);
bool CB_control_mouse(gControl *sender, int type);
void CB_control_enter_leave(gControl *sender, int type);
bool CB_control_drag(gControl *sender);
bool CB_control_drag_move(gControl *sender);
bool CB_control_drop(gControl *sender);
void CB_control_drag_leave(gControl *sender);

#endif
