/***************************************************************************

  c_htmldocument.cpp

  gb.form.htmlview component

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#define __C_HTMLDOCUMENT_CPP

#include "c_htmldocument.h"

#define GET_CURRENT() GB_PAINT *_paint = (GB_PAINT *)DRAW.Paint.GetCurrent()
#define PAINT _paint->desc
#define CURRENT _paint

DECLARE_EVENT(EVENT_Link);
DECLARE_EVENT(EVENT_Title);

//-------------------------------------------------------------------------

static int convert_color(const litehtml::web_color &color)
{
	return GB_COLOR_MAKE(color.red, color.green, color.blue, color.alpha);
}

static int get_font_ascent(void *font)
{
	GB_VALUE *ret = GB.GetProperty(font, "Ascent");
	return ret->_integer.value;
}

static bool same_color(const litehtml::web_color &color1, const litehtml::web_color &color2)
{
	return color1.red == color2.red && color1.green == color2.green && color1.blue == color2.blue && color1.alpha == color2.alpha;
}

//-------------------------------------------------------------------------

struct clip_box
{
	typedef std::vector<clip_box> vector;
	litehtml::position	box;
	litehtml::border_radiuses radius;

	clip_box(const litehtml::position& vBox, litehtml::border_radiuses vRad)
	{
		box = vBox;
		radius = vRad;
	}

	clip_box(const clip_box& val)
	{
		box = val.box;
		radius = val.radius;
	}
	
	clip_box& operator=(const clip_box& val)
	{
		box = val.box;
		radius = val.radius;
		return *this;
	}
};

class html_document : public litehtml::document_container
{
private:
	
	litehtml::document::ptr m_html;
	bool _valid;
	int m_client_w;
	int m_client_h;
	int m_scroll_x;
	int m_scroll_y;
	clip_box::vector m_clips;
	GB_FUNCTION _func_load_image;
	GB_FUNCTION _func_set_cursor;
	GB_FUNCTION _func_load_css;
	
public:
	
	enum { MOUSE_DOWN, MOUSE_UP, MOUSE_MOVE, MOUSE_LEAVE };
	
	void *_object;
	
	html_document(void *object);
	virtual ~html_document();
	
	bool isValid() const { return _valid; }
	bool load(const char *html, const char *css);
	bool render(int w, int h);
	void draw(int x, int y, int w, int h);
	int getWidth() const;
	int getHeight() const;
	
	void rounded_rectangle(const litehtml::position &pos, const litehtml::border_radiuses &radius, bool keep = false, bool back = false);
	void begin_clip();
	void end_clip();
	void set_color(const litehtml::web_color &color) { DRAW.Paint.SetBackground(convert_color(color)); }
	void on_mouse(int event, int x = 0, int y = 0);
	void on_media_change();
	GB_IMG *get_image(const char *src, const char *baseurl);
	int find_anchor(const litehtml::string& anchor);
	
protected:
	
	virtual litehtml::uint_ptr create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
	virtual void delete_font(litehtml::uint_ptr hFont) override;
	virtual int text_width(const char* text, litehtml::uint_ptr hFont) override;
	virtual void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
	virtual int pt_to_px(int pt) const override;
	virtual int get_default_font_size() const override;
	virtual const char* get_default_font_name() const override;
	virtual void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
	virtual void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
	virtual void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
	// Note: regular <img> images are also drawn with draw_background
	// bg is guaranteed to have at least one item.
	// backgrounds in bg are in CSS order - the last one is the farthest from the user.
	// only the last background has valid background-color.
	virtual void draw_background(litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bg) override;
	virtual void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;

	virtual	void set_caption(const char* caption) override;
	virtual	void set_base_url(const char* base_url) override;
	virtual void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
	virtual void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
	virtual	void set_cursor(const char* cursor) override;
	virtual	void transform_text(litehtml::string& text, litehtml::text_transform tt) override;
	virtual void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
	virtual void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y) override;
	virtual void del_clip() override;
	virtual void get_client_rect(litehtml::position& client) const override;
	virtual litehtml::element::ptr create_element( const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) override;
	virtual void get_media_features(litehtml::media_features& media) const override;
	virtual void get_language(litehtml::string& language, litehtml::string& culture) const override;
	virtual litehtml::string resolve_color(const litehtml::string& color) const override;
};

//-------------------------------------------------------------------------

html_document::html_document(void *object)
{
	m_html = NULL;
	m_client_w = 0;
	m_client_h = 0;
	m_scroll_x = 0;
	m_scroll_y = 0;
	_object = object;
	_valid = true;
	
	GB.GetFunction(&_func_load_image, THIS, "_LoadImage", "ss", "Image");
	GB.GetFunction(&_func_set_cursor, THIS, "_SetCursor", "s", NULL);
	GB.GetFunction(&_func_load_css, THIS, "_LoadCSS", "ss", "s");
}

html_document::~html_document()
{
}

bool html_document::render(int w, int h)
{
	if (!m_html)
		return true;
	
	m_client_w = w;
	m_client_h = h;
  m_html->media_changed();
	m_html->render(w);
	return false;
}

void html_document::draw(int x, int y, int w, int h)
{
	if (!m_html)
		return;
	
	litehtml::position pos;
	pos.x = m_scroll_x = x;
	pos.y = m_scroll_y = y;
	pos.width = w;
	pos.height = h;
	
	m_html->draw((litehtml::uint_ptr)this, 0, 0, &pos);
}

int html_document::getWidth() const
{
	return m_html ? m_html->width() : 0;
}

int html_document::getHeight() const
{
	return m_html ? m_html->height() : 0;
}

bool html_document::load(const char *html, const char *css)
{
	m_html = litehtml::document::createFromString(html, this, css ? css : litehtml::master_css);
	m_client_w = 0;
	m_client_h = 0;
	return (m_html == NULL);
}

litehtml::uint_ptr html_document::create_font(const char *faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm)
{
	GB_FUNCTION func;
	GB_VALUE *ret;
	GB_VALUE val;
	void *font;
	int len;
	
	len = strlen(faceName);
	
	if (len >=2 && faceName[0] == '\'' && faceName[len - 1] == '\'')
	{
		faceName++;
		len -=2;
	}
	
	if (litehtml::t_strncasecmp(faceName, "sans-serif", len) == 0)
	{
		faceName = get_default_font_name();
		len = strlen((char *)faceName);
	}
	
	if (litehtml::t_strncasecmp(faceName, "monospace", len) == 0 && THIS->monospace_font_name)
	{
		faceName = THIS->monospace_font_name;
		len = GB.StringLength(THIS->monospace_font_name);
	}
	
	font = GB.New(GB.FindClass("Font"), NULL, NULL);
	
	val.type = GB_T_CSTRING;
	val._string.value.addr = (char *)faceName;
	val._string.value.start = 0;
	val._string.value.len = len;
	GB.SetProperty(font, "Name", &val);
	
	val.type = GB_T_FLOAT;
	val._float.value = size * 1200 / pt_to_px(1200);
	GB.SetProperty(font, "Size", &val);
	
	val.type = GB_T_BOOLEAN;
	val._boolean.value = weight >= 550;
	GB.SetProperty(font, "Bold", &val);
	
	val.type = GB_T_BOOLEAN;
	val._boolean.value = italic == litehtml::font_style_italic;
	GB.SetProperty(font, "Italic", &val);
	
	val.type = GB_T_BOOLEAN;
	val._boolean.value = decoration & litehtml::font_decoration_underline ? -1 : 0;
	GB.SetProperty(font, "Underline", &val);
	
	val.type = GB_T_BOOLEAN;
	val._boolean.value = decoration & litehtml::font_decoration_linethrough ? -1 : 0;
	GB.SetProperty(font, "Strikeout", &val);
	
	fm->ascent = get_font_ascent(font);
	
	ret = GB.GetProperty(font, "Descent");
	fm->descent = ret->_integer.value;
	
	ret = GB.GetProperty(font, "Height");
	fm->height = ret->_integer.value;
	
	GB.GetFunction(&func, font, "TextHeight", "s", "i");
	GB.Push(1, GB_T_STRING, "x", 1);
	ret = GB.Call(&func, 1, FALSE);
	fm->x_height = ret->_integer.value;
	
	//fm->draw_spaces = (italic == litehtml::fontStyleItalic || decoration);
	
	GB.Ref(font);
	return (litehtml::uint_ptr)font;
}

void html_document::delete_font(litehtml::uint_ptr hFont)
{
	GB.Unref(POINTER(&hFont));
}

int html_document::text_width(const char *text, litehtml::uint_ptr hFont)
{
	static GB_FUNCTION _func = { 0 };
	GB_VALUE *ret;
	GET_CURRENT();
	
	if (CURRENT)
	{
		float w;
		PAINT->TextSize(CURRENT, text, strlen(text), &w, NULL);
		return (int)ceil(w);
	}
	else
	{
		if (!GB_FUNCTION_IS_VALID(&_func))
			GB.GetFunction(&_func, (void *)hFont, "TextWidth", "s", "i");
		_func.object = (void *)hFont;

		GB.Push(1, GB_T_STRING, text, strlen(text));
		ret = GB.Call(&_func, 1, FALSE);
		return ret->_integer.value;
	}
}

void html_document::draw_text(litehtml::uint_ptr hdc, const char *text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos)
{
	GET_CURRENT();
	
	begin_clip();
	PAINT->MoveTo(CURRENT, (float)pos.x, (float)(pos.y + get_font_ascent((void *)hFont)));
	set_color(color);
	PAINT->Font(CURRENT, TRUE, (void **)&hFont);
	PAINT->Text(CURRENT, text, strlen(text), -1, -1, GB_DRAW_ALIGN_DEFAULT, TRUE);
	end_clip();
}

int html_document::pt_to_px(int pt) const
{
	GET_CURRENT();
	if (CURRENT)
		return (int)(0.4 + pt * CURRENT->resolutionX / 72.0);
	else
	{
		GB.Error(NULL);
		return (int)(0.4 + pt * THIS->resolution / 72.0);
	}
}

int html_document::get_default_font_size() const
{
	return pt_to_px(THIS->default_font_size ? THIS->default_font_size : 12);
}

const char *html_document::get_default_font_name() const
{
	return THIS->default_font_name ? THIS->default_font_name : "sans-serif";
}

void html_document::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
	GET_CURRENT();
	float lw;
	
	begin_clip();
	
	switch(marker.marker_type)
	{
		case litehtml::list_style_type_circle:
			
			PAINT->Ellipse(CURRENT, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, 0, M_PI * 2, FALSE);
			lw = 0.5;
			PAINT->LineWidth(CURRENT, TRUE, &lw);
			set_color(marker.color);
			PAINT->Stroke(CURRENT, FALSE);
			break;
			
		case litehtml::list_style_type_disc:
			
			PAINT->Ellipse(CURRENT, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, 0, M_PI * 2, FALSE);
			set_color(marker.color);
			PAINT->Fill(CURRENT, FALSE);
			break;
			
		case litehtml::list_style_type_square:
			
			PAINT->FillRect(CURRENT, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, convert_color(marker.color));
			break;
			
		default:
			/*do nothing*/
			break;
	}
	
	end_clip();
}

