/***************************************************************************

  CClipboard.cpp

  (c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#define __CCLIPBOARD_CPP

#include "gambas.h"

#include <QApplication>
#include <QClipboard>
#include <QImage>
#include <QEvent>
#include <QColor>
#include <QDrag>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QWidget>

#include "gb.form.const.h"
#include "CWidget.h"
#include "CImage.h"
#include "CClipboard.h"

#include "main.h"

CDRAG_INFO CDRAG_info = { 0 };
bool CDRAG_dragging = false;
void *CDRAG_destination = NULL;
static char CDRAG_action = DRAG_MOVE;

static CPICTURE *_picture = 0;
static int _picture_x = -1;
static int _picture_y = -1;

enum { MIME_UNKNOWN, MIME_TEXT, MIME_IMAGE };
enum { CLIPBOARD_DEFAULT, CLIPBOARD_SELECTION };

static int _current_clipboard = CLIPBOARD_DEFAULT;

static int get_type(const QMimeData *src)
{
	if (src->hasImage())
		return MIME_IMAGE;
#if QT6
	else if (src->formats().indexOf(QRegularExpression("text/.*")) >= 0)
#else
	else if (src->formats().indexOf(QRegExp("text/.*")) >= 0)
#endif
		return MIME_TEXT;
	else
		return MIME_UNKNOWN;
}

static QString get_format(const QMimeData *src, int i = 0, bool charset = false)
{
	QStringList formats = src->formats();
	QString format;
	
	if (i >= 0 && i < formats.count())
	{
		format = formats.at(i);

		if (!charset)
		{
			int pos = format.indexOf(';');
			if (pos >= 0)
				format = format.left(pos);
		}
	}

	return format;
}

static void get_formats(const QMimeData *src, GB_ARRAY array)
{
	int i, j;
	QStringList formats = src->formats();
	QString fmt;
	
	for (i = 0; i < formats.count(); i++)
	{
		fmt = get_format(src, i, true); 
		if (!fmt[0].isLower())
			continue;
		for (j = 0; j < GB.Array.Count(array); j++)
		{
			if (strcasecmp(fmt.toUtf8().data(), *((char **)GB.Array.Get(array, j))) == 0)
				break;
		}
		if (j < GB.Array.Count(array))
			continue;
		//fmt = get_format(src, i);
		
		*((char **)GB.Array.Add(array)) = GB.NewZeroString(fmt.toUtf8().data());
	}
}

static QString get_first_format(const QMimeData *src)
{
	int i;
	QString format;
	
	for (i = 0;; i++)
	{
		format = get_format(src, i);
		if (format.length() && !format[0].isLower())
			continue;
		break;
	}
	
	return format;
}

static bool paste(const QMimeData *data, const char *fmt)
{
	QString format;
	QByteArray ba;
	int type;
#if QT5
#else
	QTextCodec *codec = NULL;
#endif

	if (fmt)
		format = fmt;
	else
		format = get_first_format(data);
		
	if (!data->hasFormat(format))
	{
		GB.ReturnVariant(NULL);
		return TRUE;
	}

	if (format.startsWith("text/"))
		type = MIME_TEXT;
	else
		type = get_type(data);

	switch(type)
	{
		case MIME_TEXT:
			
			ba = data->data(format);
			
			if (ba.size())
			{
#if QT5
				GB.ReturnNewString(ba.constData(), ba.size());
#else
				if (((uchar)ba[0] == 0xFE && (uchar)ba[1] == 0xFF) || ((uchar)ba[0] == 0xFF && (uchar)ba[1] == 0xFE))
					codec = QTextCodec::codecForUtfText(ba, NULL);
				
				if (codec)
					RETURN_NEW_STRING(codec->toUnicode(ba));
				else
					GB.ReturnNewString(ba.constData(), ba.size());
#endif
			}
			else
				GB.ReturnNull();
			break;
		
		case MIME_IMAGE:
			{
				QImage *image = new QImage();
#if QT5
				if (data->hasFormat("image/png"))
					*image = QImage::fromData(data->data("image/png"), "PNG");
				else
#endif
					*image = qvariant_cast<QImage>(data->imageData());

				*image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
				GB.ReturnObject(CIMAGE_create(image));
			}
			break;
		
		default:
			GB.ReturnNull();
	}
	
	GB.ReturnConvVariant();
	return FALSE;
}



/***************************************************************************

	Clipboard

***************************************************************************/

