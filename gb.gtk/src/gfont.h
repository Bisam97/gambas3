/***************************************************************************

  gfont.h

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

#ifndef __GFONT_H
#define __GFONT_H

#include "gshare.h"

class gFont : public gShare
{
public:
	gFont();
	virtual ~gFont();
  
  static void assign(gFont **dst, gFont *src = 0) { gShare::assign((gShare **)dst, src); }
  static void set(gFont **dst, gFont *src = 0) { gShare::assign((gShare **)dst, src); src->unref(); }
  
	static void init();
	static void exit();
	static int count();
	static const char *familyItem(int pos);

	gFont *copy();
	void copyTo(gFont *dst);
	void mergeFrom(gFont *src);
	bool equals(gFont *src);
	
	int ascent();
	float ascentF();
	int descent();
	bool fixed();
	bool scalable();
	char **styles();

	bool bold();
	bool italic();
	char* name();
	int resolution();
	double size();
	bool strikeout() const { return _strikeout; }
	bool underline() const { return _underline; }
	int grade();

	void setBold(bool vl);
	void setItalic(bool vl);
	void setName(char *nm);
	void setResolution(int vl);
	void setSize(double sz);
	void setGrade(int grade);
	void setStrikeout(bool vl);
	void setUnderline(bool vl);

	const char *toString();
	const char *toFullString();
	void setFromString(const char *str);
	
	int width(const char *text, int len = -1);
	int height(const char *text, int len = -1);
	int height();
	void textSize(const char *text, int len, float *w, float *h);
	void richTextSize(const char *txt, int len, float sw, float *w, float *h);
	
	bool mustFixSpacing() const { return _must_fix_spacing; }
	
	static gFont *desktopFont();
	static void setDesktopFont(gFont *vl);
	static int desktopScale();

//"Private"
	gFont(PangoFontDescription *fd);
	PangoFontDescription *desc() { return pango_context_get_font_description(_context); }
	bool isAllSet();
	void setAll(bool v);
	void setAllFrom(gFont *font);
	void reset();
	
	unsigned _bold_set : 1;
	unsigned _italic_set : 1;
	unsigned _name_set : 1;
	unsigned _size_set : 1;
	unsigned _strikeout_set : 1;
	unsigned _underline_set : 1;
	
private:
	
	bool _underline;
	bool _strikeout;
	
	void realize();
	void initFlags();
	void checkMustFixSpacing();
	PangoFontMetrics *metrics() {	return pango_context_get_metrics(_context, NULL, NULL); }
	void invalidateMetrics();
	
	PangoContext* _context;
	int _height;
	unsigned _must_fix_spacing : 1;
	
	static gFont *_desktop_font;
	static int _desktop_scale;
#ifdef GTK3
	static GtkStyleProvider *_desktop_css;
#endif	
};

#endif
