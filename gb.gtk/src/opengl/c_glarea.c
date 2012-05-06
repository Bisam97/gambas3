/***************************************************************************

  c_glarea.c

  (c) 2012 Benoît Minisini <gambas@users.sourceforge.net>

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

#define __C_GLAREA_C

#include <GL/gl.h>
#include "c_glarea.h"

//#include "gl.h"

//#include <iostream>

#if 0
static int glWidgetCount = 0;
static QGLWidget *sharedWidget = 0;

DECLARE_EVENT(EVENT_Open);
DECLARE_EVENT(EVENT_Draw);
DECLARE_EVENT(EVENT_Resize);

static void makeSharing()
{
	if (sharedWidget)
		return;

	sharedWidget = new QGLWidget();
	//qDebug("make_sharing: %p", sharedWidget);
}

BEGIN_METHOD_VOID(CGLAREA_exit)

	//qDebug("CGLAREA_exit: %p", sharedWidget);
	delete sharedWidget;
	sharedWidget = 0;

END_METHOD

BEGIN_METHOD(CGLAREA_new, GB_OBJECT parent)

	if (!QGLFormat::hasOpenGL())
	{
		GB.Error( "This system has no OpenGL support");
		return;
	}

	makeSharing();
	GLarea *area = new GLarea(QT.GetContainer(VARG(parent)), THIS, sharedWidget);
	glWidgetCount++;

	QT.InitWidget(area, _object, false);
	area->show();

END_METHOD

BEGIN_METHOD_VOID(CGLAREA_free)

	glWidgetCount--;

	// clean up the shared datas between GLwidgets if there is no more CGLarea
	if (glWidgetCount <= 0)
	{
		delete sharedWidget;
		sharedWidget = 0;
		makeSharing();
		glWidgetCount = 0;
	}


END_METHOD

BEGIN_METHOD_VOID(CGLAREA_update)

	WIDGET->updateGL();

END_METHOD

BEGIN_METHOD_VOID(CGLAREA_select)

	WIDGET->makeCurrent();
	// really needed ?
	GL.Init();

END_METHOD

BEGIN_METHOD(CGLAREA_text, GB_STRING text; GB_INTEGER x; GB_INTEGER y)

	QString text;
	int x, y;
	GLboolean _LIGHTING = glIsEnabled(GL_LIGHTING);
	GLboolean _TEXTURE_2D = glIsEnabled(GL_TEXTURE_2D);

	if (_LIGHTING)
		glDisable(GL_LIGHTING);
	if (_TEXTURE_2D)
		glDisable(GL_TEXTURE_2D);

	text = QSTRING_ARG(text);
	x = VARG(x);
	y = VARG(y);

	WIDGET->renderText(x, y, text, WIDGET->font());

	if (_LIGHTING)
		glEnable(GL_LIGHTING);
	if (_TEXTURE_2D)
		glEnable(GL_TEXTURE_2D);

END_METHOD

/**************************************************************************/

GB_DESC CGlareaDesc[] =
{
  GB_DECLARE("GLarea", sizeof(CGLAREA)), GB_INHERITS("Control"),

  GB_STATIC_METHOD("_exit", NULL, CGLAREA_exit, NULL),

  GB_METHOD("_new", NULL, CGLAREA_new, "(Parent)Container;"),
  GB_METHOD("_free", NULL, CGLAREA_free, NULL),
  GB_METHOD("Update", NULL, CGLAREA_update, NULL),
  GB_METHOD("Refresh", NULL, CGLAREA_update, NULL),
  GB_METHOD("Select", NULL, CGLAREA_select, NULL),
  GB_METHOD("Text", NULL, CGLAREA_text, "(Text)s(X)i(Y)i"),

  GB_CONSTANT("_Properties", "s", CGLAREA_PROPERTIES),

  GB_EVENT("Open", NULL, NULL, &EVENT_Open),
  GB_EVENT("Draw", NULL, NULL, &EVENT_Draw),
  GB_EVENT("Resize", NULL, NULL, &EVENT_Resize),

  GB_END_DECLARE
};

/* class GLarea */

GLarea::GLarea(QWidget *parent,CGLAREA *object, QGLWidget *sharing): QGLWidget(parent, 0, sharing)
{
  setFocusPolicy(Qt::WheelFocus);
  setInputMethodEnabled(true);
 _area = object; 
};


void GLarea::initializeGL()
{
	GL.Init();
	GB.Raise(_area, EVENT_Open, 0);
}

