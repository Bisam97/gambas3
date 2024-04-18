/***************************************************************************

  gmenu.cpp

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

#include "widgets.h"
#include "gmainwindow.h"
#include "gapplication.h"
#include "gdesktop.h"
#include "gmouse.h"
#include "gmenu.h"
#include "CStyle.h"

//#define DEBUG_DELETE 1

typedef
	struct {
		int x;
		int y;
		}
	MenuPosition;

gMenu *gMenu::_current_popup = NULL;
int gMenu::_in_popup = 0;
int gMenu::_popup_count = 0;

static GList *menus = NULL;
static GList *menus_destroyed = NULL;

static gint my_menu_shell_enter_notify(GtkWidget *widget, GdkEventCrossing *event)
{
	GtkWidgetClass *klass = GTK_WIDGET_GET_CLASS(widget);
	GtkWidget *menu_item;
	gMenu *menu;

	if (event->mode == GDK_CROSSING_GTK_GRAB ||
			event->mode == GDK_CROSSING_GTK_UNGRAB ||
			event->mode == GDK_CROSSING_STATE_CHANGED)
		goto __PREVIOUS;

	menu_item = gtk_get_event_widget((GdkEvent*) event);
	if (!menu_item)
		goto __PREVIOUS;
	
	menu = (gMenu *)g_object_get_data(G_OBJECT(menu_item), "gambas-menu");
	if (menu)
		menu->ensureChildMenu();
	
__PREVIOUS:

	if (klass->_gtk_reserved6)
		return ((gint (*)(GtkWidget *, GdkEventCrossing *))klass->_gtk_reserved6)(widget, event);
	else
		return 0;
}

static void patch_classes(void)
{
	GtkWidgetClass *klass;
	
	klass = (GtkWidgetClass *)g_type_class_peek(GTK_TYPE_MENU_SHELL);
	if (klass->enter_notify_event != my_menu_shell_enter_notify)
	{
		//fprintf(stderr, "patch_class: %p\n", klass->enter_notify_event);
		klass->_gtk_reserved6 = (void (*)())klass->enter_notify_event;
		klass->enter_notify_event = my_menu_shell_enter_notify;
	}
	
	klass = (GtkWidgetClass *)g_type_class_peek(GTK_TYPE_MENU_BAR);
	if (klass->enter_notify_event != my_menu_shell_enter_notify)
	{
		//fprintf(stderr, "patch_class: %p\n", klass->enter_notify_event);
		klass->_gtk_reserved6 = (void (*)())klass->enter_notify_event;
		klass->enter_notify_event = my_menu_shell_enter_notify;
	}
}


static void cb_destroy(GtkWidget *object, gMenu *data)
{
	if (data->ignoreDestroy()) 
		return;
	
	delete data;
}

static void cb_activate(GtkMenuItem *menuitem, gMenu *data)
{
	if (data->ignoreActivate()) 
		return;
	
	if (data->_popup)
		return;

	if (data->radio())
		data->updateRadio();
	else if (data->toggle())
		data->updateChecked();
	else if (data->checked())
	{
		data->_ignore_activate = true;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), true);
	}
	
	//fprintf(stderr, "cb_activate: %s\n", data->name());
	CB_menu_click(data);
}

static void cb_show(GtkWidget *menu, gMenu *data)
{
	if (!data->_opened)
	{
		data->_opened = true;
		CB_menu_show(data);
		//gtk_menu_reposition(GTK_MENU(menu));
	}
}

static gboolean cb_map(GtkWidget *menu, gMenu *data)
{
	//fprintf(stderr, "cb_map: >>> %s %d\n", data->name(), data->_mapping);

	if (data->_mapping)
		return false;

	data->_mapping = true;
	
	gtk_widget_hide(gtk_widget_get_parent(menu));
	data->hideSeparators();
	gtk_widget_show(gtk_widget_get_parent(menu));
	gtk_menu_reposition(GTK_MENU(menu));

	data->_mapping = false;

	//fprintf(stderr, "cb_map: <<<\n");
	return false;
}

static gboolean cb_unmap(GtkWidget *menu, gMenu *data)
{
	//fprintf(stderr, "cb_unmap: >>> %s %d\n", data->name(), data->_mapping);
	
	if (data->_mapping)
		return false;

	data->_opened = false;
	CB_menu_hide(data);
	//gtk_widget_set_size_request(menu, -1, -1);

	//fprintf(stderr, "cb_unmap: <<<\n");
	return false;
}

static int get_menu_pos(GtkWidget *menu)
{
	GList *children, *iter;
	int pos;
	
	if (!gtk_widget_get_parent(menu))
	{
		//g_debug("get_menu_pos: no parent for menu %p", menu);
		return -1;
	}
	
	children = gtk_container_get_children(GTK_CONTAINER(gtk_widget_get_parent(menu)));
  iter = g_list_first(children);
  
  for(pos = 0;; pos++) 
  {
    if (iter->data == (gpointer)menu) 
      break; 
    iter = g_list_next(iter);
  }
  
  g_list_free(children);

  return pos;
}


void gMenu::update()
{
	GtkMenuShell *shell = NULL;
	gint pos;
	int size;
	int ds = gDesktop::scale();
	char *buf;
	
	if (!_text || !*_text)
		_style = SEPARATOR;
	else if (!_popup && (_radio || _toggle || _checked))
		_style = CHECK;
	else
		_style = NORMAL;
	
	if (_no_update)
		return;
	
	//g_debug("%p: START UPDATE (menu = %p _popup = %p parent = %p)", this, menu);
	
	if (_style != _oldstyle)
	{
		//fprintf(stderr, "update %s\n", name());
	
		if (_popup)
		{
			g_object_ref(G_OBJECT(_popup));
			if (_style == NORMAL)
				gtk_menu_item_set_submenu(menu, NULL);
		}

		if (menu)
		{
			pos = get_menu_pos(GTK_WIDGET(menu));
			//shell = (GtkMenuShell*)GTK_WIDGET(menu)->parent;
			if (_style != NOTHING)
				_ignore_destroy = true;
			gtk_widget_hide(GTK_WIDGET(menu));
			gtk_widget_destroy(GTK_WIDGET(menu));
			_shortcut_key = 0;
			_shortcut_mods = (GdkModifierType)0;
			//g_debug("%p: delete old menu/separator", this);
		}
		else
		{
			pos = -1;
			//shell = NULL;
		}
		
		if (_style != NOTHING)
		{
			if (_style == SEPARATOR)
			{
				menu = (GtkMenuItem *)gtk_separator_menu_item_new();

				hbox = NULL;
				label = NULL;
				shlabel = NULL;
				image = NULL;
				
				#ifdef GTK3
				#else
					GtkRequisition req;
					gtk_widget_size_request(GTK_WIDGET(menu), &req);
					if (req.height > 5)
						gtk_widget_set_size_request(GTK_WIDGET(menu), -1, 5);
				#endif
				//g_debug("%p: create new separator %p", this, menu);
			}
			else
			{
				if (_style == CHECK)
				{
					menu = (GtkMenuItem *)gtk_check_menu_item_new();
					if (radio())
						gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(menu), true);
					if (checked())
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), true);
				}
				else
				{
					menu = (GtkMenuItem *)gtk_menu_item_new();
				}
				//g_debug("%p: create new menu %p", this, menu);
				
				if (!_toplevel)
				{
					#ifdef GTK3
					hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, ds);
					#else
					hbox = gtk_hbox_new(false, ds);
					#endif
					//set_gdk_bg_color(hbox, 0xFF0000);
					
					image = gtk_image_new();
					//check = gtk_fixed_new();
					label = gtk_label_new_with_mnemonic("");
					shlabel = gtk_label_new("");
					
					#if GTK_CHECK_VERSION(3, 16, 0)
					gtk_label_set_xalign(GTK_LABEL(shlabel), 0);
					#else
					gtk_misc_set_alignment(GTK_MISC(shlabel), 0, 0.5);
					#endif
					gtk_size_group_add_widget(parentMenu()->getSizeGroup(), shlabel);
					
					size = window()->font()->height();

					//gtk_widget_set_size_request(check, size, size);
					//ON_DRAW(check, this, cb_check_expose, cb_check_draw);
					//g_signal_connect_after(G_OBJECT(check), "expose-event", G_CALLBACK(cb_check_expose), (gpointer)this);

					gtk_widget_set_size_request(image, size, size);
					
					gtk_container_add(GTK_CONTAINER(menu), GTK_WIDGET(hbox));
					//gtk_box_pack_start(GTK_BOX(hbox), check, false, false, 0);
					gtk_box_pack_start(GTK_BOX(hbox), image, false, false, 0);
					gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
					gtk_box_pack_end(GTK_BOX(hbox), shlabel, false, false, 0);
				}
				else
				{
					hbox = NULL;
					shlabel = NULL;
					image = NULL;
					//check = NULL;
					label = gtk_label_new_with_mnemonic("");
					gtk_container_add(GTK_CONTAINER(menu), label);
				}
				
				if (_popup)
				{
					gtk_menu_item_set_submenu(menu, GTK_WIDGET(_popup));
					g_object_unref(G_OBJECT(_popup));
				}
					
				
				//set_gdk_fg_color(label, get_gdk_fg_color(GTK_WIDGET(shell)));
				//set_gdk_bg_color(label, get_gdk_bg_color(GTK_WIDGET(shell)));
			}
			
			gtk_widget_show_all(GTK_WIDGET(menu));
			
			if (_toplevel)
			{
				gMainWindow *win = (gMainWindow *)pr;
				
				//set_gdk_fg_color(GTK_WIDGET(menu), win->foreground());
				//set_gdk_bg_color(GTK_WIDGET(menu), win->background());	
				
				//gtk_menu_shell_append(GTK_MENU_SHELL(win->menuBar), GTK_WIDGET(menu));
				shell = GTK_MENU_SHELL(win->menuBar);
			}
			else
			{
				gMenu *parent = parentMenu();
				
				if (!parent->_popup)
				{
					parent->_popup = (GtkMenu *)gtk_menu_new();
					g_object_ref_sink(parent->_popup);
					
					//fprintf(stderr, "creates a new child menu container in parent %s\n", parent->name());
					
					//g_signal_connect(G_OBJECT(parent->_popup), "size-allocate", G_CALLBACK(cb_size_allocate), (gpointer)parent);
					g_signal_connect(G_OBJECT(parent->_popup), "show", G_CALLBACK(cb_show), (gpointer)parent);
					g_signal_connect(G_OBJECT(parent->_popup), "map", G_CALLBACK(cb_map), (gpointer)parent);
					g_signal_connect(G_OBJECT(parent->_popup), "unmap", G_CALLBACK(cb_unmap), (gpointer)parent);
					gtk_widget_show_all(GTK_WIDGET(parent->_popup));
					
					parent->update();
					
					if (parent->style() == NORMAL)
						gtk_menu_item_set_submenu(parent->menu, GTK_WIDGET(parent->_popup));
					
					//parent->setColor();
				}
				shell = GTK_MENU_SHELL(parent->_popup);
			}
	
			if (shell)
			{
				patch_classes();
				
				if (pos < 0)
				{
					gtk_menu_shell_append(shell, GTK_WIDGET(menu));
					//g_debug("%p: append to parent %p", this, shell);
				}
				else
				{
					gtk_menu_shell_insert(shell, GTK_WIDGET(menu), pos);
					//g_debug("%p: insert into parent %p", this, shell);
				}
			}
			
			g_signal_connect(G_OBJECT(menu), "destroy", G_CALLBACK(cb_destroy), (gpointer)this);
			g_signal_connect(G_OBJECT(menu), "activate", G_CALLBACK(cb_activate), (gpointer)this);
			
			g_object_set_data(G_OBJECT(menu), "gambas-menu", this);
		}
		
		_oldstyle = _style;
		updateVisible();
	}
	
	if (_style == NORMAL || _style == CHECK)
	{
		gMnemonic_correctText(_text, &buf);
		gtk_label_set_text_with_mnemonic(GTK_LABEL(label), buf);
		g_free(buf);
		
		if (!_toplevel)
		{
			if (_shortcut)
			{
				buf = g_strconcat("\t", _shortcut, "  ",(void *)NULL);
				gtk_label_set_text(GTK_LABEL(shlabel), buf);
				g_free(buf);
			}
			else
				gtk_label_set_text(GTK_LABEL(shlabel), "\t");

			updatePicture();
		}
		
		//setColor();
		setFont();

		if (_disabled)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(menu), FALSE);
			updateShortcutRecursive();
		}
	}

	//g_debug("%p: END UPDATE", this);	
}

void gMenu::updatePicture()
{
	int size;
	gPicture *pic;
	
	if (!image || isTopLevel())
		return;
	
	if (!_picture)
	{
		gtk_image_set_from_pixbuf(GTK_IMAGE(image), NULL);
		return;
	}
	
	gtk_widget_get_size_request(image, NULL, &size);
	size = size & ~3;
	
	pic = _picture->stretch(size, size, true);
	if (_disabled)
		pic->makeGray();
	
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pic->getPixbuf());
	
	delete pic;
}

void gMenu::initialize()
{
	//fprintf(stderr, "gMenu::gMenu: %p (%p)\n", this, pr);
	
	hFree = NULL;
	_popup = NULL;
	image = NULL;
	label = NULL;
	shlabel = NULL;
	//check = NULL;
	menu = NULL;
	_toplevel = false;
	
	_text = NULL;
	_shortcut = NULL;
	_shortcut_key = 0;
	_shortcut_mods = (GdkModifierType)0;
	_checked = false;
	_picture = NULL;
	_name = NULL;
	_toggle = false;
	_radio = false;

	_style = NOTHING;
	_oldstyle = NOTHING;
	
	_ignore_destroy = false;
	_ignore_activate = false;
	_no_update = false;
	_destroyed = false;
	_delete_later = false;
	_action = false;
	_visible = false;
	_opened = false;
	_exec = false;
	_disabled = false;
	_mapping = false;
	_proxy_for = false;
	
	_proxy = NULL;
	
	sizeGroup = NULL;
	_children = NULL;
	
	menus = g_list_append (menus,(gpointer)this);
}


static gboolean cb_menubar_changed(GtkWidget *widget, gMainWindow *data)
{
	data->configure();
	return false;
}

gMenu::gMenu(gMainWindow *par, bool hidden)
{
	pr = (gpointer)par;

	if (!par->menuBar)
	{
		par->menuBar = (GtkMenuBar*)gtk_menu_bar_new();
		g_signal_connect_after(G_OBJECT(par->menuBar), "map", G_CALLBACK(cb_menubar_changed), (gpointer)par);
		g_signal_connect(G_OBJECT(par->menuBar),"unmap", G_CALLBACK(cb_menubar_changed), (gpointer)par);
		par->embedMenuBar(par->border);
	}
	
  initialize();
	_toplevel = true;
	
	accel = par->accel;
	g_object_ref(accel);
	
	setText(NULL);
	setVisible(!hidden);
}

gMenu::gMenu(gMenu *par, bool hidden)
{
	pr = (gpointer)par;
	
	initialize();
	//sizeGroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	
	if (!par) return;
	if (!par->menu) return;
	
	par->insert(this);
	
	accel = par->accel;
	g_object_ref(accel);
	
	setText(NULL);
	setVisible(!hidden);
}

void gMenu::dispose()
{
	GList *item;
	gMenu *mn;
	int i;

	if (_destroyed)
		return;
	
	#if DEBUG_DELETE
	fprintf(stderr, "dispose: %s %p --> %p\n", name(), this, pr);
	#endif
	
	setProxy(NULL);
	ensureChildMenu();
	
  if (_proxy_for)
	{
		#define CLEAR_POINTER(_field) if ((_field) == (void *)this) _field = NULL
		
		item = g_list_first(menus);
		while (item)
		{
			mn = (gMenu*)item->data;
			//CLEAR_POINTER(mn->pr);
			CLEAR_POINTER(mn->_proxy);
			item = g_list_next(item);
		}
	}
	
	if (_children)
	{
		for (i = 0; i < childCount(); i++)
			child(i)->removeParent();
		g_ptr_array_unref(_children);
		_children = NULL;
	}

	//if (_style != NOTHING)
	{
		if (shlabel && (!_toplevel) && pr)
			gtk_size_group_remove_widget(((gMenu*)pr)->sizeGroup, shlabel);
		
		if (sizeGroup) 
			g_object_unref(G_OBJECT(sizeGroup));
		
		if (accel)
			g_object_unref(accel);	
	}

	gMenu *parent = parentMenu();
	if (parent)
	{
		parent->remove(this);
		parent = NULL;
	}

	menus = g_list_remove(menus, (gpointer)this);
	
	#if DEBUG_DELETE
	fprintf(stderr, "dispose: >>> %s\n", name());
	#endif
}

gMenu::~gMenu()
{
	#if DEBUG_DELETE
	fprintf(stderr, "~gMenu: %s %p --> %p\n", name(), this, pr);
	#endif
	
	dispose();
  
	_no_update = true;

	setText(NULL);
	setPicture(NULL);
	setShortcut(NULL);
	
	_style = NOTHING;
	
	if (_popup)
		g_object_unref(_popup);
	
	if (_current_popup == this)
		_current_popup = NULL;

	menus_destroyed = g_list_remove(menus_destroyed, this);
	
	#if DEBUG_DELETE
	fprintf(stderr, "~gMenu: >>> %s\n", name());
	#endif
	CB_menu_finish(this);
}

void gMenu::setEnabled(bool vl)
{
	if (vl != _disabled)
		return;
	
	_disabled = !vl;
	gtk_widget_set_sensitive(GTK_WIDGET(menu), vl);
	updateShortcutRecursive();
}

bool gMenu::isFullyEnabled() const
{
	const gMenu *menu = this;
	
	for(;;)
	{
		if (menu->_exec)
			return true;

		if (!menu->isEnabled() || !menu->isVisible())
			return false;

		if (menu->isTopLevel())
			return true;

		menu = menu->parentMenu();
	}
}

void gMenu::updateShortcut()
{
	if (_no_update)
		return;
	
	if (isTopLevel())
		return;
	
	if (_shortcut_key)
	{
		//fprintf(stderr, "gtk_widget_remove_accelerator: (%s %p) accel = %p (%d,%d)\n", name(), menu, accel, _shortcut_key, _shortcut_mods);
		gtk_widget_remove_accelerator(GTK_WIDGET(menu), accel, _shortcut_key, _shortcut_mods);
		_shortcut_key = 0;
	}
	
	if (isVisible() && isFullyEnabled() && _shortcut)
	{
		gt_shortcut_parse(_shortcut, &_shortcut_key, &_shortcut_mods);
		if (_shortcut_key)
		{
			//fprintf(stderr, "gtk_widget_add_accelerator: (%s %p) accel = %p (%d,%d)\n", name(), menu, accel, _shortcut_key, _shortcut_mods);
			gtk_widget_add_accelerator(GTK_WIDGET(menu), "activate", accel, _shortcut_key, _shortcut_mods, (GtkAccelFlags)0);
		}
	}
}

void gMenu::updateShortcutRecursive()
{
	gMenu *ch;
	int i;
	
	/*if (_exec)
		return;*/

	updateShortcut();

	for (i = 0;; i++)
	{
		ch = child(i);
		if (!ch)
			break;
		ch->updateShortcutRecursive();
	}
}

