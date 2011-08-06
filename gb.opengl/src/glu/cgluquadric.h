/***************************************************************************

	cgluquadric.h

	(c) 2000-2011 Benoît Minisini <gambas@users.sourceforge.net>
	(and Tomek Kolodziejczyk)

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

#ifndef __CGLUQUADRIC_H
#define __CGLUQUADRIC_H

#include "gambas.h"
#include "main.h"
#include "GLU.h"

#ifndef __CGLUQUADRIC_C

extern GB_DESC GluQuadricDesc[];

#else

#define THIS OBJECT(CGLUQUADRIC)
#define QUADRIC (THIS->quadric)

#endif

typedef
	struct {
		GB_BASE ob;
		GLUquadric *quadric;
		}
	CGLUQUADRIC;

CGLUQUADRIC *CGLUQUADRIC_create();

/* Gluquadric structure as described in glu.h source

	struct GLUquadric 
	{
		GLint	normals;
		GLboolean	textureCoords;
		GLint	orientation;
		GLint	drawStyle;
		void	(GLAPIENTRY *errorCallback)( GLint );
	};
*/
	
#endif
