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

#include "CReader.h"
#include "reader.h"
#include "element.h"
#include "gbi.h"

#undef THIS
#define THIS (static_cast<CReader*>(_object)->reader)

BEGIN_METHOD_VOID(CReader_new)

THIS = new Reader;

END_METHOD

BEGIN_METHOD_VOID(CReader_free)

delete THIS;

END_METHOD

BEGIN_METHOD(CReader_ReadChar, GB_STRING car)

if(!LENGTH(car)) return;
GB.ReturnInteger(THIS->ReadChar(STRING(car)[0]));

END_METHOD

BEGIN_PROPERTY(CReader_keepData)

if(READ_PROPERTY)
{
    GB.ReturnBoolean(THIS->keepMemory);
}
else
{
    THIS->keepMemory = VPROP(GB_BOOLEAN);
    if(THIS->keepMemory && THIS->foundNode)
    {
        //TODO :
        //THIS->storedElements->push_back(THIS->foundNode);
        //GB.Ref(THIS->foundNode);
    }
}

END_PROPERTY

BEGIN_PROPERTY(CReader_pos)

if(!READ_PROPERTY) return;

GB.ReturnInteger(THIS->pos);

END_PROPERTY

BEGIN_METHOD_VOID(CReaderNodeAttr_next)

if(!THIS->foundNode) 
{
    GB.StopEnum(); return;
}

if(!THIS->foundNode->isElement())
{
    GB.StopEnum(); return;
}

Attribute *attr = *reinterpret_cast<Attribute**>((GB.GetEnum()));
if(attr == 0)
{
    attr = THIS->foundNode->toElement()->firstAttribute;
    *reinterpret_cast<Attribute**>(GB.GetEnum()) = attr;
}
else
{
    attr = (Attribute*)attr->nextNode;
    *reinterpret_cast<Attribute**>(GB.GetEnum()) = attr;
}    

if(attr == 0) {GB.StopEnum(); THIS->curAttrEnum = 0; return;}

GB.ReturnNewString(attr->attrValue, attr->lenAttrValue);

THIS->curAttrEnum = attr;


END_METHOD

BEGIN_METHOD(CReaderNodeAttr_get, GB_STRING name)

if(!THIS->foundNode) 
{
    return;
}

if(!THIS->foundNode->isElement()) return;

Attribute *attr = THIS->foundNode->toElement()->getAttribute(STRING(name), LENGTH(name));

GB.ReturnNewString(attr->attrValue, attr->lenAttrValue);

END_METHOD

BEGIN_METHOD(CReaderNodeAttr_put, GB_STRING value; GB_STRING name)

if(!THIS->foundNode) 
{
    return;
}
if(!THIS->foundNode->isElement()) return;
THIS->foundNode->toElement()->setAttribute(STRING(name), LENGTH(name), 
                                           STRING(value), LENGTH(value));

END_METHOD

BEGIN_PROPERTY(CReaderNodeAttr_count)

if(!THIS->foundNode) 
{
    GB.ReturnInteger(0);
    return;
}

if(THIS->foundNode->isElement())
{
    GB.ReturnInteger(THIS->foundNode->toElement()->attributeCount);
}
else
{
GB.ReturnInteger(0);
}

END_PROPERTY

BEGIN_METHOD(CReaderReadFlags_get, GB_INTEGER flag)

int flag = VARG(flag);
if(flag > FLAGS_COUNT || flag < 0) return;
GB.ReturnBoolean(THIS->flags[flag]);

END_METHOD

BEGIN_METHOD(CReaderReadFlags_put, GB_BOOLEAN value; GB_INTEGER flag)

int flag = VARG(flag);
if(flag > FLAGS_COUNT || flag < 0 || flag == READ_ERR_EOF) return;
THIS->flags[flag] = VARG(value);

END_METHOD

BEGIN_PROPERTY(CReaderNode_type)

/*if(!THIS->foundNode) 
{
    GB.ReturnInteger(0);
    return;
}


if(THIS->curAttrEnum)
{
    GB.ReturnInteger(READ_ATTRIBUTE);
    return;
}

switch(THIS->foundNode->getType())
{
    case Node::ElementNode:
        GB.ReturnInteger(NODE_ELEMENT);break;
    case Node::Comment:
        GB.ReturnInteger(NODE_COMMENT);break;
    case Node::NodeText:
        GB.ReturnInteger(NODE_TEXT);break;
    case Node::CDATA:
        GB.ReturnInteger(NODE_CDATA);break;
    default:
        GB.ReturnInteger(0);
}
*/

GB.ReturnInteger(THIS->state);
END_PROPERTY

BEGIN_PROPERTY(CReaderNode_Value)

if(!THIS->foundNode) 
{
    GB.ReturnNull();
    return;
}


if(THIS->curAttrEnum)
{
    GB.ReturnNewString(THIS->curAttrEnum->attrValue, THIS->curAttrEnum->lenAttrValue);
    return;
}

char *data; size_t len;
THIS->foundNode->GBTextContent(data, len);
GB.ReturnString(data);

END_PROPERTY

BEGIN_PROPERTY(CReaderNode_Name)

if(!THIS->foundNode) 
{
    GB.ReturnNull();
    return;
}

if(THIS->curAttrEnum)
{
    GB.ReturnNewString(THIS->curAttrEnum->attrName, THIS->curAttrEnum->lenAttrName);
    return;
}

