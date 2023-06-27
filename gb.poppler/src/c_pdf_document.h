/***************************************************************************

  c_pdf_document.h

  gb.poppler component

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

#ifndef __C_PDF_DOCUMENT_H
#define __C_PDF_DOCUMENT_H

//#include <SplashOutputDev.h>
//#include "splash/SplashBitmap.h"
#include "glib/poppler.h"
#include "cpp/poppler-document.h"
#include "cpp/poppler-page.h"
#include "cpp/poppler-page-renderer.h"
#include "main.h"

#ifndef __C_PDF_DOCUMENT_CPP
extern GB_DESC PdfDesc[];
extern GB_DESC PdfDocumentDesc[];
extern GB_DESC PdfPageDesc[];
extern GB_DESC PdfDocumentIndexDesc[];
extern GB_DESC PdfIndexDesc[];
extern GB_DESC PdfActionDesc[];
extern GB_DESC PdfActionGotoDesc[];
extern GB_DESC PdfActionLaunchDesc[];
extern GB_DESC PdfActionURIDesc[];
#endif

typedef
  struct {
		GB_BASE ob;
		PopplerAction *action;
	}
	CPDFACTION;

typedef
	struct {
		GB_BASE ob;
		CPDFACTION *action;
		int index;
		int parent;
		int children;
		unsigned opened : 1;
	}
	CPDFINDEX;

typedef
	struct {
		GB_BASE ob;
		char *buffer;
		int length;
		PopplerDocument *doc;
		PopplerPage **pages;
		PopplerPage *current;
		double resolution;
		int rotation;
		CPDFINDEX **index;
		poppler::document *rdoc;
		poppler::page_renderer *renderer;
	}
	CPDFDOCUMENT;


#define THIS ((CPDFDOCUMENT *)_object)
#define THIS_INDEX ((CPDFINDEX *)_object)
#define THIS_ACTION ((CPDFACTION *)_object)
#define ACTION (THIS_ACTION->action)
	
#endif /* __C_PDF_DOCUMENT_H */
