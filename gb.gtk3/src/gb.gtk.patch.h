/***************************************************************************

  gb.gtk.patch.h

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

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

#ifndef GB_GTK_PATCH_H
#define GB_GTK_PATCH_H

typedef
	struct {
		void (*get_preferred_height)(GtkWidget *widget, gint *minimum_height, gint *natural_height);
		void (*get_preferred_width_for_height)(GtkWidget *widget, gint height, gint *minimum_width, gint *natural_width);
		void (*get_preferred_width)(GtkWidget *widget, gint *minimum_width, gint *natural_width);
		void (*get_preferred_height_for_width)(GtkWidget *widget, gint width, gint *minimum_height, gint *natural_height);
#if GTK_CHECK_VERSION(3, 10, 0)
		void (*get_preferred_height_and_baseline_for_width)(GtkWidget *widget, gint width, gint *minimum, gint *natural, gint *minimum_baseline, gint *natural_baseline);
		void (*size_allocate)(GtkWidget *widget, GtkAllocation *allocation);
#endif
	}
	PATCH_FUNCS;

#define PATCH_OLD_FUNC ((PATCH_FUNCS *)(klass->_gtk_reserved6))

#define PATCH_DECLARE_COMMON(_type, _name) \
static void _name##get_preferred_width(GtkWidget *widget, gint *minimum_size, gint *natural_size) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass*)g_type_class_peek(_type); \
	(*PATCH_OLD_FUNC->get_preferred_width)(widget, minimum_size, natural_size); \
	if (minimum_size && must_patch(widget)) \
		*minimum_size = 0; \
} \
static void _name##get_preferred_height(GtkWidget *widget, gint *minimum_size, gint *natural_size) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(_type); \
	(*PATCH_OLD_FUNC->get_preferred_height)(widget, minimum_size, natural_size); \
	if (minimum_size && must_patch(widget)) \
		*minimum_size = 0; \
} \
static void _name##get_preferred_height_for_width(GtkWidget *widget, gint width, gint *minimum_size, gint *natural_size) \
{ \
	if (minimum_size && must_patch(widget)) \
	{ \
		*minimum_size = 0; \
		*natural_size = 0; \
		return; \
	} \
	GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(_type); \
	(*PATCH_OLD_FUNC->get_preferred_height_for_width)(widget, width, minimum_size, natural_size); \
} \
static void _name##get_preferred_width_for_height(GtkWidget *widget, gint height, gint *minimum_size, gint *natural_size) \
{ \
	if (minimum_size && must_patch(widget)) \
	{ \
		*minimum_size = 0; \
		*natural_size = 0; \
		return; \
	} \
	GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(_type); \
	(*PATCH_OLD_FUNC->get_preferred_height_for_width)(widget, height, minimum_size, natural_size); \
}

#if GTK_CHECK_VERSION(3, 14, 0)

#define PATCH_DECLARE_SIZE(_type, _name) \
static void _name##size_allocate(GtkWidget *widget, GtkAllocation *allocation) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(_type); \
	(*PATCH_OLD_FUNC->size_allocate)(widget, allocation); \
	gtk_widget_set_clip(widget, allocation); \
}

#elif GTK_CHECK_VERSION(3, 10, 0)

#define PATCH_DECLARE_SIZE(_type, _name) \
static void _name##size_allocate(GtkWidget *widget, GtkAllocation *allocation) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(_type); \
	(*PATCH_OLD_FUNC->size_allocate)(widget, allocation); \
}

#else

#define PATCH_DECLARE_SIZE(_type, _name)

#endif

#define PATCH_DECLARE(type) PATCH_DECLARE_COMMON(type, type##_) PATCH_DECLARE_SIZE(type, type##_)

#define PATCH_DECLARE_BASELINE(type) \
static void type##_get_preferred_height_and_baseline_for_width(GtkWidget *widget, gint width, gint *minimum, gint *natural, gint *minimum_baseline, gint *natural_baseline) \
{ \
	if (minimum && minimum_baseline && must_patch(widget)) \
	{ \
		GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(type); \
		if (PATCH_OLD_FUNC->get_preferred_height_and_baseline_for_width) \
			(*PATCH_OLD_FUNC->get_preferred_height_and_baseline_for_width)(widget, width, minimum, natural, minimum_baseline, natural_baseline); \
		else \
		{ \
			*minimum_baseline = 0; \
			*natural_baseline = 0; \
		} \
		*minimum = 0; \
		*natural = 0; \
		return; \
	} \
	GtkWidgetClass *klass = (GtkWidgetClass *)g_type_class_peek(type); \
	if (PATCH_OLD_FUNC->get_preferred_height_and_baseline_for_width) \
		(*PATCH_OLD_FUNC->get_preferred_height_and_baseline_for_width)(widget, width, minimum, natural, minimum_baseline, natural_baseline); \
}

//fprintf(stderr, "patching [%p %s] (%p %p)\n", klass, G_OBJECT_TYPE_NAME(widget), klass->get_preferred_width, klass->get_preferred_height);
//		fprintf(stderr, "PATCH_CLASS: %s\n", G_OBJECT_TYPE_NAME(widget));

#if GTK_CHECK_VERSION(3,10,0)

#define PATCH_CLASS(widget, type) \
if (G_OBJECT_TYPE(widget) == type) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)GTK_WIDGET_GET_CLASS(widget); \
	if (klass->get_preferred_width != type##_get_preferred_width) \
	{ \
		PATCH_FUNCS *funcs = g_new0(PATCH_FUNCS, 1); \
		funcs->get_preferred_width = klass->get_preferred_width; \
		funcs->get_preferred_height = klass->get_preferred_height; \
		funcs->get_preferred_height_for_width = klass->get_preferred_height_for_width; \
		funcs->get_preferred_width_for_height = klass->get_preferred_width_for_height; \
		funcs->size_allocate = klass->size_allocate; \
		klass->_gtk_reserved6 = (void(*)())funcs; \
		klass->get_preferred_width = type##_get_preferred_width; \
		klass->get_preferred_height = type##_get_preferred_height; \
		klass->get_preferred_height_for_width = type##_get_preferred_height_for_width; \
		klass->get_preferred_width_for_height = type##_get_preferred_width_for_height; \
		klass->size_allocate = type##_size_allocate; \
	} \
}

#define PATCH_CLASS_BASELINE(widget, type) \
if (G_OBJECT_TYPE(widget) == type) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)GTK_WIDGET_GET_CLASS(widget); \
	if (klass->get_preferred_width != type##_get_preferred_width) \
	{ \
		PATCH_FUNCS *funcs = g_new0(PATCH_FUNCS, 1); \
		funcs->get_preferred_width = klass->get_preferred_width; \
		funcs->get_preferred_height = klass->get_preferred_height; \
		funcs->get_preferred_height_for_width = klass->get_preferred_height_for_width; \
		funcs->get_preferred_width_for_height = klass->get_preferred_width_for_height; \
		funcs->size_allocate = klass->size_allocate; \
		funcs->get_preferred_height_and_baseline_for_width = klass->get_preferred_height_and_baseline_for_width; \
		klass->_gtk_reserved6 = (void(*)())funcs; \
		klass->get_preferred_width = type##_get_preferred_width; \
		klass->get_preferred_height = type##_get_preferred_height; \
		klass->get_preferred_height_for_width = type##_get_preferred_height_for_width; \
		klass->get_preferred_width_for_height = type##_get_preferred_width_for_height; \
		klass->size_allocate = type##_size_allocate; \
		if (klass->get_preferred_height_and_baseline_for_width) \
			klass->get_preferred_height_and_baseline_for_width = type##_get_preferred_height_and_baseline_for_width; \
	} \
}

#else

#define PATCH_CLASS(widget, type) \
if (G_OBJECT_TYPE(widget) == type) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)GTK_WIDGET_GET_CLASS(widget); \
	if (klass->get_preferred_width != type##_get_preferred_width) \
	{ \
		PATCH_FUNCS *funcs = g_new0(PATCH_FUNCS, 1); \
		funcs->get_preferred_width = klass->get_preferred_width; \
		funcs->get_preferred_height = klass->get_preferred_height; \
		funcs->get_preferred_height_for_width = klass->get_preferred_height_for_width; \
		funcs->get_preferred_width_for_height = klass->get_preferred_width_for_height; \
		klass->_gtk_reserved6 = (void(*)())funcs; \
		klass->get_preferred_width = type##_get_preferred_width; \
		klass->get_preferred_height = type##_get_preferred_height; \
		klass->get_preferred_height_for_width = type##_get_preferred_height_for_width; \
		klass->get_preferred_width_for_height = type##_get_preferred_width_for_height; \
	} \
}

#define PATCH_CLASS_BASELINE PATCH_CLASS

#endif

#define PATCH_CLASS_FIXED(widget, type) \
if (G_OBJECT_TYPE(widget) == type) \
{ \
	GtkWidgetClass *klass = (GtkWidgetClass *)GTK_WIDGET_GET_CLASS(widget); \
	klass->draw = draw_container; \
}	
 
#endif
