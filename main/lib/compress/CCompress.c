/***************************************************************************

	CCompress.c

	(c) 2003-2004 Daniel Campos Fern�dez <danielcampos@netcourrier.com>

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

#define __CCOMPRESS_C


#include "CCompress.h"
#include "main.h"
#include <stdio.h>

#define Check_Driver()  if (!THIS->driver) { GB.Error("No driver specified"); return; }

#define Check_Level() 	if (!MISSING(Level)) \
				level=VARG(Level); \
			else \
				level=THIS->driver->default_compression(); \
									\
			if ( (level < THIS->driver->min_compression()) || (level > THIS->driver->max_compression()) ) \
			if (level != THIS->driver->default_compression()) \
			{ \
				GB.Error("Invalid compression level"); \
				return; \
			} 

//*************************************************************************
//############################### COMPRESS ################################
//*************************************************************************

static void set_type(void *_object, const char *type)
{
	if (THIS->stream.desc)
	{
		GB.Error("Type can not be changed while the stream is opened");
		return;
	}

	THIS->driver = COMPRESS_GetDriver(type);
}

static void *create_compress(const char *type)
{
	COMPRESS_DRIVER *driver;
	void *_object;

	driver = COMPRESS_GetDriver(type);
	if (!driver)
		return NULL;

	_object = GB.New(GB.FindClass("Compress"), NULL, NULL);
	THIS->driver = driver;
	return _object;
}


/*************************************************
Gambas object "Constructor"
*************************************************/

BEGIN_METHOD_VOID(Compress_new)

	THIS->driver = NULL;
	THIS->stream.desc = NULL;

END_METHOD


BEGIN_METHOD(Compress_String, GB_STRING Source; GB_INTEGER Level; GB_BOOLEAN AllowGrow)

	int level;
	char *target=NULL;
	unsigned int lent;
	int allow=0;
	
	Check_Driver();
	Check_Level();
	lent=0;
	if (!MISSING(AllowGrow)) 
		if (VARG(AllowGrow)) allow = 1;
	
	THIS->driver->Compress.String(&target, &lent, STRING(Source), LENGTH(Source), level);
		
	if (!lent) { GB.ReturnVoidString(); return; }
	if ( (!allow) && (LENGTH(Source)<=lent) ) 
	{
		if (target) GB.Free(POINTER(&target)); 
		GB.ReturnNewString (STRING(Source), LENGTH(Source));
		return; 
	}
	
	GB.ReturnNewString (target,lent);
	if (target) GB.Free(POINTER(&target));
	
END_METHOD


BEGIN_METHOD(Compress_File, GB_STRING Source; GB_STRING Target; GB_INTEGER Level)

	int level;
	
	Check_Driver();
	Check_Level();
	THIS->driver->Compress.File(STRING(Source), STRING(Target), level);

END_METHOD


BEGIN_METHOD(Compress_Open, GB_STRING Path; GB_INTEGER Level)

	int level;

	Check_Driver();
	Check_Level();
	
	if (THIS->stream.desc) 
	{
		GB.Error("File is already opened");
		return;
	}
	
	THIS->driver->Compress.Open(STRING(Path), level,  &THIS->stream);

END_METHOD


BEGIN_PROPERTY(Compress_Min)

	if (!THIS->driver) { GB.ReturnInteger(0); return; }
	GB.ReturnInteger(THIS->driver->min_compression());

END_PROPERTY


BEGIN_PROPERTY(Compress_Max)

	if (!THIS->driver) { GB.ReturnInteger(0); return; }
	GB.ReturnInteger(THIS->driver->max_compression());

END_PROPERTY


BEGIN_PROPERTY(Compress_Default)

	if (!THIS->driver) { GB.ReturnInteger(0); return; }
	GB.ReturnInteger(THIS->driver->default_compression());

END_PROPERTY


BEGIN_PROPERTY(Compress_Type)

	if (READ_PROPERTY)
	{
		if (!THIS->driver) { GB.ReturnVoidString(); return; }
		GB.ReturnNewZeroString(THIS->driver->name);
		return;
	}

	set_type(THIS, GB.ToZeroString(PROP(GB_STRING)));
	
END_PROPERTY


BEGIN_METHOD(Compress_get, GB_STRING type)

	GB.ReturnObject(create_compress(GB.ToZeroString(ARG(type))));

END_METHOD


/*******************************************************************
Interface declaration
*******************************************************************/

GB_DESC CompressDesc[] =
{
	GB_DECLARE("Compress", sizeof(CCOMPRESS)),
	
	GB_INHERITS("Stream"),
	
	GB_PROPERTY_READ("Min", "i", Compress_Min),
	GB_PROPERTY_READ("Max", "i", Compress_Max),
	GB_PROPERTY_READ("Default", "i", Compress_Default),
	GB_PROPERTY("Type", "s", Compress_Type),

	GB_METHOD("_new", NULL, Compress_new, NULL),
	
	GB_METHOD("String", "s", Compress_String, "(Source)s[(Level)i(AllowGrow)b]"),
	GB_METHOD("File", NULL, Compress_File, "(Source)s(Target)s[(Level)i]"),
	GB_METHOD("Open", NULL, Compress_Open, "(Path)s[(Level)i]"),

	GB_STATIC_METHOD("_get", "Compress", Compress_get, "(Type)s"),

	GB_END_DECLARE
};

