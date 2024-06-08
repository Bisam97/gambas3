/***************************************************************************

  gapplication.cpp

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

#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include "widgets.h"

#ifndef GTK3
#include "x11.h"
#include "sm/sm.h"
#endif

#include "gapplication.h"
#include "gtrayicon.h"
#include "gdesktop.h"
#include "gkey.h"
#include "gmenu.h"
#include "gdialog.h"
#include "gclipboard.h"
#include "gmouse.h"
#include "gprinter.h"
#include "gmainwindow.h"

//#define DEBUG_ENTER_LEAVE 1
//#define DEBUG_FIND_CONTROL 1
//#define DEBUG_FOCUS 1

#ifdef GTK3
static GtkApplication *_app;
GtkStyleProvider *_tooltip_css = NULL;
#endif

static bool _debug_keypress = false;

/**************************************************************************

	Global event handler

**************************************************************************/

static bool _focus_change = false;
static bool _doing_focus_change = false;

static GtkWindowGroup *get_window_group(GtkWidget *widget)
{
  GtkWidget *toplevel = NULL;

  if (widget)
    toplevel = gtk_widget_get_toplevel(widget);

  if (GTK_IS_WINDOW(toplevel))
    return gtk_window_get_group(GTK_WINDOW(toplevel));
  else
    return gtk_window_get_group(NULL);
}

static gControl *find_child(gControl *control, int rx, int ry, gControl *button_grab = NULL, bool skip_ignore_mouse = false)
{
	gContainer *cont;
	gControl *child;
	gMainWindow *window;
	int x, y;
	int cx, cy, cw, ch;
	#ifdef GTK3
	GtkAllocation a;
	#endif

	if (gApplication::_control_grab)
		return gApplication::_control_grab;

	if (button_grab)
	{
		#if DEBUG_FIND_CONTROL
		fprintf(stderr, "find_child -> %s (button grab)\n", button_grab->name());
		#endif
		return button_grab;
	}

	window = control->topLevel();
	control = window;

	if (skip_ignore_mouse && control->isIgnoreMouse())
		return NULL;
	
	#ifdef GTK3
	gtk_widget_get_allocation(window->frame, &a);
	//fprintf(stderr, "find_child: %d %d window: %d %d %d %d\n", rx, ry, a.x, a.y, a.width, a.height);
	rx -= a.x;
	ry -= a.y;
	#endif

	#if DEBUG_FIND_CONTROL
	fprintf(stderr, "find_child: [%s %p] %s (%d %d)\n", window->name(), window, control->name(), rx, ry);
	#endif

	while (control->isContainer())
	{
		control->getScreenPos(&x, &y);
		#ifdef GTK3
		if (!control->isTopLevel())
		{
			x -= a.x;
			y -= a.y;
		}
		#endif

		#if DEBUG_FIND_CONTROL
		fprintf(stderr, "  screen pos %s = %d %d\n", control->name(), x ,y);
		#endif
		
		cont = (gContainer *)control;

		cx = cont->clientX();
		cy = cont->clientY();
		cw = cont->clientWidth();
		ch = cont->clientHeight();

		#if DEBUG_FIND_CONTROL
		fprintf(stderr, "  client area of %s: %d %d %d %d\n", control->name(), cx, cy, cw, ch);
		#endif

		x = rx - x;
		y = ry - y;
		if (x < cx || y < cy || x >= (cx + cw) || y >= (cy + ch))
		{
			#if DEBUG_FIND_CONTROL
			fprintf(stderr, "  outside of client area of %s\n", control->name());
			#endif
			control = NULL;
			break;
		}

		#if DEBUG_FIND_CONTROL
		fprintf(stderr, "  find coord %d %d\n", x, y);
		#endif
		child = cont->find(x, y, skip_ignore_mouse);
		if (!child)
			break;

		control = child;
	}

	#if DEBUG_FIND_CONTROL
	fprintf(stderr, "find_child -> %s\n", control ? control->name() : "NULL");
	#endif

	return control;
}

void gApplication::checkHoveredControl(gControl *control)
{
	if (gApplication::_enter != control)
	{
		#if DEBUG_ENTER_LEAVE
		fprintf(stderr, "checkHoveredControl: %s\n", control->name());
		#endif

		gControl *leave = gApplication::_enter;

		while (leave && leave != control && !leave->isAncestorOf(control))
		{
			#if DEBUG_ENTER_LEAVE
			fprintf(stderr, "checkHoveredControl: leave: %s\n", leave->name());
			#endif
			leave->emitLeaveEvent();
			leave = leave->parent();
		}

		#if DEBUG_ENTER_LEAVE
		fprintf(stderr, "checkHoveredControl: _enter <- %s\n", control ? control->name() : "ø");
		#endif

		if (control)
		{
			#if DEBUG_ENTER_LEAVE
			fprintf(stderr, "checkHoveredControl: enter: %s\n", control->name());
			#endif
			control->emitEnterEvent();
		}
	}
}

