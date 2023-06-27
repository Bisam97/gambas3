/***************************************************************************

	gfont.cpp

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
#include "font-parser.h"
#include "gapplication.h"
#include "gdesktop.h"
#include "gtools.h"
#include "gb.form.font.h"

#include <math.h>

gFont *gFont::_desktop_font = NULL;
int gFont::_desktop_scale = 0;
#ifdef GTK3
GtkStyleProvider *gFont::_desktop_css = NULL;
#endif

static int FONT_n_families;
GList *FONT_families = NULL;
static int _nfont = 0;

//---------------------------------------------------------------------------

static int count_newlines(const char *str, int len)
{
	int i, n;
	
	for (i = 0, n = 0; i < len; i++)
	{
		if (str[i] == '\n')
			n++;
	}
	return n;
}

//---------------------------------------------------------------------------

void gFont::setFromString(const char *str)
{
	gchar **tokens, **p;
	gchar *copy, *elt;
	int grade;
	double size;
	int len;
	
	if (!str || !*str)
		return;
		
	tokens = g_strsplit(str, ",", 0);
	
	p = tokens;
	for(p = tokens; *p; p++)
	{
		copy = g_strdup(*p);
		elt = g_strstrip(copy);
		
		if (!strcasecmp(elt, "bold"))
			setBold(true);
		else if (!strcasecmp(elt, "italic"))
			setItalic(true);
		else if (!strcasecmp(elt, "underline"))
			setUnderline(true);
		else if (!strcasecmp(elt, "strikeout"))
			setStrikeout(true);
		else if (elt[0] == '+' || elt[0] == '-' || elt[0] == '0')
		{
			grade = atoi(elt);
			if (grade || elt[0] == '0')
				setGrade(grade);
		}
		else
		{
			size = atof(elt);
			if (isdigit(*elt) && size != 0.0)
				setSize(size);
			else
			{
				setBold(false);
				setItalic(false);
				setUnderline(false);
				setStrikeout(false);
				
				len = strlen(elt);
				if (len > 2 && elt[0] == '"' && elt[len - 1] == '"')
				{
					elt[len - 1] = 0;
					elt++;
				}
				setName(elt);
			}
		}

		g_free(copy);
	}
	
	g_strfreev(tokens);
}


void gFont::init()
{
	PangoFontFamily **_families;
	PangoContext *ct;
	char *buf1,*buf2; 
	int bucle;

	ct = gdk_pango_context_get();
	pango_context_list_families(ct, &_families, &FONT_n_families);
	
	for (bucle = 0; bucle<FONT_n_families; bucle++)
	{
		buf1 = (char *)pango_font_family_get_name(_families[bucle]);
		if (buf1)
		{
			buf2 = (char *)g_malloc(sizeof(char)*(strlen(buf1)+1));
			strcpy(buf2,buf1);
			FONT_families = g_list_prepend (FONT_families,buf2);
		}
	}
	
	if (FONT_families)
		FONT_families = g_list_sort(FONT_families, (GCompareFunc)strcasecmp);
	
	g_free(_families);
	g_object_unref(G_OBJECT(ct));
}

void gFont::exit()
{
	GList *iter;
	
	gFont::assign(&_desktop_font);
	
	iter = FONT_families;
	
	if (iter)
	{
		iter = g_list_first(iter);
		while (iter)
		{
			g_free(iter->data);
			iter=iter->next;
		}
	}
	
	if (FONT_families) g_list_free(FONT_families);
	
	//if (_nfont)
	//  fprintf(stderr, "WARNING: %d gFont objects were not freed.\n", _nfont);
}

int gFont::count()
{
	if (!FONT_families) gFont::init();
	return FONT_n_families;
}

const char *gFont::familyItem(int pos)
{
	if (!FONT_families) gFont::init();
	if ( (pos < 0) || (pos >= FONT_n_families) ) return NULL; 
	
	return (const char *)g_list_nth(FONT_families, pos)->data;
}

#if 0
void gFont::updateWidget()
{
	if (!wid) return;

	PangoFontDescription *desc = pango_context_get_font_description(ct);
	gtk_widget_modify_font(wid,desc);
	
	if (G_OBJECT_TYPE(wid)==GTK_TYPE_LABEL)
	{
		PangoAttrList *pal=pango_attr_list_new();
		if (_strikeout)
		{
			PangoAttribute *pa=pango_attr_strikethrough_new(true);
			pa->start_index = 0;
			pa->end_index = g_utf8_strlen(gtk_label_get_text(GTK_LABEL(wid)), -1); 
			pango_attr_list_insert(pal, pa);
		}
		if (_underline)
		{
			PangoAttribute *pa=pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
			pa->start_index = 0;
			pa->end_index = g_utf8_strlen(gtk_label_get_text(GTK_LABEL(wid)), -1); 
			pango_attr_list_insert(pal, pa);
		}
		gtk_label_set_attributes(GTK_LABEL(wid), pal);
		pango_attr_list_unref(pal);
	}

}
#endif

void gFont::reset()
{
	_strikeout = false;
	_underline = false;

	_bold_set = false;
	_italic_set = false;
	_name_set = false;
	_size_set = false;
	_strikeout_set = false;
	_underline_set = false;
}

void gFont::setAll(bool v)
{
	_bold_set = v;
	_italic_set = v;
	_name_set = v;
	_size_set = v;
	_strikeout_set = v;
	_underline_set = v;
}

void gFont::setAllFrom(gFont *font)
{
	if (!font)
		setAll(false);
	else
	{
		_bold_set = font->_bold_set;
		_italic_set = font->_italic_set;
		_name_set = font->_name_set;
		_size_set = font->_size_set;
		_strikeout_set = font->_strikeout_set;
		_underline_set = font->_underline_set;
	}
}

void gFont::realize()
{
	_context = NULL;
	_height = 0;

	reset();
	
	_nfont++;  
}

void gFont::invalidateMetrics()
{
	pango_context_changed(_context);
	_height = 0;
}

void gFont::initFlags()
{
	gFont *comp = gDesktop::font();
	
	_bold_set = comp->bold() != bold();
	_italic_set = comp->italic() != italic();
	_name_set = strcmp(comp->name(), name()) != 0;
	_size_set = comp->size() != size();
	_strikeout_set = comp->strikeout() != strikeout();
	_underline_set = comp->underline() != underline();

	checkMustFixSpacing();
}


gFont::gFont() : gShare()
{
	PangoFontDescription *fd;
	bool free_fd = false;

	realize();
	_context = gdk_pango_context_get();

	if (_desktop_font)
	{
		_desktop_font->copyTo(this);
		reset();
	}
	
	if (_desktop_font)
		fd = _desktop_font->desc();
	else
	{	
#ifdef GTK3

		char *font;

		g_object_get(gtk_settings_get_default(), "gtk-font-name", &font, (char *)NULL);
		fd = pango_font_description_from_string(font);
		g_free(font);
		free_fd = true;

#else
	
		fd = gtk_widget_get_default_style()->font_desc;
	
#endif
	}
	
	pango_context_set_font_description(_context, fd);
	
	if (free_fd)
		pango_font_description_free(fd);
	
	checkMustFixSpacing();
}

gFont::gFont(PangoFontDescription *fd) : gShare()
{
	realize();
	_context = gdk_pango_context_get();
	pango_context_set_font_description(_context, fd);
	initFlags();
}

void gFont::copyTo(gFont *dst)
{
	dst->reset();
	if (_name_set) dst->setName(name());
	if (_size_set) dst->setSize(size());
	if (_bold_set) dst->setBold(bold());
	if (_italic_set) dst->setItalic(italic());
	if (_underline_set) dst->setUnderline(underline());
	if (_strikeout_set) dst->setStrikeout(strikeout());
}

void gFont::mergeFrom(gFont *src)
{
	if (!_name_set && src->_name_set) setName(src->name());
	if (!_size_set && src->_size_set) setSize(src->size());
	if (!_bold_set && src->_bold_set) setBold(src->bold());
	if (!_italic_set && src->_italic_set) setItalic(src->italic());
	if (!_underline_set && src->_underline_set) setUnderline(src->underline());
	if (!_strikeout_set && src->_strikeout_set) setStrikeout(src->strikeout());
}

gFont *gFont::copy()
{
	gFont *f = new gFont();
	copyTo(f);
	return f;
}

gFont::~gFont()
{
	g_object_unref(_context);
	_nfont--;
}


int gFont::ascent()
{
	return gt_pango_to_pixel(pango_font_metrics_get_ascent(metrics()));
}

float gFont::ascentF()
{
	return (float)pango_font_metrics_get_ascent(metrics()) / PANGO_SCALE;
}

int gFont::descent()
{
	return gt_pango_to_pixel(pango_font_metrics_get_descent(metrics()));
}

bool gFont::bold()
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	PangoWeight w;
		
	w = pango_font_description_get_weight(desc);
	return (w > PANGO_WEIGHT_NORMAL);
}

void gFont::setBold(bool vl)
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);

	if (vl)
		pango_font_description_set_weight(desc,PANGO_WEIGHT_BOLD);
	else
		pango_font_description_set_weight(desc,PANGO_WEIGHT_NORMAL);
	
	_bold_set = true;
	invalidateMetrics();
}

bool gFont::italic()
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);

	return pango_font_description_get_style(desc) !=PANGO_STYLE_NORMAL; 
}

void gFont::setItalic(bool vl)
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);

	if (vl)
		pango_font_description_set_style(desc,PANGO_STYLE_ITALIC);
	else
		pango_font_description_set_style(desc,PANGO_STYLE_NORMAL);
		
	_italic_set = true;
	invalidateMetrics();
}

char* gFont::name()
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	return (char *)pango_font_description_get_family(desc);
}

void gFont::setName(char *nm)
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	
	pango_font_description_set_family(desc, nm);
	
	_name_set = true;
	invalidateMetrics();
	
	checkMustFixSpacing();
}

double gFont::size()
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	return (double)pango_font_description_get_size(desc) / (double)PANGO_SCALE;
}

int gFont::grade()
{
	double desktop = gDesktop::font()->size();
	return SIZE_TO_GRADE(size(), desktop);
}

void gFont::setSize(double sz)
{
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	
	pango_font_description_set_size(desc, (int)(sz * PANGO_SCALE + 0.5));
	
	_size_set = true;
	invalidateMetrics();
}

void gFont::setGrade(int grade)
{
	double desktop = gDesktop::font()->size();
	
	if (grade < FONT_GRADE_MIN)
		grade = FONT_GRADE_MIN;
	else if (grade > FONT_GRADE_MAX)
		grade = FONT_GRADE_MAX;
	
	setSize(GRADE_TO_SIZE(grade, desktop));
}

const char *gFont::toString()
{
	char *family;
	GString *desc;
	char *ret;
	int s;
	
	desc = g_string_new(NULL);
	
	family = name();
	if (isdigit(*family) && atof(family))
		g_string_append_printf(desc, "\"%s\"", family);
	else
		g_string_append(desc, family);
	
	s = (int)(size() * 10 + 0.5);
	
	g_string_append_printf(desc, ",%d", s / 10);
	if (s % 10)
		g_string_append_printf(desc, ".%d", s % 10);
	if (bold())
		g_string_append(desc, ",Bold");
	if (italic())
		g_string_append(desc, ",Italic");
	if (underline())
		g_string_append(desc, ",Underline");
	if (strikeout())
		g_string_append(desc, ",Strikeout");

	ret = g_string_free(desc, false);
	gt_free_later(ret);
	
	return ret;
}

const char *gFont::toFullString()
{
	GString *desc = g_string_new("");
	char *ret;
	
	g_string_append_printf(desc, "[ ");

	if (_name_set)
		g_string_append_printf(desc, "%s ", name());

	if (_size_set)
		g_string_append_printf(desc, "%g ", (double)((int)(size() * 10 + 0.5)) / 10);
	
	if (_bold_set)
		g_string_append_printf(desc, "%s ", bold() ? "Bold" : "NotBold");
	if (_italic_set)
		g_string_append_printf(desc, "%s ", italic() ? "Italic" : "NotItalic");
	if (_underline_set)
		g_string_append_printf(desc, "%s ", underline() ? "Underline" : "NotUnderline");
	if (_strikeout_set)
		g_string_append_printf(desc, "%s ", strikeout() ? "Strikeout" : "NotStrikeout");

	g_string_append_printf(desc, "]");
	
	ret = g_string_free(desc, false);
	gt_free_later(ret);
	return ret;
}

void gFont::textSize(const char *text, int len, float *w, float *h)
{
	PangoLayout *ly;
	PangoRectangle ink_rect, rect = { 0 };
	
	if (text && len)
	{
		ly = pango_layout_new(_context);
		pango_layout_set_text(ly, text, len);	
		gt_set_layout_from_font(ly, this);
		pango_layout_get_extents(ly, &ink_rect, &rect);
		g_object_unref(ly);
		rect.width = Max(rect.width, ink_rect.width);
		rect.height = Max(rect.height, ink_rect.height);
	}
	
	if (w) *w = (float)rect.width / PANGO_SCALE;
	if (h)
	{
		*h = (float)rect.height / PANGO_SCALE;
		if (mustFixSpacing())
			*h += 1;
	}
}

int gFont::width(const char *text, int len)
{
	float fw;
	textSize(text, len, &fw, NULL);
	return gt_pango_to_pixel(fw * PANGO_SCALE);
}

int gFont::height(const char *text, int len)
{
	int n = count_newlines(text, len);
	return height() * (n + 1);
}

int gFont::height()
{
	if (!_height)
	{
		float h1, h2;
		textSize("A\nA", 3, NULL, &h1);
		textSize("A\nA\nA", 5, NULL, &h2);
		_height = gt_pango_to_pixel((h2 - h1) * PANGO_SCALE);
	}
	return _height;
}

bool gFont::scalable()
{
	bool ret=false;
	
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	//PangoFontDescription *tmp;
	const char* name=pango_font_description_get_family(desc);
	PangoFontFamily **families;
	PangoFontFace **faces;
	int *sizes;
	int n_families;
	int n_faces;
	int n_sizes;
	//int b2;
	const char *buf;
	
	if (!name) return false;
	
	pango_context_list_families(_context, &families,&n_families);
	
	if (!families) return false;
	
	for (int bucle=0;bucle<n_families;bucle++)
	{
		buf=pango_font_family_get_name(families[bucle]);
		if (!strcmp(buf,name))
		{
			pango_font_family_list_faces(families[bucle],&faces,&n_faces);
			if (faces)
			{
				pango_font_face_list_sizes(faces[0],&sizes,&n_sizes);
				if (sizes) 
					g_free(sizes);
				else
					ret=true;
				g_free(faces);
				g_free(families);
				return ret;
			}
			else
			{
				g_free(families);
				return false;
			}
		}
		
	}
	
	g_free(families);
	return false;
}

bool gFont::fixed()
{
	bool ret=false;
	
	PangoFontDescription *desc = pango_context_get_font_description(_context);
	const char* name=pango_font_description_get_family(desc);
	PangoFontFamily **families;
	int n_families;
	const char *buf;
	
	if (!name) return false;
	
	pango_context_list_families(_context, &families,&n_families);
	
	if (!families) return false;
	
	for (int bucle=0;bucle<n_families;bucle++)
	{
		buf=pango_font_family_get_name(families[bucle]);
		if (!strcmp(buf,name))
		{
			ret=pango_font_family_is_monospace (families[bucle]);
			g_free(families);
			return ret;
		}
		
	}
	
	g_free(families);
	return false;
}

char** gFont::styles()
{
	return 0;
}

int gFont::resolution()
{
	return 0;
}

void gFont::setResolution(int vl)
{
}

void gFont::setStrikeout(bool vl)
{
	_strikeout = vl;
	_strikeout_set = true;
}

void gFont::setUnderline(bool vl)
{
	_underline = vl;
	_underline_set = true;
}

bool gFont::isAllSet()
{
	return 
		_bold_set
		&& _italic_set
		&& _name_set
		&& _size_set
		&& _strikeout_set
		&& _underline_set;
}

void gFont::richTextSize(const char *text, int len, float sw, float *w, float *h)
{
	PangoLayout *ly;
	PangoRectangle ink_rect, rect = { 0 };
	char *html;
	
	if (text && len)
	{
		ly = pango_layout_new(_context);
		if (sw > 0)
		{
			pango_layout_set_wrap(ly, PANGO_WRAP_WORD_CHAR);
			pango_layout_set_width(ly, (int)ceilf(sw * PANGO_SCALE));
		}
		html = gt_html_to_pango_string(text, len, false);
		pango_layout_set_markup(ly, html, -1);	
		gt_add_layout_from_font(ly, this);
		pango_layout_get_extents(ly, &ink_rect, &rect);
		g_free(html);
		g_object_unref(ly);
		rect.width = Max(rect.width, ink_rect.width);
		rect.height = Max(rect.height, ink_rect.height);
	}
	
	if (w) *w = (float)rect.width / PANGO_SCALE;
	if (h)
	{
		*h = (float)rect.height / PANGO_SCALE;
		if (mustFixSpacing())
			*h += 1;
	}
}

void gFont::checkMustFixSpacing()
{
	_must_fix_spacing = ::strcmp(name(), "Gambas") == 0 || ::strcmp(name(), "GambasRound") == 0 ;
}

bool gFont::equals(gFont *src)
{
	if (src == this)
		return true;
	
	return !::strcmp(name(), src->name()) && bold() == src->bold() && italic() == src->italic()
		&& size() == src->size() && strikeout() == src->strikeout() && underline() == src->underline();
}


int gFont::desktopScale()
{
	if (!_desktop_scale)
	{
		gFont *ft = desktopFont();
		_desktop_scale = GET_DESKTOP_SCALE(ft->size(), gDesktop::resolution());
	}

	return _desktop_scale;
}


gFont* gFont::desktopFont()
{
	if (!_desktop_font)
	{
		_desktop_font = new gFont();
		_desktop_font->setAll(true);
	}
	
	return _desktop_font;
}

#ifndef GTK3
static void cb_update_font(gControl *control)
{
	control->updateFont();
}
#endif

void gFont::setDesktopFont(gFont *ft)
{
	gFont::set(&_desktop_font, ft ? ft->copy() : new gFont());
	_desktop_scale = 0;

#ifndef GTK3
	
	gApplication::forEachControl(cb_update_font);
	
#else

	gt_define_style_sheet(&_desktop_css, NULL);

	if (ft)
	{
		GString *css = g_string_new(NULL);
		g_string_append(css, "* {\n");
		gt_css_add_font(css, _desktop_font);
		g_string_append(css, "}");
		gt_define_style_sheet(&_desktop_css, css);
	}

#endif
}