static GB_ARRAY _clipboard_formats[2] = { NULL };
static bool _clipboard_has_changed[2] = { TRUE };

#define CURRENT_MODE() (_current_clipboard == CLIPBOARD_SELECTION ? QClipboard::Selection : QClipboard::Clipboard)

void CLIPBOARD_has_changed(QClipboard::Mode mode)
{
	int clipboard = mode == QClipboard::Selection ? CLIPBOARD_SELECTION : CLIPBOARD_DEFAULT;
	GB.Unref(POINTER(&_clipboard_formats[clipboard]));
	_clipboard_formats[clipboard] = NULL;
	_clipboard_has_changed[clipboard] = TRUE;
}

static GB_ARRAY load_clipboard_formats()
{
	if (!_clipboard_formats[_current_clipboard])
	{
		//qDebug("load clipboard formats");
		GB.Array.New(&_clipboard_formats[_current_clipboard], GB_T_STRING, 0);
		get_formats(QApplication::clipboard()->mimeData(CURRENT_MODE()), _clipboard_formats[_current_clipboard]);
		GB.Ref(_clipboard_formats[_current_clipboard]);
	}
	
	return _clipboard_formats[_current_clipboard];
}

static int get_clipboard_type()
{
	int i;
	QString format;
	GB_ARRAY formats;
	
	formats = load_clipboard_formats();
	
	for (i = 0; i < GB.Array.Count(formats); i++)
	{
		format = *((char **)GB.Array.Get(formats, i));
		if (format.startsWith("text/"))
			return MIME_TEXT;
		else if (format.startsWith("image/"))
			return MIME_IMAGE;
		else if (format == "application/x-qt-image")
			return MIME_IMAGE;
	}
	
	return MIME_UNKNOWN;
}

BEGIN_METHOD_VOID(Clipboard_exit)

	CLIPBOARD_has_changed(QClipboard::Clipboard);
	CLIPBOARD_has_changed(QClipboard::Selection);

END_METHOD


BEGIN_METHOD_VOID(Clipboard_Clear)

	QApplication::clipboard()->clear(CURRENT_MODE());

END_METHOD


BEGIN_PROPERTY(Clipboard_Format)

	GB_ARRAY formats = load_clipboard_formats();
	
	if (GB.Array.Count(formats) == 0)
		GB.ReturnVoidString();
	else
		GB.ReturnString(*((char **)GB.Array.Get(formats, 0)));

END_PROPERTY


BEGIN_PROPERTY(Clipboard_Formats)

	GB.ReturnObject(load_clipboard_formats());

END_PROPERTY


BEGIN_PROPERTY(Clipboard_Type)

	GB.ReturnInteger(get_clipboard_type());

END_PROPERTY


BEGIN_METHOD(Clipboard_Copy, GB_VARIANT data; GB_STRING format)

	QString format;
	QMimeData *data = new QMimeData();

	if (VARG(data).type == GB_T_STRING)
	{
		if (MISSING(format))
			format = "text/plain";
		else
		{
			format = TO_QSTRING(GB.ToZeroString(ARG(format)));
			if (format.left(5) != "text/")
				goto _BAD_FORMAT;
			if (format.length() == 5)
				goto _BAD_FORMAT;
		}

		data->setData(format, QByteArray(VARG(data).value._string, GB.StringLength(VARG(data).value._string)));
		QApplication::clipboard()->setMimeData(data, CURRENT_MODE());
	}
	else if (VARG(data).type >= GB_T_OBJECT && GB.Is(VARG(data).value._object, CLASS_Image))
	{
		QImage img;

		if (!MISSING(format))
			goto _BAD_FORMAT;

		img = *CIMAGE_get((CIMAGE *)VARG(data).value._object);
		img.detach();

		QApplication::clipboard()->setImage(img, CURRENT_MODE());
	}
	else
		goto _BAD_FORMAT;

	return;

_BAD_FORMAT:

	GB.Error("Bad clipboard format");

END_METHOD