static void gambas_handle_event(GdkEvent *event)
{
  GtkWidget *widget;
	GtkWidget *current_grab;
  GtkWidget *grab;
	GtkWindowGroup *group;
#ifdef GTK3
	GdkDevice *device;
#endif
	gControl *control = NULL, *save_control;
	gControl *button_grab;
	int x, y, xs, ys, xc, yc;
	bool cancel;
	int type;
	bool handle_event = false;
	bool send_to_window = false;

	if (gApplication::_fix_printer_dialog)
	{
		widget = gtk_get_event_widget(event);
		if (widget)
		{
			//fprintf(stderr, "type: %s\n", G_OBJECT_TYPE_NAME(widget));
			if (!strcmp(G_OBJECT_TYPE_NAME(gtk_widget_get_toplevel(widget)), "GtkPrintUnixDialog"))
			{
				if (event->type == GDK_WINDOW_STATE)
				{
					//fprintf(stderr, "event: GDK_WINDOW_STATE!\n");
					widget = gtk_window_get_default_widget(GTK_WINDOW(gtk_widget_get_toplevel(widget)));
					if (widget && GTK_IS_BUTTON(widget))
					{
						GtkPrintUnixDialog *dialog = GTK_PRINT_UNIX_DIALOG(gtk_widget_get_toplevel(widget));
						gPrinter::fixPrintDialog(dialog);
						gApplication::_fix_printer_dialog = false;
						//fprintf(stderr, "gtk_button_clicked: %s\n", gtk_button_get_label(GTK_BUTTON(widget)));
						if (gApplication::_close_next_window)
							gtk_button_clicked(GTK_BUTTON(widget));
						gApplication::_close_next_window = false;
						//return;
						//g_timeout_add(0, (GSourceFunc)close_dialog, GTK_BUTTON(widget));
						goto __HANDLE_EVENT;
					}
					//fprintf(stderr, "event: MAP! <<< end\n");
				}
			}
		}
	}

	/*if (event->type == GDK_GRAB_BROKEN)
	{
		if (gApplication::_in_popup)
			fprintf(stderr, "**** GDK_GRAB_BROKEN inside popup: %s %swindow = %p grab_window = %p popup_window = %p\n", event->grab_broken.keyboard ? "keyboard" : "pointer",
							event->grab_broken.implicit ? "implicit " : "", event->grab_broken.window, event->grab_broken.grab_window, gApplication::_popup_grab_window);
	}*/
	
	if (!((event->type >= GDK_MOTION_NOTIFY && event->type <= GDK_FOCUS_CHANGE) || event->type == GDK_SCROLL))
		goto __HANDLE_EVENT;

	if (gApplication::_disable_input_events)
	{
		switch(event->type)
		{
			case GDK_ENTER_NOTIFY:
			case GDK_LEAVE_NOTIFY:
			case GDK_BUTTON_PRESS:
			case GDK_2BUTTON_PRESS:
			case GDK_BUTTON_RELEASE:
			case GDK_MOTION_NOTIFY:
			case GDK_SCROLL:
			case GDK_KEY_PRESS:
			case GDK_KEY_RELEASE:
				//gApplication::pushInputEvent(event);
				return;
			default:
				;
		}
	}

	widget = gtk_get_event_widget(event);
	
	if (!widget)
		goto __HANDLE_EVENT;
	
	if (_debug_keypress && (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE))
	{
		fprintf(stderr, "[%p] %s: keyval = %d state = %08X (%08X) is_modifier = %d hardware = %d send_event = %d for %p\n", event, event->type == GDK_KEY_PRESS ? "GDK_KEY_PRESS" : "GDK_KEY_RELEASE",
						event->key.keyval, event->key.state, event->key.state & ~GDK_MODIFIER_MASK, event->key.is_modifier, event->key.hardware_keycode, event->key.send_event, widget);
	}

	/*if ((event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE))
	{
		if (event->key.state & ~GDK_MODIFIER_MASK) // == 0)
		{
			if (_debug_keypress)
				fprintf(stderr, "ignore key event\n");
			goto __HANDLE_EVENT;
		}
	}*/

#ifdef GTK3
	device = gdk_event_get_device (event);
	group = get_window_group(widget);
	current_grab = gtk_window_group_get_current_device_grab(group, device);
	if (!current_grab)
		current_grab = gtk_window_group_get_current_grab(group); //gtk_grab_get_current();
#else
	group = get_window_group(widget);
	current_grab = gtk_window_group_get_current_grab(group); //gtk_grab_get_current();
#endif

	button_grab = gApplication::_button_grab;

	if (gApplication::_control_grab)
	{
		control = gApplication::_control_grab;
		widget = control->border;
		//fprintf(stderr, "[1] _control_grab: %s -> widget = %p\n", control->name(), widget);
		goto __FOUND_WIDGET;
	}

	if (gMenu::currentPopup())
	{
		grab = GTK_WIDGET(gMenu::currentPopup()->_popup);
		//fprintf(stderr, "[2] popup menu: grab = %p\n", grab);
		if (get_window_group(grab) != get_window_group(widget) && (event->type == GDK_ENTER_NOTIFY || event->type == GDK_LEAVE_NOTIFY))
			goto __RETURN;
	}
	else
	{
		grab = current_grab; //gtk_window_group_get_current_grab(get_window_group(widget));
		//fprintf(stderr, "[3] popup: grab = %p / %p / %p\n", gApplication::_popup_grab, grab, gtk_grab_get_current());
		if (!grab)
			grab = gApplication::_popup_grab;
		//fprintf(stderr, "[4] grab = %p\n", grab);
		//fprintf(stderr, "search grab for widget %p -> group = %p -> grab = %p WINDOW = %d\n", widget, get_window_group(widget), grab, GTK_IS_WINDOW(grab));
		//if (grab && grab != widget && !GTK_IS_WINDOW(grab))
		//	goto __HANDLE_EVENT;

		//if (!grab && gApplication::_popup_grab)
		//	grab = gApplication::_popup_grab;
	}
		//gdk_window_get_user_data(gApplication::_popup_grab_window, (gpointer *)&grab);

	if (grab)
	{
		control = gt_get_control(grab);
		//fprintf(stderr, "grab = %p -> %p %s\n", grab, control, control ? control->name() : "");

		if (!control)
			goto __HANDLE_EVENT;
	}

	if (event->type == GDK_FOCUS_CHANGE)
	{
		control = NULL;
		//if (GTK_IS_WINDOW(widget))
		control = gt_get_control(widget);
		
		if (!control || control->isDesign())
			goto __HANDLE_EVENT;
		//fprintf(stderr, "GDK_FOCUS_CHANGE: widget = %p %d : %s %d\n", widget, GTK_IS_WINDOW(widget), control ? control->name() : NULL, event->focus_change.in);

		//if (GTK_IS_WINDOW(widget))
		{
			//control = gt_get_control(widget);
			if (control)
				gApplication::setActiveControl(control, event->focus_change.in);
			else if (event->focus_change.in)
			{
				//fprintf(stderr, "GDK_FOCUS_CHANGE: setActiveWindow(NULL)\n");
				gMainWindow::setActiveWindow(NULL);
			}
		}

		if (event->focus_change.in && grab && widget != grab && !gtk_widget_is_ancestor(widget, grab))
		{
			//fprintf(stderr, "Check popup grab\n");
			gApplication::grabPopup();
			// Must continue, otherwise things are broken by some styles
			//return;
		}

		goto __HANDLE_EVENT;
	}

	if (grab && widget != grab && !gtk_widget_is_ancestor(widget, grab))
	{
		//fprintf(stderr, "-> widget = grab\n");
		widget = grab;
	}

	//fprintf(stderr, "grab = %p widget = %p %d\n", grab, widget, grab && !gtk_widget_is_ancestor(widget, grab));

	while (widget)
	{
		control = gt_get_control(widget);
		if (control || grab)
			break;
		widget = gtk_widget_get_parent(widget);
	}

	/*if (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE || event->type == GDK_MOTION_NOTIFY)
	{
		fprintf(stderr, "[%s] widget = %p grab = %p _popup_grab = %p _button_grab = %p\n",
						event->type == GDK_BUTTON_PRESS ? "down" : event->type == GDK_BUTTON_RELEASE ? "up" : "move",
						widget, grab, gApplication::_popup_grab, gApplication::_button_grab);
		//fprintf(stderr, "widget = %p (%p) grab = %p (%p)\n", widget, widget ? g_object_get_data(G_OBJECT(widget), "gambas-control") : 0,
		//				grab, grab ? g_object_get_data(G_OBJECT(grab), "gambas-control") : 0);
	}*/

	/*if (event->type == GDK_BUTTON_PRESS || event->type == GDK_KEY_PRESS)
	{
		fprintf(stderr, "[GDK_BUTTON_PRESS] widget = %p control = %p grab = %p _popup_grab = %p _button_grab = %p\n",
						widget, control, grab, gApplication::_popup_grab, gApplication::_button_grab);
	}*/

	if (!widget || !control)
		goto __HANDLE_EVENT;

__FOUND_WIDGET:

	//fprintf(stderr, "control = %p %s\n", control, control->name());

	/*switch ((int)event->type)
	{
		case GDK_ENTER_NOTIFY:
			fprintf(stderr, "ENTER: %p %s\n", control, control ? control->name() : 0);
			break;

		case GDK_LEAVE_NOTIFY:
			fprintf(stderr, "LEAVE: %p %s\n", control, control ? control->name() : 0);
			break;
	}*/

	//group = get_window_group(widget);
	//if (group != gApplication::currentGroup())
	//	goto __HANDLE_EVENT;

	cancel = false;

	gApplication::updateLastEvent(event);

	switch ((int)event->type)
	{
		case GDK_ENTER_NOTIFY:

			if (event->crossing.detail == GDK_NOTIFY_INFERIOR)
				break;
			
			control = find_child(control, (int)event->crossing.x_root, (int)event->crossing.y_root);
			if (!control)
				goto __HANDLE_EVENT;

#if DEBUG_ENTER_LEAVE
			fprintf(stderr, "GDK_ENTER_NOTIFY: %s (%s) %d %d %p [%d] %p\n", control->name(), gApplication::_enter ? gApplication::_enter->name() : "ø", (int)event->crossing.x_root, (int)event->crossing.y_root, event->crossing.window, gdk_window_is_input_only(event->crossing.window), event->crossing.subwindow);
#endif

			if (button_grab)
			{
				gApplication::_enter_after_button_grab = control;
				break;
			}

			if (gApplication::_leave)
			{
				if (gApplication::_leave == control || gApplication::_leave->isAncestorOf(control))
					gApplication::_leave = NULL;
			}

			gApplication::checkHoveredControl(control);

			/*
			if (gApplication::_leave == control)
			{
				#if DEBUG_ENTER_LEAVE
				fprintf(stderr, "enter ignored: %s\n", control->name());
				#endif
				gApplication::_leave = NULL;
			}
			else if (gApplication::_enter != control)
			{
				if (check_crossing_event(event))
				{
					#if DEBUG_ENTER_LEAVE
					fprintf(stderr, "enter: %s\n", control->name());
					#endif
					gApplication::checkHoveredControl(control);
				}
			}*/

			break;

		case GDK_LEAVE_NOTIFY:

#if DEBUG_ENTER_LEAVE
			fprintf(stderr, "GDK_LEAVE_NOTIFY: %s (%d %d) %p %p\n", control->name(), event->crossing.mode, event->crossing.detail, event->crossing.window, event->crossing.subwindow);
#endif

			if (button_grab)
				break;

			if (event->crossing.detail == GDK_NOTIFY_INFERIOR)
				break;
			
			//control = find_child(control, (int)event->button.x_root, (int)event->button.y_root);

			gApplication::_leave = control;
			/*
			if (gdk_events_pending() && gApplication::_leave == NULL)
			{
				if (check_crossing_event(event))
				{
					#if DEBUG_ENTER_LEAVE
					fprintf(stderr, "leave later: %s\n", control->name());
					#endif
					gApplication::_leave = control;
				}
			}
			else if (gApplication::_leave != control)
			{
				if (check_crossing_event(event))
				{
					if (gApplication::_leave == control)
						gApplication::_leave = NULL;

					#if DEBUG_ENTER_LEAVE
					fprintf(stderr, "leave: %s\n", control->name());
					#endif
					control->emitLeaveEvent();
				}
			}
			*/

			//if (widget != control->border && widget != control->widget)
			//	goto __RETURN;

			break;

		case GDK_BUTTON_PRESS:
		case GDK_2BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
		{
			/*if (event->type == GDK_BUTTON_PRESS)
				fprintf(stderr, "GDK_BUTTON_PRESS: %p / %p / %p\n", control, button_grab, gApplication::_control_grab);*/
			/*else if (event->type == GDK_BUTTON_RELEASE)
				fprintf(stderr, "GDK_BUTTON_RELEASE: %p / %p\n", control, button_grab);*/

			switch ((int)event->type)
			{
				case GDK_BUTTON_PRESS: type = gEvent_MousePress; break;
				case GDK_2BUTTON_PRESS: type = gEvent_MouseDblClick; break;
				default: type = gEvent_MouseRelease; break;
			}

			save_control = find_child(control, (int)event->button.x_root, (int)event->button.y_root, button_grab, true);
			
			/*if (type == gEvent_MousePress)
				fprintf(stderr, "save_control = %p %s\n", save_control, save_control ? save_control->name() : "");*/
			
			if (save_control)
				save_control = save_control->ignoreDesign();
			
			if (!save_control)
			{
				if (type == gEvent_MousePress && control->isTopLevel())
				{
					gMainWindow *win = ((gMainWindow *)control);
					if (win->isPopup())
						win->close();
				}
				
				//fprintf(stderr, "handle event %s\n", type == gEvent_MousePress ? "press" : type == gEvent_MouseRelease ? "release" : "other");
			
				goto __HANDLE_EVENT;
			}

			control = save_control;

#if GTK_CHECK_VERSION(3, 4, 0)
			bool menu = gdk_event_triggers_context_menu(event);
#else
			bool menu = (event->button.button == 3) && (event->type == GDK_BUTTON_PRESS);
#endif

			if (event->type != GDK_BUTTON_RELEASE)
			{
				#if DEBUG_FOCUS
				fprintf(stderr, "GDK_BUTTON_PRESS: %s canFocus = %d design = %d\n", control->name(), control->canFocus(), control->isDesign());
				#endif
				if (control->canFocusOnClick())
					control->setFocus();
				if (!control->_no_auto_grab)
					gApplication::setButtonGrab(control);
			}

			if (event->type == GDK_BUTTON_PRESS)
				gMouse::handleClickCount(event);

		__BUTTON_TRY_PROXY:
		
			cancel = false;

			if (control->isDesign() || control->isEnabled())
			{
				if (event->type == GDK_BUTTON_PRESS || CB_control_can_raise(control, type))
				{
					control->getScreenPos(&xc, &yc);
					xs = (int)event->button.x_root;
					ys = (int)event->button.y_root;
					x = xs - xc;
					y = ys - yc;

					gMouse::validate();
					gMouse::setEvent(event);
					//gMouse::setValid(1,(int)event->x,(int)event->y,event->button,event->state,data->screenX(),data->screenY());
					gMouse::setMouse(x, y, xs, ys, event->button.button, event->button.state);
					switch ((int)event->type)
					{
						case GDK_BUTTON_PRESS:
							gMouse::setControl(control);
							gMouse::setStart(x, y);
							cancel = CB_control_mouse(control, gEvent_MousePress);
							break;

						case GDK_2BUTTON_PRESS:
							cancel = CB_control_mouse(control, gEvent_MouseDblClick);
							break;

						case GDK_BUTTON_RELEASE:
							gMouse::setControl(NULL);
							cancel = CB_control_mouse(control, gEvent_MouseRelease);
							break;
					}

					gMouse::invalidate();
				}
			}

			if (type == gEvent_MouseRelease && control->_grab)
			{
				gApplication::exitLoop(control);
			}

			if (!cancel)
			{
				if (control->_proxy_for)
				{
					control = control->_proxy_for;
					//fprintf(stderr, "PRESS: try %s\n", control->name());
					goto __BUTTON_TRY_PROXY;
				}
			}

			if (menu)
			{
				control = save_control;
				while (control)
				{
					//fprintf(stderr, "menu %s D = %d DI = %d\n", control->name(), control->isDesign(), control->isDesignIgnore());
					if (CB_control_mouse(control, gEvent_MouseMenu))
					{
						cancel = true;
						break;
					}
					if (control->hasNativePopup())
						goto __HANDLE_EVENT;
					
					control = control->_proxy_for;
				}
			}

			if (cancel)
			{
				gMouse::resetTranslate();
				goto __RETURN;
			}

			if (widget != save_control->border && widget != save_control->widget)
			{
				//fprintf(stderr, "widget = %p, control = %p %p %s\n", widget, save_control->border, save_control->widget, save_control->name());
				gMouse::resetTranslate();
				goto __RETURN;
			}

			break;
		}

		case GDK_MOTION_NOTIFY:

			gdk_event_request_motions(&event->motion);

			save_control = control = find_child(control, (int)event->motion.x_root, (int)event->motion.y_root, button_grab);
			if (!control)
				goto __HANDLE_EVENT;

			control = control->ignoreDesign();
			/*while (control->isDesignIgnore())
				control = control->parent();*/
			//fprintf(stderr, "GDK_MOTION_NOTIFY: (%p %s) grab = %p state = %d tracking = %d\n", control, control->name(), button_grab, event->motion.state, control->isTracking());

			gApplication::checkHoveredControl(control);

		__MOTION_TRY_PROXY:

			//fprintf(stderr, "--> try (%p %s) / %s\n", control, control->name(), control->_proxy_for ? control->_proxy_for->name() : "-");

			if (!control->isDesign() && !control->isEnabled())
				goto __HANDLE_EVENT;

			if ((CB_control_can_raise(control, gEvent_MouseMove) || CB_control_can_raise(control, gEvent_MouseDrag))
					&& (control->isTracking() || (event->motion.state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK))))
			{
				control->getScreenPos(&xc, &yc);
				xs = (int)event->motion.x_root;
				ys = (int)event->motion.y_root;
				x = xs - xc;
				y = ys - yc;

				gMouse::validate();
				gMouse::setEvent(event);
				gMouse::setMouse(x, y, xs, ys, 0, event->motion.state);

				//fprintf(stderr, "pressure = %g\n", gMouse::getAxis(GDK_AXIS_PRESSURE));

				cancel = CB_control_mouse(control, gEvent_MouseMove);

				//if (data->acceptDrops() && gDrag::checkThreshold(data, gMouse::x(), gMouse::y(), gMouse::startX(), gMouse::startY()))
				if (!cancel && (event->motion.state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK))
						//&& (abs(gMouse::x() - gMouse::y()) + abs(gMouse::startX() - gMouse::startY())) > 8)
						&& gDrag::checkThreshold(control, gMouse::x(), gMouse::y(), gMouse::startX(), gMouse::startY()))
				{
					//fprintf(stderr, "gEvent_MouseDrag: event = %p\n", gApplication::lastEvent());
					cancel = CB_control_mouse(control, gEvent_MouseDrag);
				}
				gMouse::invalidate();

				if (cancel)
					goto __RETURN;
			}

			if (control->_proxy_for)
			{
				control = control->_proxy_for;
				//fprintf(stderr, "MOVE: try %s\n", control->name());
				goto __MOTION_TRY_PROXY;
			}

			gMouse::resetTranslate();
			//if (widget != save_control->border && widget != save_control->widget)
			//	goto __RETURN;

			break;

		case GDK_SCROLL:

			save_control = control = find_child(control, (int)event->scroll.x_root, (int)event->scroll.y_root, NULL, true);
			if (!control)
				goto __HANDLE_EVENT;

			control = control->ignoreDesign();
			/*while (control->isDesignIgnore())
				control = control->parent();*/
			
		__SCROLL_TRY_PROXY:

			if (!control->isDesign() && !control->isEnabled())
				goto __HANDLE_EVENT;

			if (CB_control_can_raise(control, gEvent_MouseWheel))
			{
				int dir, dt, ort;
				
				control->setFocus();

				control->getScreenPos(&xc, &yc);
				xs = (int)event->scroll.x_root;
				ys = (int)event->scroll.y_root;
				x = xs - xc;
				y = ys - yc;

				dir = event->scroll.direction;

#ifdef GTK3
				if (dir == GDK_SCROLL_SMOOTH)
				{
					/*gdouble dx = 0, dy = 0;
					gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy);
					if (fabs(dy) > fabs(dx))
						dir = (dy < 0) ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
					else
						dir = (dx < 0) ? GDK_SCROLL_LEFT : GDK_SCROLL_RIGHT;*/
					goto __HANDLE_EVENT;
				}
#endif

				switch (dir)
				{
					case GDK_SCROLL_DOWN: dt = -1; ort = 1; break;
					case GDK_SCROLL_LEFT: dt = -1; ort = 0; break;
					case GDK_SCROLL_RIGHT:  dt = 1; ort = 0; break;
					case GDK_SCROLL_UP: default: dt = 1; ort = 1; break;
				}

				gMouse::validate();
				gMouse::setEvent(event);
				gMouse::setMouse(x, y, xs, ys, 0, event->scroll.state);
				gMouse::setWheel(dt, ort);
				cancel = CB_control_mouse(control, gEvent_MouseWheel);
				gMouse::invalidate();
			}

			if (cancel)
			{
				gMouse::resetTranslate();
				goto __RETURN;
			}

			if (control->_proxy_for)
			{
				control = control->_proxy_for;
				goto __SCROLL_TRY_PROXY;
			}

			if (!control->_use_wheel)
			{
				control = control->parent();
				if (control)
					goto __SCROLL_TRY_PROXY;
			}

			if (widget != save_control->border && widget != save_control->widget)
			{
				gMouse::resetTranslate();
				goto __RETURN;
			}

			break;

		case GDK_KEY_PRESS:

			if (event->key.keyval)
				gKey::_last_key_press = event->key.keyval;
			send_to_window = control->isTopLevel();
			goto __HANDLE_EVENT;

		case GDK_KEY_RELEASE:

			if (event->key.keyval)
				gKey::_last_key_release = event->key.keyval;
			send_to_window = control->isTopLevel();
			goto __HANDLE_EVENT;
			
		default:
			
			handle_event = true;
			goto __RETURN;
	}

