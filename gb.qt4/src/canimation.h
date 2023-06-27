/***************************************************************************

  canimation.h

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

#ifndef __CANIMATION_H
#define __CANIMATION_H

#include "gambas.h"

#include <QMovie>
#include <QBuffer>

#ifndef __CANIMATION_CPP
extern GB_DESC AnimationDesc[];
#else

#define THIS ((CANIMATION *)_object)
#define MOVIE (((CANIMATION *)_object)->movie)

#endif

typedef
  struct {
    GB_BASE ob;
    QByteArray *data;
		QBuffer *buffer;
    QMovie *movie;
    char *addr;
    int len;
    }
  CANIMATION;

class CAnimationManager : public QObject
{
	Q_OBJECT

public:

	static CAnimationManager manager;

public slots:

	void change(void);
};

#endif
