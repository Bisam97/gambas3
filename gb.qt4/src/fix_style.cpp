/***************************************************************************

	fix_stylecpp

	(c) Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __FIX_STYLE_CPP

#include <QRect>
#include <QStyleOptionSpinBox>
#include <QPainter>
#include <QApplication>

#include "gb_common.h"
#include "CStyle.h"
#include "fix_style.h"

//-------------------------------------------------------------------------

void FixStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter * painter, const QWidget * widget) const
{
	QStyleOptionButton newOption;

	if (element == CE_CheckBoxLabel || element == CE_RadioButtonLabel)
	{
		newOption = *(QStyleOptionButton *)option;
		newOption.direction = qApp->layoutDirection();
		option = &newOption;
	}
	
	QProxyStyle::drawControl(element, option, painter, widget);
}

//-------------------------------------------------------------------------

QFontMetrics *FixBreezeStyle::fm = NULL;

void FixBreezeStyle::fixFontMetrics(QStyleOption *option)
{
	if (!fm)
	{
		QFont f = qApp->font();
		f.setPointSize(1);
		fm = new QFontMetrics(f);
	}
		
	option->fontMetrics = *fm;
}

QRect FixBreezeStyle::subControlRect(ComplexControl element, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const
{
	/*if (element == CC_ComboBox)
	{
		if (subControl == SC_ComboBoxEditField)
		{
			const QStyleOptionComboBox *comboBoxOption( qstyleoption_cast<const QStyleOptionComboBox*>(option));
			const bool editable( comboBoxOption->editable );
			const bool flat( editable && !comboBoxOption->frame );
			QRect rect(option->rect);
			QRect labelRect;
			
			const int frameWidth(pixelMetric(PM_ComboBoxFrameWidth, option, widget));
			labelRect = QRect(
					rect.left(), rect.top(),
					rect.width() - 20, //Metrics::MenuButton_IndicatorWidth,
					rect.height() );

			// remove margins
			if (!flat)
			{
				if (CSTYLE_fix_breeze)
					labelRect.adjust(frameWidth, 2, 0, -2 );
				else if (CSTYLE_fix_oxygen)
					labelRect.adjust(frameWidth, 4, 0, -4 );
			}

			return visualRect( option, labelRect );
		}
	}
	else*/
	if (element == CC_Slider)
	{
		const QStyleOptionSlider *sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
		const bool horizontal( sliderOption->orientation == Qt::Horizontal );
		
		QRect result(QProxyStyle::subControlRect(element, option, subControl, widget));
		
		if (horizontal)
			result.moveTop((widget->height() - result.height()) / 2);
		else
			result.moveLeft((widget->width() - result.width()) / 2);
		
		return result;
	}
	
	return QProxyStyle::subControlRect(element, option, subControl, widget);
}

QRect FixBreezeStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
	if (element == SE_LineEditContents)
	{
		const QStyleOptionFrame* frameOption(qstyleoption_cast<const QStyleOptionFrame*>(option));
		
		const bool flat( frameOption->lineWidth == 0 );
		if(flat)
			return option->rect;
		
		QRect rect( option->rect );

		const int frameWidth(pixelMetric(PM_DefaultFrameWidth, option, widget));
		rect.adjust(frameWidth, 2, -frameWidth, -2);
	
		return rect;
	}
	
	return QProxyStyle::subElementRect(element, option, widget);
}

void FixBreezeStyle::drawPrimitive( PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
{
	if (element == PE_FrameLineEdit)
	{
		QStyleOption newOption = *option;
		fixFontMetrics(&newOption);
		//qDebug("PE_FrameLineEdit: %d / %d", option->fontMetrics.height(), option->rect.height());
		QProxyStyle::drawPrimitive(element, &newOption, painter, widget);
		return;
	}
		
	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void FixBreezeStyle::drawComplexControl(ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
	/*if (element == CC_SpinBox)
	{
		QStyleOptionSpinBox newOption;	
		const QStyleOptionSpinBox *spinBoxOption( qstyleoption_cast<const QStyleOptionSpinBox*>( option ) );
		
		if (option->subControls & SC_SpinBoxFrame)
		{
			if (spinBoxOption->frame)
			{
				if( option->subControls & SC_SpinBoxFrame )
				{
					newOption = *spinBoxOption;
					newOption.subControls &= ~SC_SpinBoxFrame;
					option = &newOption;
					
					drawPrimitive( PE_FrameLineEdit, option, painter, widget );
				}
			}
		}
	}
	else
	if (element == CC_ComboBox)
	{
		QStyleOptionComboBox newOption;	
		const QStyleOptionComboBox* comboBoxOption( qstyleoption_cast<const QStyleOptionComboBox*>( option ) );

		if (option->subControls & SC_ComboBoxFrame)
		{
			if (comboBoxOption->editable)
			{
				if (comboBoxOption->frame)
				{
					newOption = *comboBoxOption;
					newOption.subControls &= ~SC_ComboBoxFrame;
					
					drawPrimitive(PE_FrameLineEdit, &newOption, painter, widget );
					QProxyStyle::drawComplexControl(element, &newOption, painter, widget);
					return;
				}
			}
		}
	}
	else*/
	if (element == CC_Slider)
	{
		//QStyleOptionSlider newOption;
		const QStyleOptionSlider *sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
		const bool horizontal( sliderOption->orientation == Qt::Horizontal );
		
		if (!(sliderOption->subControls & SC_SliderTickmarks))
		{
			QRect handle(QProxyStyle::subControlRect(element, option, SC_SliderHandle, widget));
			//newOption = *sliderOption;
			//option = &newOption;
			
			painter->save();
			if (horizontal)
				painter->translate(0, (widget->height() - handle.height()) / 2);
			else
				painter->translate((option->rect.width() - handle.width()) / 2, 0);
				//newOption.rect = QRect(newOption.rect.x(), (newOption.rect.height() - handle.height()) / 2, newOption.rect.width(), handle.height());
			
			QProxyStyle::drawComplexControl(element, option, painter, widget);
			painter->restore();
			return;
		}
	}
	
	QProxyStyle::drawComplexControl(element, option, painter, widget);
}

void FixBreezeStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter * painter, const QWidget * widget) const
{
	if (element == CE_PushButtonBevel)
	{
		QStyleOptionButton newOption = *(QStyleOptionButton *)option;
		newOption.iconSize = QSize(0, 0);
		option = &newOption;
		QProxyStyle::drawControl(element, option, painter, widget);
		return;
	}
	
	FixStyle::drawControl(element, option, painter, widget);
}

