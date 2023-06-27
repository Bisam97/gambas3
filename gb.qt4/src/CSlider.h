/***************************************************************************

  CSlider.h

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

#ifndef __CSLIDER_H
#define __CSLIDER_H

#include <QSlider>
#include <QResizeEvent>

#include "gambas.h"

#include "CWidget.h"

#ifndef __CSLIDER_CPP

extern GB_DESC SliderDesc[];

#else

#define THIS    ((CSLIDER *)_object)
#define WIDGET  ((MySlider *)((QT_WIDGET *)_object)->widget)

#endif

typedef
  struct {
    CWIDGET widget;
    }
  CSLIDER;

class MySlider : public QSlider
{
  Q_OBJECT

public:

  explicit MySlider(QWidget *);
	void updateOrientation();

protected:

  void resizeEvent(QResizeEvent *);
};



class CSlider : public QObject
{
  Q_OBJECT

public:

  static CSlider manager;

public slots:

  void event_change(void);
  //void event_sliderpressed(void);
  //void event_slidermove(void);
  //void event_sliderreleased(void);

};

#endif