BEGIN_METHOD(Clipboard_Paste, GB_STRING format)

	if (!paste(QApplication::clipboard()->mimeData(CURRENT_MODE()), MISSING(format) ?  NULL : GB.ToZeroString(ARG(format))))
		_clipboard_has_changed[_current_clipboard] = FALSE;

END_METHOD

BEGIN_PROPERTY(Clipboard_Current)

	if (READ_PROPERTY)
		GB.ReturnInteger(_current_clipboard);
	else
	{
		int val = VPROP(GB_INTEGER);
		if (val != CLIPBOARD_DEFAULT && val != CLIPBOARD_SELECTION)
			GB.Error(GB_ERR_ARG);
		else
			_current_clipboard = val;
	}

END_PROPERTY


BEGIN_PROPERTY(Clipboard_HasChanged)

	GB.ReturnBoolean(_clipboard_has_changed[_current_clipboard]);

END_PROPERTY

GB_DESC CClipboardDesc[] =
{
	GB_DECLARE_STATIC("Clipboard"),

	GB_STATIC_METHOD("_exit", NULL, Clipboard_exit, NULL),

	GB_CONSTANT("None", "i", 0),
	GB_CONSTANT("Text", "i", 1),
	GB_CONSTANT("Image", "i", 2),
	
	GB_CONSTANT("Default", "i", 0),
	GB_CONSTANT("Selection", "i", 1),

	GB_STATIC_METHOD("Clear", NULL, Clipboard_Clear, NULL),

	GB_STATIC_PROPERTY_READ("Format", "s", Clipboard_Format),
	GB_STATIC_PROPERTY_READ("Formats", "String[]", Clipboard_Formats),
	GB_STATIC_PROPERTY_READ("Type", "i", Clipboard_Type),
	GB_STATIC_PROPERTY_READ("HasChanged", "b", Clipboard_HasChanged),
	
	GB_STATIC_PROPERTY("Current", "i", Clipboard_Current),

	GB_STATIC_METHOD("Copy", NULL, Clipboard_Copy, "(Data)v[(Format)s]"),
	GB_STATIC_METHOD("Paste", "v", Clipboard_Paste, "[(Format)s]"),

	GB_END_DECLARE
};


//---------------------------------------------------------------------------

static void hide_frame(CWIDGET *control)
{
	GB.Push(1, GB_T_OBJECT, control);
	CALL_GUI("_HideDNDFrame", NULL, NULL, 1, FALSE);
}

void CDRAG_hide_frame(CWIDGET *control)
{
	hide_frame(control);
}

static void show_frame(CWIDGET *control, int x, int y, int w, int h)
{
	GB.Push(5, GB_T_OBJECT, control, GB_T_INTEGER, x, GB_T_INTEGER, y, GB_T_INTEGER, w, GB_T_INTEGER, h);
	CALL_GUI("_ShowDNDFrame", NULL, NULL, 5, FALSE);
}


/** Drag *****************************************************************/

void CDRAG_clear(bool valid)
{
	if (valid)
		CDRAG_info.valid++;
	else
		CDRAG_info.valid--;

	if (CDRAG_info.valid == 0)
		CLEAR(&CDRAG_info);
}

static void post_exit_drag(intptr_t param)
{
	CDRAG_dragging = false;
}

