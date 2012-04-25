#include "CDocument.h"

/*========== Document */

#undef THIS
#define THIS (static_cast<HtmlDocument*>(_object))

BEGIN_METHOD(CDocument_new, GB_STRING path)

if(!MISSING(path))
{
    char *content; int len;
    if(GB.LoadFile(CSTRING(path), LENGTH(path), &content, &len)) return;
    try
    {
        THIS->setContent(StringToWString(content));
    }
    catch(HTMLParseException &e)
    {
        GB.Error(e.what());
    }
}
else
{
THIS->root->setTagName(L"html");

Element *head = GBI::New<Element>("XmlElement");
head->setTagName(L"head");
THIS->root->appendChild(head);

Element *body = GBI::New<Element>("XmlElement");
body->setTagName(L"body");
THIS->root->appendChild(body);

//Meta utf-8
Element *meta = GBI::New<Element>("XmlElement");
meta->setTagName(L"meta");
meta->setAttribute(L"charset", L"utf-8");
head->appendChild(meta);

meta = GBI::New<Element>("XmlElement");
meta->setTagName(L"meta");
meta->setAttribute(L"http-equiv", L"Content-Type");
meta->setAttribute(L"content",L"text/html; charset=utf-8");
head->appendChild(meta);

//Title
Element *title = GBI::New<Element>("XmlElement");
title->setTagName(L"title");
head->appendChild(title);

}

THIS->virt = new HtmlDocument::Virtual(THIS);

END_METHOD

BEGIN_METHOD_VOID(CDocument_free)

GB.Unref(POINTER(&(THIS->root)));

END_METHOD

BEGIN_PROPERTY(CDocument_Html5)

if(READ_PROPERTY)
{
    GB.ReturnBoolean(THIS->html5);
}
else
{
    THIS->html5 = VPROP(GB_BOOLEAN);
}

END_PROPERTY

BEGIN_PROPERTY(CDocument_content)

if(READ_PROPERTY)
{
    GB.ReturnNewZeroString(WStringToString(THIS->getContent()).c_str());
}
else
{
if(PLENGTH() <= 0) return;
    try
    {
        THIS->setContent(PSTRING());
    }
    catch(HTMLParseException &e)
    {
        GB.Error(e.what());
    }
}

END_PROPERTY

BEGIN_PROPERTY(CDocument_Title)

if(READ_PROPERTY)
{
    GB.ReturnNewZeroString(WStringToString(THIS->getTitle()).c_str());
}
else
{
if(PLENGTH() <= 0) return;
    THIS->setTitle(PSTRING());
}

END_PROPERTY

BEGIN_PROPERTY(CDocument_favicon)

if(READ_PROPERTY)
{
    GB.ReturnNewZeroString(WStringToString(THIS->getFavicon()).c_str());
}
else
{
if(PLENGTH() <= 0) return;
THIS->setFavicon(PSTRING());
}

END_PROPERTY

BEGIN_PROPERTY(CDocument_lang)

if(READ_PROPERTY)
{
    GB.ReturnNewZeroString(WStringToString(THIS->getLang()).c_str());
}
else
{
if(PLENGTH() <= 0) return;
    THIS->setLang(PSTRING());
}

END_PROPERTY

BEGIN_PROPERTY(CDocument_root)

GB.ReturnObject(THIS->getRoot());

END_PROPERTY

BEGIN_PROPERTY(CDocument_head)

GB.ReturnObject(THIS->getHead());

END_PROPERTY

BEGIN_PROPERTY(CDocument_body)

GB.ReturnObject(THIS->getBody());

END_PROPERTY

BEGIN_METHOD(CDocument_getElementById, GB_STRING id; GB_INTEGER depth)

GB.ReturnObject(THIS->getElementById(STRING(id), VARGOPT(depth, -1)));

END_METHOD

BEGIN_METHOD(CDocument_createElement, GB_STRING tagName)

if(LENGTH(tagName) <= 0) return;
GB.ReturnObject(THIS->createElement(MISSING(tagName) ? L"" : STRING(tagName)));

END_METHOD

BEGIN_PROPERTY(CDocument_All)

GB.ReturnObject(THIS->getAll()->array);

END_PROPERTY

BEGIN_METHOD(CDocument_getElementsByClassName, GB_STRING className; GB_INTEGER depth)

if(LENGTH(className) <= 0) return;
GB.ReturnObject(THIS->getElementsByClassName(STRING(className), VARGOPT(depth, -1))->array);

END_METHOD

BEGIN_METHOD(CDocument_getElementsByTagName, GB_STRING className; GB_INTEGER depth)