GB_IMG *html_document::get_image(const char *src, const char *baseurl)
{
	GB_IMG *img;
	
	if (!GB_FUNCTION_IS_VALID(&_func_load_image))
		return NULL;
	GB.Push(2, GB_T_STRING, src, 0, GB_T_STRING, baseurl, 0);
	return (GB_IMG *)((GB_OBJECT *)GB.Call(&_func_load_image, 2, FALSE))->value;
}

void html_document::get_image_size(const char *src, const char *baseurl, litehtml::size& sz)
{
	GB_IMG *img = get_image(src, baseurl);
	
	if (img)
	{
		sz.width = img->width;
		sz.height = img->height;
	}
}

void html_document::load_image(const char *src, const char *baseurl, bool redraw_on_ready)
{
	get_image(src, baseurl);
}

void html_document::draw_background(litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bgs)
{
	litehtml::background_paint bg;
	bool first = false;

	GET_CURRENT();
	
	begin_clip();

	for (int i = bgs.size() - 1; i >= 0; i--)
	{
		bg = bgs[i];

		if (!first)
		{
			first = true;

			rounded_rectangle(bg.border_box, bg.border_radius);
			PAINT->Clip(CURRENT, FALSE);

			PAINT->Rectangle(CURRENT, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height);

			if (bg.color.alpha)
			{
				PAINT->Clip(CURRENT, TRUE);
				set_color(bg.color);
				PAINT->Fill(CURRENT, FALSE);
			}
			else
				PAINT->Clip(CURRENT, FALSE);
		}

		if (!bg.image.empty())
		{
			GB_IMG *image = get_image(bg.image.c_str(), bg.baseurl.c_str());
			int x, y;

			if (image)
			{
				if (bg.attachment == litehtml::background_attachment_fixed)
					DRAW.Paint.Translate(m_scroll_x, m_scroll_y);

				switch(bg.repeat)
				{
					case litehtml::background_repeat_no_repeat:

						PAINT->DrawImage(CURRENT, image, bg.position_x, bg.position_y, bg.image_size.width, bg.image_size.height, 1.0, NULL);
						break;

					case litehtml::background_repeat_repeat_x:

						x = -((bg.position_x - bg.clip_box.x + bg.image_size.width - 1) / bg.image_size.width * bg.image_size.width);
						while (x < bg.clip_box.width)
						{
							PAINT->DrawImage(CURRENT, image, bg.position_x + x, bg.position_y, bg.image_size.width, bg.image_size.height, 1.0, NULL);
							x += bg.image_size.width;
						}
						break;

					case litehtml::background_repeat_repeat_y:

						y = -((bg.position_y - bg.clip_box.y + bg.image_size.height - 1) / bg.image_size.height * bg.image_size.height);
						while (y < bg.clip_box.height)
						{
							PAINT->DrawImage(CURRENT, image, bg.position_x, bg.position_y + y, bg.image_size.width, bg.image_size.height, 1.0, NULL);
							y += bg.image_size.height;
						}
						break;

					case litehtml::background_repeat_repeat:

						x = -((bg.position_x - bg.clip_box.x + bg.image_size.width - 1) / bg.image_size.width * bg.image_size.width);
						while (x < bg.clip_box.width)
						{
							y = -((bg.position_y - bg.clip_box.y + bg.image_size.height - 1) / bg.image_size.height * bg.image_size.height);
							while (y < bg.clip_box.height)
							{
								PAINT->DrawImage(CURRENT, image, bg.position_x + x, bg.position_y + y, bg.image_size.width, bg.image_size.height, 1.0, NULL);
								y += bg.image_size.height;
							}
							x += bg.image_size.width;
						}
						break;
				}
			}
		}
	}

	end_clip();
}