void *CDRAG_drag(CWIDGET *source, GB_VARIANT_VALUE *data, GB_STRING *fmt)
{
	QDrag *drag;
	QMimeData *mimeData;
	QString format;
	void *dest;

	if (GB.CheckObject(source))
		return NULL;

	if (CDRAG_dragging)
	{
		GB.Error("Undergoing drag");
		return NULL;
	}

	mimeData = new QMimeData();

	if (data->type == GB_T_STRING)
	{
		if (fmt == NULL)
			format = "text/plain";
		else
		{
			format = TO_QSTRING(GB.ToZeroString(fmt));
			if (format.left(5) != "text/")
				goto _BAD_FORMAT;
			if (format.length() == 5)
				goto _BAD_FORMAT;
		}
		
		mimeData->setData(format, QByteArray(data->value._string, GB.StringLength(data->value._string)));
	}
	else if (data->type >= GB_T_OBJECT && GB.Is(data->value._object, CLASS_Image))
	{
		QImage img;

		if (fmt)
			goto _BAD_FORMAT;

		img = *CIMAGE_get((CIMAGE *)data->value._object);
		img.detach();
		
		mimeData->setImageData(img);
	}
	else
		goto _BAD_FORMAT;

	source->flag.dragging = true;
	
	drag = new QDrag(source->widget);
	drag->setMimeData(mimeData);
	
	if (_picture)
	{
		drag->setPixmap(*(_picture->pixmap));
		if (_picture_x >= 0 && _picture_y >= 0)
			drag->setHotSpot(QPoint(_picture_x, _picture_y));
	}

	CDRAG_dragging = true;
	
	/*if (CDRAG_destination)
	{
		fprintf(stderr, "%d: unref destination %p\n", __LINE__, CDRAG_destination);
		GB.Unref(POINTER(&CDRAG_destination));
	}*/

	CDRAG_action = DRAG_MOVE;
	
	//qDebug("start drag");
	drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);

	source->flag.dragging = false;
	//qDebug("end drag");
	
	hide_frame(NULL);
	GB.Post((GB_CALLBACK)post_exit_drag, 0);

	dest = CDRAG_destination;
	if (dest)
	{
		//fprintf(stderr, "%d: unref destination %p\n", __LINE__, CDRAG_destination);
		GB.Unref(POINTER(&CDRAG_destination));
		CDRAG_destination = NULL;
	}

	return dest;

_BAD_FORMAT:

	GB.Error("Bad drag format");
	return NULL;
}


static void update_action(QDropEvent *e)
{
#if QT6
	Qt::KeyboardModifiers mod = e->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
#else
	Qt::KeyboardModifiers mod = e->keyboardModifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
#endif
	
	if (mod == Qt::ControlModifier)
	{
		e->setDropAction(Qt::CopyAction);
		CDRAG_action = DRAG_COPY;
	}
	else if (mod == Qt::ShiftModifier)
	{
		e->setDropAction(Qt::LinkAction);
		CDRAG_action = DRAG_LINK;
	}
	else
	{
		e->setDropAction(Qt::MoveAction);
		CDRAG_action = DRAG_MOVE;
	}
}


bool CDRAG_drag_enter(QWidget *w, CWIDGET *control, QDropEvent *e)
{
	bool cancel;

	update_action(e);
	
	if (!GB.CanRaise(control, EVENT_Drag))
	{
		if (GB.CanRaise(control, EVENT_DragMove) || GB.CanRaise(control, EVENT_Drop))
			e->accept();
		else
		{
			if (qobject_cast<QLineEdit *>(w) || qobject_cast<QTextEdit *>(w))
				return false;

			e->ignore();
		}
		return true;
	}
	
	//fprintf(stderr, "CDRAG_drag_enter: %s %s\n", GB.GetClassName(control), control->name);

	CDRAG_clear(true);
	CDRAG_info.event = e;

	cancel = GB.Raise(control, EVENT_Drag, 0);
	
	CDRAG_clear(false);
	
	if (cancel)
		e->ignore();
	else
		e->accept();
	return cancel;
}

#define EXT(_ob) ((CWIDGET_EXT *)((CWIDGET *)_ob)->ext)

void CDRAG_drag_leave(CWIDGET *control)
{
	//fprintf(stderr, "CDRAG_drag_leave: %s %s\n", GB.GetClassName(control), control->name);

	CDRAG_hide_frame(control);
	
	GB.Raise(control, EVENT_DragLeave, 0);
}


bool CDRAG_drag_move(QWidget *w, CWIDGET *control, QDropEvent *e)
{
	bool cancel;
	QPoint p;

	//qDebug("CDRAG_drag_move: (%s %p) %p", GB.GetClassName(control), control, qobject_cast<MyListView *>(QWIDGET(control)));

	update_action(e);

	if (!GB.CanRaise(control, EVENT_DragMove))
		return true;
	
	//fprintf(stderr, "CDRAG_drag_move: %s %s\n", GB.GetClassName(control), control->name);
	
	CDRAG_clear(true);
	CDRAG_info.event = e;

#if QT6
	p = e->position().toPoint();
#else
	p = e->pos();
#endif
	p = w->mapTo(QWIDGET(control), p);
	CDRAG_info.x = p.x();
	CDRAG_info.y = p.y();

	cancel = GB.Raise(control, EVENT_DragMove, 0);
	if (cancel)
		e->ignore();
	else
		e->accept();

	CDRAG_clear(false);
	
	return cancel;
}

