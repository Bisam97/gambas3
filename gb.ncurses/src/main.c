/*
 * main.c - gb.ncurses main object
 *
 * Copyright (C) 2012 Tobias Boege <tobias@gambas-buch.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#define __MAIN_C

#include "c_ncurses.h"
#include "c_window.h"
#include "c_key.h"
#include "main.h"

GB_INTERFACE GB EXPORT;

GB_DESC *GB_CLASSES[] EXPORT =
{
  CNCursesDesc,
  CWindowDesc,
  CWindowAttrsDesc,
  CCharAttrsDesc,
	CScreenDesc,
  CKeyDesc,
  NULL
};

static void hook_error(int code, char *error, char *where)
{
	NCURSES_exit();
}

static void hook_init()
{
	NCURSES_init();
}

int EXPORT GB_INIT()
{
	GB.Hook(GB_HOOK_ERROR, (void *) hook_error);
	GB.Hook(GB_HOOK_MAIN, (void *) hook_init);
	return 0;
}


void EXPORT GB_EXIT()
{
	NCURSES_exit();
}

