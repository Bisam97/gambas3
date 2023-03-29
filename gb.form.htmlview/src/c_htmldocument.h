/***************************************************************************

  c_htmldocument.h

  gb.form.htmlview component

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

#ifndef __C_HTMLDOCUMENT_H
#define __C_HTMLDOCUMENT_H

#include "litehtml/html.h"
#include "litehtml/document.h"
#include "litehtml/html_tag.h"
#include "litehtml/stylesheet.h"
#include "litehtml/element.h"
#include "litehtml/html_tag.h"

#include "main.h"

class html_document;

typedef
	struct {
		GB_BASE ob;
		char *html;
		html_document *doc;
		char *css;
		char *default_font_name;
		char *monospace_font_name;
		int default_font_size;
		int screen_width;
		int screen_height;
		int resolution;
		char *link;
		char *base;
	}
	CHTMLDOCUMENT;

#ifndef __C_HTMLDOCUMENT_CPP

extern GB_DESC HtmlDocumentDesc[];

#else

#define THIS ((CHTMLDOCUMENT *)_object)

#endif

#endif