bool CDRAG_drag_drop(QWidget *w, CWIDGET *control, QDropEvent *e)
{
	QPoint p;

	//hide_frame();

	if (!GB.CanRaise(control, EVENT_Drop))
		return false;

	// Hack for QScrollView
	/*if (CWIDGET_test_flag(control, WF_SCROLLVIEW) && qobject_cast<MyListView *>(QWIDGET(control)))
		((MyListView *)QWIDGET(control))->contentsDropEvent((QDragMoveEvent *)e);*/
	
	//fprintf(stderr, "CDRAG_drag_drop: %s %s\n", GB.GetClassName(control), control->name);

	e->accept();

	CDRAG_clear(true);
	CDRAG_info.event = e;
	CDRAG_destination = control;
	//fprintf(stderr, "%d: ref destination %p\n", __LINE__, CDRAG_destination);
	GB.Ref(CDRAG_destination);

#if QT6
	p = e->position().toPoint();
#else
	p = e->pos();
#endif
	p = w->mapTo(QWIDGET(control), p);
	CDRAG_info.x = p.x();
	CDRAG_info.y = p.y();

	GB.Raise(control, EVENT_Drop, 0);

	if (!CDRAG_dragging) // DnD run outside of the application
	{
		//fprintf(stderr, "%d: unref destination %p\n", __LINE__, CDRAG_destination);
		GB.Unref(&CDRAG_destination);
		CDRAG_destination = NULL;
		hide_frame(control);
	}
	
	CDRAG_clear(false);
	
	return true;
}

BEGIN_METHOD(Drag_call, GB_OBJECT source; GB_VARIANT data; GB_STRING format)

	GB.ReturnObject(CDRAG_drag((CWIDGET *)VARG(source), &VARG(data), MISSING(format) ? NULL : ARG(format)));

END_METHOD

BEGIN_PROPERTY(Drag_Target)

	if (READ_PROPERTY)
		GB.ReturnObject(CDRAG_destination);
	else
		GB.StoreObject(PROP(GB_OBJECT), POINTER(&CDRAG_destination));

END_PROPERTY


BEGIN_METHOD_VOID(Drag_exit)

	hide_frame(NULL);
	GB.Unref(POINTER(&_picture));

END_METHOD

BEGIN_PROPERTY(Drag_Icon)

	if (READ_PROPERTY)
		GB.ReturnObject(_picture);
	else
		GB.StoreObject(PROP(GB_OBJECT), POINTER(&_picture));

END_PROPERTY

BEGIN_PROPERTY(Drag_IconX)

	if (READ_PROPERTY)
		GB.ReturnInteger(_picture_x);
	else
		_picture_x = VPROP(GB_INTEGER);

END_PROPERTY

BEGIN_PROPERTY(Drag_IconY)

	if (READ_PROPERTY)
		GB.ReturnInteger(_picture_y);
	else
		_picture_y = VPROP(GB_INTEGER);

END_PROPERTY

#define CHECK_VALID() \
	if (!CDRAG_info.valid) \
	{ \
		GB.Error("No drag data"); \
		return; \
	}


BEGIN_PROPERTY(Drag_Type)

	CHECK_VALID();

	GB.ReturnInteger(get_type(CDRAG_info.event->mimeData()));

END_PROPERTY


BEGIN_PROPERTY(Drag_Format)

	CHECK_VALID();

	RETURN_NEW_STRING(get_format(CDRAG_info.event->mimeData()));

END_PROPERTY


BEGIN_PROPERTY(Drag_Formats)

	GB_ARRAY array;
	
	CHECK_VALID();
	
	GB.Array.New(&array, GB_T_STRING, 0);
	get_formats(CDRAG_info.event->mimeData(), array);
	GB.ReturnObject(array);

END_PROPERTY


BEGIN_PROPERTY(Drag_Data)

	if (!CDRAG_info.valid)
	{
		GB.ReturnVariant(NULL);
		return;
	}

	paste(CDRAG_info.event->mimeData(), NULL);

END_PROPERTY