void html_document::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root)
{
	litehtml::position inner_pos;
	litehtml::border_radiuses inner_radius;
	bool btop, bright, bbottom, bleft;
	int btw, brw, bbw, blw;
	
	btw = borders.top.width;
	brw = borders.right.width;
	bbw = borders.bottom.width;
	blw = borders.left.width;
	
	btop = btw > 0 && borders.top.style > litehtml::border_style_hidden; // && borders.top.color.alpha > 0;
	bright = brw > 0 && borders.right.style > litehtml::border_style_hidden; // && borders.right.color.alpha > 0;
	bbottom = bbw > 0 && borders.bottom.style > litehtml::border_style_hidden; // && borders.bottom.color.alpha > 0;
	bleft = blw > 0 && borders.left.style > litehtml::border_style_hidden; // && borders.left.color.alpha > 0;
	
	if (!btop && !bright && !bbottom && !bleft)
		return;
	
	/*if (!btop) btw = 0;
	if (!bright) brw = 0;
	if (!bbottom) bbw = 0;
	if (!bleft) blw = 0;*/
	
	GET_CURRENT();
	
	begin_clip();

	inner_radius = borders.radius;
	
	inner_radius.top_left_y = inner_radius.top_left_x;
	inner_radius.top_right_y = inner_radius.top_right_x;
	inner_radius.bottom_left_y = inner_radius.bottom_left_x;
	inner_radius.bottom_right_y = inner_radius.bottom_right_x;
	
	inner_radius.top_left_x -= blw;
	inner_radius.top_left_y -= btw;
	inner_radius.top_right_x -= brw;
	inner_radius.top_right_y -= btw;
	inner_radius.bottom_left_x -= blw;
	inner_radius.bottom_left_y -= bbw;
	inner_radius.bottom_right_x -= brw;
	inner_radius.bottom_right_y -= bbw;
	inner_radius.fix_values();
	
	inner_pos = draw_pos;
	
	inner_pos.x += blw;
	inner_pos.width -= blw + brw;
	inner_pos.y += btw;
	inner_pos.height -= btw + bbw;
	
	if (same_color(borders.left.color, borders.right.color) && same_color(borders.left.color, borders.top.color) && same_color(borders.left.color, borders.bottom.color))
	{
		rounded_rectangle(draw_pos, borders.radius);
		if (inner_pos.width > 0 && inner_pos.height > 0)
			rounded_rectangle(inner_pos, inner_radius, true, true);
	
		set_color(borders.left.color);
		PAINT->Fill(CURRENT, FALSE);
	}
	else
	{
		if (bleft)
		{
			PAINT->Save(CURRENT);
			PAINT->MoveTo(CURRENT, draw_pos.x, draw_pos.y);
			PAINT->LineTo(CURRENT, draw_pos.x + blw * 2, draw_pos.y + btw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + blw * 2, draw_pos.y + draw_pos.height - bbw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x, draw_pos.y + draw_pos.height);
			PAINT->LineTo(CURRENT, draw_pos.x, draw_pos.y);
			PAINT->Clip(CURRENT, FALSE);
			
			rounded_rectangle(draw_pos, borders.radius);
			if (inner_pos.width > 0 && inner_pos.height > 0)
				rounded_rectangle(inner_pos, inner_radius, true, true);
		
			set_color(borders.left.color);
			PAINT->Fill(CURRENT, FALSE);
			PAINT->Restore(CURRENT);
		}
		
		if (bright)
		{
			PAINT->Save(CURRENT);
			PAINT->MoveTo(CURRENT, draw_pos.x + draw_pos.width, draw_pos.y);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width - brw * 2, draw_pos.y + btw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width - brw * 2, draw_pos.y + draw_pos.height - bbw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width, draw_pos.y + draw_pos.height);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width, draw_pos.y);
			PAINT->Clip(CURRENT, FALSE);
			
			rounded_rectangle(draw_pos, borders.radius);
			if (inner_pos.width > 0 && inner_pos.height > 0)
				rounded_rectangle(inner_pos, inner_radius, true, true);
		
			set_color(borders.right.color);
			PAINT->Fill(CURRENT, FALSE);
			PAINT->Restore(CURRENT);
		}
		
		if (btop)
		{
			PAINT->Save(CURRENT);
			PAINT->MoveTo(CURRENT, draw_pos.x - 1, draw_pos.y);
			PAINT->LineTo(CURRENT, draw_pos.x - 1 + blw * 2, draw_pos.y + btw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width + 1 - brw * 2, draw_pos.y + btw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width + 1, draw_pos.y);
			PAINT->LineTo(CURRENT, draw_pos.x - 1, draw_pos.y);
			PAINT->Clip(CURRENT, FALSE);
			
			rounded_rectangle(draw_pos, borders.radius);
			if (inner_pos.width > 0 && inner_pos.height > 0)
				rounded_rectangle(inner_pos, inner_radius, true, true);
		
			set_color(borders.top.color);
			PAINT->Fill(CURRENT, FALSE);
			PAINT->Restore(CURRENT);
		}
		
		if (bbottom)
		{
			PAINT->Save(CURRENT);
			PAINT->MoveTo(CURRENT, draw_pos.x - 1, draw_pos.y + draw_pos.height);
			PAINT->LineTo(CURRENT, draw_pos.x - 1 + blw * 2, draw_pos.y + draw_pos.height - bbw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width + 1 - brw * 2, draw_pos.y + draw_pos.height - bbw * 2);
			PAINT->LineTo(CURRENT, draw_pos.x + draw_pos.width + 1, draw_pos.y + draw_pos.height);
			PAINT->LineTo(CURRENT, draw_pos.x - 1, draw_pos.y + draw_pos.height);
			PAINT->Clip(CURRENT, FALSE);
			
			rounded_rectangle(draw_pos, borders.radius);
			if (inner_pos.width > 0 && inner_pos.height > 0)
				rounded_rectangle(inner_pos, inner_radius, true, true);
		
			set_color(borders.bottom.color);
			PAINT->Fill(CURRENT, FALSE);
			PAINT->Restore(CURRENT);
		}
	}
	
	end_clip();
}