__HANDLE_EVENT:

	handle_event = !control || !control->isDesign();

__RETURN:

	if (event->type == GDK_BUTTON_RELEASE && gApplication::_button_grab)
	{
		if (gApplication::_enter_after_button_grab)
		{
			gApplication::checkHoveredControl(gApplication::_enter_after_button_grab);
			gApplication::_enter_after_button_grab = NULL;
		}
		gApplication::setButtonGrab(NULL);
	}

	if (handle_event)
		gtk_main_do_event(event);

	if (send_to_window)
		gcb_key_event(widget, event, control);

	if (!gdk_events_pending()) // && event->type != GDK_ENTER_NOTIFY && event->type != GDK_LEAVE_NOTIFY)
	{
		if (gApplication::_leave)
		{
			//if () // || (gApplication::_leave != control && check_crossing_event(event)))
			#if DEBUG_ENTER_LEAVE
			fprintf(stderr, "post leave: %s\n", gApplication::_leave->name());
			#endif

			if (gApplication::_enter == gApplication::_leave)
				gApplication::_enter = NULL;

			gApplication::_leave->emitLeaveEvent();

			gApplication::_leave = NULL;
		}
	}

	gApplication::_event = NULL;
}



/**************************************************************************

gApplication

**************************************************************************/

bool gApplication::_init = false;
bool gApplication::_busy = false;
char *gApplication::_title = NULL;
char *gApplication::_theme = NULL;
bool gApplication::_dark_theme = false;
bool gApplication::_dark_theme_set = false;
int gApplication::_in_popup = 0;
GtkWidget *gApplication::_popup_grab = NULL;
int gApplication::_loopLevel = 0;
void *gApplication::_loop_owner = 0;
GtkWindowGroup *gApplication::_group = NULL;
gControl *gApplication::_enter = NULL;
gControl *gApplication::_leave = NULL;
gControl *gApplication::_ignore_until_next_enter = NULL;
gControl *gApplication::_button_grab = NULL;
gControl *gApplication::_enter_after_button_grab = NULL;
gControl *gApplication::_control_grab = NULL;
gControl *gApplication::_active_control = NULL;
gControl *gApplication::_previous_control = NULL;
gControl *gApplication::_old_active_control = NULL;
bool (*gApplication::onKeyEvent)(int) = NULL;
guint32 gApplication::_event_time = 0;
bool gApplication::_close_next_window = false;
bool gApplication::_fix_printer_dialog = false;
gMainWindow *gApplication::_main_window = NULL;
void (*gApplication::onEnterEventLoop)();
void (*gApplication::onLeaveEventLoop)();
bool gApplication::_must_quit = false;
GdkEvent *gApplication::_event = NULL;
bool gApplication::_keep_focus = false;
bool gApplication::_disable_mapping_events = false;

