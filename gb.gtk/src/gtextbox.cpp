/***************************************************************************

	gtextbox.cpp

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
#include "gapplication.h"
#include "gkey.h"
#include "gdesktop.h"
#include "gtextbox.h"

#ifdef GTK3

#define MAX_ICONS 2

#if GTK_CHECK_VERSION(3,14,0)
	
struct _GtkEntryPrivate
{
  void         *icons[MAX_ICONS];

  GtkEntryBuffer        *buffer;
  GtkIMContext          *im_context;
  GtkWidget             *popup_menu;

  GdkWindow             *text_area;
};

#else

struct _GtkEntryPrivate
{
	void         *icons[MAX_ICONS];

	GtkEntryBuffer        *buffer;
	GtkIMContext          *im_context;
	GtkWidget             *popup_menu;

	GdkDevice             *device;

	GdkWindow             *text_area;
};

#endif

#define TEXT_AREA(_entry) (GTK_ENTRY(_entry)->priv->text_area)

#else

#define TEXT_AREA(_entry) (GTK_ENTRY(_entry)->text_area)

#endif


static gboolean raise_change(gTextBox *data)
{
	if (data->_changed)
	{
		CB_textbox_change(data);
		data->_changed = false;
	}
	return FALSE;
}

static void cb_before_insert(GtkEditable *editable, gchar *new_text, gint new_text_length, gint *position, gTextBox *data)
{
	if (gKey::gotCommit())
	{
		gcb_im_commit(NULL, new_text, data);
		if (gKey::canceled())
			g_signal_stop_emission_by_name (G_OBJECT(editable), "insert-text");
		*position = gtk_editable_get_position(editable);
	}
}

static void cb_change_insert(GtkEditable *editable, gchar *new_text, gint new_text_length, gint *position, gTextBox *data)
{
	data->_changed = false;
	gtk_editable_set_position(editable, *position);
	CB_textbox_change(data);
	*position = gtk_editable_get_position(editable);
}

static void cb_change_delete(GtkEditable *editable, gint start_pos, gint end_pos, gTextBox *data)
{
	if (!data->_changed)
	{
		data->_changed = true;
		g_timeout_add(0, (GSourceFunc)raise_change, data);
	}
}

static void cb_activate(GtkEntry *editable, gTextBox *data)
{
	CB_textbox_activate(data);
}

static void cb_cursor(GtkWidget *entry, GParamSpec *param, gTextBox *data)
{
	if (data->_last_position != data->position())
	{
		data->_last_position = data->position();
		CB_textbox_cursor(data);
	}
}

gTextBox::gTextBox(gContainer *parent) : gControl(parent)
{
#ifndef GTK3
	_placeholder = NULL;
#endif
	_changed = false;
	_has_border = true;
	_has_native_popup = true;
	_text_area_visible = true;
	_last_position = 0;

	_no_background = TRUE;
	
	entry = widget = gtk_entry_new();
	realize();
	setColorBase();
	initEntry();
}

gTextBox::~gTextBox()
{
#ifndef GTK3
	if (_placeholder) g_free(_placeholder);
#endif
}

/*static void cb_im_commit(GtkIMContext *context, const char *str, gpointer pointer)
{
	fprintf(stderr, "cb_im_commit: %s\n", str);
}*/

void gTextBox::initEntry()
{
	_has_input_method = entry != NULL;

	if (!entry)
		return;
	
	// This imitates the QT behaviour, where change signal is raised after the cursor has been moved.
	g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(cb_before_insert), (gpointer)this);
	g_signal_connect_after(G_OBJECT(entry), "insert-text", G_CALLBACK(cb_change_insert), (gpointer)this);
	g_signal_connect_after(G_OBJECT(entry), "delete-text", G_CALLBACK(cb_change_delete), (gpointer)this);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(cb_activate), (gpointer)this);
	g_signal_connect(G_OBJECT(entry), "notify::cursor-position", G_CALLBACK(cb_cursor), (gpointer)this);
	//g_signal_connect(getInputMethod(), "commit", G_CALLBACK(cb_im_commit), (gpointer)this);
}

char* gTextBox::text()
{
	return (char*)gtk_entry_get_text(GTK_ENTRY(entry));
}

void gTextBox::setText(const char *vl)
{
	int save;
	
	if (!vl) vl = "";
	
	if (!entry || !strcmp(vl, text()))
		return;
	
	save = _last_position;
	lock();
	gtk_entry_set_text(GTK_ENTRY(entry), vl);
	gtk_editable_set_position(GTK_EDITABLE(entry), -1);
	unlock();
	CB_textbox_change(this);
	_last_position = save;
	cb_cursor(entry, NULL, this);
}