void gMenu::setText(const char *text)
{
	g_free(_text);
	if (text)
		_text = g_strdup(text);
	else
		_text = NULL;
		
	update();
}

bool gMenu::isVisible() const
{
	if (!menu) return false;
	return _visible;	
}

void gMenu::updateVisible()
{
	bool vl = _visible;
	
	if (_toplevel && _style != NORMAL)
		vl = false;
	
	//fprintf(stderr, "gMenu::updateVisible: %s '%s' %d\n", name(), text(), vl);
	
	gtk_widget_set_visible(GTK_WIDGET(menu), vl);
	updateShortcutRecursive();
	//g_object_set(G_OBJECT(menu),"visible",vl,(void *)NULL);
	
	if (_toplevel && pr)
		((gMainWindow *)pr)->checkMenuBar();
}

void gMenu::setVisible(bool vl)
{
	if (!menu) return;
	if (vl == _visible) return;
	
	_visible = vl;
	updateVisible();
}

void gMenu::setPicture(gPicture *pic)
{
	//fprintf(stderr, "gMenu::setPicture: %p\n", pic);
	gPicture::assign(&_picture, pic);
	update();
}

void gMenu::setChecked(bool vl)
{
	if (vl == _checked || _popup)
		return;
	
	_checked = vl;
	if (_toggle || _radio)
	{
		_ignore_activate = true;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), vl);
	}
	else
		update();
}

