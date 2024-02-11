/***************************************************************************

  gb.form.gui.h

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

#ifndef __GB_FORM_GUI_H
#define __GB_FORM_GUI_H

#define CALL_GUI(_name, _sign, _type, _nparam, _release) \
({ \
	static bool init = FALSE; \
	static GB_FUNCTION func; \
 \
	if (!init) \
	{ \
		GB.GetFunction(&func, (void *)GB.FindClass("_Gui"), _name, _sign, _type); \
		init = TRUE; \
	} \
 \
	GB.Call(&func, _nparam, _release); \
})

#endif
