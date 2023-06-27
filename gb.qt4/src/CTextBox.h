/***************************************************************************

	CTextBox.h

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

#ifndef __CTEXTBOX_H
#define __CTEXTBOX_H

#include <QComboBox>
#include <QEvent>

#include "gambas.h"

#include "CWidget.h"

#ifndef __CTEXTBOX_CPP

extern GB_DESC CTextBoxSelectionDesc[];
extern GB_DESC CTextBoxDesc[];

#else

#define QLINEEDIT(object) ((QLineEdit *)((CWIDGET *)object)->widget)

#define TEXTBOX ((QLineEdit *)((CWIDGET *)_object)->widget)
#define THIS ((CTEXTBOX *)_object)

#endif

typedef
	struct {
		CWIDGET widget;
		int start;
		int length;
		int locked;
		}
	CTEXTBOX;


class CTextBox : public QObject
{
	Q_OBJECT

public:

	static CTextBox manager;

public slots:

	void onChange(void);
	void onActivate(void);
	void onCursor(void);
	//void onSelectionChanged(void);

};


#endif