void gMenu::setToggle(bool vl)
{
	if (vl == _toggle)
		return;

	_toggle = vl;
	update();
}

void gMenu::setRadio(bool vl)
{
	if (vl == _radio)
		return;
	
	_radio = vl;
	update();
}

int gMenu::childCount() const
{
	if (!_children)
		return 0;
	else
		return _children->len;
}

gMenu *gMenu::child(int index) const
{
	if (!_children || index < 0 || index >= (int)_children->len)
		return NULL;
	else
		return (gMenu *)g_ptr_array_index(_children, index);
}

void gMenu::destroy()
{
	if (_destroyed)
		return;
	
	hide();
	dispose();
	menus_destroyed = g_list_prepend(menus_destroyed, (gpointer)this);
	_destroyed = true;
}

#if GTK_CHECK_VERSION(3, 22, 0)
#else
static void position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, MenuPosition *pos)
{
	*x = pos->x;
	*y = pos->y;
	*push_in = true;
}
#endif

void gMenu::doPopup(bool move, int x, int y, gControl *ref)
{
	if (!_popup)
		return;
	
	gMenu *save_current_popup = _current_popup;
	GtkWidget *save_grab = gApplication::_popup_grab;

	_current_popup = this;
	gApplication::_popup_grab = GTK_WIDGET(_popup);
	
	_in_popup++;
	_popup_count++;
	_exec = true;
	updateShortcutRecursive();

#if GTK_CHECK_VERSION(3, 22, 0)

	GdkWindow *win;
	GdkRectangle rect;
	GdkEvent *event;
	GdkEvent *last_event;
	
	gt_disable_warnings(true);
	
	event = gdk_event_new(GDK_BUTTON_PRESS);
	event->button.time = gApplication::lastEventTime(); //GDK_CURRENT_TIME;
	
	last_event = gApplication::lastEvent();
	if (last_event && last_event->type == GDK_BUTTON_PRESS)
	{
		event->button.button = last_event->button.button;
		event->button.window = last_event->button.window;
	}
	else
	{
		event->button.button = 1;
		event->button.window = ref ? gtk_widget_get_window(ref->window()->border) : gtk_widget_get_window(window()->border);
	}
	
	gdk_event_set_device(event, gMouse::getPointer());
	
	if (move)
	{
		win = gdk_event_get_window(event);
		gdk_window_get_origin(win, &rect.x, &rect.y);

		rect.x = x - rect.x;
		rect.y = y - rect.y;
		rect.width = rect.height = 1;
		
		gtk_menu_popup_at_rect(_popup, win, &rect, GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_NORTH_WEST, event);
	}
	else
		gtk_menu_popup_at_pointer(_popup, event);

	gt_disable_warnings(false);
	
	event->button.window = NULL;
	gdk_event_free(event);
	
#else
	
	MenuPosition *pos = NULL;
	
	if (move)
	{
		pos = new MenuPosition;
		pos->x = x;
		pos->y = y;
	}
	
	gtk_menu_popup(_popup, NULL, NULL, move ? (GtkMenuPositionFunc)position_menu : NULL, (gpointer)pos, 0, gApplication::lastEventTime());

#endif
	
#if GTK_CHECK_VERSION(2, 20, 0)
	while (_current_popup && _popup && gtk_widget_get_mapped(GTK_WIDGET(_popup)))
#else
	while (_current_popup && _popup && GTK_WIDGET_MAPPED(_popup))
#endif
		MAIN_do_iteration(false);

	_exec = false;
	updateShortcutRecursive();

	_current_popup = save_current_popup;
	gApplication::_popup_grab = save_grab;

	_in_popup--;

#ifdef GTK3
#else
	if (pos)
		delete pos;
#endif
	
	// flush the event loop so that the main window is reactivated before the click menu event is raised

	while (gApplication::eventsPending())
		MAIN_do_iteration(false);
}