void GLarea::paintGL()
{
	static bool CleanupOnFirstShow = 0;
	
	if (!CleanupOnFirstShow)
	{
		// clear to avoid garbage
		CleanupOnFirstShow = true;
		qglClearColor(Qt::black);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	
	GB.Raise(_area, EVENT_Draw, 0);
}

void GLarea::resizeGL(int w, int h)
{
	GB.Raise(_area, EVENT_Resize, 0);
}
#endif


//-- GLArea -----------------------------------------------------------------

DECLARE_EVENT(EVENT_Open);
DECLARE_EVENT(EVENT_Draw);
DECLARE_EVENT(EVENT_Resize);

static void cb_init_ext(GtkWidget *widget)
{
	GdkGLConfig *config; // la config qui va aller avec
	
	static const gint attrList[] = { // les paramètres de la config
		GDK_GL_DOUBLEBUFFER,
		GDK_GL_RGBA,
		GDK_GL_RED_SIZE, 1,
		GDK_GL_GREEN_SIZE, 1,
		GDK_GL_BLUE_SIZE, 1,
		GDK_GL_ALPHA_SIZE, 1,
		GDK_GL_DEPTH_SIZE, 1,
		GDK_GL_ATTRIB_LIST_NONE };
	
	config = gdk_gl_config_new(attrList);
	
	/* ajout du support Opengl */
	gtk_widget_set_gl_capability(widget, config, NULL, TRUE, GDK_GL_RGBA_TYPE);
}

static void init_control(void *_object)
{
	GdkGLContext *context;
	GdkGLDrawable *surface;
	GtkWidget *widget = THIS->widget;
	
	if (THIS->init)
		return;
	
	//fprintf(stderr, "init_control: %p\n", THIS);

	/* recuperation du contexte et de la surface de notre widget */
	context = gtk_widget_get_gl_context(widget);
	surface = gtk_widget_get_gl_drawable(widget);
	
	/* activation du contexte */
	if (gdk_gl_drawable_gl_begin(surface, context))
	{
		GL.Init();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		GB.Raise(THIS, EVENT_Open, 0);
		GB.Raise(THIS, EVENT_Resize, 0);
		gdk_gl_drawable_gl_end(surface); // désactivation du contexte
	}
	
	THIS->init = TRUE;
	
	return;
}

static gboolean cb_reshape_ext(GtkWidget *widget, GdkEventConfigure *ev, void *_object)
{
	/* recuperation du contexte et de la surface de notre widget */
	GdkGLContext *context;
	GdkGLDrawable *surface;

	if (!THIS->init)
		return TRUE;
	
	//fprintf(stderr, "cb_reshape_ext: %p\n", THIS);
		
	if (!gtk_widget_is_gl_capable(widget))
	{
		fprintf(stderr, "not capable!\n");
		return TRUE;
	}
	
	context = gtk_widget_get_gl_context(widget);
	surface = gtk_widget_get_gl_drawable(widget);

	/* activation du contexte */
	if(gdk_gl_drawable_gl_begin(surface, context))
	{
		//reshape(ev−>height,ev−>width); // redimensionnement Opengl
		GB.Raise(THIS, EVENT_Resize, 0);
		gdk_gl_drawable_gl_end(surface); // désactivation du contexte
	}

	return TRUE;
}

static gboolean cb_draw_ext(GtkWidget *widget, GdkEventExpose *e, void *_object)
{
	/* recuperation du contexte et de la surface de notre widget */
	GdkGLContext *context;
	GdkGLDrawable *surface;

	//fprintf(stderr, "cb_draw_ext: %p\n", THIS);
	
	if (!gtk_widget_is_gl_capable(widget))
	{
		fprintf(stderr, "not capable!\n");
		return TRUE;
	}
	
	context = gtk_widget_get_gl_context(widget);
	surface = gtk_widget_get_gl_drawable(widget);

	/* activation du contexte */
	if(gdk_gl_drawable_gl_begin(surface, context))
	{
		//draw(); // dessin Opengl
		init_control(THIS);
		GB.Raise(THIS, EVENT_Draw, 0);
		gdk_gl_drawable_swap_buffers(surface); // permutation tampons
		gdk_gl_drawable_gl_end(surface); // désactivation du contexte
	}

	return TRUE;
}


BEGIN_METHOD(GLArea_new, GB_OBJECT parent)

	THIS->widget = GTK.CreateGLArea(THIS, VARG(parent), cb_init_ext);
	
	if (!gtk_widget_is_gl_capable(THIS->widget))
	{
		GB.Error("Unable to set OpenGL capability");
		return;
	}
	
	g_signal_connect(G_OBJECT(THIS->widget), "configure-event", G_CALLBACK(cb_reshape_ext), (gpointer)THIS);
	g_signal_connect(G_OBJECT(THIS->widget), "expose-event", G_CALLBACK(cb_draw_ext), (gpointer)THIS);
	
END_METHOD

//---------------------------------------------------------------------------

GB_DESC GLAreaDesc[] =
{
  GB_DECLARE("GLArea", sizeof(CGLAREA)), GB_INHERITS("Control"),

  //GB_STATIC_METHOD("_exit", NULL, GLArea_exit, NULL),

  GB_METHOD("_new", NULL, GLArea_new, "(Parent)Container;"),
  //GB_METHOD("_free", NULL, GLArea_free, NULL),
  //GB_METHOD("Update", NULL, GLArea_update, NULL),
  //GB_METHOD("Refresh", NULL, GLArea_Refresh, NULL),
  //GB_METHOD("Select", NULL, CGLAREA_select, NULL),
  //GB_METHOD("Text", NULL, CGLAREA_text, "(Text)s(X)i(Y)i"),

  //GB_CONSTANT("_Properties", "s", CGLAREA_PROPERTIES),

  GB_EVENT("Open", NULL, NULL, &EVENT_Open),
  GB_EVENT("Draw", NULL, NULL, &EVENT_Draw),
  GB_EVENT("Resize", NULL, NULL, &EVENT_Resize),

  GB_END_DECLARE
};