#ifdef GTK3
char* gTextBox::placeholder() const
{
	return (char*)gtk_entry_get_placeholder_text(GTK_ENTRY(entry));
}

void gTextBox::setPlaceholder(const char *vl)
{
	if (!vl) vl = "";
	
	if (!entry)
		return;
		
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry), vl);
}
#else
char* gTextBox::placeholder() const
{
	return _placeholder;
}

void gTextBox::setPlaceholder(const char *vl)
{
	if (_placeholder) g_free(_placeholder);
	_placeholder = g_strdup(vl);
}

#endif

bool gTextBox::password() const
{
	if (entry)
		return !gtk_entry_get_visibility(GTK_ENTRY(entry));
	else
		return false;
}

void gTextBox::setPassword(bool vl)
{
	if (!entry)
		return;
		
	gtk_entry_set_visibility(GTK_ENTRY(entry),!vl);
	if (vl)
		gtk_entry_set_invisible_char(GTK_ENTRY(entry), (gunichar)0x25CF);
}

bool gTextBox::isReadOnly() const
{
	return !gtk_editable_get_editable(GTK_EDITABLE(entry));
}

void gTextBox::setReadOnly(bool vl)
{
	gtk_editable_set_editable(GTK_EDITABLE(entry),!vl);
}

int gTextBox::position() const
{
	if (entry)
		return gtk_editable_get_position(GTK_EDITABLE(entry));
	else
		return 0;
}

void gTextBox::setPosition(int pos)
{
	int len;
	
	if (!entry)
		return;
		
	len = length();
		
	if (pos < 0) 
		pos = 0;
	else if (pos > len)
		pos = -1;

	gtk_editable_set_position(GTK_EDITABLE(entry), pos);
}

void gTextBox::setBorder(bool vl)
{
	if (!entry)
		return;
	
	if (vl == _has_border)
		return;
	
	_has_border = vl;
	
	gtk_entry_set_has_frame(GTK_ENTRY(entry), vl);
#ifdef GTK3
	updateStyleSheet(true);
	setMinimumSize();
#endif
}	

void gTextBox::insert(char *txt, int len)
{
	if (!entry || !len || !txt) return;
	
	lock();
	gtk_editable_delete_selection(GTK_EDITABLE(entry));
	unlock();
	int pos = position();
	gtk_editable_insert_text(GTK_EDITABLE(entry), txt, len, &pos);
}

int gTextBox::length()
{
	const gchar *buf;
	
	if (!entry)
		return 0;
	
	buf = gtk_entry_get_text(GTK_ENTRY(entry));
	if (!buf) return 0;
	
	return g_utf8_strlen(buf, -1);
}

int gTextBox::maxLength() const
{
	if (entry)
		return gtk_entry_get_max_length(GTK_ENTRY(entry));
	else
		return 0;
}

void gTextBox::setMaxLength(int vl)
{
	if (!entry)
		return;
	if (vl<0) vl=0;
	if (vl>65536) vl=0;
	gtk_entry_set_max_length(GTK_ENTRY(entry), vl);
	
}

bool gTextBox::isSelected() const
{
	if (entry)
		return gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), NULL, NULL);
	else
		return false;
}

int gTextBox::selStart() const
{
	int start;
	
	if (!entry)
		return -1;
	
	gtk_editable_get_selection_bounds(GTK_EDITABLE(entry),&start,NULL);
	return start;
}

int gTextBox::selLength() const
{
	int start,end;

	if (!entry)
		return 0;

	gtk_editable_get_selection_bounds(GTK_EDITABLE(entry),&start,&end);
	return end - start;
}

char* gTextBox::selText() const
{
	int start,end;

	if (!entry)
		return NULL;
	gtk_editable_get_selection_bounds(GTK_EDITABLE(entry),&start,&end);
	return gtk_editable_get_chars(GTK_EDITABLE(entry),start,end);
	
}

void gTextBox::setSelText(char *txt,int len)
{
	int start,end;

	if (!entry)
		return;

	gtk_editable_get_selection_bounds(GTK_EDITABLE(entry),&start,&end);	
	gtk_editable_delete_text(GTK_EDITABLE(entry),start,end);
	gtk_editable_insert_text(GTK_EDITABLE(entry),txt,len,&start);
	
}

