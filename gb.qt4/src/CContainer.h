/***************************************************************************

  CContainer.h

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

#ifndef __CCONTAINER_H
#define __CCONTAINER_H

#include <QResizeEvent>
#include <QShowEvent>
#include <QChildEvent>
#include <QEvent>
#include <QStyle>
#include <QStyleOptionFrame>

#include "gambas.h"

#include "CConst.h"
#include "CWidget.h"

typedef
	struct {
		unsigned mode : 4;
		unsigned locked : 1;
		unsigned dirty : 1;
		unsigned margin : 1;
		unsigned spacing : 1;
		unsigned padding : 8;
		unsigned indent : 1;
		unsigned centered : 1;
		unsigned autoresize : 1;
		unsigned invert : 1;
		unsigned _reserved: 12;
		}
	CARRANGEMENT;

#define ARRANGEMENT_FLAG_PROPERTIES \
	GB_PROPERTY("AutoResize", "b", Container_AutoResize), \
	GB_PROPERTY("Padding", "i", Container_Padding), \
	GB_PROPERTY("Spacing", "b", Container_Spacing), \
	GB_PROPERTY("Margin", "b", Container_Margin), \
	GB_PROPERTY("Indent", "b", Container_Indent), \
	GB_PROPERTY("Invert", "b", Container_Invert), \
	GB_PROPERTY("Centered", "b", Container_Centered)

#define ARRANGEMENT_PROPERTIES \
	GB_PROPERTY("Arrangement", "i", Container_Arrangement), \
	ARRANGEMENT_FLAG_PROPERTIES

	
#ifndef __CCONTAINER_CPP

extern GB_DESC ContainerDesc[];
extern GB_DESC ContainerChildrenDesc[];
extern GB_DESC UserControlDesc[];
extern GB_DESC UserContainerDesc[];

#else

typedef
	struct {
		ushort paint;
		ushort font;
		ushort change;
		ushort resize;
	}
	CUSERCONTROL_FUNC;

typedef
	struct {
		CCONTAINER parent;
		CUSERCONTROL_FUNC func;
		}
	CUSERCONTROL;
	
typedef
	struct {
		CCONTAINER parent;
		CUSERCONTROL_FUNC func;
		int32_t save;
		}
	CUSERCONTAINER;
	
typedef 
	struct {
		GB_BASE ob;
		CCONTAINER *container;
		CWIDGET **children;
	}
	CCONTAINERCHILDREN;

#define WIDGET QWIDGET(_object)
#define THIS ((CCONTAINER *)_object)
#define THIS_CHILDREN ((CCONTAINERCHILDREN *)_object)
#define CONTAINER (THIS->container)
#define THIS_ARRANGEMENT (((CCONTAINER_ARRANGEMENT *)_object))
#define THIS_USERCONTROL (((CUSERCONTROL *)_object))
#define THIS_USERCONTAINER (((CUSERCONTAINER *)_object))

//#define CCONTAINER_PROPERTIES CWIDGET_PROPERTIES ",Arrangement"

#endif

DECLARE_PROPERTY(Container_ClientX);
DECLARE_PROPERTY(Container_ClientY);

DECLARE_PROPERTY(Container_Arrangement);
DECLARE_PROPERTY(Container_AutoResize);
DECLARE_PROPERTY(Container_Padding);
DECLARE_PROPERTY(Container_Spacing);
DECLARE_PROPERTY(Container_Margin);
DECLARE_PROPERTY(Container_Indent);
DECLARE_PROPERTY(Container_Border);
DECLARE_PROPERTY(Container_SimpleBorder);
DECLARE_PROPERTY(Container_Invert);
DECLARE_PROPERTY(Container_Centered);

void CCONTAINER_arrange(void *_object);
void CCONTAINER_get_max_size(void *_object, int xc, int yc, int wc, int hc, int *w, int *h);
void CCONTAINER_insert_child(void *_object);
void CCONTAINER_remove_child(void *_object);
void CCONTAINER_decide(CWIDGET *control, bool *width, bool *height);

void CCONTAINER_draw_border(QPainter *p, char border, QWidget *w);
void CCONTAINER_draw_border_without_widget(QPainter *p, char border, QStyleOptionFrame &opt);
void CCONTAINER_set_border(char *border, char new_border, QWidget *wid);
int CCONTAINER_get_border_width(char border);

void CCONTAINER_update_design(void *_object);

void *CCONTAINER_get_first_child(void *_object);
void *CCONTAINER_get_last_child(void *_object);

void CUSERCONTROL_send_change_event();

class MyFrame : public QWidget
{
	Q_OBJECT
	
public:
	explicit MyFrame(QWidget *);
	
	int frameStyle() const { return _frame; }
	void setFrameStyle(int frame);
	void drawFrame(QPainter *);
	int frameWidth();
	void setPixmap(QPixmap *pixmap);
	void setPaintBackgroundColor(bool bg) { _bg = bg; }

protected:

	virtual void setStaticContents(bool on);
	virtual void paintEvent(QPaintEvent *);

private:

	int _frame;
	QPixmap *_pixmap;
	bool _bg;
};

class MyContainer : public MyFrame
{
	Q_OBJECT

public:

	MyContainer(QWidget *);
	~MyContainer();

	bool inDrawEvent();
	
protected:

	virtual void showEvent(QShowEvent *);
	virtual void hideEvent(QHideEvent *);
	virtual void paintEvent(QPaintEvent *);
	virtual void changeEvent(QEvent *);
	virtual void resizeEvent(QResizeEvent *);
};

#endif
