/***************************************************************************

  c_pdf_document.cpp

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

#define __C_PDF_DOCUMENT_CPP

#if defined __has_include
#  if __has_include (<string_view>)
#    include <string_view>
#  endif
#endif

#include <Page.h>

#include "c_pdf_document.h"

struct _PopplerPage
{
  GObject parent_instance;
  PopplerDocument *document;
  Page *page;
  int index;
  void *text;
};

#define GET_CURRENT_PAGE() (((_PopplerPage *)THIS->current)->page)

struct _PopplerDocument
{
  GObject parent_instance;
  void *initer;
  PDFDoc *doc;

  GList *layers;
  GList *layers_rbgroups;
  void *output_dev;
};

#define GET_DOCUMENT() (((_PopplerDocument *)THIS->doc)->doc)

/*static poppler::rotation_enum conv_rotation(int angle)
{
	if (angle < 0)
		angle = 360 - (-angle) % 360;
	else
		angle = angle % 360;
	
	switch (angle)
	{
		case 0: return poppler::rotate_0;
		case 90: return poppler::rotate_90;
		case 180: return poppler::rotate_180;
		case 270: return poppler::rotate_270;
		default: return poppler::rotate_0;
	}
}*/

static GEOM_RECTF *make_rect(PopplerRectangle *rect)
{
	GEOM_RECTF *ob = GEOM.CreateRectF();
	ob->x = rect->x1;
	ob->y = rect->y1;
	ob->w = rect->x2 - rect->x1;
	ob->h = rect->y2 - rect->y1;
	return ob;
}

//--------------------------------------------------------------------------

BEGIN_METHOD(PdfDocument_new, GB_STRING path; GB_STRING password)

	const char *password;
	GError *error = NULL;
	std::string rpasswd;

	if (GB.LoadFile(STRING(path), LENGTH(path), &THIS->buffer, &THIS->length))
		return;

	if (MISSING(password))
		password = NULL;
	else
		password = GB.ToZeroString(ARG(password));
	
#if POPPLER_CHECK_VERSION(0,82,0)
	GBytes *bytes = g_bytes_new_static(THIS->buffer, THIS->length);
	THIS->doc = poppler_document_new_from_bytes(bytes, password, &error);
	g_bytes_unref(bytes);
#else
	THIS->doc = poppler_document_new_from_data(THIS->buffer, THIS->length, password, &error);
#endif
	if (!THIS->doc)
	{
		GB.Error(error->message);
		return;
	}
	
	THIS->resolution = 72.0;
	
	if (password)
		rpasswd = password;
	THIS->rdoc = poppler::document::load_from_raw_data(THIS->buffer, THIS->length, rpasswd, rpasswd);
	
	THIS->renderer = new poppler::page_renderer;
	
	THIS->renderer->set_render_hint(poppler::page_renderer::antialiasing, true);
	THIS->renderer->set_render_hint(poppler::page_renderer::text_antialiasing, true);
	THIS->renderer->set_render_hint(poppler::page_renderer::text_hinting, false);

END_METHOD

BEGIN_METHOD_VOID(PdfDocument_free)

	int i;
	
	if (THIS->doc)
	{
		if (THIS->pages)
		{
			for (i = 0; i < poppler_document_get_n_pages(THIS->doc); i++)
			{
				if (THIS->pages[i])
					g_object_unref(THIS->pages[i]);
			}

			GB.Free(POINTER(&THIS->pages));
		}

		if (THIS->index)
		{
			for (i = 0; i < GB.Count(THIS->index); i++)
				GB.Unref(POINTER(&THIS->index[i]));

			GB.FreeArray(POINTER(&THIS->index));
		}

		delete THIS->renderer;
		delete THIS->rdoc;
		g_object_unref(THIS->doc);
	}
	
	GB.ReleaseFile(THIS->buffer, THIS->length);
	
END_METHOD

BEGIN_PROPERTY(PdfDocument_Count)

	GB.ReturnInteger(poppler_document_get_n_pages(THIS->doc));

END_PROPERTY

