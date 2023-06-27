/***************************************************************************

	main.c

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

#define __MAIN_C

#include "main.h"

GB_INTERFACE *GB_PTR EXPORT;


BEGIN_METHOD(Test_Load, GB_STRING name)

	GB.ReturnObject((void *)GB.LoadClassFrom(GB.ToZeroString(ARG(name)), NULL));
	
END_METHOD


GB_DESC TestDesc[] =
{
	GB_DECLARE_STATIC("__Test"),

	GB_STATIC_METHOD("Load", "Class", Test_Load, "(Class)s"),

	GB_END_DECLARE
};


GB_DESC *GB_CLASSES [] EXPORT =
{
	TestDesc,
	NULL
};


int EXPORT GB_INIT(void)
{
	return 0;
}


void EXPORT GB_EXIT()
{
}

