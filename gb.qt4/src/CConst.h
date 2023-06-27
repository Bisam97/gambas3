/***************************************************************************

  CConst.h

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

#ifndef __CCONST_H
#define __CCONST_H

#include "gambas.h"
#include "gb.form.const.h"

#ifndef __CCONST_CPP
extern GB_DESC AlignDesc[];
extern GB_DESC ArrangeDesc[];
extern GB_DESC BorderDesc[];
extern GB_DESC ScrollDesc[];
extern GB_DESC SelectDesc[];
extern GB_DESC DirectionDesc[];
#endif

int CCONST_convert(int *tab, int value, int def, bool to_qt);
int CCONST_alignment(int value, int def, bool to_qt);
int CCONST_horizontal_alignment(int value, int def, bool to_qt);
int CCONST_line_style(int value, int def, bool to_qt);
int CCONST_fill_style(int value, int def, bool to_qt);

#endif
