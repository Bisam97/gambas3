/***************************************************************************

  CDialog.cpp

  (c) 2004-2006 - Daniel Campos Fernández <dcamposf@gmail.com>

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

#define __CDIALOG_CPP

#include "CDialog.h"
#include "CFont.h"

static GB_ARRAY dialog_filter = NULL;


BEGIN_METHOD_VOID(CDIALOG_exit)

  GB.StoreObject(NULL, POINTER(&dialog_filter));

END_METHOD


BEGIN_PROPERTY(Dialog_Title)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(gDialog::title());
	else
		gDialog::setTitle(GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(Dialog_Filter)

  char **filters;
  char *filter;
  int i;

  if (READ_PROPERTY)
    GB.ReturnObject(dialog_filter);
  else
  {
    GB.StoreObject(PROP(GB_OBJECT), POINTER(&dialog_filter));
    GB.NewArray(&filters, sizeof(char *), 0);
    if (dialog_filter)
    {
			for (i = 0; i < (GB.Array.Count(dialog_filter) - 1); i += 2)
			{
				filter = *((char **)GB.Array.Get(dialog_filter, i));
				*((char **)GB.Add(&filters)) = filter;
				filter = *((char **)GB.Array.Get(dialog_filter, i + 1));
				*((char **)GB.Add(&filters)) = filter;
			}
		}
    
    gDialog::setFilter(filters, GB.Count(filters));
    GB.FreeArray(&filters);
  }

END_PROPERTY


BEGIN_PROPERTY(Dialog_Paths)

	GB_ARRAY Array=NULL;
	char **buf=NULL;
	long b=0;
	
	buf=gDialog::paths();
	if (buf)
	{
		while (buf[b++]);
		GB.Array.New(&Array,GB_T_STRING,b-1);
		
		b=0;
		while (buf[b])
		{
			*((char **)GB.Array.Get(Array, b)) = GB.NewZeroString(buf[b]);
			b++;
		}
		GB.ReturnObject(Array);
	}
	
END_PROPERTY


BEGIN_PROPERTY(Dialog_Path)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString( gDialog::path());
	else
		gDialog::setPath(GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY


BEGIN_PROPERTY(Dialog_ShowHidden)

	if (READ_PROPERTY)
		GB.ReturnBoolean(gDialog::showHidden());
	else
		gDialog::setShowHidden(VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Dialog_Font)

	CFONT *ft;
	
	if (READ_PROPERTY)
	{
    gFont *font = gDialog::font();
    if (font)
    	ft = CFONT_create(font->copy());
    else
    	ft = NULL;
    	
    GB.ReturnObject(ft);
		return;
	}
	
	ft=(CFONT*)VPROP(GB_OBJECT);
	if (ft && ft->font)
		gDialog::setFont(ft->font);

END_PROPERTY


BEGIN_PROPERTY(Dialog_Color)

	if (READ_PROPERTY) { GB.ReturnInteger( gDialog::color() ); return; }
	gDialog::setColor(VPROP(GB_INTEGER));

END_PROPERTY


BEGIN_METHOD(Dialog_OpenFile,GB_BOOLEAN Multi;)

	bool Multi=false;
	
	if (!MISSING(Multi)) Multi=VARG(Multi);

 	GB.ReturnBoolean( gDialog::openFile(Multi) );

END_METHOD


BEGIN_METHOD_VOID(Dialog_SaveFile)

 	GB.ReturnBoolean( gDialog::saveFile() );

END_METHOD


BEGIN_METHOD_VOID(Dialog_SelectDirectory)

 	GB.ReturnBoolean( gDialog::selectFolder() );

END_METHOD


BEGIN_METHOD_VOID(Dialog_SelectColor)

	GB.ReturnBoolean ( gDialog::selectColor() );

END_METHOD


BEGIN_METHOD_VOID(Dialog_SelectFont)

	GB.ReturnBoolean ( gDialog::selectFont() );

END_METHOD


BEGIN_PROPERTY(Dialog_FilterIndex)

	if (READ_PROPERTY)
		GB.ReturnInteger(gDialog::filterIndex());
	else
		gDialog::setFilterIndex(VPROP(GB_INTEGER));

END_PROPERTY


GB_DESC CDialogDesc[] =
{
  GB_DECLARE("Dialog", 0), GB_VIRTUAL_CLASS(),

  GB_STATIC_METHOD("_exit", 0, CDIALOG_exit, 0),

  GB_STATIC_METHOD("OpenFile", "b", Dialog_OpenFile, "[(Multi)b]"),
  GB_STATIC_METHOD("SaveFile", "b", Dialog_SaveFile, 0),
  GB_STATIC_METHOD("SelectDirectory", "b", Dialog_SelectDirectory, 0),
  GB_STATIC_METHOD("SelectColor", "b", Dialog_SelectColor, 0),
  GB_STATIC_METHOD("SelectFont", "b", Dialog_SelectFont, 0),

  GB_STATIC_PROPERTY_READ("Paths", "String[]", Dialog_Paths),
  
  GB_STATIC_PROPERTY("Title", "s", Dialog_Title),
  GB_STATIC_PROPERTY("Path", "s", Dialog_Path),
  GB_STATIC_PROPERTY("Filter", "String[]", Dialog_Filter),
	GB_STATIC_PROPERTY("FilterIndex", "i", Dialog_FilterIndex),
  GB_STATIC_PROPERTY("Color", "i", Dialog_Color),
  GB_STATIC_PROPERTY("Font", "Font", Dialog_Font),
  GB_STATIC_PROPERTY("ShowHidden", "b", Dialog_ShowHidden),
  
  GB_END_DECLARE
};