void gMenu::popup(int x, int y, gControl *ref)
{
	doPopup(true, x, y, ref);
}

void gMenu::popup()
{
	doPopup(false);
}

void gMenu::close()
{
	if (!_popup)
		return;
	
	gtk_menu_popdown(_popup);
}

int gMenu::winChildCount(gMainWindow *par)
{
	GList *item;
	gMenu *mn;
	int ct=0;
	
	if (!menus) return 0;
	
	item=g_list_first(menus);
	while (item)
	{
		mn=(gMenu*)item->data;
		if (mn->pr == (void*)par) ct++;
		item=g_list_next(item);
	}
	
	return ct;
}

gMenu* gMenu::winChildMenu(gMainWindow *par,int pos)
{
	GList *item;
	gMenu *mn;
	int ct=0;
	
	if (!menus) return NULL;
	
	item=g_list_first(menus);
	while (item)
	{
		mn=(gMenu*)item->data;
		if (mn->pr == (void*)par)
		{
			if (ct==pos) return mn;
			ct++;
		}
		item=g_list_next(item);
	}
	
	return NULL;
}

gMenu *gMenu::findFromName(gMainWindow *win, const char *name)
{
	int i;
	int count;
	gMenu *menu;
	
	for(;;)
	{
		count = winChildCount(win);
		for (i = 0; i < count; i++)
		{
			menu = winChildMenu(win, i);
			if (!strcasecmp(menu->name(), name))
				return menu;
		}
		
		if (!win->parent())
			break;
		win = win->parent()->window();
		if (!win)
			break;
	}
	
	return NULL;
}

