/***************************************************************************

	csvgimage.cpp

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

#define __CSVGIMAGE_CPP

#include <cairo.h>
#include <cairo-svg.h>

#include "main.h"
#include "gambas.h"
#include "widgets.h"
#include "cpaint_impl.h"
#include "csvgimage.h"

#define MM_TO_PT(_mm) ((_mm) * 72 / 25.4)
#define PT_TO_MM(_pt) ((_pt) / 72 * 25.4)

static void release(CSVGIMAGE *_object)
{
	if (HANDLE)
	{
		g_object_unref(G_OBJECT(HANDLE));
		HANDLE = NULL;
	}

	if (SURFACE)
	{
		cairo_surface_destroy(SURFACE);
		THIS->surface = NULL;
	}

	THIS->width = THIS->height = 0;
}


static const char *load_file(CSVGIMAGE *_object, const char *path, int len_path)
{
	RsvgHandle *handle = NULL;
	char *addr;
	int len;
	const char *err = NULL;

	if (GB.LoadFile(path, len_path, &addr, &len))
		return "Unable to load SVG file";

	handle = rsvg_handle_new_from_data((const guint8 *)addr, len / sizeof(guint8), NULL);
	if (!handle)
	{
		err = "Unable to load SVG file: invalid format";
		g_object_unref(G_OBJECT(handle));
		goto __RETURN;
	}

	rsvg_handle_set_dpi(handle, 72);

	if (HANDLE)
		g_object_unref(G_OBJECT(HANDLE));

	THIS->handle = handle;

#if LIBRSVG_CHECK_VERSION(2,46,0)
	gboolean has_width, has_height, has_viewbox;
	RsvgLength width;
	RsvgLength height;
	RsvgRectangle viewbox;
	rsvg_handle_get_intrinsic_dimensions(handle, &has_width, &width, &has_height, &height, &has_viewbox, &viewbox);
	if (has_viewbox)
	{
		THIS->width = viewbox.width;
		THIS->height = viewbox.height;
	}
	else if (has_width && has_height && width.unit == height.unit)
	{
		THIS->width = width.length;
		THIS->height = height.length;
	}
#else
	RsvgDimensionData dim;
	rsvg_handle_get_dimensions(handle, &dim);
	THIS->width = dim.width;
	THIS->height = dim.height;
#endif

__RETURN:

	GB.ReleaseFile(addr, len);
	return err;
}


static void paint_svg(CSVGIMAGE *_object, cairo_t *context, double x, double y, double w, double h)
{
	cairo_matrix_t matrix;
	double sx, sy, ss;

	if (!context)
		return;

	if (THIS->width <= 0 || THIS->height <= 0)
		return;

	if (!HANDLE && !SURFACE)
		return;

	cairo_get_matrix(context, &matrix);

	if (HANDLE)
	{
		if (w <= 0)
		{
			w = THIS->width;
			sx = 1;
		}
		else
			sx = w / THIS->width;

		if (h <= 0)
		{
			h = THIS->height;
			sy = 1;
		}
		else
			sy = h / THIS->height;

		#if LIBRSVG_CHECK_VERSION(2,46,0)

		RsvgRectangle view;
		view.x = x;
		view.y = y;
		view.width = w;
		view.height = h;

		if ((h/w) != (THIS->height/THIS->width))
		{
			//fprintf(stderr, "%g / %g\n", h/w, THIS->height/THIS->width);

			if ((h/w) < (THIS->height/THIS->width))
			{
				ss = w/h/(THIS->width/THIS->height);
				cairo_translate(context, -((x + w / 2) * (ss - 1)), 0);
				cairo_scale(context, ss, 1);
			}
			else
			{
				ss = h/w/(THIS->height/THIS->width);
				cairo_translate(context, 0, -((y + h / 2) * (ss - 1)));
				cairo_scale(context, 1, ss);
			}
		}

		rsvg_handle_render_document(HANDLE, context, &view, NULL);

		cairo_set_matrix(context, &matrix);

		#endif

		cairo_scale(context, sx, sy);
		cairo_translate(context, x, y);

		#if !LIBRSVG_CHECK_VERSION(2,46,0)

		rsvg_handle_render_cairo(HANDLE, context);

		#endif
	}

	if (SURFACE)
	{
		//cairo_surface_finish(SURFACE);

		cairo_save(context);
		cairo_set_source_surface(context, SURFACE, 0, 0);
		cairo_paint(context);
		cairo_restore(context);
	}

	cairo_set_matrix(context, &matrix);
}


cairo_surface_t *SVGIMAGE_begin(CSVGIMAGE *_object)
{
	if (!SURFACE)
	{
		if (THIS->width <= 0 || THIS->height <= 0)
		{
			GB.Error("SvgImage size is not defined");
			return NULL;
		}

		/*THIS->file = GB.NewZeroString(GB.TempFile(NULL));
		SURFACE = cairo_svg_surface_create(THIS->file, THIS->width, THIS->height);*/

		SURFACE = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);