BEGIN_PROPERTY(PdfDocument_Max)

	GB.ReturnInteger(poppler_document_get_n_pages(THIS->doc) - 1);

END_PROPERTY

#define IMPLEMENT_DOC_STRING_PROP(_name, _func) \
BEGIN_PROPERTY(PdfDocument_##_name) \
	GB.ReturnNewZeroString(poppler_document_get_##_func(THIS->doc)); \
END_PROPERTY

IMPLEMENT_DOC_STRING_PROP(Author, author)
IMPLEMENT_DOC_STRING_PROP(Creator, creator)
IMPLEMENT_DOC_STRING_PROP(Producer, producer)
IMPLEMENT_DOC_STRING_PROP(Subject, subject)
IMPLEMENT_DOC_STRING_PROP(Title, title)
IMPLEMENT_DOC_STRING_PROP(Keywords, keywords)

#define IMPLEMENT_DOC_DATE_PROP(_name, _func) \
BEGIN_PROPERTY(PdfDocument_##_name) \
	GB_DATE date; \
	GB.MakeDateFromTime(poppler_document_get_##_func(THIS->doc), 0, &date); \
	GB.ReturnDate(&date); \
END_PROPERTY

IMPLEMENT_DOC_DATE_PROP(CreationDate, creation_date)
IMPLEMENT_DOC_DATE_PROP(ModificationDate, modification_date)

BEGIN_PROPERTY(PdfDocument_Version)

	guint major, minor;
	char buf[32];
	int len;
	
	poppler_document_get_pdf_version(THIS->doc, &major, &minor);

	len = snprintf(buf, sizeof(buf), "%d.%d", major, minor);
	GB.ReturnNewString(buf, len);

END_PROPERTY

BEGIN_PROPERTY(PdfDocument_Linearized)

	GB.ReturnBoolean(poppler_document_is_linearized(THIS->doc));

END_PROPERTY

BEGIN_METHOD(PdfDocument_get, GB_INTEGER index)

	int index = VARG(index);
	
	if (index < 0 || index >= poppler_document_get_n_pages(THIS->doc))
	{
		GB.Error(GB_ERR_BOUND);
		return;
	}

	if (!THIS->pages)
		GB.AllocZero(POINTER(&THIS->pages), sizeof(void *) * poppler_document_get_n_pages(THIS->doc));
	
	if (!THIS->pages[index])
		THIS->pages[index] = poppler_document_get_page(THIS->doc, index);
	
	THIS->current = THIS->pages[index];
	
	RETURN_SELF();

END_METHOD

BEGIN_PROPERTY(PdfDocument_Resolution)

	if (READ_PROPERTY)
		GB.ReturnFloat(THIS->resolution);
	else
	{
		double res = VPROP(GB_FLOAT);
		
		if (res <= 0)
		{
			GB.Error(GB_ERR_ARG);
			return;
		}
		THIS->resolution = res;
	}

END_PROPERTY

BEGIN_PROPERTY(PdfDocument_Rotation)

	if (READ_PROPERTY)
		GB.ReturnInteger(THIS->rotation);
	else
		THIS->rotation = VPROP(GB_INTEGER);

END_PROPERTY

static int fill_index(void *_object, PopplerIndexIter *iter, int parent)
{
	PopplerIndexIter *child;
	CPDFACTION *action;
	int n = 0;
	GB_CLASS class_action = GB.FindClass("PdfAction");
	
  do
    {
			CPDFINDEX *item = (CPDFINDEX *)GB.New(GB.FindClass("PdfIndex"), NULL, NULL);

			item->index = GB.Count(THIS->index);
			item->parent = parent;
			item->opened = poppler_index_iter_is_open(iter);
			
			action = (CPDFACTION *)GB.New(class_action, NULL, NULL);
			action->action = poppler_index_iter_get_action(iter);
			GB.Ref(action);
			item->action = action;
			
			*(void **)GB.Add(&THIS->index) = item;
			GB.Ref(item);
			n++;
			
      child = poppler_index_iter_get_child(iter);
      if (child)
			{
        item->children = fill_index(_object, child, item->index);
				poppler_index_iter_free(child);
			}
			
    }
  while (poppler_index_iter_next(iter));
	
	return n;
}

BEGIN_PROPERTY(PdfDocument_Index)

	PopplerIndexIter *iter;

	if (!THIS->index)
	{
		GB.NewArray(&THIS->index, sizeof(void *), 0);
		iter = poppler_index_iter_new(THIS->doc);
		if (iter)
		{
			fill_index(THIS, iter, -1);
			poppler_index_iter_free(iter);
		}
	}

	RETURN_SELF();

END_PROPERTY

BEGIN_METHOD(PdfDocument_Find, GB_STRING label)

	PopplerPage *page = poppler_document_get_page_by_label(THIS->doc, GB.ToZeroString(ARG(label)));

	if (!page)
		GB.ReturnInteger(-1);
	else
		GB.ReturnInteger(poppler_page_get_index(page));
	
END_METHOD

#define IMPLEMENT_DOC_HINT_PROP(_name, _hint) \
BEGIN_PROPERTY(PdfDocument_##_name) \
	if (READ_PROPERTY) \
		GB.ReturnBoolean(THIS->renderer->render_hints() & poppler::page_renderer::_hint); \
	else \
		THIS->renderer->set_render_hint(poppler::page_renderer::_hint, VPROP(GB_BOOLEAN)); \
END_PROPERTY

IMPLEMENT_DOC_HINT_PROP(Antialiasing, antialiasing)
IMPLEMENT_DOC_HINT_PROP(TextAntialiasing, text_antialiasing)
IMPLEMENT_DOC_HINT_PROP(TextHinting, text_hinting)

//--------------------------------------------------------------------------

/*BEGIN_PROPERTY(PdfPage_Orientation)

	GB.ReturnInteger(THIS->current->orientation());

END_PROPERTY*/

BEGIN_METHOD(PdfPage_Render, GB_INTEGER x; GB_INTEGER y; GB_INTEGER width; GB_INTEGER height; GB_INTEGER rotation; GB_FLOAT res)

	poppler::page *page;
	poppler::rectf size;
	poppler::rotation_enum rot;
	poppler::image image;
	
	const char *data = NULL;
	int rotation = VARGOPT(rotation, THIS->rotation);
	int orientation = 0;
	double res = VARGOPT(res, THIS->resolution);
	int width, height;
	int x, y, w, h;
	
	page = THIS->rdoc->create_page(poppler_page_get_index(THIS->current));
	
	/*switch (page->orientation())
	{
		case poppler::page::portrait: orientation = 0; break;
		case poppler::page::landscape: orientation = 90; break;
		case poppler::page::upside_down: orientation = 180; break;
		case poppler::page::seascape: orientation = 270; break;
	}*/

	orientation = (orientation + rotation + 720) % 360;
	
	switch (orientation)
	{
		case 90: rot = poppler::rotate_90; break;
		case 180: rot = poppler::rotate_180; break;
		case 270: rot = poppler::rotate_270; break;
		default: rot = poppler::rotate_0;
	}
	
	size = page->page_rect(poppler::media_box);
	
	if (orientation % 180)
	{
		width = (int)(size.height() * res / 72.0);
		height = (int)(size.width() * res / 72.0);
	}
	else
	{
		width = (int)(size.width() * res / 72.0);
		height = (int)(size.height() * res / 72.0);
	}
	
	x = VARGOPT(x, 0);
	y = VARGOPT(y, 0);
	w = VARGOPT(width, width);
	h = VARGOPT(height, height);
	
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	
	if ((x + w) > width)
		w = width - x;

	if ((y + h) > height)
		h = height - y;
	
	if (w > 0 && h > 0)
	{
		image = THIS->renderer->render_page(page, res, res, x, y, w, h, rot);
		data = image.const_data();
	}

	GB.ReturnObject(IMAGE.Create(w, h, GB_IMAGE_BGRA, (unsigned char *)data));
		
END_METHOD

BEGIN_PROPERTY(PdfPage_Thumbnail)

  GB.ReturnNull();

END_PROPERTY

BEGIN_PROPERTY(PdfPage_Label)

	GB.ReturnNewZeroString(poppler_page_get_label(THIS->current));

END_PROPERTY

BEGIN_PROPERTY(PdfPage_Text)

	GB.ReturnNewZeroString(poppler_page_get_text(THIS->current));

END_METHOD

BEGIN_METHOD(PdfPage_GetText, GB_FLOAT x; GB_FLOAT y; GB_FLOAT w; GB_FLOAT h)

	PopplerRectangle rect;
	
	rect.x1 = VARG(x);
	rect.y1 = VARG(y);
	rect.x2 = rect.x1 + VARG(w);
	rect.y2 = rect.y1 + VARG(h);
	
	GB.ReturnNewZeroString(poppler_page_get_selected_text(THIS->current, POPPLER_SELECTION_GLYPH, &rect));

END_METHOD

BEGIN_PROPERTY(PdfPage_Width)

	double w;
	poppler_page_get_size(THIS->current, &w, NULL);
	GB.ReturnFloat(w);

END_PROPERTY

BEGIN_PROPERTY(PdfPage_Height)

	double h;
	poppler_page_get_size(THIS->current, NULL, &h);
	GB.ReturnFloat(h);

END_PROPERTY

BEGIN_METHOD(PdfPage_FindText, GB_STRING search; GB_INTEGER options)

	GList *rects, *r;
	GB_ARRAY result;
	GEOM_RECTF *rect;
	
	rects = r = poppler_page_find_text_with_options(THIS->current, GB.ToZeroString(ARG(search)), (PopplerFindFlags)VARGOPT(options, POPPLER_FIND_DEFAULT));
	
	GB.Array.New(&result, GB.FindClass("RectF"), 0);
	
	while (r)
	{
		rect = make_rect((PopplerRectangle *)r->data);
		GB.Ref(rect);
		*(GEOM_RECTF **)GB.Array.Add(result) = rect;
		r = r->next;
	}
	
	g_list_free(rects);
	
	GB.ReturnObject(result);

END_METHOD

//--------------------------------------------------------------------------

BEGIN_PROPERTY(PdfDocumentIndex_Count)

	GB.ReturnInteger(GB.Count(THIS->index));

END_PROPERTY

BEGIN_PROPERTY(PdfDocumentIndex_Max)

	GB.ReturnInteger(GB.Count(THIS->index) - 1);

END_PROPERTY

BEGIN_METHOD(PdfDocumentIndex_get, GB_INTEGER index)

	int index = VARG(index);
	
	if (index < 0 || index >= GB.Count(THIS->index))
	{
		GB.Error(GB_ERR_BOUND);
		return;
	}
	
	GB.ReturnObject(THIS->index[index]);

END_PROPERTY

BEGIN_METHOD_VOID(PdfDocumentIndex_next)

	int *index = (int *)GB.GetEnum();

	if (*index >= GB.Count(THIS->index))
		GB.StopEnum();
	else
	{
		GB.ReturnObject(THIS->index[*index]);
		(*index)++;
	}

END_METHOD

//--------------------------------------------------------------------------

BEGIN_METHOD_VOID(PdfIndex_free)

	GB.Unref(POINTER(&THIS_INDEX->action));

END_METHOD

BEGIN_PROPERTY(PdfIndex_Parent)

	GB.ReturnInteger(THIS_INDEX->parent);

END_PROPERTY

BEGIN_PROPERTY(PdfIndex_Children)

	GB.ReturnInteger(THIS_INDEX->children);

END_PROPERTY

BEGIN_PROPERTY(PdfIndex_Opened)

	GB.ReturnBoolean(THIS_INDEX->opened);

END_PROPERTY

BEGIN_PROPERTY(PdfIndex_Action)

	GB.ReturnObject(THIS_INDEX->action);

END_PROPERTY

BEGIN_PROPERTY(PdfIndex_Text)

	GB.ReturnNewZeroString(THIS_INDEX->action->action->any.title);

END_PROPERTY


//--------------------------------------------------------------------------

BEGIN_METHOD_VOID(PdfAction_free)

	poppler_action_free(ACTION);

END_METHOD

BEGIN_PROPERTY(PdfAction_Type)

	const char *type;

	switch(ACTION->type)
	{
		case POPPLER_ACTION_NONE: type = "None"; break;
		case POPPLER_ACTION_GOTO_DEST: type = "GotoDest"; break;
		case POPPLER_ACTION_GOTO_REMOTE: type = "GotoRemote"; break;
		case POPPLER_ACTION_LAUNCH: type = "Launch"; break;
		case POPPLER_ACTION_URI: type = "URI"; break;
		case POPPLER_ACTION_NAMED: type = "Named"; break;
		case POPPLER_ACTION_MOVIE: type = "Movie"; break;
		case POPPLER_ACTION_RENDITION: type = "Rendition"; break;
		case POPPLER_ACTION_OCG_STATE: type = "OGCState"; break;
		case POPPLER_ACTION_JAVASCRIPT: type =" Javascript"; break;
		default: type = NULL;
	}

	GB.ReturnConstZeroString(type);

END_PROPERTY

BEGIN_PROPERTY(PdfAction_Text)

	GB.ReturnNewZeroString(ACTION->any.title);

END_PROPERTY

static PopplerDest *get_dest(PopplerAction *action)
{
	switch(action->type)
	{
		case POPPLER_ACTION_GOTO_DEST: return action->goto_dest.dest;
		case POPPLER_ACTION_GOTO_REMOTE: return action->goto_remote.dest;
		default: return NULL;
	}
}

BEGIN_PROPERTY(PdfAction_Page)

	PopplerDest *dest = get_dest(ACTION);
	
	if (dest)
		GB.ReturnInteger(dest->page_num);
	else
		GB.ReturnInteger(-1);
	
END_PROPERTY

BEGIN_PROPERTY(PdfAction_Zoom)

	PopplerDest *dest = get_dest(ACTION);
	
	if (dest)
		GB.ReturnFloat(dest->zoom);
	else
		GB.ReturnFloat(0);
	
END_PROPERTY

BEGIN_PROPERTY(PdfAction_Rect)

	PopplerDest *dest = get_dest(ACTION);
	
	if (dest)
	{
		GEOM_RECTF *rect = GEOM.CreateRectF();
		rect->x = dest->left;
		rect->y = dest->top;
		rect->w = dest->right - dest->left;
		rect->h = dest->bottom - dest->top;
		GB.ReturnObject(rect);
	}
	else
		GB.ReturnNull();

END_PROPERTY

BEGIN_PROPERTY(PdfAction_Target)

	const char *target;

	switch(ACTION->type)
	{
		case POPPLER_ACTION_GOTO_REMOTE: target = ACTION->goto_remote.file_name; break;
		case POPPLER_ACTION_LAUNCH: target = ACTION->launch.file_name; break;
		case POPPLER_ACTION_URI: target = ACTION->uri.uri; break;
		case POPPLER_ACTION_NAMED: target = ACTION->named.named_dest; break;
		default: target = NULL;
	}
	
	GB.ReturnNewZeroString(target);

END_PROPERTY

BEGIN_PROPERTY(PdfAction_Arguments)

	const char *args;

	switch(ACTION->type)
	{
		case POPPLER_ACTION_LAUNCH: args = ACTION->launch.params; break;
		default: args = NULL;
	}

	GB.ReturnNewZeroString(args);

END_PROPERTY

//--------------------------------------------------------------------------

GB_DESC PdfActionDesc[] = 
{
	GB_DECLARE("PdfAction", sizeof(CPDFACTION)),
	
	GB_METHOD("_free", NULL, PdfAction_free, NULL),
	GB_PROPERTY_READ("Type", "s", PdfAction_Type),
	GB_PROPERTY_READ("Text", "s", PdfAction_Text),
	GB_PROPERTY_SELF("Goto", ".PdfActionGoto"),
	GB_PROPERTY_SELF("Launch", ".PdfActionLaunch"),
	GB_PROPERTY_SELF("URI", ".PdfActionURI"),
	
	GB_END_DECLARE
};

GB_DESC PdfActionGotoDesc[] = 
{
	GB_DECLARE_VIRTUAL(".PdfActionGoto"),
	
	GB_PROPERTY_READ("Page", "i", PdfAction_Page),
	GB_PROPERTY_READ("Zoom", "f", PdfAction_Zoom),
	GB_PROPERTY_READ("Rect", "RectF", PdfAction_Rect),
	GB_PROPERTY_READ("Target", "s", PdfAction_Target),
						 
	GB_END_DECLARE
};

GB_DESC PdfActionLaunchDesc[] = 
{
	GB_DECLARE_VIRTUAL(".PdfActionLaunch"),
	
	GB_PROPERTY_READ("Target", "s", PdfAction_Target),
	GB_PROPERTY_READ("Arguments", "s", PdfAction_Arguments),
						 
	GB_END_DECLARE
};

GB_DESC PdfActionURIDesc[] = 
{
	GB_DECLARE_VIRTUAL(".PdfActionURI"),
	
	GB_PROPERTY_READ("Target", "s", PdfAction_Target),
						 
	GB_END_DECLARE
};

GB_DESC PdfIndexDesc[] = 
{
	GB_DECLARE("PdfIndex", sizeof(CPDFINDEX)),
	
	GB_METHOD("_free", NULL, PdfIndex_free, NULL),
	
	GB_PROPERTY_READ("Action", "PdfAction", PdfIndex_Action),
	GB_PROPERTY_READ("Text", "s", PdfIndex_Text),
	GB_PROPERTY_READ("Parent", "i", PdfIndex_Parent),
	GB_PROPERTY_READ("Children", "i", PdfIndex_Children),
	GB_PROPERTY_READ("Opened", "b", PdfIndex_Opened),
	
	GB_END_DECLARE
};

GB_DESC PdfDocumentIndexDesc[] =
{
	GB_DECLARE_VIRTUAL(".PdfDocumentIndex"),
	
	GB_PROPERTY_READ("Count", "i", PdfDocumentIndex_Count),
	GB_PROPERTY_READ("Max", "i", PdfDocumentIndex_Max),
	GB_METHOD("_get", "PdfIndex", PdfDocumentIndex_get, "(Index)i"),
	GB_METHOD("_next", "PdfIndex", PdfDocumentIndex_next, NULL),
	
	GB_END_DECLARE
};

GB_DESC PdfPageDesc[] =
{
	GB_DECLARE_VIRTUAL(".PdfPage"),
	
	//GB_PROPERTY_READ("Orientation", "i", PdfPage_Orientation),
	
	GB_METHOD("Render", "Image", PdfPage_Render, "[(X)i(Y)i(Width)i(Height)i(Rotation)i(Resolution)f]"),
	GB_PROPERTY_READ("Label", "s", PdfPage_Label),
	GB_PROPERTY_READ("Text", "s", PdfPage_Text),
	GB_METHOD("GetText", "s", PdfPage_GetText,"(X)f(Y)f(Width)f(Height)f"),
	GB_PROPERTY_READ("Width", "f", PdfPage_Width),
	GB_PROPERTY_READ("Height", "f", PdfPage_Height),
	GB_PROPERTY_READ("W", "f", PdfPage_Width),
	GB_PROPERTY_READ("H", "f", PdfPage_Height),
	GB_METHOD("FindText", "RectF[]", PdfPage_FindText, "(Search)s[(Options)i]"),
	GB_PROPERTY_READ("Thumbnail", "Image", PdfPage_Thumbnail),
	
	GB_END_DECLARE
};

GB_DESC PdfDocumentDesc[] =
{
	GB_DECLARE("PdfDocument", sizeof(CPDFDOCUMENT)),

	/*GB_CONSTANT("Unknown","i",actionUnknown),
	GB_CONSTANT("Goto","i",actionGoTo),
	GB_CONSTANT("GotoRemote","i",actionGoToR),
	GB_CONSTANT("Launch","i",actionLaunch),
	GB_CONSTANT("Uri","i",actionURI),
	GB_CONSTANT("Named","i",actionNamed),
	GB_CONSTANT("Movie","i",actionMovie),

	GB_CONSTANT("Normal","i",0),
	GB_CONSTANT("Sideways","i",90),
	GB_CONSTANT("Inverted","i",180),
	GB_CONSTANT("SidewaysInverted","i",270),*/

	GB_METHOD("_new", 0, PdfDocument_new, "(Path)s[(Owner)s(Password)s]"),
	GB_METHOD("_free", 0, PdfDocument_free, 0),

	GB_METHOD("_get", ".PdfPage", PdfDocument_get, "(Index)i"),
	GB_METHOD("Find", "i", PdfDocument_Find, "(Label)s"),

	//GB_PROPERTY("Zoom", "f", PDFDOCUMENT_scale),
	//GB_PROPERTY("Orientation", "i", PDFDOCUMENT_rotation),

	//GB_PROPERTY_READ("Ready","b",PDFDOCUMENT_ready),
	GB_PROPERTY_READ("Count","i",PdfDocument_Count),
	GB_PROPERTY_READ("Max","i",PdfDocument_Max),
	//GB_PROPERTY_READ("HasIndex","b",PDFDOCUMENT_has_index),
	//GB_PROPERTY_READ("Index",".PdfDocument.Index",PDFDOCUMENT_index),
	//GB_PROPERTY_READ("Info",".PdfDocument.Info",PDFDOCUMENT_info),
	
	GB_PROPERTY_READ("Author", "s", PdfDocument_Author),
	GB_PROPERTY_READ("Creator", "s", PdfDocument_Creator),
	GB_PROPERTY_READ("Producer", "s", PdfDocument_Producer),
	GB_PROPERTY_READ("Subject", "s", PdfDocument_Subject),
	GB_PROPERTY_READ("Title", "s", PdfDocument_Title),
	GB_PROPERTY_READ("Keywords", "s", PdfDocument_Keywords),

	GB_PROPERTY_READ("CreationDate", "d", PdfDocument_CreationDate),
	GB_PROPERTY_READ("ModificationDate", "d", PdfDocument_ModificationDate),

	GB_PROPERTY_READ("Version", "s", PdfDocument_Version),
	GB_PROPERTY_READ("Linearized", "b", PdfDocument_Linearized),
	
	GB_PROPERTY("Resolution", "f", PdfDocument_Resolution),
	GB_PROPERTY("Rotation", "i", PdfDocument_Rotation),
	
	GB_PROPERTY("Antialiasing", "b", PdfDocument_Antialiasing),
	GB_PROPERTY("TextAntialiasing", "b", PdfDocument_TextAntialiasing),
	GB_PROPERTY("TextHinting", "b", PdfDocument_TextHinting),
	
	GB_PROPERTY_READ("Index", ".PdfDocumentIndex", PdfDocument_Index),
	
	GB_END_DECLARE
};

GB_DESC PdfDesc[] = 
{
	GB_DECLARE_STATIC("Pdf"),
	
	/*GB_CONSTANT("Landscape", "i", poppler::page::landscape),
	GB_CONSTANT("Portrait", "i", poppler::page::portrait),
	GB_CONSTANT("Seascape", "i", poppler::page::seascape),
	GB_CONSTANT("UpsideDown", "i", poppler::page::upside_down),*/

	GB_CONSTANT("CaseSensitive", "i", POPPLER_FIND_CASE_SENSITIVE),
	GB_CONSTANT("Backwards", "i", POPPLER_FIND_BACKWARDS),
	GB_CONSTANT("WholeWordsOnly", "i", POPPLER_FIND_WHOLE_WORDS_ONLY),
#if POPPLER_CHECK_VERSION(0,73,0)
	GB_CONSTANT("IgnoreDiacritics", "i", POPPLER_FIND_IGNORE_DIACRITICS),
#endif

	GB_END_DECLARE
};