bool gApplication::_disable_input_events = false;
//GQueue *gApplication::_input_events = NULL;

bool gApplication::_fix_breeze = false;
bool gApplication::_fix_oxygen = false;
int gApplication::_scrollbar_size = 0;
int gApplication::_scrollbar_big_size = 0;

void gApplication::grabPopup()
{
	//fprintf(stderr, "grabPopup: %p\n", _popup_grab);

	if (!_popup_grab)
		return;

	gt_grab(_popup_grab, TRUE, _event_time); //GDK_CURRENT_TIME);
}

void gApplication::ungrabPopup()
{
	//fprintf(stderr, "ungrabPopup: %p\n", _popup_grab);
	//gtk_grab_remove(_popup_grab);

	if (_popup_grab)
	{
		_popup_grab = NULL;
		gt_ungrab();
	}
}

#ifdef GTK3

bool gApplication::areTooltipsEnabled()
{
	return _tooltip_css == NULL;
}

void gApplication::enableTooltips(bool vl)
{
	if (vl == areTooltipsEnabled())
		return;

	gt_define_style_sheet(&_tooltip_css, NULL);

	if (!vl)
		gt_define_style_sheet(&_tooltip_css, g_string_new("tooltip { opacity: 0; }"));
}

#else

bool gApplication::areTooltipsEnabled()
{
  gboolean enabled;
  GtkSettings *settings;

  settings = gtk_settings_get_default();

  g_object_get (settings, "gtk-enable-tooltips", &enabled, (char *)NULL);

  return enabled;
}