#if 0
		if (HANDLE)
		{
			cairo_t *context = cairo_create(SURFACE);

#if LIBRSVG_CHECK_VERSION(2,46,0)
			RsvgRectangle view = { 0, 0, THIS->width, THIS->height };
			rsvg_handle_render_document(HANDLE, context, &view, NULL);
#else
			rsvg_handle_render_cairo(HANDLE, context);
#endif
			cairo_destroy(context);
		}
#endif
	}

	return SURFACE;
}


void SVGIMAGE_end(CSVGIMAGE *_object)
{
	//const char *err;

	//cairo_surface_finish(SURFACE);

	/*if ((err = load_file(THIS, THIS->file, GB.StringLength(THIS->file))))
	{
		GB.Error(err);
		return;
	}*/
}

//-------------------------------------------------------------------------


BEGIN_METHOD(SvgImage_new, GB_FLOAT width; GB_FLOAT height)

	THIS->width = VARGOPT(width, 0);
	THIS->height = VARGOPT(height, 0);

END_METHOD


BEGIN_METHOD_VOID(SvgImage_free)

	release(THIS);

END_METHOD


BEGIN_PROPERTY(SvgImage_Width)

	if (READ_PROPERTY)
		GB.ReturnFloat(THIS->width);
	else
		THIS->width = VPROP(GB_FLOAT);

END_PROPERTY


BEGIN_PROPERTY(SvgImage_Height)

	if (READ_PROPERTY)
		GB.ReturnFloat(THIS->height);
	else
		THIS->height = VPROP(GB_FLOAT);

END_PROPERTY


BEGIN_METHOD(SvgImage_Resize, GB_FLOAT width; GB_FLOAT height)

	THIS->width = VARG(width);
	THIS->height = VARG(height);

END_METHOD


BEGIN_METHOD(SvgImage_Load, GB_STRING path)

	CSVGIMAGE *svgimage;
	const char *err;

	svgimage = (CSVGIMAGE *)GB.New(CLASS_SvgImage, NULL, NULL);

	if ((err = load_file(svgimage, STRING(path), LENGTH(path))))
	{
		GB.Unref(POINTER(&svgimage));
		GB.Error(err);
		return;
	}

	GB.ReturnObject(svgimage);

END_METHOD


BEGIN_METHOD(SvgImage_Paint, GB_FLOAT x; GB_FLOAT y; GB_FLOAT w; GB_FLOAT h)

	cairo_t *context = PAINT_get_current_context();
	double tx, ty;

	if (!context)
		return;

	if (THIS->width <= 0 || THIS->height <= 0)
		return;

	cairo_get_current_point(context, &tx, &ty);
	paint_svg(THIS, context, VARGOPT(x, tx), VARGOPT(y, ty), VARGOPT(w, -1), VARGOPT(h, -1));

END_METHOD


BEGIN_METHOD(SvgImage_Save, GB_STRING file)

	cairo_surface_t *svg;
	cairo_t *context;

	if (THIS->width <= 0 || THIS->height <= 0)
	{
		GB.Error("SvgImage size is not defined");
		return;
	}

	svg = cairo_svg_surface_create(GB.FileName(STRING(file), LENGTH(file)), THIS->width, THIS->height);

	context = cairo_create(svg);

	paint_svg(THIS, context, 0, 0, -1, -1);

	cairo_destroy(context);

	cairo_surface_destroy(svg);

END_METHOD


BEGIN_METHOD_VOID(SvgImage_Clear)

	release(THIS);

END_METHOD


//-------------------------------------------------------------------------

GB_DESC SvgImageDesc[] =
{
	GB_DECLARE("SvgImage", sizeof(CSVGIMAGE)),

	GB_METHOD("_new", NULL, SvgImage_new, "[(Width)f(Height)f]"),
	GB_METHOD("_free", NULL, SvgImage_free, NULL),

	GB_PROPERTY("Width", "f", SvgImage_Width),
	GB_PROPERTY("W", "f", SvgImage_Width),
	GB_PROPERTY("Height", "f", SvgImage_Height),
	GB_PROPERTY("H", "f", SvgImage_Height),
	GB_METHOD("Resize", NULL, SvgImage_Resize, "(Width)f(Height)f"),

	GB_STATIC_METHOD("Load", "SvgImage", SvgImage_Load, "(Path)s"),
	GB_METHOD("Save", NULL, SvgImage_Save, "(Path)s"),
	GB_METHOD("Paint", NULL, SvgImage_Paint, "[(X)f(Y)f(Width)f(Height)f]"),

	GB_METHOD("Clear", NULL, SvgImage_Clear, NULL),

	GB_INTERFACE("Paint", &PAINT_Interface),

	GB_END_DECLARE
};