void gMenu::setShortcut(char *shortcut)
{
	if (_shortcut)
	{
		g_free(_shortcut);
		_shortcut = NULL;
	}

	if (shortcut)
		_shortcut = g_strdup(shortcut);
	
	updateShortcut();
	update();
}

gMainWindow *gMenu::window()
{
  if (!pr)
    return NULL;

  if (_toplevel)
    return (gMainWindow *)pr;
    
  return ((gMenu *)pr)->window();
}

void gMenu::setName(char *name)
{
	if (_name)
	{
		g_free(_name);
		_name = NULL;
	}
	
	if (name) 
		_name = g_strdup(name);
}

void gMenu::hideSeparators()
{
	gMenu *ch;
	gMenu *last_ch;
	bool is_sep;
	bool last_sep;
	//bool show_check = false;
	bool show_image = false;
	int i;

	if (!_popup)
		return;
	
	last_sep = true;
	last_ch = 0;
	
	for (i = 0; i < childCount(); i++)
	{
		ch = child(i);
		
		is_sep = ch->style() == SEPARATOR;
		
		if (is_sep)
		{
			if (last_sep)
			{
				ch->hide();
			}
			else
			{
				ch->show();
				last_sep = true;
				last_ch = ch;
			}
		}
		else
		{
			if (ch->isVisible())
			{
				ch->ensureChildMenu();
				last_sep = false;
				/*if (ch->radio() || ch->toggle() || ch->checked())
					show_check = true;*/
				if (ch->picture())
					show_image = true;
			}
		}
	}
	
	if (last_sep && last_ch)
		last_ch->hide();

	for (i = 0; i < childCount(); i++)
	{
		ch = child(i);
		if (!ch->image || !ch->isVisible())
			continue;
		
		if (show_image)
			gtk_widget_show(ch->image);
		else
			gtk_widget_hide(ch->image);
	}
}

