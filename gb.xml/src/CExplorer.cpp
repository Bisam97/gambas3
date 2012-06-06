/***************************************************************************

  (c) 2012 Adrien Prokopowicz <prokopy@users.sourceforge.net>

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

#include "CExplorer.h"
#include "CDocument.h"
#include "explorer.h"
#include "gbi.h"
#include "document.h"
#include "element.h"

#undef THIS
#define THIS (static_cast<CExplorer*>(_object)->explorer)

BEGIN_METHOD(CExplorerReadFlags_get, GB_INTEGER flag)

int flag = VARG(flag);
if(flag > FLAGS_COUNT || flag < 0) return;
GB.ReturnBoolean(THIS->flags[flag]);

END_METHOD

BEGIN_METHOD(CExplorerReadFlags_put, GB_BOOLEAN value; GB_INTEGER flag)

int flag = VARG(flag);
if(flag > FLAGS_COUNT || flag < 0 || flag == READ_ERR_EOF) return;
THIS->flags[flag] = VARG(value);

END_METHOD

BEGIN_PROPERTY(CExplorer_Node)

GBI::Return(THIS->curNode);

END_PROPERTY

BEGIN_METHOD_VOID(CExplorer_Read)

GB.ReturnInteger(THIS->Read());

END_METHOD

BEGIN_METHOD(CExplorer_new, GB_OBJECT doc)

THIS = new Explorer;
if(!MISSING(doc))
{
    THIS->Load(VARGOBJ(CDocument, doc)->doc);
}

END_METHOD

BEGIN_METHOD_VOID(CExplorer_free)

THIS->Clear();

END_METHOD

BEGIN_METHOD(CExplorer_open, GB_STRING path)

Document *doc = new Document;
char *content; int len;

if(GB.LoadFile(STRING(path), LENGTH(path), &content, &len))
{
    GB.Error("Error loading file.");
    return;
}

    try
    {
        doc->setContent(content, len);
    }
    catch(XMLParseException &e)
    {
        GB.Error(e.what());
    }
    GB.ReleaseFile(content, len);

THIS->Load(doc);

END_METHOD

BEGIN_PROPERTY(CExplorer_eof)

GB.ReturnBoolean(THIS->eof);

END_PROPERTY

BEGIN_PROPERTY(CExplorer_state)

GB.ReturnInteger(THIS->state);

END_PROPERTY

BEGIN_METHOD(CExplorer_document, GB_OBJECT doc)

THIS->Load(VARGOBJ(CDocument, doc)->doc);

END_METHOD

GB_DESC CExplorerReadFlagsDesc[] =
{
    GB_DECLARE(".XmlExplorerReadFlags", 0), GB_VIRTUAL_CLASS(),

    GB_METHOD("_get", "b", CExplorerReadFlags_get, "(Flag)i"),
    GB_METHOD("_put", "b", CExplorerReadFlags_put, "(Value)b(Flag)i"),

    GB_END_DECLARE
};

GB_DESC CExplorerDesc[] =
{
    GB_DECLARE("XmlExplorer", sizeof(CExplorer)),

    GB_METHOD("_new", "", CExplorer_new, "[(Document)XmlDocument]"),
    GB_METHOD("_free", "", CExplorer_free, ""),
    GB_METHOD("Load", "", CExplorer_document, "(Document)XmlDocument"),
    GB_PROPERTY_SELF("ReadFlags", ".XmlExplorerReadFlags"),
    GB_PROPERTY_READ("Node", "XmlNode", CExplorer_Node),
    GB_PROPERTY_READ("Eof", "b", CExplorer_eof),
    GB_PROPERTY_READ("State", "i", CExplorer_state),
    GB_METHOD("Read", "i", CExplorer_Read, ""),
    GB_METHOD("Open", "", CExplorer_open, "(Path)s"),

    GB_END_DECLARE
};
