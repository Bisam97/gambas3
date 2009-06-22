#ifndef __GFONT_H
#define __GFONT_H

#include "gshare.h"

class gFont : public gShare
{
public:
	gFont();
	gFont(const char *name);
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
	int ascent();
	int descent();
	bool fixed();
	bool scalable();
	char **styles();

	bool bold();
	bool italic();
	char* name();
	int resolution();
	double size();
	bool strikeOut();
	bool underline();
	int grade();

	void setBold(bool vl);
	void setItalic(bool vl);
	void setName(char *nm);
	void setResolution(int vl);
	void setSize(double sz);
	void setGrade(int grade);
	void setStrikeOut(bool vl);
	void setUnderline(bool vl);

	const char *toString();
	const char *toFullString();
	int width(const char *text, int len = -1);
	int height(const char *text, int len = -1);
	int height();

//"Private"
	gFont(GtkWidget *wg);
	gFont(PangoFontDescription *fd);
	PangoContext* ct;
	PangoFontDescription *desc() { return pango_context_get_font_description(ct); }
	bool isAllSet();
	void reset();
	
	unsigned _bold_set : 1;
	unsigned _italic_set : 1;
	unsigned _name_set : 1;
	unsigned _size_set : 1;
	unsigned _strikeout_set : 1;
	unsigned _underline_set : 1;

private:
	
	bool uline;
	bool strike;
	void realize();
};

#endif