void html_document::set_caption(const char *caption)
{
	GB.Raise(THIS, EVENT_Title, 1, GB_T_STRING, caption, strlen(caption));
}

void html_document::set_base_url(const char *base_url)
{
	GB.FreeString(&THIS->base);
	THIS->base = GB.NewZeroString(base_url);
}

void html_document::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el)
{
}

void html_document::on_anchor_click(const char *url, const litehtml::element::ptr& el)
{
	GB.FreeString(&THIS->link);
	THIS->link = GB.NewZeroString(url);
}

void html_document::set_cursor(const char *cursor)
{
	if (!GB_FUNCTION_IS_VALID(&_func_set_cursor))
		return;
	GB.Push(1, GB_T_STRING, cursor, strlen(cursor));
	GB.Call(&_func_set_cursor, 1, TRUE);
}

void html_document::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
	static GB_FUNCTION func_capitalize = { 0 };
	static GB_FUNCTION func_upper = { 0 };
	static GB_FUNCTION func_lower = { 0 };
	
	GB_FUNCTION *func;
	GB_VALUE *ret;
	
	switch(tt)
	{
		case litehtml::text_transform_capitalize:
			if (!GB_FUNCTION_IS_VALID(&func_capitalize))
				GB.GetFunction(&func_capitalize, (void *)GB.FindClass("String"), "UCaseFirst", "s", "s");
			func = &func_capitalize;
			break;
			
    case litehtml::text_transform_uppercase:
			if (!GB_FUNCTION_IS_VALID(&func_upper))
				GB.GetFunction(&func_upper, (void *)GB.FindClass("String"), "Upper", "s", "s");
			func = &func_upper;
			break;
			
    case litehtml::text_transform_lowercase:
			if (!GB_FUNCTION_IS_VALID(&func_lower))
				GB.GetFunction(&func_lower, (void *)GB.FindClass("String"), "Lower", "s", "s");
			func = &func_lower;
			break;
	}
	
	GB.Push(1, GB_T_STRING, text.data(), text.length());
	ret = GB.Call(func, 1, FALSE);
	text.assign(ret->_string.value.addr + ret->_string.value.start, ret->_string.value.len);
}