GB.ReturnObject(THIS->getGBElementsByTagName(STRING(className), VARGOPT(depth, -1))->array);

END_METHOD

BEGIN_METHOD(CDocumentStyleSheets_add, GB_STRING path; GB_STRING media)

THIS->AddStyleSheet(STRING(path), MISSING(media) ? L"screen" : STRING(media));

END_METHOD

BEGIN_METHOD(CDocumentStyleSheets_addIfNotIE, GB_STRING path; GB_STRING media)

THIS->AddStyleSheetIfNotIE(STRING(path), MISSING(media) ? L"screen" : STRING(media));

END_METHOD

BEGIN_METHOD(CDocumentStyleSheets_addIfIE, GB_STRING path; GB_STRING cond; GB_STRING media)

THIS->AddStyleSheetIfIE(STRING(path), STRINGOPT(cond, L"IE"), STRINGOPT(media, L"screen"));

END_METHOD

BEGIN_METHOD(CDocumentScripts_add, GB_STRING path)

THIS->AddScript(STRING(path));

END_METHOD

BEGIN_METHOD(CDocumentScripts_addIfNotIE, GB_STRING path)

THIS->AddScriptIfNotIE(STRING(path));

END_METHOD

BEGIN_METHOD(CDocumentScripts_addIfIE, GB_STRING path; GB_STRING cond)

THIS->AddScriptIfIE(STRING(path), STRINGOPT(cond, L"IE"));

END_METHOD

/*BEGIN_METHOD(CDocument_forceSetContent, GB_STRING data)

try
{
    THIS->setContent(STRING(data), true);
}
catch(HTMLParseException &e)
{
    GB.Error(e.what());
}

END_METHOD*/

BEGIN_PROPERTY(CDocument_base)

if(READ_PROPERTY)
{
    GB.ReturnNewZeroString(WStringToString(THIS->getBase()).c_str());
}
else
{
    THIS->setBase(PSTRING());
}

END_PROPERTY


GB_DESC CDocumentStyleSheetsDesc[] =
{
    GB_DECLARE(".HtmlDocumentStyleSheets", 0), GB_VIRTUAL_CLASS(),

    GB_METHOD("Add", "", CDocumentStyleSheets_add, "(Source)s[(Media)s]"),
    GB_METHOD("AddIfIE", "", CDocumentStyleSheets_addIfIE, "(Source)s[(Condition)s(Media)s]"),
    GB_METHOD("AddIfNotIE", "", CDocumentStyleSheets_addIfNotIE, "(Source)s[(Media)s]"),

    GB_END_DECLARE
};

GB_DESC CDocumentScriptsDesc[] =
{
    GB_DECLARE(".HtmlDocumentScripts", 0), GB_VIRTUAL_CLASS(),

    GB_METHOD("Add", "", CDocumentScripts_add, "(Source)s"),
    GB_METHOD("AddIfIE", "", CDocumentScripts_addIfIE, "(Source)s[(Condition)s]"),
    GB_METHOD("AddIfNotIE", "", CDocumentScripts_addIfNotIE, "(Source)s"),

    GB_END_DECLARE
};

GB_DESC CDocumentDesc[] =
{
    GB_DECLARE("HtmlDocument", sizeof(HtmlDocument)), GB_INHERITS("XmlDocument"),

    GB_METHOD("_new", "", CDocument_new, "[(Path)s]"),
    GB_METHOD("_free", "", CDocument_free, ""),

    GB_PROPERTY("Html5", "b", CDocument_Html5),
    //GB_METHOD("ForceSetContent", "", CDocument_forceSetContent, "(Data)s"),
    GB_PROPERTY("Title", "s", CDocument_Title),
    GB_PROPERTY("Favicon", "s", CDocument_favicon),
    GB_PROPERTY("Lang", "s", CDocument_lang),
    GB_PROPERTY("Base", "s", CDocument_base),
    GB_PROPERTY_READ("Head", "XmlElement", CDocument_head),
    GB_PROPERTY_READ("Body", "XmlElement", CDocument_body),

    GB_PROPERTY_SELF("StyleSheets", ".HtmlDocumentStyleSheets"),
    GB_PROPERTY_SELF("Scripts", ".HtmlDocumentScripts"),

    GB_METHOD("GetElementById", "XmlElement", CDocument_getElementById, "(Id)s[(Depth)i]"),
    GB_METHOD("GetElementsByClassName", "XmlElement[]", CDocument_getElementsByClassName, "(ClassName)s[(Depth)i]"),


    GB_END_DECLARE
};
