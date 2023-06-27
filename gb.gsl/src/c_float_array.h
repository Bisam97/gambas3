/***************************************************************************

	c_float_array.h

	gb.gsl component

	(c) Benoît Minisini <benoit.minisini@gambas-basic.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 1, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.

***************************************************************************/

#ifndef __C_FLOAT_ARRAY_H
#define __C_FLOAT_ARRAY_H

#include "main.h"

#ifndef __C_FLOAT_ARRAY_C
extern GB_DESC FloatArrayStatDesc[];
extern GB_DESC FloatArrayDesc[];
#endif

typedef
	struct {
		GB_BASE ob;
		GB_ARRAY_BASE array;
	}
	CARRAY;

#endif /* __C_COMPLEX_H */