void gMenu::setFont()
{
	gMainWindow *win = window();
#ifdef GTK3
	if (label) gt_widget_update_css(GTK_WIDGET(label), win->font(), GB_COLOR_DEFAULT, GB_COLOR_DEFAULT);
	if (shlabel) gt_widget_update_css(GTK_WIDGET(shlabel), win->font(), GB_COLOR_DEFAULT, GB_COLOR_DEFAULT);
#else
	if (label) gtk_widget_modify_font(GTK_WIDGET(label), win->font()->desc());
	if (shlabel) gtk_widget_modify_font(GTK_WIDGET(shlabel), win->font()->desc());
#endif
}

/*void gMenu::setColor()
{
	gMainWindow *win = window();
	
	if (pr == win)
	{
		if (label) set_gdk_fg_color(GTK_WIDGET(label), win->foreground());
	}
	//if (shortcut) set_gdk_fg_color(GTK_WIDGET(shortcut), win->foreground());
}*/

void gMenu::updateColor(gMainWindow *win)
{
	//GList *item;
	//gMenu *mn;

	if (!win->menuBar)
		return;
	
	#ifdef GTK3
	gt_widget_update_css(GTK_WIDGET(win->menuBar), NULL, win->background(), win->foreground());
	#else
	set_gdk_bg_color(GTK_WIDGET(win->menuBar), win->background());
	set_gdk_fg_color(GTK_WIDGET(win->menuBar), win->foreground());
	#endif

	/*if (!menus) 
		return;
	
	item = g_list_first(menus);
	while (item)
	{
		mn = (gMenu*)item->data;
		//if (mn->pr == (void*)win)
			mn->setColor();
		item = g_list_next(item);
	}*/
}