BEGIN_METHOD(Drag_Paste, GB_STRING format)

	if (!CDRAG_info.valid)
	{
		GB.ReturnVariant(NULL);
		return;
	}

	paste(CDRAG_info.event->mimeData(), MISSING(format) ?  NULL : GB.ToZeroString(ARG(format)));

END_METHOD


BEGIN_PROPERTY(Drag_Action)

	CHECK_VALID();
	GB.ReturnInteger(CDRAG_action);

END_PROPERTY


BEGIN_PROPERTY(Drag_Source)

	CHECK_VALID();

	GB.ReturnObject(CWidget::get(CDRAG_info.event->source()));

END_PROPERTY


BEGIN_PROPERTY(Drag_X)

	CHECK_VALID();

	if (READ_PROPERTY)
		GB.ReturnInteger(CDRAG_info.x);
	else
		CDRAG_info.x = VPROP(GB_INTEGER);

END_PROPERTY


BEGIN_PROPERTY(Drag_Y)

	CHECK_VALID();

	if (READ_PROPERTY)
		GB.ReturnInteger(CDRAG_info.y);
	else
		CDRAG_info.y = VPROP(GB_INTEGER);

END_PROPERTY


BEGIN_PROPERTY(Drag_Pending)

	GB.ReturnBoolean(CDRAG_dragging);

END_PROPERTY

BEGIN_METHOD(Drag_Show, GB_OBJECT control; GB_INTEGER x; GB_INTEGER y; GB_INTEGER w; GB_INTEGER h)

	if (GB.CheckObject(VARG(control)))
		return;
		
	/*if (!CDRAG_dragging)
	{
		GB.Error("No undergoing drag");
		return;
	}*/

	if (MISSING(x) || MISSING(y) || MISSING(w) || MISSING(h))
		show_frame((CWIDGET *)VARG(control), 0, 0, -1, -1);
	else
		show_frame((CWIDGET *)VARG(control), VARG(x), VARG(y), VARG(w), VARG(h));

END_METHOD

BEGIN_METHOD_VOID(Drag_Hide)

	hide_frame(NULL);

END_METHOD


GB_DESC CDragDesc[] =
{
	GB_DECLARE_STATIC("Drag"),

	GB_CONSTANT("None", "i", MIME_UNKNOWN),
	GB_CONSTANT("Text", "i", MIME_TEXT),
	GB_CONSTANT("Image", "i", MIME_IMAGE),

	GB_CONSTANT("Copy", "i", DRAG_COPY),
	GB_CONSTANT("Link", "i", DRAG_LINK),
	GB_CONSTANT("Move", "i", DRAG_MOVE),

	GB_STATIC_PROPERTY("Icon", "Picture", Drag_Icon),
	GB_STATIC_PROPERTY("IconX", "i", Drag_IconX),
	GB_STATIC_PROPERTY("IconY", "i", Drag_IconY),

	GB_STATIC_PROPERTY_READ("Data", "v", Drag_Data),
	GB_STATIC_PROPERTY_READ("Format", "s", Drag_Format),
	GB_STATIC_PROPERTY_READ("Formats", "String[]", Drag_Formats),
	GB_STATIC_PROPERTY_READ("Type", "i", Drag_Type),
	GB_STATIC_PROPERTY_READ("Action", "i", Drag_Action),
	GB_STATIC_PROPERTY_READ("Source", "Control", Drag_Source),
	GB_STATIC_PROPERTY("X", "i", Drag_X),
	GB_STATIC_PROPERTY("Y", "i", Drag_Y),
	GB_STATIC_PROPERTY_READ("Pending", "b", Drag_Pending),
	GB_STATIC_PROPERTY("_Target", "Control", Drag_Target),

	GB_STATIC_METHOD("_call", "Control", Drag_call, "(Source)Control;(Data)v[(Format)s]"),
	GB_STATIC_METHOD("_exit", NULL, Drag_exit, NULL),
	GB_STATIC_METHOD("Show", NULL, Drag_Show, "(Control)Control;[(X)i(Y)i(Width)i(Height)i]"),
	GB_STATIC_METHOD("Hide", NULL, Drag_Hide, NULL),
	GB_STATIC_METHOD("Paste", "v", Drag_Paste, "[(Format)s]"),

	GB_END_DECLARE
};