void gApplication::enableTooltips(bool vl)
{
  GtkSettings *settings;
	gboolean enabled = vl;
  settings = gtk_settings_get_default();
  g_object_set (settings, "gtk-enable-tooltips", enabled, (char *)NULL);
}

#endif

static void do_nothing()
{
}

#ifndef GTK3
static gboolean master_client_save_yourself(GnomeClient *client, gint phase, GnomeSaveStyle save_style, gboolean is_shutting_down, GnomeInteractStyle interact_style, gboolean fast, gpointer user_data)
{
	if (gApplication::mainWindow())
	{
		//fprintf(stderr, "master_client_save_yourself: %d\n", X11_window_get_desktop((Window)(gApplication::mainWindow()->handle())));
		session_manager_set_desktop(X11_window_get_desktop((Window)(gApplication::mainWindow()->handle())));
	}
	return true;
}

static void master_client_die(GnomeClient *client, gpointer user_data)
{
	if (gApplication::mainWindow())
		gApplication::mainWindow()->close();
	else
		gMainWindow::closeAll();

	gApplication::quit();
	MAIN_check_quit();
}
#endif

static void cb_theme_changed(GtkSettings *settings, GParamSpec *param, gpointer data)
{
	gApplication::onThemeChange();
}

void gApplication::init(int *argc, char ***argv)
{
	GtkSettings *settings;
	//GSettings *settings;
	
	#ifdef GTK3

	char *appid;
	GB_FUNCTION func;

	GB.GetFunction(&func, (void *)GB.FindClass("_Gui"), "_InitApp", NULL, "s");
	appid = GB.ToZeroString((GB_STRING *)GB.Call(&func, 0, FALSE));

	//fprintf(stderr, "appid = %s\n", appid);
	#if GLIB_CHECK_VERSION(2, 74, 0)
	_app = gtk_application_new(NULL,  G_APPLICATION_DEFAULT_FLAGS);
	#else
	_app = gtk_application_new(NULL,  G_APPLICATION_FLAGS_NONE);
	#endif
	g_set_prgname(appid); // wayland window uses that as window class instead of the appid defined by the application!
	g_application_register(G_APPLICATION(_app), NULL, NULL);
	g_object_set(G_OBJECT(_app), "register-session", TRUE, NULL);

	#else

	session_manager_init(argc, argv);
	g_signal_connect(gnome_master_client(), "save-yourself", G_CALLBACK(master_client_save_yourself), NULL);
	g_signal_connect(gnome_master_client(), "die", G_CALLBACK(master_client_die), NULL);

	#endif

	getStyleName();

	settings = gtk_settings_get_default();
	g_signal_connect(G_OBJECT(settings), "notify::gtk-theme-name", G_CALLBACK(cb_theme_changed), 0);
	//g_signal_connect(G_OBJECT(settings), "notify::gtk-application-prefer-dark-theme", G_CALLBACK(cb_theme_changed), 0);*/

	//settings = g_settings_new("org.gnome.desktop.interface");
	//g_signal_connect(G_OBJECT(settings), "changed::color-scheme", G_CALLBACK(cb_theme_changed), 0);
	//g_signal_connect(G_OBJECT(settings), "changed::gtk-theme", G_CALLBACK(cb_theme_changed), 0);

	gdk_event_handler_set((GdkEventFunc)gambas_handle_event, NULL, NULL);

	gKey::init();
	gDesktop::init();

	onEnterEventLoop = do_nothing;
	onLeaveEventLoop = do_nothing;

	_group = gtk_window_group_new();

	_loop_owner = 0;

	char *env = getenv("GB_GTK_DEBUG_KEYPRESS");
	if (env && strcmp(env, "0"))
		_debug_keypress = true;

#ifdef GTK3
	// Override theme
	GtkCssProvider *css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css, "button { min-width:0;min-height:0; } button.combo { padding-top:0;padding-bottom:0; }", -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif
	
	gApplication::_init = true;
}