void gMenu::updateFont(gMainWindow *win)
{
	GList *item;
	gMenu *mn;
	
	if (win->menuBar)
	{
		//fprintf(stderr, "set menu bar font\n");
#ifdef GTK3
		gt_widget_update_css(GTK_WIDGET(win->menuBar), win->ownFont() ? win->font() : NULL, GB_COLOR_DEFAULT, GB_COLOR_DEFAULT);
#else
		gtk_widget_modify_font(GTK_WIDGET(win->menuBar), win->ownFont() ? win->font()->desc() : NULL);
#endif
	}
	
	if (!menus) 
		return;
	
	item = g_list_first(menus);
	while (item)
	{
		mn = (gMenu*)item->data;
		if (mn->pr == (void*)win)
			mn->setFont();
		item=g_list_next(item);
	}
}

void gMenu::updateRadio()
{
	gMenu *parent, *ch;
	int i;
	int start = -1;

	parent = parentMenu();
	if (!parent)
		return;
	
	for (i = 0; i < parent->childCount(); i++)
	{
		ch = parent->child(i);
		if (ch->radio())
		{
			if (start < 0)
				start = i;
			if (ch == this)
				break;
		}
		else
			start = -1;
	}

	if (start >= 0)
	{
		for (i = start; i < parent->childCount(); i++)
		{
			ch = parent->child(i);
			if (!ch->radio())
				break;

			ch->setChecked(ch == this);
		}
	}
}

