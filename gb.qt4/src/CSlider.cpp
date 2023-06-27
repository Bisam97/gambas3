/***************************************************************************

	CSlider.cpp

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

#define __CSLIDER_CPP

#include "main.h"
#include "gambas.h"

#include "gb.form.const.h"
#include "CWidget.h"
#include "CSlider.h"

DECLARE_EVENT(EVENT_Change);


/***************************************************************************

	class MySlider

***************************************************************************/


MySlider::MySlider(QWidget *parent)
: QSlider(parent)
{
}

void MySlider::updateOrientation()
{
	CSLIDER *_object = (CSLIDER *)CWidget::get(this);
	
	if (!THIS->widget.flag.orientation)
	{
		if (width() >= height())
			setOrientation(Qt::Horizontal);
		else
			setOrientation(Qt::Vertical);
	}
}

void MySlider::resizeEvent(QResizeEvent *e)
{
	CSLIDER *_object = (CSLIDER *)CWidget::get(this);

	QSlider::resizeEvent(e);
	
	if (!THIS->widget.flag.orientation)
		WIDGET->updateOrientation();
}


/***************************************************************************

	Slider

***************************************************************************/

BEGIN_METHOD(Slider_new, GB_OBJECT parent)

	MySlider *wid = new MySlider(QCONTAINER(VARG(parent)));

	THIS->widget.flag.wheel = true;
	
	QObject::connect(wid, SIGNAL(valueChanged(int)), &CSlider::manager, SLOT(event_change()));
	//QObject::connect(wid, SIGNAL(sliderPressed()), &CSlider::manager,
	//SLOT(event_sliderpressed()));
	//QObject::connect(wid, SIGNAL(sliderMoved(int)), &CSlider::manager,
	//SLOT(event_slidermove()));
	//QObject::connect(wid, SIGNAL(sliderReleased()), &CSlider::manager,
	//SLOT(event_sliderreleased()));

	wid->setTracking(true); //Set the tracking off by default
	wid->setMinimum(0);
	wid->setMaximum(100);
	wid->setSingleStep(1);
	wid->setPageStep(10);

	CWIDGET_new(wid, _object);

END_METHOD


BEGIN_PROPERTY(Slider_Tracking)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->hasTracking());
	else
		WIDGET->setTracking(VPROP(GB_BOOLEAN));

END_PROPERTY


/*BEGIN_PROPERTY(CSLIDER_tickinterval)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->tickInterval());
	else
		WIDGET->setTickInterval(VPROP(GB_INTEGER));

END_PROPERTY*/

/*
BEGIN_PROPERTY(CSLIDER_orientation)

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

BEGIN_PROPERTY(Slider_Value)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->value());
	else
		WIDGET->setValue(VPROP(GB_INTEGER));

END_PROPERTY

/*
BEGIN_PROPERTY(CSLIDER_tickmarks)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->tickmarks());
	else
	{
		switch PROPERTY(GB_INTEGER)
		{
			case QSlider::NoMarks: WIDGET->setTickmarks(QSlider::NoMarks);break;
			case QSlider::Both: WIDGET->setTickmarks(QSlider::Both);break;
			case QSlider::Above: WIDGET->setTickmarks(QSlider::Above);break;
			case QSlider::Below: WIDGET->setTickmarks(QSlider::Below);break;
			//case QSlider::Left: WIDGET->setTickmarks(QSlider::Left);break;
			//case QSlider::Right: WIDGET->setTickmarks(QSlider::Right);break;
			default: WIDGET->setTickmarks(QSlider::NoMarks);
		}
	}

END_PROPERTY
*/


BEGIN_PROPERTY(Slider_Mark)

	if (READ_PROPERTY)
		GB.ReturnBoolean(WIDGET->tickPosition() != QSlider::NoTicks);
	else
	{
		if (VPROP(GB_BOOLEAN))
			WIDGET->setTickPosition(QSlider::TicksBothSides);
		else
			WIDGET->setTickPosition(QSlider::NoTicks);
	}

END_PROPERTY


BEGIN_PROPERTY(Slider_MinValue)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->minimum());
	else
		WIDGET->setMinimum(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(Slider_MaxValue)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->maximum());
	else
		WIDGET->setMaximum(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(Slider_LineStep)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->singleStep());
	else
		WIDGET->setSingleStep(VPROP(GB_INTEGER));

END_PROPERTY

BEGIN_PROPERTY(Slider_PageStep)

	if (READ_PROPERTY)
		GB.ReturnInteger(WIDGET->pageStep());
	else
	{
		WIDGET->setPageStep(VPROP(GB_INTEGER));
		WIDGET->update();
	}

END_PROPERTY

BEGIN_PROPERTY(Slider_Orientation)

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

CSlider CSlider::manager;

void CSlider::event_change(void)
{
	GET_SENDER();
	GB.Raise(THIS, EVENT_Change, 0);
}

//-------------------------------------------------------------------------

GB_DESC SliderDesc[] =
{
	GB_DECLARE("Slider", sizeof(CSLIDER)), GB_INHERITS("Control"),

	GB_METHOD("_new", NULL, Slider_new, "(Parent)Container;"),

	GB_PROPERTY("Tracking", "b", Slider_Tracking),
	GB_PROPERTY("Value", "i", Slider_Value),
	GB_PROPERTY("Mark", "b", Slider_Mark),
	GB_PROPERTY("MinValue", "i", Slider_MinValue),
	GB_PROPERTY("MaxValue", "i", Slider_MaxValue),
	GB_PROPERTY("Step", "i", Slider_LineStep),
	GB_PROPERTY("PageStep", "i", Slider_PageStep),
	GB_PROPERTY("Orientation", "i", Slider_Orientation),

	GB_EVENT("Change", NULL, NULL, &EVENT_Change),

	SLIDER_DESCRIPTION,

	GB_END_DECLARE
};