void html_document::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
	GB_VALUE *ret;
	
	if (!GB_FUNCTION_IS_VALID(&_func_load_css))
		return;
	
	GB.Push(2, GB_T_STRING, url.data(), url.length(), GB_T_STRING, baseurl.data(), baseurl.length());
	ret = GB.Call(&_func_load_css, 2, FALSE);
	text.assign(ret->_string.value.addr + ret->_string.value.start, ret->_string.value.len);
}

void html_document::rounded_rectangle(const litehtml::position &pos, const litehtml::border_radiuses &radius, bool keep, bool back)
{
	float rtlx, rtlx2, rtrx, rtrx2, rblx, rblx2, rbrx, rbrx2;
	float rtly, rtly2, rtry, rtry2, rbly, rbly2, rbry, rbry2;
	float mrx, mry;
	
	float x = pos.x;
	float y = pos.y;
	float w = pos.width;
	float h = pos.height;
	
	if (w <= 0 || h <= 0)
		return;
	
	GET_CURRENT();

	if (radius.top_left_x == 0 && radius.top_left_y == 0 && radius.top_right_x == 0 && radius.top_right_y == 0
	    && radius.bottom_left_x == 0 && radius.bottom_left_y == 0 && radius.bottom_right_x == 0 && radius.bottom_right_y == 0)
	{
		if (back)
		{
			PAINT->MoveTo(CURRENT, x, y);
			PAINT->LineTo(CURRENT, x, y + h);
			PAINT->LineTo(CURRENT, x + w, y + h);
			PAINT->LineTo(CURRENT, x + w, y);
			PAINT->LineTo(CURRENT, x, y);
		}
		else
		{
			PAINT->MoveTo(CURRENT, x, y);
			PAINT->LineTo(CURRENT, x + w, y);
			PAINT->LineTo(CURRENT, x + w, y + h);
			PAINT->LineTo(CURRENT, x, y + h);
			PAINT->LineTo(CURRENT, x, y);
		}
		
		return;
	}
	
	if (false) //keep)
	{
		mrx = w / 2;
		mry = y / 2;
	}
	else
	{
		mrx = mry = Min(w, h) / 2;
	}
	
	rtlx = Min(radius.top_left_x, mrx);
	rtlx2 = rtlx * (1-0.55228475);
	
	if (keep)
	{
		rtly = Min(radius.top_left_y, mry);
		rtly2 = rtly * (1-0.55228475);
	}
	else
	{
		rtly = rtlx;
		rtly2 = rtlx2;
	}
	
	rtrx = Min(radius.top_right_x, mrx);
	rtrx2 = rtrx * (1-0.55228475);
	
	if (keep)
	{
		rtry = Min(radius.top_right_y, mry);
		rtry2 = rtry * (1-0.55228475);
	}
	else
	{
		rtry = rtrx;
		rtry2 = rtrx2;
	}
	
	rblx = Min(radius.bottom_left_x, mrx);
	rblx2 = rblx * (1-0.55228475);
	
	if (keep)
	{
		rbly = Min(radius.bottom_left_y, mry);
		rbly2 = rbly * (1-0.55228475);
	}
	else
	{
		rbly = rblx;
		rbly2 = rblx2;
	}
	
	rbrx = Min(radius.bottom_right_x, mrx);
	rbrx2 = rbrx * (1-0.55228475);

	if (keep)
	{
		rbry = Min(radius.bottom_right_y, mry);
		rbry2 = rbry * (1-0.55228475);
	}
	else
	{
		rbry = rbrx;
		rbry2 = rbrx2;
	}
	
	if (back)
	{
		PAINT->MoveTo(CURRENT, x + w - rtrx, y);
		PAINT->LineTo(CURRENT, x + rtlx, y);
		PAINT->CurveTo(CURRENT, x + rtlx2, y, x, y + rtly2, x, y + rtly);
		PAINT->LineTo(CURRENT, x, y + h - rbly);
		PAINT->CurveTo(CURRENT, x, y + h - rbly2, x + rblx2, y + h, x + rblx, y + h);
		PAINT->LineTo(CURRENT, x + w - rbrx, y + h);
		PAINT->CurveTo(CURRENT, x + w - rbrx2, y + h, x + w, y + h - rbry2, x + w, y + h - rbry);
		PAINT->LineTo(CURRENT, x + w, y + rtry);
		PAINT->CurveTo(CURRENT, x + w, y + rtry2, x + w - rtrx2, y, x + w - rtrx, y);
	}
	else
	{
		PAINT->MoveTo(CURRENT, x + rtlx, y);
		PAINT->LineTo(CURRENT, x + w - rtrx, y);
		PAINT->CurveTo(CURRENT, x + w - rtrx2, y, x + w, y + rtry2, x + w, y + rtry);
		PAINT->LineTo(CURRENT, x + w, y + h - rbry);
		PAINT->CurveTo(CURRENT, x + w, y + h - rbry2, x + w - rbrx2, y + h, x + w - rbrx, y + h);
		PAINT->LineTo(CURRENT, x + rblx, y + h);
		PAINT->CurveTo(CURRENT, x + rblx2, y + h, x, y + h - rbly2, x, y + h - rbly);
		PAINT->LineTo(CURRENT, x, y + rtly);
		PAINT->CurveTo(CURRENT, x, y + rtly2, x + rtlx2, y, x + rtlx, y);
	}
}