void gApplication::exit()
{
	#ifdef GTK3
	g_object_unref(_app);
	#else
	session_manager_exit();
	#endif

	if (_title)
		g_free(_title);
	if (_theme)
		g_free(_theme);

	gKey::exit();
	gTrayIcon::exit();
  gDialog::exit();
  gFont::exit();
  gt_exit();
}

gControl* gApplication::controlItem(GtkWidget *wid)
{
	gControl *control;

	while (wid)
	{
		control = gt_get_control(wid);
		if (control)
			return control;
		wid = gtk_widget_get_parent(wid);
	}

	return NULL;
}

void gApplication::setBusy(bool b)
{
	if (b == _busy)
		return;

	_busy = b;

#ifdef GTK3
	if (_busy)
		g_application_mark_busy(G_APPLICATION(_app));
	else
		g_application_unmark_busy(G_APPLICATION(_app));
#endif

	if (gApplication::_enter)
		gApplication::_enter->updateCurrentCursor();

	gdk_display_flush(gdk_display_get_default());
}

#if 0
static bool _dirty = false;

static gboolean update_geometry(void *data)
{
	GList *iter;
	gControl *control;

	if (gContainer::_arrangement_level)
		return true;

	_dirty = false;
	//g_debug(">>>> update_geometry");
	iter = g_list_first(gControl::controlList());
	while (iter)
	{
		control = (gControl *)iter->data;
		control->updateGeometry();
		iter = iter->next;
	}
	//g_debug("<<<<");

	return false;
}

void gApplication::setDirty()
{
	if (_dirty)
		return;

	_dirty = true;
	g_timeout_add(0, (GSourceFunc)update_geometry, NULL);
}
#endif

void gApplication::setDefaultTitle(const char *title)
{
	if (_title)
		g_free(_title);
	_title = g_strdup(title);
}

GtkWindowGroup *gApplication::enterGroup()
{
	gControl *control = _enter;
	GtkWindowGroup *oldGroup = _group;
	_group = gtk_window_group_new();

	_enter = _leave = NULL;

	while (control)
	{
		CB_control_enter_leave(control, gEvent_Leave);
		control = control->parent();
	}

	return oldGroup;
}

void gApplication::exitGroup(GtkWindowGroup *oldGroup)
{
	g_object_unref(_group);
	_group = oldGroup;
}

void gApplication::enterLoop(void *owner, bool showIt, GtkWindow *modal)
{
	void *old_owner = _loop_owner;
	int l = _loopLevel;
	bool d; 

	if (showIt) ((gControl *)owner)->show();

	_loopLevel++;
	_loop_owner = owner;
	setButtonGrab(NULL);

	d = gApplication::disableInputEvents(false);
	(*onEnterEventLoop)();
	do
	{
		MAIN_do_iteration(false);
	}
	while (_loopLevel > l);
	(*onLeaveEventLoop)();
	gApplication::disableInputEvents(d);

	_loop_owner = old_owner;
}

void gApplication::enterPopup(gMainWindow *owner)
{
	void *old_owner;
	int l;
	bool d;
	GtkWindow *window = GTK_WINDOW(owner->border);
	GtkWidget *old_popup_grab;

	_in_popup++;

	// Remove possible current button grab
	gApplication::setButtonGrab(NULL);
//
	gtk_window_set_modal(window, true);
	owner->show();
	gdk_window_set_override_redirect(gtk_widget_get_window(owner->border), true);
	
	if (!owner->isDestroyed())
	{
		old_popup_grab = _popup_grab;
		_popup_grab = owner->border;

		if (_in_popup == 1)
			owner->_grab_on_show = TRUE;

		l = _loopLevel;
		old_owner = _loop_owner;

		_loopLevel++;
		_loop_owner = owner;

		d = gApplication::disableInputEvents(false);
		(*onEnterEventLoop)();
		//fprintf(stderr, "event loop <----\n");
		do
		{
			MAIN_do_iteration(false);
		}
		while (_loopLevel > l);
		//fprintf(stderr, "event loop ---->\n");
		(*onLeaveEventLoop)();
		gApplication::disableInputEvents(d);

		if (_in_popup == 1)
		{
			if (owner->_grab_on_show)
				owner->_grab_on_show = FALSE;
			else
				gApplication::ungrabPopup();
		}
		
		_popup_grab = old_popup_grab;

		_loop_owner = old_owner;

		if (owner->border)
		{
			gdk_window_set_override_redirect(gtk_widget_get_window(owner->border), false);
			gtk_window_set_modal(window, false);
		}
	}

	_in_popup--;
}

void gApplication::exitLoop(void *owner)
{
	if (!hasLoop(owner))
		return;

	if (_loopLevel > 0)
		_loopLevel--;
}

GtkWindowGroup *gApplication::currentGroup()
{
	if (_group)
		return _group;
	else
		return gtk_window_get_group(NULL);
}

