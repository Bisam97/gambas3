/***************************************************************************

  c_glarea.c

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

#define __C_GLAREA_C

#include <GL/gl.h>
#include "c_glarea.h"

DECLARE_EVENT(EVENT_Open);
DECLARE_EVENT(EVENT_Draw);
DECLARE_EVENT(EVENT_Resize);

//-------------------------------------------------------------------------

static void init_control(void *_object)
{
	if (THIS->init)
		return;
	
	GL.Init();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	GB.Raise(THIS, EVENT_Open, 0);
	THIS->init = TRUE;
	
	return;
}

static void cb_realize(GtkGLArea *area, void *_object)
{
	gtk_gl_area_make_current(area);
	init_control(THIS);
}

static gboolean cb_resize(GtkGLArea *area, int width, int height, void *_object)
{
	GB.Raise(THIS, EVENT_Resize, 0);
	return TRUE;
}

static gboolean cb_render(GtkGLArea *area, GdkGLContext *context, void *_object)
{
	GB.Raise(THIS, EVENT_Draw, 0);
	return TRUE;
}

//-------------------------------------------------------------------------

BEGIN_METHOD(GLArea_new, GB_OBJECT parent)

	THIS->widget = gtk_gl_area_new();

	GTK.CreateControl(THIS, VARG(parent), THIS->widget, CCF_NONE);
	gtk_widget_set_can_focus(THIS->widget, TRUE);

	//gtk_gl_area_set_has_alpha(WIDGET, TRUE);
	gtk_gl_area_set_has_depth_buffer(WIDGET, TRUE);
	gtk_gl_area_set_has_stencil_buffer(WIDGET, TRUE);
	
	g_signal_connect(G_OBJECT(THIS->widget), "realize", G_CALLBACK(cb_realize), (gpointer)THIS);
	g_signal_connect(G_OBJECT(THIS->widget), "resize", G_CALLBACK(cb_resize), (gpointer)THIS);
	g_signal_connect(G_OBJECT(THIS->widget), "render", G_CALLBACK(cb_render), (gpointer)THIS);
	
END_METHOD

//---------------------------------------------------------------------------

GB_DESC GLAreaDesc[] =
{
  GB_DECLARE("GLArea", sizeof(CGLAREA)), GB_INHERITS("Control"),

  GB_METHOD("_new", NULL, GLArea_new, "(Parent)Container;"),
  
  GB_CONSTANT("_Group", "s", "Special"),

  GB_EVENT("Open", NULL, NULL, &EVENT_Open),
  GB_EVENT("Draw", NULL, NULL, &EVENT_Draw),
  GB_EVENT("Resize", NULL, NULL, &EVENT_Resize),

  GB_END_DECLARE
};