void html_document::set_clip( const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y )
{
	litehtml::position clip_pos = pos;
	litehtml::position client_pos;
	get_client_rect(client_pos);
	if(!valid_x)
	{
		clip_pos.x = client_pos.x;
		clip_pos.width = client_pos.width;
	}
	if(!valid_y)
	{
		clip_pos.y = client_pos.y;
		clip_pos.height = client_pos.height;
	}
	m_clips.emplace_back(clip_pos, bdr_radius);
}

void html_document::del_clip()
{
	if(!m_clips.empty())
		m_clips.pop_back();
}

void html_document::begin_clip()
{
	GET_CURRENT();
	
	PAINT->Save(CURRENT);
	
	for(const auto& clip_box : m_clips)
	{
		rounded_rectangle(clip_box.box, clip_box.radius);
		PAINT->Clip(CURRENT, FALSE);
	}
}

void html_document::end_clip()
{
	GET_CURRENT();
	
	PAINT->Restore(CURRENT);
}


void html_document::get_client_rect(litehtml::position& client) const
{
	client.x = 0;
	client.y = 0;
	client.width = m_client_w;
	client.height = m_client_h;
}

std::shared_ptr<litehtml::element> html_document::create_element(const char *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc)
{
	return NULL;
}

void html_document::get_media_features(litehtml::media_features& media) const
{
	media.type = litehtml::media_type_screen;
	media.width = m_client_w;
	media.height = m_client_h;
	media.device_width = THIS->screen_width;
	media.device_height = THIS->screen_height;
	media.color = 8;
	media.monochrome = 0;
	media.color_index = 256;
	media.resolution = THIS->resolution ? THIS->resolution : 96;
}

