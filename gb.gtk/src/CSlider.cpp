/***************************************************************************

  CSlider.cpp

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

#define __CSLIDER_CPP

#include "main.h"
#include "gambas.h"
#include "widgets.h"

#include "CSlider.h"
#include "CContainer.h"
#include "CWidget.h"

DECLARE_EVENT(EVENT_Change);

void CB_slider_change(gSlider *sender)
{
  CB_GET_OBJECT(sender);
	GB.Raise(THIS, EVENT_Change, 0);
}

//---------------------------------------------------------------------------

BEGIN_METHOD(Slider_new, GB_OBJECT parent)

	InitControl(new gSlider(CONTAINER(VARG(parent))),(CWIDGET*)THIS);
	
END_METHOD

BEGIN_METHOD(ScrollBar_new, GB_OBJECT parent)

	InitControl(new gScrollBar(CONTAINER(VARG(parent))),(CWIDGET*)THIS);
	
END_METHOD

BEGIN_PROPERTY(Slider_Tracking)

	if (READ_PROPERTY) { GB.ReturnBoolean(SLIDER->tracking()); return; }
	SLIDER->setTracking(VPROP(GB_BOOLEAN));

END_PROPERTY

BEGIN_PROPERTY(Slider_Value)

	if (READ_PROPERTY) { GB.ReturnInteger(SLIDER->value()); return; }
	SLIDER->setValue(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(Slider_MinValue)

	if (READ_PROPERTY) { GB.ReturnInteger(SLIDER->min()); return; }
	SLIDER->setMin(VPROP(GB_INTEGER));	

END_PROPERTY

BEGIN_PROPERTY(Slider_MaxValue)

	if (READ_PROPERTY) { GB.ReturnInteger(SLIDER->max()); return; }
	SLIDER->setMax(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(Slider_LineStep)

	if (READ_PROPERTY) { GB.ReturnInteger(SLIDER->step()); return; }
	SLIDER->setStep(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(Slider_PageStep)

	if (READ_PROPERTY) { GB.ReturnInteger(SLIDER->pageStep()); return; }
	SLIDER->setPageStep(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(CSLIDER_mark)

	if (READ_PROPERTY){ GB.ReturnBoolean(SLIDER->mark()); return; }
	SLIDER->setMark(VPROP(GB_BOOLEAN));

END_PROPERTY

BEGIN_PROPERTY(Slider_DefaultSize)

	GB.ReturnInteger(SLIDER->getDefaultSize());

END_PROPERTY

BEGIN_PROPERTY(Slider_Orientation)

	if (READ_PROPERTY)
		GB.ReturnInteger(SLIDER->orientation());
	else
		SLIDER->setOrientation(VPROP(GB_INTEGER));

END_PROPERTY

//-------------------------------------------------------------------------

GB_DESC SliderDesc[] =
{
  GB_DECLARE("Slider", sizeof(CSLIDER)), GB_INHERITS("Control"),

  GB_METHOD("_new", 0, Slider_new, "(Parent)Container;"),

  GB_PROPERTY("Tracking", "b", Slider_Tracking),
  GB_PROPERTY("Value", "i", Slider_Value),
  GB_PROPERTY("Mark", "b", CSLIDER_mark),
  GB_PROPERTY("MinValue", "i", Slider_MinValue),
  GB_PROPERTY("MaxValue", "i", Slider_MaxValue),
  GB_PROPERTY("Step", "i", Slider_LineStep),
  GB_PROPERTY("PageStep", "i", Slider_PageStep),
  GB_PROPERTY("Orientation", "i", Slider_Orientation),

  GB_EVENT("Change", 0, 0, &EVENT_Change),

  SLIDER_DESCRIPTION,

  GB_END_DECLARE
};

GB_DESC ScrollBarDesc[] =
{
  GB_DECLARE("ScrollBar", sizeof(CSCROLLBAR)), GB_INHERITS("Control"),

  GB_METHOD("_new", 0, ScrollBar_new, "(Parent)Container;"),

  GB_PROPERTY_READ("DefaultSize", "i", Slider_DefaultSize),

  GB_PROPERTY("Tracking", "b", Slider_Tracking),
  GB_PROPERTY("Value", "i", Slider_Value),
  GB_PROPERTY("MinValue", "i", Slider_MinValue),
  GB_PROPERTY("MaxValue", "i", Slider_MaxValue),
  GB_PROPERTY("Step", "i", Slider_LineStep),
  GB_PROPERTY("PageStep", "i", Slider_PageStep),
  GB_PROPERTY("Orientation", "i", Slider_Orientation),

  GB_EVENT("Change", 0, 0, &EVENT_Change),

  SCROLLBAR_DESCRIPTION,

	GB_CONSTANT("Auto", "i", ORIENTATION_AUTO),
	GB_CONSTANT("Horizontal", "i", ORIENTATION_HORIZONTAL),
	GB_CONSTANT("Vertical", "i", ORIENTATION_VERTICAL),

  GB_END_DECLARE
};


