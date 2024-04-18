/***************************************************************************

	CColor.cpp

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

#define __CCOLOR_CPP

#include <qapplication.h>
#include <qcolor.h>
#include <qpalette.h>

#include "gb.form.const.h"
#include "gambas.h"

#include "CWidget.h"
#include "CColor.h"

GB_COLOR _link_foreground = GB_COLOR_DEFAULT;
GB_COLOR _visited_foreground = GB_COLOR_DEFAULT;
GB_COLOR _tooltip_foreground = GB_COLOR_DEFAULT;
GB_COLOR _tooltip_background = GB_COLOR_DEFAULT;

static bool _palette_init = false;
static GB_COLOR _palette_previous[COLOR_COUNT] = { 0 };
static GB_COLOR _palette[COLOR_COUNT] = { 0 };


static int get_luminance(QColor col)
{
	return (int)(0.299 * col.red() + 0.587 * col.green() + 0.114 * col.blue());
}

static uint get_light_foreground()
{
	return IMAGE.MergeColor(qApp->palette().color(QPalette::Window).rgb() & 0xFFFFFF, qApp->palette().color(QPalette::WindowText).rgb() & 0xFFFFFF, 0.3);
}

static uint get_light_background()
{
	return IMAGE.MergeColor(qApp->palette().color(QPalette::Base).rgb() & 0xFFFFFF, qApp->palette().color(QPalette::Highlight).rgb() & 0xFFFFFF, 0.5);
}

static uint get_tooltip_foreground()
{
	QColor bg = qApp->palette().color(QPalette::ToolTipBase);
	QColor fg = qApp->palette().color(QPalette::ToolTipText);
	int lbg = get_luminance(bg);
	int lfg = get_luminance(fg);

	if (abs(lbg - lfg) <= 64)
		fg.setHsv(fg.hue(), fg.saturation(), 255 - fg.value());

	return fg.rgb() & 0xFFFFFF;
}

QColor CCOLOR_light_foreground()
{
	return TO_QCOLOR(get_light_foreground());
}

QColor CCOLOR_make(GB_COLOR color)
{
	int b = color & 0xFF;
	int g = (color >> 8) & 0xFF;
	int r = (color >> 16) & 0xFF;
	int a = (color >> 24) ^ 0xFF;
	
	return QColor(r, g, b, a);
}

static GB_COLOR get_role_color(QPalette::ColorRole role)
{
	return QApplication::palette().color(role).rgb() & 0xFFFFFF;
}

static GB_COLOR get_color(int color)
{
	switch(color)
	{
		case COLOR_BACKGROUND: return get_role_color(QPalette::Window);
		case COLOR_FOREGROUND: return get_role_color(QPalette::WindowText);
		case COLOR_SELECTED_BACKGROUND: return get_role_color(QPalette::Highlight);
		case COLOR_SELECTED_FOREGROUND: return get_role_color(QPalette::HighlightedText);
		case COLOR_LIGHT_BACKGROUND: return get_light_background();
		case COLOR_LIGHT_FOREGROUND: return get_light_foreground();
		case COLOR_TEXT_BACKGROUND: return get_role_color(QPalette::Base);
		case COLOR_TEXT_FOREGROUND: return get_role_color(QPalette::Text);
		case COLOR_BUTTON_BACKGROUND: return get_role_color(QPalette::Button);
		case COLOR_BUTTON_FOREGROUND: return get_role_color(QPalette::ButtonText);
		case COLOR_TOOLTIP_BACKGROUND: return get_color(QPalette::ToolTipBase);
		case COLOR_TOOLTIP_FOREGROUND: return get_tooltip_foreground();
		case COLOR_LINK_FOREGROUND: return get_color(QPalette::Link);
		case COLOR_VISITED_FOREGROUND: return get_color(QPalette::LinkVisited);
		default: return get_role_color(QPalette::Window);
	}
}

static void return_color(int color)
{
	GB.ReturnInteger(_palette[color]);
}

static void handle_color(void *_param, int color, GB_COLOR *var)
{
	if (READ_PROPERTY)
		GB.ReturnInteger(*var == GB_COLOR_DEFAULT ? _palette[color] : *var);
	else
		*var = VPROP(GB_INTEGER);
}

static void update_color(CWIDGET *widget)
{
	int i;
	bool update = false;
	GB_COLOR bg = CWIDGET_get_background(widget);
	GB_COLOR fg = CWIDGET_get_foreground(widget);

	if (bg != GB_COLOR_DEFAULT)
	{
		for (i = 0; i < COLOR_COUNT; i++)
		{
			if (bg == _palette_previous[i])
			{
				bg = _palette[i];
				//fprintf(stderr, "bg == %d !\n", i);
				update = true;
				break;
			}
		}
	}

	if (fg != GB_COLOR_DEFAULT)
	{
		for (i = 0; i < COLOR_COUNT; i++)
		{
			if (fg == _palette_previous[i])
			{
				fg = _palette[i];
				//fprintf(stderr, "fg == %d !\n", i);
				update = true;
				break;
			}
		}
	}

	if (update)
	{
		//fprintf(stderr, "update_color: '%s'\n", widget->name);
		CWIDGET_set_color(widget, bg, fg);
	}
}

bool COLOR_update_palette()
{
	bool update = false;
	int i, j;
	uchar r, g, b, a;

	for (i = 0; i < COLOR_COUNT; i++)
	{
		_palette_previous[i] = _palette[i];
		_palette[i] = get_color(i);

		for (j = 0; j < i; j++)
		{
			if (_palette[i] == _palette[j])
			{
				GB_COLOR_SPLIT(_palette[i], r, g, b, a);
				if (g > 127)
					g--;
				else
					g++;
				_palette[i] = GB_COLOR_MAKE(r, g, b, a);
				j = 0;
			}
		}

		if (_palette_init && _palette_previous[i] != _palette[i])
		{
			//fprintf(stderr, "[%d] = %08X -> %08X\n", i, _palette_previous[i], _palette[i]);
			update = true;
		}
	}

	if (update)
	{
		//fprintf(stderr, "update palette!\n");
		CWidget::each(update_color);
	}

	_palette_init = true;
	return update;
}

//-------------------------------------------------------------------------

BEGIN_PROPERTY(Color_Background)

	return_color(COLOR_BACKGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_Foreground)

	return_color(COLOR_FOREGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_TextBackground)

	return_color(COLOR_TEXT_BACKGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_TextForeground)

	return_color(COLOR_TEXT_FOREGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_SelectedBackground)

	return_color(COLOR_SELECTED_BACKGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_SelectedForeground)

	return_color(COLOR_SELECTED_FOREGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_ButtonBackground)

	return_color(COLOR_BUTTON_BACKGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_ButtonForeground)

	return_color(COLOR_BUTTON_FOREGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_LightBackground)

	return_color(COLOR_LIGHT_BACKGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_LightForeground)

	return_color(COLOR_LIGHT_FOREGROUND);

END_PROPERTY

BEGIN_PROPERTY(Color_TooltipBackground)

	handle_color(_param, COLOR_TOOLTIP_BACKGROUND, &_tooltip_background);

END_PROPERTY

BEGIN_PROPERTY(Color_TooltipForeground)

	handle_color(_param, COLOR_TOOLTIP_FOREGROUND, &_tooltip_foreground);

END_PROPERTY

BEGIN_PROPERTY(Color_LinkForeground)

	handle_color(_param, COLOR_LINK_FOREGROUND, &_link_foreground);

END_PROPERTY

BEGIN_PROPERTY(Color_VisitedForeground)

	handle_color(_param, COLOR_VISITED_FOREGROUND, &_visited_foreground);

END_PROPERTY

BEGIN_METHOD(Color_Change, GB_INTEGER color)

	GB_COLOR color = VARG(color);
	int i;

	if (color != GB_COLOR_DEFAULT)
	{
		for (i = 0; i < COLOR_COUNT; i++)
		{
			if (color == _palette_previous[i])
			{
				color = _palette[i];
				break;
			}
		}
	}

	GB.ReturnInteger(color);

END_METHOD

GB_DESC CColorDesc[] =
{
	GB_DECLARE_STATIC("Color"),

	GB_STATIC_PROPERTY_READ("Background", "i", Color_Background),
	GB_STATIC_PROPERTY_READ("SelectedBackground", "i", Color_SelectedBackground),
	GB_STATIC_PROPERTY_READ("LightBackground", "i", Color_LightBackground),
	GB_STATIC_PROPERTY_READ("TextBackground", "i", Color_TextBackground),
	GB_STATIC_PROPERTY_READ("ButtonBackground", "i", Color_ButtonBackground),

	GB_STATIC_PROPERTY_READ("Foreground", "i", Color_Foreground),
	GB_STATIC_PROPERTY_READ("LightForeground", "i", Color_LightForeground),
	GB_STATIC_PROPERTY_READ("SelectedForeground", "i", Color_SelectedForeground),
	GB_STATIC_PROPERTY_READ("TextForeground", "i", Color_TextForeground),
	GB_STATIC_PROPERTY_READ("ButtonForeground", "i", Color_ButtonForeground),
	
	GB_STATIC_PROPERTY("TooltipBackground", "i", Color_TooltipBackground),
	GB_STATIC_PROPERTY("TooltipForeground", "i", Color_TooltipForeground),
	
	GB_STATIC_PROPERTY("LinkForeground", "i", Color_LinkForeground),
	GB_STATIC_PROPERTY("VisitedForeground", "i", Color_VisitedForeground),

	GB_STATIC_METHOD("Change", "i", Color_Change, "(Color)i"),

	GB_END_DECLARE
};