void html_document::get_language(litehtml::string& language, litehtml::string & culture) const
{
	language = "en";
	culture = "";
}

litehtml::string	html_document::resolve_color(const litehtml::string& color) const
{
	return litehtml::string();
}

void html_document::on_mouse(int event, int x, int y)
{
	litehtml::position::vector redraw_boxes;
	bool ret;
	
	if (!m_html)
		return;
	
	switch (event)
	{
		case MOUSE_DOWN:
			ret = m_html->on_lbutton_down(x, y, x, y, redraw_boxes);
			break;
		
		case MOUSE_UP:
			GB.FreeString(&THIS->link);
			ret = m_html->on_lbutton_up(x, y, x, y, redraw_boxes);
			break;
		
		case MOUSE_MOVE:
			ret = m_html->on_mouse_over(x, y, x, y, redraw_boxes);
			break;
			
		case MOUSE_LEAVE:
			ret = m_html->on_mouse_leave(redraw_boxes);
			break;
			
		default:
			ret = false;
	}
	
	if (ret)
	{
		GB_FUNCTION func;
		if (GB.GetFunction(&func, THIS, "_Refresh", "iiii", NULL))
			return;
	
		for(auto& pos : redraw_boxes)
		{
			GB.Push(4, GB_T_INTEGER, pos.x, GB_T_INTEGER, pos.y, GB_T_INTEGER, pos.width, GB_T_INTEGER, pos.height);
			GB.Call(&func, 4, TRUE);
		}
	}
	
	if (event == MOUSE_UP && THIS->link)
		GB.Raise(THIS, EVENT_Link, 1, GB_T_STRING, THIS->link, GB.StringLength(THIS->link));
}

void html_document::on_media_change()
{
	if (m_html)
		m_html->media_changed();
}

int html_document::find_anchor(const litehtml::string& anchor)
{
	litehtml::element::ptr el;
	litehtml::string selector;
	
	if(!m_html || anchor.empty())
		return -1;
	
	selector.assign("#");
	selector.append(anchor);
	el = m_html->root()->select_one(selector);
	if (!el)
	{
		selector.assign("[name=");
		selector.append(anchor);
		selector.append("]");
		el = m_html->root()->select_one(selector);
	}
	if (!el)
		return -1;
	
	return el->get_placement().y;
}


//-------------------------------------------------------------------------

static void reload_document(void *_object)
{
	delete THIS->doc;
	THIS->doc = NULL;
	
	if (THIS->html && *THIS->html)
	{	
		THIS->doc = new html_document(THIS);

		if (THIS->doc->load(THIS->html, THIS->css))
			GB.Error("Unable to parse HTML");
	}
}

//-------------------------------------------------------------------------

BEGIN_METHOD_VOID(HtmlDocument_new)

	// Check needed interface

END_METHOD

BEGIN_METHOD_VOID(HtmlDocument_free)

	GB.FreeString(&THIS->link);
	GB.FreeString(&THIS->base);
	GB.FreeString(&THIS->html);
	GB.FreeString(&THIS->css);
	GB.FreeString(&THIS->monospace_font_name);
	GB.FreeString(&THIS->default_font_name);
	delete THIS->doc;
	
END_METHOD

BEGIN_PROPERTY(HtmlDocument_Html)

	if (READ_PROPERTY)
		GB.ReturnString(THIS->html);
	else
	{
		GB.StoreString(PROP(GB_STRING), &THIS->html);
		reload_document(THIS);
	}

END_PROPERTY

BEGIN_METHOD(HtmlDocument_Render, GB_INTEGER w; GB_INTEGER h)

	if (THIS->doc)
		THIS->doc->render(VARG(w), VARG(h));

END_METHOD

BEGIN_METHOD(HtmlDocument_Draw, GB_INTEGER x; GB_INTEGER y; GB_INTEGER w; GB_INTEGER h)

	if (THIS->doc)
		THIS->doc->draw(VARG(x), VARG(y), VARG(w), VARG(h));

END_METHOD

BEGIN_PROPERTY(HtmlDocument_Width)

	if (THIS->doc)
		GB.ReturnInteger(THIS->doc->getWidth());
	else
		GB.ReturnInteger(0);

END_PROPERTY

BEGIN_PROPERTY(HtmlDocument_Height)

	if (THIS->doc)
		GB.ReturnInteger(THIS->doc->getHeight());
	else
		GB.ReturnInteger(0);

END_PROPERTY

BEGIN_METHOD(HtmlDocument_LoadCss, GB_STRING css)

	GB.StoreString(PROP(GB_STRING), &THIS->css);
	reload_document(THIS);

END_METHOD