bool gMenu::setProxy(gMenu *proxy)
{
	gMenu *check = proxy;

	while (check)
	{
		if (check == this)
			return true;

		check = check->_proxy;
	}

	_proxy = proxy;
	if (proxy)
		proxy->_proxy_for = true;
	
	return false;
}

GtkMenu *gMenu::getSubMenu()
{
	if (_proxy)
		return _proxy->getSubMenu();
	else
		return _popup;
}

void gMenu::ensureChildMenu()
{
	GtkMenu *sub_menu = getSubMenu();
	
	// TODO: create parent menu?
	if (sub_menu && gtk_menu_item_get_submenu(menu) != (GtkWidget *)sub_menu)
	{
		//fprintf(stderr, "ensureChildMenu: %p\n", sub_menu);
		g_object_ref(sub_menu);
		/*attach = gtk_menu_get_attach_widget(sub_menu);
		if (attach)
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(attach), NULL);*/
		if (gtk_menu_get_attach_widget(sub_menu))
			gtk_menu_detach(sub_menu);
		gtk_menu_item_set_submenu(menu, GTK_WIDGET(sub_menu));
		g_object_unref(sub_menu);
	}
}


GtkSizeGroup *gMenu::getSizeGroup()
{
	if (!sizeGroup)
		sizeGroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	return sizeGroup;
}

void gMenu::insert(gMenu *child)
{
	if (!_children)
		_children = g_ptr_array_new();
	
	g_ptr_array_add(_children, child);
}

void gMenu::remove(gMenu *child)
{
	g_ptr_array_remove(_children, child);
}

void gMenu::willBeDeletedLater()
{
	//gMenu *parent = parentMenu();
	_delete_later = TRUE;
	dispose();
}

void gMenu::removeParent()
{
	#if DEBUG_DELETE
	fprintf(stderr, "removeParent: %s %p --> %p\n", name(), this, pr);
	#endif
	pr = NULL;
}

void gMenu::updateChecked()
{
	if (_style == CHECK)
		_checked = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu));
	else
		_checked = false;
}

bool gMenu::ignoreDestroy()
{
	if (_ignore_destroy)
	{
		_ignore_destroy = false;
		return true;
	}
	else
		return false;
}

bool gMenu::ignoreActivate()
{
	if (_ignore_activate)
	{
		_ignore_activate = false;
		return true;
	}
	else
		return false;
}

void gMenu::cleanRemovedMenus()
{
	GList *iter;
	gMenu *menu;

	if (!menus_destroyed) return;

	for(;;)
	{
		iter = g_list_first(menus_destroyed);
		if (!iter)
			break;
		menu = (gMenu *)iter->data;
		gtk_widget_destroy(GTK_WIDGET(menu->menu));
	}

	menus_destroyed = NULL;
}
