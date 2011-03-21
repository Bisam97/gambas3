/***************************************************************************

  gclipboard.h

  (c) 2000-2011 Benoît Minisini <gambas@users.sourceforge.net>

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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/
#ifndef __GCLIPBOARD_H
#define __GCLIPBOARD_H

class gPicture;

class gClipboard
{
public:
	enum
	{
		Nothing = 0,
		Text = 1,
		Image = 2
	};
	
	static void init();
	static void clear();
	static char *getFormat(int n = 0);
	static int getType();
	static void setText(char *text, char *format = 0);
	static char *getText();
	static void setImage(gPicture *image);
	static gPicture *getImage();
};

#endif