void gApplication::updateLastEvent(GdkEvent *e)
{
	_event = e;
	_event_time = gdk_event_get_time(e);
}

void gApplication::updateLastEventTime()
{
	_event_time = gtk_get_current_event_time();
}

static void post_focus_change(void *)
{
	gControl *current, *control, *next;

	#if DEBUG_FOCUS
	fprintf(stderr, "post_focus_change: %d %d\n", !_focus_change, _doing_focus_change);
	#endif
	

	if (!_focus_change || _doing_focus_change)
		return;

	#if DEBUG_FOCUS
	fprintf(stderr, "post_focus_change: %s -> %s\n", gApplication::_old_active_control ? gApplication::_old_active_control->name() : "nil", gApplication::_active_control ? gApplication::_active_control->name() : "nil");
	#endif
	
	_doing_focus_change = true;

	for(;;)
	{
		current = gApplication::activeControl();
		if (current == gApplication::_old_active_control)
			break;

		control = gApplication::_old_active_control;
		//if (control) fprintf(stderr, "check focus out %s\n", control->name());
		while (control)
		{
			next = control->_proxy_for;
			#if DEBUG_FOCUS
			fprintf(stderr, "focus out %s\n", control->name());
			#endif
			CB_control_focus(control, gEvent_FocusOut);
			control = next;
		}
		
		gApplication::_old_active_control = current;
		gMainWindow::setActiveWindow(current);

		control = current; //gApplication::activeControl();
		//if (control) fprintf(stderr, "check focus in %s\n", control->name());
		while (control)
		{
			next = control->_proxy_for;
			#if DEBUG_FOCUS
			fprintf(stderr, "focus in %s\n", control->name());
			#endif
			CB_control_focus(control, gEvent_FocusIn);
			control = next;
		}
	}

	_focus_change = false;
	_doing_focus_change = false;
	
	#if DEBUG_FOCUS
	fprintf(stderr, "post_focus_change: END\n");
	#endif
}

void gApplication::finishFocus()
{
	post_focus_change(NULL);
}

static void handle_focus_change()
{
	if (_focus_change)
		return;

	_focus_change = true;
	GB.Post((void (*)())post_focus_change, (intptr_t)NULL);
}

void gApplication::setActiveControl(gControl *control, bool on)
{
	if (control->isWindow() && on)
	{
		gControl *focus = ((gMainWindow *)control)->getInitialFocus();
		if (focus != control)
		{
			focus->setFocus();
			control = focus;
		}
	}
	
	while (!control->canFocus())
	{
		control = control->parent();
		if (!control)
			return;
	}
	
	if (on == (_active_control == control))
		return;

	#if DEBUG_FOCUS
	fprintf(stderr, "setActiveControl: %s %s %d / %d\n", GB.GetClassName(control->hFree), control->name(), on, _focus_change);
	#endif
	
	/*if (!::strcmp(GB.GetClassName(control->hFree), "FEditor") && !on)
		BREAKPOINT();*/
	
	if (_active_control && !_focus_change)
		_previous_control = _active_control;

	_active_control = on ? control : NULL;
	gKey::setActiveControl(_active_control);
	handle_focus_change();
}

int gApplication::getScrollbarSize()
{
	if (g_type_from_name("OsBar"))
	{
		char *env = getenv("LIBOVERLAY_SCROLLBAR");
		if (!env || *env != '0')
			return 1;
	}

#ifdef GTK3

	if (_scrollbar_size == 0)
	{
		GtkWidget *widget = 
		#ifdef GTK3
			gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
		#else
			gtk_hscrollbar_new(NULL);
		#endif
		gtk_widget_show(widget);
		gtk_widget_get_preferred_width(widget, NULL, &_scrollbar_size); //, &minimum_size, &natural_size);
		gtk_widget_get_preferred_height(widget, NULL, &_scrollbar_big_size); //, &minimum_size, &natural_size);
		gtk_widget_destroy(widget);
		
		if (_fix_breeze)
			_scrollbar_size += 3;
		//fprintf(stderr, "getScrollbarSize = %d\n", size);
	}
	
	return _scrollbar_size;
	
#else
	
	gint trough_border;
	gint slider_width;

	gt_get_style_property(GTK_TYPE_SCROLLBAR, "slider-width", &slider_width);
	gt_get_style_property(GTK_TYPE_SCROLLBAR, "trough-border", &trough_border);

	return (trough_border) * 2 + slider_width;
	
#endif
}

int gApplication::getScrollbarBigSize()
{
#ifdef GTK3
	getScrollbarSize();
	return _scrollbar_big_size;
#else
	return getScrollbarSize();
#endif
}

int gApplication::getScrollbarSpacing()
{
	gint v;

	gt_get_style_property(GTK_TYPE_SCROLLED_WINDOW, "scrollbar-spacing", &v);

	return v;
}

int gApplication::getInnerWidth()
{
	if (_fix_oxygen)
		return 1;
	else
		return 0;
}

int gApplication::getFrameWidth()
{
	int w;
#ifdef GTK3
	int h;

	getBoxFrame(&w, &h);
	w = h;

#else
	GtkStyle *style;
	gint focus_width;
	gboolean interior_focus;
	//int inner;

	style = gt_get_style(GTK_TYPE_ENTRY);

	gt_get_style_property(GTK_TYPE_ENTRY, "focus-line-width", &focus_width);
	gt_get_style_property(GTK_TYPE_ENTRY, "interior-focus", &interior_focus);

	w = MIN(style->xthickness, style->ythickness);

	if (!interior_focus)
		w += focus_width;

	w += getInnerWidth();
#endif

	return w;
}

void gApplication::getBoxFrame(int *pw, int *ph)
{
	int w, h;

#ifdef GTK3

	GtkStyleContext *context = gt_get_style(GTK_TYPE_ENTRY);
  GtkBorder border;
	GtkBorder padding;
	int radius;

	gtk_style_context_get_padding(context, STATE_FOCUSED, &padding);
	//fprintf(stderr, "padding: %d %d %d %d\n", padding.top, padding.right, padding.bottom, padding.left);
	gtk_style_context_get_border(context, STATE_FOCUSED, &border);
	//fprintf(stderr, "border: %d %d %d %d\n", border.top, border.right, border.bottom, border.left);

  gtk_style_context_get(context, STATE_FOCUSED, GTK_STYLE_PROPERTY_BORDER_RADIUS, &radius, NULL);
	//fprintf(stderr, "border-radius: %d\n", radius);
	radius /= 2;

	w = MAX(border.left + padding.left, border.right + padding.right);
	w = MAX(w, radius);

	h = MAX(border.top + padding.top, border.bottom + padding.bottom);//, MAX(padding.top, padding.bottom));
	h = MAX(h, radius);

	w = MAX(2, w);
	h = MAX(2, h);

#else

	GtkStyle *style;
	gint focus_width;
	gboolean interior_focus;
	int inner;

	style = gt_get_style(GTK_TYPE_ENTRY);

	gt_get_style_property(GTK_TYPE_ENTRY, "focus-line-width", &focus_width);
	gt_get_style_property(GTK_TYPE_ENTRY, "interior-focus", &interior_focus);

	w = style->xthickness;
	h = style->ythickness;

	if (!interior_focus)
	{
		w += focus_width;
		h += focus_width;
	}

	inner = getInnerWidth();
	w += inner;
	h += inner;

#endif

	*pw = w;
	*ph = h;
}

