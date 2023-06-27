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

#include "gambas.h"

#include "CWidget.h"
#include "CColor.h"

GB_COLOR _link_foreground = COLOR_DEFAULT;
GB_COLOR _visited_foreground = COLOR_DEFAULT;
GB_COLOR _tooltip_foreground = COLOR_DEFAULT;
GB_COLOR _tooltip_background = COLOR_DEFAULT;

static uint get_light_foreground()
{
	return IMAGE.MergeColor(qApp->palette().color(QPalette::Window).rgb() & 0xFFFFFF, qApp->palette().color(QPalette::WindowText).rgb() & 0xFFFFFF, 0.3);
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

static GB_COLOR get_color(QPalette::ColorRole role)
{
	return QApplication::palette().color(role).rgb() & 0xFFFFFF;
}

static void return_color(QPalette::ColorRole role)
{
	GB.ReturnInteger(get_color(role));
}

static void handle_color(void *_param, GB_COLOR color, GB_COLOR *var)
{
	if (READ_PROPERTY)
		GB.ReturnInteger(*var == COLOR_DEFAULT ? color : *var);
	else
		*var = VPROP(GB_INTEGER);
}

BEGIN_PROPERTY(Color_Background)

	return_color(QPalette::Window);

END_PROPERTY

BEGIN_PROPERTY(Color_Foreground)

	return_color(QPalette::WindowText);

END_PROPERTY

BEGIN_PROPERTY(Color_TextBackground)

	return_color(QPalette::Base);

END_PROPERTY

BEGIN_PROPERTY(Color_TextForeground)

	return_color(QPalette::Text);

END_PROPERTY

BEGIN_PROPERTY(Color_SelectedBackground)

	return_color(QPalette::Highlight);

END_PROPERTY

BEGIN_PROPERTY(Color_SelectedForeground)

	return_color(QPalette::HighlightedText);

END_PROPERTY

BEGIN_PROPERTY(Color_ButtonBackground)

	return_color(QPalette::Button);

END_PROPERTY

BEGIN_PROPERTY(Color_ButtonForeground)

	return_color(QPalette::ButtonText);

END_PROPERTY

BEGIN_PROPERTY(Color_LightBackground)

	uint col = IMAGE.MergeColor(qApp->palette().color(QPalette::Base).rgb() & 0xFFFFFF, qApp->palette().color(QPalette::Highlight).rgb() & 0xFFFFFF, 0.5);
	GB.ReturnInteger(col);

END_PROPERTY

BEGIN_PROPERTY(Color_LightForeground)

	GB.ReturnInteger(get_light_foreground());

END_PROPERTY

BEGIN_PROPERTY(Color_TooltipBackground)

	handle_color(_param, get_color(QPalette::ToolTipBase), &_tooltip_background);

END_PROPERTY

static int get_luminance(QColor col)
{
	return (int)(0.299 * col.red() + 0.587 * col.green() + 0.114 * col.blue());
}

BEGIN_PROPERTY(Color_TooltipForeground)

	QColor bg = qApp->palette().color(QPalette::ToolTipBase);
	QColor fg = qApp->palette().color(QPalette::ToolTipText);
	int lbg = get_luminance(bg);
	int lfg = get_luminance(fg);
	GB_COLOR col;
	
	if (abs(lbg - lfg) <= 64)
		fg.setHsv(fg.hue(), fg.saturation(), 255 - fg.value());
		
	col = fg.rgb() & 0xFFFFFF;
	
	handle_color(_param, col, &_tooltip_foreground);

END_PROPERTY

BEGIN_PROPERTY(Color_LinkForeground)

	handle_color(_param, get_color(QPalette::Link), &_link_foreground);

END_PROPERTY

BEGIN_PROPERTY(Color_VisitedForeground)

	handle_color(_param, get_color(QPalette::LinkVisited), &_visited_foreground);

END_PROPERTY

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

	GB_END_DECLARE
};


