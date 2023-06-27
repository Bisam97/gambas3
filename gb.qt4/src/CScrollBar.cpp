/***************************************************************************

	CScrollBar.cpp

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

#define __CSCROLLBAR_CPP

#include "main.h"
#include "gambas.h"

#include <QStyle>

#include "gb.form.const.h"
#include "CWidget.h"
#include "CScrollBar.h"

DECLARE_EVENT(EVENT_Change);

/***************************************************************************

	class MyScrollBar

***************************************************************************/


MyScrollBar::MyScrollBar(QWidget *parent)
: QScrollBar(parent)
{
}

void MyScrollBar::updateOrientation()
{
	CSCROLLBAR *_object = (CSCROLLBAR *)CWidget::get(this);
	
	if (!THIS->widget.flag.orientation)
	{
		if (width() >= height())
			setOrientation(Qt::Horizontal);
		else
			setOrientation(Qt::Vertical);
	}
}

void MyScrollBar::resizeEvent(QResizeEvent *e)
{
	CSCROLLBAR *_object = (CSCROLLBAR *)CWidget::get(this);

	QScrollBar::resizeEvent(e);

	if (!THIS->widget.flag.orientation)
		updateOrientation();
}


/***************************************************************************

	ScrollBar

***************************************************************************/

BEGIN_METHOD(ScrollBar_new, GB_OBJECT parent)

	MyScrollBar *wid = new MyScrollBar(QCONTAINER(VARG(parent)));

	THIS->widget.flag.wheel = true;

	QObject::connect(wid, SIGNAL(valueChanged(int)), &CScrollBar::manager,
	SLOT(event_change()));

	wid->setTracking(true);
	wid->setMinimum(0);
	wid->setMaximum(100);
	wid->setSingleStep(1);
	wid->setPageStep(10);

	CWIDGET_new(wid, _object);

END_METHOD


BEGIN_PROPERTY(ScrollBar_Tracking)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->hasTracking());
	else
		WIDGET->setTracking(VPROP(GB_BOOLEAN));

END_PROPERTY

/*
BEGIN_PROPERTY(CSCROLLBAR_orientation)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->orientation());
	else
	{
		switch PROPERTY(GB_INTEGER)
		{
			case Qt::Vertical: WIDGET->setOrientation(Qt::Vertical);break;
			case Qt::Horizontal: WIDGET->setOrientation(Qt::Horizontal);break;
			default: WIDGET->setOrientation(Qt::Vertical);
		}
	}

END_PROPERTY
*/

BEGIN_PROPERTY(ScrollBar_Value)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->value());
	else
		WIDGET->setValue(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(ScrollBar_MinVal)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->minimum());
	else
		WIDGET->setMinimum(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(ScrollBar_MaxVal)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->maximum());
	else
		WIDGET->setMaximum(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(ScrollBar_LineStep)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->singleStep());
	else
		WIDGET->setSingleStep(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(ScrollBar_PageStep)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->pageStep());
	else
		WIDGET->setPageStep(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(ScrollBar_DefaultSize)

	GB.ReturnInteger(WIDGET->style()->pixelMetric(QStyle::PM_ScrollBarExtent));

END_PROPERTY

BEGIN_PROPERTY(ScrollBar_Orientation)

	if (READ_PROPERTY)
	{
		if (!THIS->widget.flag.orientation)
			GB.ReturnInteger(ORIENTATION_AUTO);
		else if (WIDGET->orientation() == Qt::Vertical)
			GB.ReturnInteger(ORIENTATION_VERTICAL);
		else
			GB.ReturnInteger(ORIENTATION_HORIZONTAL);
	}
	else
	{
		switch(VPROP(GB_INTEGER))
		{
			case ORIENTATION_HORIZONTAL:
				WIDGET->setOrientation(Qt::Horizontal);
				THIS->widget.flag.orientation = true;
				break;
			case ORIENTATION_VERTICAL:
				WIDGET->setOrientation(Qt::Vertical);
				THIS->widget.flag.orientation = true;
				break;
			default:
				THIS->widget.flag.orientation = false;
				WIDGET->updateOrientation();
		}
	}

END_PROPERTY

//-------------------------------------------------------------------------

CScrollBar CScrollBar::manager;

void CScrollBar::event_change(void)
{
	GET_SENDER();
	GB.Raise(THIS, EVENT_Change, 0);
}

//-------------------------------------------------------------------------

GB_DESC ScrollBarDesc[] =
{
	GB_DECLARE("ScrollBar", sizeof(CSCROLLBAR)), GB_INHERITS("Control"),

	GB_METHOD("_new", NULL, ScrollBar_new, "(Parent)Container;"),

	GB_PROPERTY_READ("DefaultSize", "i", ScrollBar_DefaultSize),

	GB_PROPERTY("Tracking", "b", ScrollBar_Tracking),
	GB_PROPERTY("Value", "i", ScrollBar_Value),
	GB_PROPERTY("MinValue", "i", ScrollBar_MinVal),
	GB_PROPERTY("MaxValue", "i", ScrollBar_MaxVal),
	GB_PROPERTY("Step", "i", ScrollBar_LineStep),
	GB_PROPERTY("PageStep", "i", ScrollBar_PageStep),
	GB_PROPERTY("Orientation", "i", ScrollBar_Orientation),

	GB_EVENT("Change", NULL, NULL, &EVENT_Change),

	SCROLLBAR_DESCRIPTION,
	
	GB_CONSTANT("Auto", "i", ORIENTATION_AUTO),
	GB_CONSTANT("Horizontal", "i", ORIENTATION_HORIZONTAL),
	GB_CONSTANT("Vertical", "i", ORIENTATION_VERTICAL),

	GB_END_DECLARE
};