char *gApplication::getStyleName()
{
	if (!_theme)
	{
		char *p;
		GtkSettings *settings = gtk_settings_get_default();
		g_object_get(settings, "gtk-theme-name", &_theme, (char *)NULL);

		p = _theme = g_strdup(_theme);
		while (*p)
		{
			*p = tolower(*p);
			p++;
		}
		
		_fix_breeze = false;
		_fix_oxygen = false;
		if (strcasecmp(_theme, "breeze") == 0 || strcasecmp(_theme, "breeze dark") == 0)
			_fix_breeze = true;
		else if (strcasecmp(_theme, "oxygen-gtk") == 0)
			_fix_oxygen = true;
	}

	return _theme;
}

#ifndef GTK3
static GdkFilterReturn x11_event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	((X11_EVENT_FILTER)data)((XEvent *)xevent);
	return GDK_FILTER_CONTINUE;
}

void gApplication::setEventFilter(X11_EVENT_FILTER filter)
{
	static X11_EVENT_FILTER save_filter = NULL;
	static GdkEventMask save_mask = (GdkEventMask)0;

	if (save_filter)
	{
		gdk_window_remove_filter(NULL, (GdkFilterFunc)x11_event_filter, (gpointer)save_filter);
		gdk_window_set_events(gdk_get_default_root_window(), save_mask);
	}

	if (filter)
	{
		save_mask = gdk_window_get_events(gdk_get_default_root_window());
		gdk_window_set_events(gdk_get_default_root_window(), (GdkEventMask)(save_mask | GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK));
		gdk_window_add_filter(NULL, (GdkFilterFunc)x11_event_filter, (gpointer)filter);
	}

	save_filter = filter;
}
#endif

void gApplication::setMainWindow(gMainWindow *win)
{
	_main_window = win;
}

void gApplication::quit()
{
	_must_quit = true;
}

int gApplication::dblClickTime()
{
  gint value;
  g_object_get (gtk_settings_get_default(), "gtk-double-click-time", &value, (char *)NULL);
	return value;
}

void gApplication::onThemeChange()
{
	if (_theme)
	{
		g_free(_theme);
		_theme = NULL;
	}
	
	getStyleName();
	_scrollbar_size = 0;
}

static void for_each_filter(gContainer *cont, GPtrArray *list, bool (*filter)(gControl *))
{
	int i;
	gControl *control;
	
	if ((*filter)(cont))
		g_ptr_array_add(list, cont);
	
	for (i = 0; i < cont->childCount(); i++)
	{
		control = cont->child(i);
		if (control->isContainer())
			for_each_filter((gContainer *)control, list, filter);
		else
		{
			if ((*filter)(control))
				g_ptr_array_add(list, control);
		}
	}
}

static void for_each_control(gContainer *cont, void (*cb)(gControl *))
{
	//GPtrArray *children;
	int i;
	gControl *control;
	
	(*cb)(cont);
	
	//children = cont->childrenCopy();
	//for (i = 0; i < children->len; i++)
	//{
	//	control = (gControl *)g_ptr_array_index(children, i);
	for (i = 0; i < cont->childCount(); i++)
	{
		control = cont->child(i);
		
		if (control->isContainer())
			for_each_control((gContainer *)control, cb);
		else
			(*cb)(control);
	}
	//g_ptr_array_unref(children);
}

// If the callback may destroy controls, the filter must be specified!

void gApplication::forEachControl(void (*cb)(gControl *), bool (*filter)(gControl *))
{
	GList *iter_win;
	gMainWindow *win;
	
	iter_win = g_list_first(gMainWindow::windows);
	while (iter_win)
	{
		win = (gMainWindow *)iter_win->data;
		iter_win = g_list_next(iter_win);
		
		if (filter)
		{
			uint i;
			gControl *control;
			GPtrArray *list = g_ptr_array_new();
			
			for_each_filter(win, list, filter);
			
			for (i = 0; i < list->len; i++)
			{
				control = (gControl *)g_ptr_array_index(list, i);
				if (control->isDestroyed())
					continue;
				//fprintf(stderr, "[%d] %s\n", i, control->name());
				(*cb)(control);
			}
			
			g_ptr_array_unref(list);
		}
		else
		{
			for_each_control(win, cb);
		}
	}
}

#if 0
void gApplication::pushInputEvent(GdkEvent *event)
{
	if (!_input_events)
		_input_events = g_queue_new();
	
	g_queue_push_tail(_input_events, gdk_event_copy(event));
}

bool gApplication::processInputEvent()
{
	GdkEvent *event;
	
	if (!areInputEventsEnabled())
		return true;
	
	if (!_input_events || g_queue_is_empty(_input_events))
		return true;
	
	event = (GdkEvent *)g_queue_pop_head(_input_events);
	gambas_handle_event(event);
	gdk_event_free(event);
	return false;
}
#endif

bool gApplication::disableInputEvents(bool disable)
{
	bool d = _disable_input_events;
	_disable_input_events = disable;
	return d;
}

bool gApplication::eventsPending()
{
	/*if (areInputEventsEnabled() && _input_events && !g_queue_is_empty(_input_events))
		return true;*/
	
	return gtk_events_pending();
		
}

#ifdef GTK3
bool gApplication::hasMiddleClickPaste()
{
  gboolean enabled;
  GtkSettings *settings;

  settings = gtk_settings_get_default();

  g_object_get(settings, "gtk-enable-primary-paste", &enabled, (char *)NULL);

  return enabled;
}
#else
bool gApplication::hasMiddleClickPaste()
{
	return TRUE;
}
#endif

void gApplication::updateDarkTheme()
{
	uint bg;
	char *env;

	if (_dark_theme_set)
		return;
	
	env = getenv("GB_GUI_DARK_THEME");
	if (env && *env)
	{
		_dark_theme = atoi(env);
		return;
	}
	

	bg = gDesktop::getColor(COLOR_BACKGROUND);
	_dark_theme = IMAGE.GetLuminance(bg) < 128;
}

void gApplication::setDarkTheme(bool v)
{
	_dark_theme = v;
	_dark_theme_set = true;
}