switch (THIS->foundNode->getType())
{
    case Node::ElementNode:
        GB.ReturnNewString(THIS->foundNode->toElement()->tagName, 
                           THIS->foundNode->toElement()->lenTagName); break;
    case Node::NodeText:
        GB.ReturnNewZeroString("#text");break;
    case Node::Comment:
        GB.ReturnNewZeroString("#comment");break;
    case Node::CDATA:
        GB.ReturnNewZeroString("#cdata");break;
    default:
        GB.ReturnNull();
}


END_PROPERTY

BEGIN_PROPERTY(CReader_Depth)

GB.ReturnInteger(THIS->depth);

END_PROPERTY

BEGIN_PROPERTY(CReader_storedNodes)

if(!READ_PROPERTY) return;

//GBI::ObjectArray<Node> *nodes = new GBI::ObjectArray<Node>("XmlNode", *(THIS->storedElements));
//GB.ReturnObject(nodes->array);
//delete nodes;

GB.ReturnObject(0);

END_PROPERTY



GB_DESC CReaderNodeTypeDesc[] =
{
	GB_DECLARE("XmlReaderNodeType", 0), GB_VIRTUAL_CLASS(),

    GB_CONSTANT("None", "i", 0),
	GB_CONSTANT("Element", "i", NODE_ELEMENT),
	GB_CONSTANT("Attribute", "i",  READ_ATTRIBUTE),
	GB_CONSTANT("Text", "i", NODE_TEXT),
	GB_CONSTANT("CDATA", "i", NODE_CDATA),
	GB_CONSTANT("EntityReference", "i", 0),
	GB_CONSTANT("Entity", "i", 0),
	GB_CONSTANT("ProcessingInstruction", "i", 0),
	GB_CONSTANT("Comment", "i", NODE_COMMENT),
	GB_CONSTANT("Document", "i", 0),
	GB_CONSTANT("DocumentType", "i", 0),
	GB_CONSTANT("DocumentFragment", "i", 0),
	GB_CONSTANT("Notation", "i", 0),
	GB_CONSTANT("Whitespace", "i",0),
	GB_CONSTANT("SignificantWhitespace", "i",0),
	GB_CONSTANT("EndElement", "i", READ_END_CUR_ELEMENT),
    GB_CONSTANT("EndStream", "i", READ_ERR_EOF),
	GB_CONSTANT("EndEntity", "i", 0),
	GB_CONSTANT("XmlDeclaration", "i",0),

	GB_END_DECLARE
};

GB_DESC CReaderReadFlagsDesc[] =
{
	GB_DECLARE(".XmlReaderReadFlags", 0), GB_VIRTUAL_CLASS(),

    GB_METHOD("_get", "b", CReaderReadFlags_get, "(Flag)i"),
    GB_METHOD("_put", "b", CReaderReadFlags_put, "(Value)b(Flag)i"),

	GB_END_DECLARE
};

GB_DESC CReaderNodeAttributesDesc[] =
{
	GB_DECLARE(".XmlReader.Node.Attributes", 0), GB_VIRTUAL_CLASS(),

    GB_METHOD("_get", "s", CReaderNodeAttr_get, "(Name)s"),
    GB_METHOD("_put", "s", CReaderNodeAttr_put, "(Value)s(Name)s"),
    GB_METHOD("_next", "s", CReaderNodeAttr_next, ""),
    GB_PROPERTY_READ("Count", "i", CReaderNodeAttr_count),

	GB_END_DECLARE
};

GB_DESC CReaderNodeDesc[] =
{
	GB_DECLARE(".XmlReader.Node", 0), GB_VIRTUAL_CLASS(),

    GB_PROPERTY_SELF("Attributes", ".XmlReader.Node.Attributes"),
	//GB_PROPERTY_READ("BaseUri","s",CRNODE_BaseUri),
	GB_PROPERTY_READ("Depth","i",CReader_Depth),
	//GB_PROPERTY_READ("IsDefault","b",CRNODE_IsDefault),
	//GB_PROPERTY_READ("IsEmptyElement","b",CRNODE_IsEmptyElement),
	//GB_PROPERTY_READ("LocalName","s",CRNODE_LocalName),
	GB_PROPERTY_READ("Name", "s", CReaderNode_Name),
	//GB_PROPERTY_READ("NamespaceUri", "s", CRNODE_NamespaceUri),
	//GB_PROPERTY_READ("Prefix", "s", CRNODE_Prefix),
	//GB_PROPERTY_READ("QuoteChar", "s", CRNODE_QuoteChar),
	GB_PROPERTY_READ("Type","i",CReaderNode_type),
	GB_PROPERTY_READ("Value", "s", CReaderNode_Value),
	//GB_PROPERTY_READ("XmlLang", "s", CRNODE_XmlLang),

	GB_END_DECLARE
};

GB_DESC CReaderDesc[] =
{
    GB_DECLARE("_XmlReader", sizeof(CReader)),

    GB_METHOD("_new", "", CReader_new, ""),
    GB_METHOD("_free", "", CReader_free, ""),
    GB_METHOD("_ReadChar", "i", CReader_ReadChar, "(Char)s"),

    GB_PROPERTY("KeepData", "b", CReader_keepData),
    GB_PROPERTY_READ("Pos", "i", CReader_pos),
    GB_PROPERTY_SELF("Node", ".XmlReader.Node"),
    GB_PROPERTY_READ("StoredNodes", "XmlNode[]", CReader_storedNodes),
    GB_PROPERTY_READ("Depth","i",CReader_Depth),
    
    GB_PROPERTY_SELF("ReadFlags", ".XmlReaderReadFlags"),

    GB_END_DECLARE
};