BEGIN_METHOD_VOID(HtmlDocument_Reload)

	reload_document(THIS);

END_METHOD

BEGIN_METHOD(HtmlDocument_SetDefaultFont, GB_STRING name; GB_INTEGER size)

	THIS->default_font_size = VARG(size);
	GB.StoreString(ARG(name), &THIS->default_font_name);
	
END_METHOD

BEGIN_METHOD(HtmlDocument_SetMonospaceFont, GB_STRING name)

	GB.StoreString(ARG(name), &THIS->monospace_font_name);

END_METHOD

BEGIN_METHOD(HtmlDocument_OnMouseDown, GB_INTEGER x; GB_INTEGER y)

	if (THIS->doc)
		THIS->doc->on_mouse(html_document::MOUSE_DOWN, VARG(x), VARG(y));
	
END_METHOD

BEGIN_METHOD(HtmlDocument_OnMouseUp, GB_INTEGER x; GB_INTEGER y)

	if (THIS->doc)
		THIS->doc->on_mouse(html_document::MOUSE_UP, VARG(x), VARG(y));
	
END_METHOD

BEGIN_METHOD(HtmlDocument_OnMouseMove, GB_INTEGER x; GB_INTEGER y)

	if (THIS->doc)
		THIS->doc->on_mouse(html_document::MOUSE_MOVE, VARG(x), VARG(y));
	
END_METHOD

BEGIN_METHOD_VOID(HtmlDocument_OnLeave)

	if (THIS->doc)
		THIS->doc->on_mouse(html_document::MOUSE_LEAVE);
	
END_METHOD

BEGIN_METHOD(HtmlDocument_SetMedia, GB_INTEGER screen_width; GB_INTEGER screen_height; GB_INTEGER resolution)

	THIS->screen_width = VARG(screen_width);
	THIS->screen_height = VARG(screen_height);
	THIS->resolution = VARG(resolution);
	
	if (THIS->doc)
		THIS->doc->on_media_change();

END_METHOD

BEGIN_PROPERTY(HtmlDocument_Link)

	GB.ReturnString(THIS->link);

END_PROPERTY

BEGIN_PROPERTY(HtmlDocument_Base)

	GB.ReturnString(THIS->base);

END_PROPERTY

BEGIN_METHOD(HtmlDocument_FindAnchor, GB_STRING anchor)

	if (THIS->doc)
	{
		litehtml::string anchor;
		anchor.assign(STRING(anchor), LENGTH(anchor));
		GB.ReturnInteger(THIS->doc->find_anchor(anchor));
	}
	else
		GB.ReturnInteger(-1);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC HtmlDocumentDesc[] = 
{
	GB_DECLARE("_HtmlDocument", sizeof(CHTMLDOCUMENT)),
	
	GB_METHOD("_new", NULL, HtmlDocument_new, NULL),
	GB_METHOD("_free", NULL, HtmlDocument_free, NULL),
	
	GB_METHOD("Render", NULL, HtmlDocument_Render, "(Width)i(Height)i"),
	GB_METHOD("Draw", NULL, HtmlDocument_Draw, "(X)i(Y)i(Width)i(Height)i"),
	
	GB_PROPERTY("Html", "s", HtmlDocument_Html),
	GB_METHOD("LoadCss", NULL, HtmlDocument_LoadCss, "(Css)s"),
	GB_METHOD("SetDefaultFont", NULL, HtmlDocument_SetDefaultFont, "(Name)s(Size)i"),
	GB_METHOD("SetMonospaceFont", NULL, HtmlDocument_SetMonospaceFont, "(Name)s"),
	GB_METHOD("SetMedia", NULL, HtmlDocument_SetMedia, "(ScreenWidth)i(ScreenHeight)i(Resolution)i"),
	GB_METHOD("Reload", NULL, HtmlDocument_Reload, NULL),
	GB_METHOD("FindAnchor", "i", HtmlDocument_FindAnchor, "(Anchor)s"),
	
	GB_METHOD("OnMouseDown", NULL, HtmlDocument_OnMouseDown, "ii"),
	GB_METHOD("OnMouseUp", NULL, HtmlDocument_OnMouseUp, "ii"),
	GB_METHOD("OnMouseMove", NULL, HtmlDocument_OnMouseMove, "ii"),
	GB_METHOD("OnLeave", NULL, HtmlDocument_OnLeave, NULL),
	
	GB_PROPERTY_READ("Width", "i", HtmlDocument_Width),
	GB_PROPERTY_READ("W", "i", HtmlDocument_Width),
	GB_PROPERTY_READ("Height", "i", HtmlDocument_Height),
	GB_PROPERTY_READ("H", "i", HtmlDocument_Height),
	GB_PROPERTY_READ("Link", "s", HtmlDocument_Link),
	GB_PROPERTY_READ("Base", "s", HtmlDocument_Base),
						 
	GB_EVENT("Link", NULL, "s", &EVENT_Link),
	GB_EVENT("Title", NULL, "s", &EVENT_Title),
	
	GB_END_DECLARE
};
