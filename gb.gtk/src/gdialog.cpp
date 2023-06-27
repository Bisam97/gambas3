/***************************************************************************

  gdialog.cpp

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

#include "widgets.h"
#include "gdialog.h"
#include "gdesktop.h"
#include "gmainwindow.h"
#include "gapplication.h"

static gColor _color = 0;
static char *_path = NULL;
static char **_paths = NULL;
static char *_title = NULL;
static gFont *_font = NULL;
static bool _show_hidden = false;
static GPtrArray *_filter = NULL;
static int _filter_index = -1;
static bool _filter_index_set = false;

static int run_dialog(GtkDialog *window)
{
  gMainWindow *active;
	GtkWindowGroup *oldGroup;
  int ret;
	bool busy;
  
	active = gDesktop::activeWindow();
	if (active)
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(active->border));
	
	//gApplication::setActiveControl(gApplication::activeControl(), false);
	//GB.CheckPost();
	
	busy = gApplication::isBusy();
	gApplication::setBusy(false);
	
	gtk_window_present(GTK_WINDOW(window));
	oldGroup = gApplication::enterGroup();
	gApplication::_loopLevel++;
	(*gApplication::onEnterEventLoop)();
	ret = gtk_dialog_run(window);
	(*gApplication::onLeaveEventLoop)();
	gApplication::_loopLevel--;
	gApplication::exitGroup(oldGroup);
	
	gApplication::setBusy(busy);
	
	return ret;
}

//-------------------------------------------------------------------------


static void free_path(void)
{
	if (_path)
	{
		g_free(_path);
		_path = NULL; 
	}
	
	if (_paths)
	{
		int i = 0;
	
		while (_paths[i])
		{
			g_free(_paths[i]);
			i++;
		}
		g_free(_paths);
		_paths=NULL;
	}
}

static void set_filters(GtkFileChooser* ch)
{	
	char **filters;
	int nfilters;
	int i, p;
	GString *name;
	char *filter;
	char **patterns;
	GtkFileFilter *ft;
	GtkFileFilter *select = NULL;
	int index;
	
	filters = gDialog::filter(&nfilters);
	if (!nfilters)
    return;
    
	if (_filter_index_set)
	{
		_filter_index_set = false;
		index = _filter_index;
	}
	else
		index = -1;

	for (i = 0; i < nfilters / 2; i ++)
  {
		filter = filters[i * 2];
		if (filter && !strcmp(filter, "*"))
			continue;
		
		ft = gtk_file_filter_new();
		
		name = g_string_new(filters[i * 2 + 1]);
		g_string_append_printf(name, " (%s)", filter);
		gtk_file_filter_set_name(ft, name->str);
		g_string_free(name, true);
		
    patterns = g_strsplit(filter, ";", 0);
    for (p = 0; patterns[p]; p++)
      gtk_file_filter_add_pattern(ft, patterns[p]);
      
    g_strfreev(patterns);
	
		gtk_file_chooser_add_filter(ch, ft);
		
		if (i == index)
			select = ft;
	}
	
	ft = gtk_file_filter_new();
	
	name = g_string_new(GB.Translate("All files"));
	g_string_append(name, " (*)");
	gtk_file_filter_set_name(ft, name->str);
	g_string_free(name, true);
		
	gtk_file_filter_add_pattern(ft, "*");
	gtk_file_chooser_add_filter(ch, ft);
	
	if (!select)
		select = ft;
	
	gtk_file_chooser_set_filter(ch, select);
}

static void find_filter(GtkFileChooser*ch)
{
	GtkFileFilter *ft = gtk_file_chooser_get_filter(ch);
	GSList *lft;
	
	if (ft)
	{
		lft = gtk_file_chooser_list_filters(ch);
		if (lft)
		{
			_filter_index = g_slist_index(lft, (gconstpointer)ft);
			if (_filter_index >= 0 && _filter_index < ((int)g_slist_length(lft) - 1))
				return;
		}
	}
	
	_filter_index = -1;
}
	
static bool run_file_dialog(GtkFileChooserDialog *msg)
{
	GSList *names,*iter;
	char *buf;
	long b=0;
	
	set_filters((GtkFileChooser*)msg);
	
	if (run_dialog(GTK_DIALOG(msg)) != GTK_RESPONSE_OK)
 	{
 		gtk_widget_destroy(GTK_WIDGET(msg));
		gDialog::setTitle(NULL);
		return true;
 	}
	
	free_path();
	
	names=gtk_file_chooser_get_filenames((GtkFileChooser*)msg);
	
	if (names)
	{
		buf=(char*)names->data;
		if (buf) {
			_path=(char*)g_malloc( sizeof(char)*(strlen(buf)+1) );
			strcpy(_path,buf);
		}
		
		b=0;
		_paths=(char**)g_malloc(sizeof(char*)*(g_slist_length(names)+1) );
		_paths[g_slist_length(names)]=NULL;
		iter=names;
		while(iter)
		{
			buf=(char*)iter->data;
			_paths[b]=(char*)g_malloc( sizeof(char)*(strlen(buf)+1) );
			strcpy(_paths[b++],buf);
			iter=iter->next;
		}
		
		g_slist_free(names);
	}
	
	find_filter((GtkFileChooser *)msg);
	
	gtk_widget_destroy(GTK_WIDGET(msg));
	gDialog::setTitle(NULL);
	return false;
}

void gDialog::exit()
{
	free_path();
	
	gDialog::setFilter(NULL, 0);
	gFont::assign(&_font);
}

gFont* gDialog::font()
{
  return _font;
}

void gDialog::setFont(gFont *ft)
{
  gFont::set(&_font, ft->copy());
}

gColor gDialog::color()
{
	return _color;
}
	
void gDialog::setColor(gColor col)
{
	_color=col;
}

char* gDialog::title()
{
	return _title;
}

void gDialog::setTitle(char *vl)
{
	if (_title)
	{
		g_free(_title);
		_title=NULL;
	}
	
	if (vl && *vl)
		_title = g_strdup(vl);
}

char *gDialog::path()
{
	return _path;
}

char **gDialog::paths()
{
	return _paths;
}

void gDialog::setPath(char *vl)
{
	if (_path)
	{
		g_free(_path);
		_path=NULL;
	}
	
	if (!vl) return;
	
	_path=(char*)g_malloc( sizeof(char)*(strlen(vl)+1) );
	strcpy(_path,vl);
}

bool gDialog::showHidden()
{
	return _show_hidden;
}

void gDialog::setShowHidden(bool v)
{
	_show_hidden = v;
}

char** gDialog::filter(int *nfilter)
{
  if (!_filter)
  {
    *nfilter = 0;
    return NULL;
  }
  
  *nfilter = _filter->len;
  return (char **)(_filter->pdata);
}

void gDialog::setFilter(char** filter, int nfilter)
{
	int i;
	
	if (_filter)
	{
    for (i = 0; i < (int)_filter->len; i++)
      g_free(g_ptr_array_index(_filter, i));
      
    g_ptr_array_free(_filter, true);
    _filter = NULL;
  }
  	
  if (!filter)
    return;
	
	_filter = g_ptr_array_new();
  for (i = 0; i < nfilter; i++)
    g_ptr_array_add(_filter, (gpointer)g_strdup(filter[i]));
}

bool gDialog::openFile(bool multi)
{
	GtkFileChooserDialog *msg;

	msg = (GtkFileChooserDialog*)gtk_file_chooser_dialog_new(
		_title ? _title : GB.Translate("Open file"),
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
#ifdef GTK3
		GB.Translate("Cancel"), GTK_RESPONSE_CANCEL, GB.Translate("Open"), GTK_RESPONSE_OK,
#else
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK,
#endif
		(void *)NULL);

	gtk_file_chooser_set_local_only((GtkFileChooser*)msg, true);
	gtk_file_chooser_set_select_multiple((GtkFileChooser*)msg, multi);
	gtk_widget_show(GTK_WIDGET(msg));
	gtk_file_chooser_unselect_all((GtkFileChooser*)msg);
	
	if (_path)
	{
		if (g_file_test(_path, G_FILE_TEST_IS_DIR))
			gtk_file_chooser_set_current_folder((GtkFileChooser*)msg, _path);
		else
			gtk_file_chooser_select_filename((GtkFileChooser*)msg, _path);
	}
	
	gtk_file_chooser_set_show_hidden((GtkFileChooser*)msg, _show_hidden);
	
	return run_file_dialog(msg);
}

bool gDialog::saveFile()
{
	GtkFileChooserDialog *msg;

	msg = (GtkFileChooserDialog*)gtk_file_chooser_dialog_new(
		_title ? _title : GB.Translate("Save file"),
		NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE,
#ifdef GTK3
		GB.Translate("Cancel"), GTK_RESPONSE_CANCEL, GB.Translate("Save"), GTK_RESPONSE_OK,
#else
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK,
#endif
		(void *)NULL);
	
	gtk_file_chooser_set_do_overwrite_confirmation((GtkFileChooser*)msg, true);
	gtk_file_chooser_set_local_only((GtkFileChooser*)msg, true);
	gtk_file_chooser_set_select_multiple((GtkFileChooser*)msg, false);
	gtk_widget_show(GTK_WIDGET(msg));
	gtk_file_chooser_unselect_all((GtkFileChooser*)msg);
	
	if (_path)
	{
		if (*_path && _path[strlen(_path) - 1] == '/' && g_file_test(_path, G_FILE_TEST_IS_DIR))
			gtk_file_chooser_set_current_folder((GtkFileChooser*)msg, _path);
		else
			gtk_file_chooser_select_filename((GtkFileChooser*)msg, _path);
	}
		
	gtk_file_chooser_set_show_hidden((GtkFileChooser*)msg, _show_hidden);

	return run_file_dialog(msg);
}

bool gDialog::selectFolder()
{
	GtkFileChooserDialog *msg;

	msg = (GtkFileChooserDialog*)gtk_file_chooser_dialog_new(
		_title ? _title : GB.Translate("Select directory"),
		NULL,
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
#ifdef GTK3
		GB.Translate("Cancel"), GTK_RESPONSE_CANCEL, GB.Translate("Open"), GTK_RESPONSE_OK,
#else
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK,
#endif
		(void *)NULL);
	 
	gtk_file_chooser_set_local_only((GtkFileChooser*)msg, true);
	gtk_file_chooser_set_select_multiple((GtkFileChooser*)msg, false);
	gtk_widget_show(GTK_WIDGET(msg));
	gtk_file_chooser_unselect_all((GtkFileChooser*)msg);
	if (_path)
	{
		if (g_file_test(_path, G_FILE_TEST_IS_DIR))
			gtk_file_chooser_set_current_folder((GtkFileChooser*)msg, _path);
		else
			gtk_file_chooser_select_filename((GtkFileChooser*)msg, _path);
	}
	
	gtk_file_chooser_set_show_hidden((GtkFileChooser*)msg, _show_hidden);

	return run_file_dialog(msg);
}

#ifdef GTK3

GType type1, type2;

bool gDialog::selectFont()
{
	GtkFontChooserDialog *dialog;
	PangoFontDescription *desc;
	gFont *font;

	type1 = pango_font_family_get_type();
	type2 = pango_font_face_get_type();
	
	dialog = (GtkFontChooserDialog *)gtk_font_chooser_dialog_new(_title, NULL);

	if (_font)
		gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(dialog), _font->desc());

	if (run_dialog(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
 	{
 		gtk_widget_destroy(GTK_WIDGET(dialog));
		gDialog::setTitle(NULL);
		return true;
 	}

	desc = gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));

	gtk_widget_destroy(GTK_WIDGET(dialog));
	gDialog::setTitle(NULL);

	font = new gFont(desc);
	setFont(font);
	gFont::assign(&font);

	pango_font_description_free(desc);

	//printf("-> %s/%s/%s/%d\n", _font->name(), _font->bold() ? "BOLD" : "", _font->italic() ? "ITALIC" : "", _font->size());

	return false;
}
#else
bool gDialog::selectFont()
{
	GtkFontSelectionDialog *msg;
	PangoFontDescription *desc;
	char *buf;
	gFont *font;
		
	if (_title)
		msg=(GtkFontSelectionDialog*)gtk_font_selection_dialog_new (_title);
	else
		msg=(GtkFontSelectionDialog*)gtk_font_selection_dialog_new ("Select Font");


	if (_font)
	{
		desc = _font->desc();
		buf = pango_font_description_to_string(desc);
		gtk_font_selection_dialog_set_font_name(msg, buf);
		g_free(buf);
	}
	
	if (run_dialog(GTK_DIALOG(msg)) != GTK_RESPONSE_OK)
 	{
 		gtk_widget_destroy(GTK_WIDGET(msg));
		gDialog::setTitle(NULL);
		return true;
 	}
	
	buf = gtk_font_selection_dialog_get_font_name(msg);
	desc = pango_font_description_from_string(buf);
	g_free(buf);
	
	gtk_widget_destroy(GTK_WIDGET(msg));
	gDialog::setTitle(NULL);
	
	font = new gFont(desc);
	setFont(font);
	gFont::assign(&font);
	
	pango_font_description_free(desc);
	
	//printf("-> %s/%s/%s/%d\n", _font->name(), _font->bold() ? "BOLD" : "", _font->italic() ? "ITALIC" : "", _font->size());
	
	return false;
}
#endif

#ifdef GTK3
bool gDialog::selectColor()
{
	GtkColorChooserDialog *dialog;
	GdkRGBA color;

	gt_color_to_frgba(_color, &color.red, &color.green, &color.blue, &color.alpha);

	dialog = (GtkColorChooserDialog *)gtk_color_chooser_dialog_new(_title, NULL);

	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &color);

	gtk_window_present(GTK_WINDOW(dialog));

	if (run_dialog(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
 	{
 		gtk_widget_destroy(GTK_WIDGET(dialog));
		gDialog::setTitle(NULL);
		return true;
 	}

	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &color);
	_color = gt_frgba_to_color(color.red, color.green, color.blue, color.alpha);

	gtk_widget_destroy(GTK_WIDGET(dialog));
	gDialog::setTitle(NULL);
	return false;
}
#else
bool gDialog::selectColor()
{
	GtkColorSelectionDialog *msg;
	GdkColor gcol;
	
	fill_gdk_color(&gcol, _color);
	
	if (_title)
		msg=(GtkColorSelectionDialog*)gtk_color_selection_dialog_new (_title);
	else
		msg=(GtkColorSelectionDialog*)gtk_color_selection_dialog_new(GB.Translate("Select Color"));
    
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(msg)), &gcol);
	
	gtk_window_present(GTK_WINDOW(msg));
	if (run_dialog(GTK_DIALOG(msg)) != GTK_RESPONSE_OK)
 	{
 		gtk_widget_destroy(GTK_WIDGET(msg));
		gDialog::setTitle(NULL);
		return true;
 	}
	
	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(msg)), &gcol);
	
	_color = gt_gdkcolor_to_color(&gcol);
	
	gtk_widget_destroy(GTK_WIDGET(msg));
	gDialog::setTitle(NULL);
	return false;
}
#endif

int gDialog::filterIndex()
{
	return _filter_index;
}

void gDialog::setFilterIndex(int index)
{
	_filter_index = index;
	_filter_index_set = true;
}