void gTextBox::selClear()
{
	int start;

	if (!entry)
		return;

	gtk_editable_get_selection_bounds(GTK_EDITABLE(entry),&start,NULL);	
	gtk_editable_select_region(GTK_EDITABLE(entry),start,start);
}

void gTextBox::selectAll()
{
	if (entry)
		gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
}

void gTextBox::select(int start,int len)
{
	if (!entry)
		return;
	if ( (len<=0) || (start<0) ) { selClear(); return; }
	gtk_editable_select_region(GTK_EDITABLE(entry),start,start+len);
}


int gTextBox::alignment() const
{
	if (entry)
		return gt_to_alignment(gtk_entry_get_alignment(GTK_ENTRY(entry)));
	else
		return ALIGN_NORMAL;
}

void gTextBox::setAlignment(int al)
{
	if (!entry)
		return;
	gtk_entry_set_alignment(GTK_ENTRY(entry), gt_from_alignment(al));
}

void gTextBox::updateCursor(GdkCursor *cursor)
{
	GdkWindow *win;
	
	gControl::updateCursor(cursor);
	if (!entry)
		return;

	win = TEXT_AREA(entry);
	if (!win)
		return;
		
	if (cursor)
		gdk_window_set_cursor(win, cursor);
	else
	{
		cursor = gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_XTERM);
		gdk_window_set_cursor(win, cursor);
#ifdef GTK3
		g_object_unref(cursor);
#else
		gdk_cursor_unref(cursor);
#endif
	}
}

void gTextBox::clear()
{
	setText("");
}


void gTextBox::setMinimumSize()
{
	int h = font()->height();
	_min_h = h + (hasBorder() ? 4 : 0);
	_min_w = h / 2 + (hasBorder() ? 4 : 0);
}


void gTextBox::setFont(gFont *ft)
{
	gControl::setFont(ft);
	setMinimumSize();
}

GtkIMContext *gTextBox::getInputMethod()
{
#ifdef GTK3
	return entry ? GTK_ENTRY(entry)->priv->im_context : NULL;
#else
	return entry ? GTK_ENTRY(entry)->im_context : NULL;
#endif
}


void gTextBox::getCursorPos(int *x, int *y, int pos)
{
	int px, py;
	PangoLayout *layout;
	PangoRectangle rect;
	
	layout = gtk_entry_get_layout(GTK_ENTRY(entry));
	pos = gtk_entry_text_index_to_layout_index(GTK_ENTRY(entry), pos < 0 ? position() : pos);
	pango_layout_get_cursor_pos(layout, pos, &rect, NULL);
	
	gtk_entry_get_layout_offsets(GTK_ENTRY(entry), &px, &py);
	
#ifdef GTK3
	GdkRectangle r;
	gtk_entry_get_text_area(GTK_ENTRY(entry), &r);
	py = r.y;
#endif
	
	*x = px + PANGO_PIXELS(rect.x);
	*y = py + PANGO_PIXELS(rect.y + rect.height);
}

void gTextBox::setFocus()
{
	bool r = isReadOnly();
	
	if (!r)
		setReadOnly(true);
	gControl::setFocus();
	if (!r)
		setReadOnly(false);
}

#ifdef GTK3
void gTextBox::customStyleSheet(GString *css)
{
	if (!_has_border)
	{
		setStyleSheetNode(css, "");
		g_string_append_printf(css, "border:none;box-shadow:none;padding-top:0;padding-bottom:0;\n");
		/*if (background() == COLOR_DEFAULT)
			g_string_append_printf(css, "background:none;");*/
	}
}
#endif

#ifdef GTK3
void gTextBox::onEnterEvent()
{
	if (!entry)
		return;
	
	if (_text_area_visible)
	{
		//fprintf(stderr, "gTextBox::onEnterEvent: show\n");
		gdk_window_show(TEXT_AREA(entry));
	}
}

void gTextBox::onLeaveEvent()
{
	if (!entry)
		return;
	
	_text_area_visible = !hasFocus() && gdk_window_is_visible(TEXT_AREA(entry));
	if (_text_area_visible)
	{
		//fprintf(stderr, "gTextBox::onEnterEvent: hide\n");
		gdk_window_hide(TEXT_AREA(entry));
	}
}
#endif

gColor gTextBox::defaultBackground() const
{
	return gDesktop::getColor(COLOR_TEXT_BACKGROUND, !isEnabled());
}
