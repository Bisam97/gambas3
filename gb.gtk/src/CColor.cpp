/***************************************************************************

  CColor.cpp

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

#define __CCOLOR_CPP

#include <math.h>

#include "CColor.h"
#include "gdesktop.h"
#include "gcolor.h"

GB_COLOR _link_foreground = GB_COLOR_DEFAULT;
GB_COLOR _visited_foreground = GB_COLOR_DEFAULT;
GB_COLOR _tooltip_foreground = GB_COLOR_DEFAULT;
GB_COLOR _tooltip_background = GB_COLOR_DEFAULT;


static void handle_color(void *_param, GB_COLOR color, GB_COLOR *var)
{
	if (READ_PROPERTY)
		GB.ReturnInteger(*var == GB_COLOR_DEFAULT ? color : *var);
	else
		*var = VPROP(GB_INTEGER);
}

BEGIN_PROPERTY(Color_Background)

	GB.ReturnInteger(gDesktop::getColor(COLOR_BACKGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_Foreground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_FOREGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_TextBackground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_TEXT_BACKGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_TextForeground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_TEXT_FOREGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_SelectedBackground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_SELECTED_BACKGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_SelectedForeground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_SELECTED_FOREGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_ButtonBackground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_BUTTON_BACKGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_ButtonForeground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_BUTTON_FOREGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_LightBackground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_LIGHT_BACKGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_LightForeground)

	GB.ReturnInteger(gDesktop::getColor(COLOR_LIGHT_FOREGROUND));

END_PROPERTY

BEGIN_PROPERTY(Color_TooltipBackground)

	handle_color(_param, gDesktop::getColor(COLOR_TOOLTIP_BACKGROUND), &_tooltip_background);

END_PROPERTY

BEGIN_PROPERTY(Color_TooltipForeground)

	handle_color(_param, gDesktop::getColor(COLOR_TOOLTIP_FOREGROUND), &_tooltip_foreground);

END_PROPERTY

BEGIN_PROPERTY(Color_LinkForeground)

	handle_color(_param, gDesktop::getColor(COLOR_LINK_FOREGROUND), &_link_foreground);

END_PROPERTY

BEGIN_PROPERTY(Color_VisitedForeground)

	handle_color(_param, gDesktop::getColor(COLOR_VISITED_FOREGROUND), &_visited_foreground);

END_PROPERTY

BEGIN_METHOD(Color_Change, GB_INTEGER color)

	GB.ReturnInteger(gDesktop::changeColor(VARG(color)));

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
	GB_STATIC_PROPERTY_READ("SelectedForeground", "i", Color_SelectedForeground),
	GB_STATIC_PROPERTY_READ("LightForeground", "i", Color_LightForeground),
	GB_STATIC_PROPERTY_READ("TextForeground", "i", Color_TextForeground),
	GB_STATIC_PROPERTY_READ("ButtonForeground", "i", Color_ButtonForeground),

	GB_STATIC_PROPERTY("TooltipBackground", "i", Color_TooltipBackground),
	GB_STATIC_PROPERTY("TooltipForeground", "i", Color_TooltipForeground),
	GB_STATIC_PROPERTY("LinkForeground", "i", Color_LinkForeground),
	GB_STATIC_PROPERTY("VisitedForeground", "i", Color_VisitedForeground),

	GB_STATIC_METHOD("Change", "i", Color_Change, "(Color)i"),

	GB_END_DECLARE
};


