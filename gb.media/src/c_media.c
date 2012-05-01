/***************************************************************************

  c_media.c

  gb.media component

  (c) 2012 Benoît Minisini <gambas@users.sourceforge.net>

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

#define __C_MEDIA_C

#include "c_media.h"

void MEDIA_raise_event(void *_object, int event)
{
	gst_element_post_message(ELEMENT, gst_message_new_application(GST_OBJECT(ELEMENT), gst_structure_new("SendEvent", "event", G_TYPE_INT, event, NULL)));
}

/*void MEDIA_raise_event_arg(void *_object, int event, char *arg)
{
	gst_element_post_message(ELEMENT, gst_message_new_application(GST_OBJECT(ELEMENT), gst_structure_new("SendEvent", "event", G_TYPE_INT, event, "arg", G_TYPE_STRING, arg, NULL)));
}*/

CMEDIACONTROL *MEDIA_get_control_from_element(void *element)
{
	if (!element)
		return NULL;
	else
		return (CMEDIACONTROL *)g_object_get_data(G_OBJECT(element), "gambas-control");
}

static void return_value(const GValue *value)
{
	switch (G_VALUE_TYPE(value))
	{
		case G_TYPE_BOOLEAN: GB.ReturnBoolean(g_value_get_boolean(value)); break;
		case G_TYPE_INT: GB.ReturnInteger(g_value_get_int(value)); break;
		case G_TYPE_UINT: GB.ReturnInteger(g_value_get_uint(value)); break;
		case G_TYPE_UINT64: GB.ReturnLong(g_value_get_uint64(value)); break;
		case G_TYPE_STRING: GB.ReturnNewZeroString(g_value_get_string(value)); break;
		case G_TYPE_FLOAT: GB.ReturnFloat(g_value_get_float(value)); break;
		case G_TYPE_DOUBLE: GB.ReturnFloat(g_value_get_double(value)); break;
		default: GB.Error("Unsupported property datatype"); GB.ReturnNull(); break;
	}
	
	GB.ReturnConvVariant();
}

static bool set_value(GValue *value, GB_VALUE *v)
{
	//g_value_init(&value, desc->value_type);

	switch (G_VALUE_TYPE(value))
	{
		case G_TYPE_BOOLEAN:
			if (GB.Conv(v, GB_T_BOOLEAN))
				return TRUE;
			g_value_set_boolean(value, v->_boolean.value);
			break;
			
		case G_TYPE_INT:
			if (GB.Conv(v, GB_T_INTEGER))
				return TRUE;
			g_value_set_int(value, v->_integer.value);
			break;
			
		case G_TYPE_UINT:
			if (GB.Conv(v, GB_T_INTEGER))
				return TRUE;
			g_value_set_uint(value, (uint)v->_integer.value);
			break;
			
		case G_TYPE_UINT64:
			if (GB.Conv(v, GB_T_LONG))
				return TRUE;
			g_value_set_uint64(value, (guint64)v->_long.value);
			break;
			
		case G_TYPE_STRING:
			if (GB.Conv(v, GB_T_STRING))
				return TRUE;
			g_value_set_string(value, GB.ToZeroString((GB_STRING *)v));
			break;
			
		case G_TYPE_FLOAT:
			if (GB.Conv(v, GB_T_FLOAT))
				return TRUE;
			g_value_set_float(value, v->_float.value);
			break;
			
		case G_TYPE_DOUBLE:
			if (GB.Conv(v, GB_T_FLOAT))
				return TRUE;
			g_value_set_double(value, v->_float.value);
			break;
			
		default:
			GB.Error("Unsupported property datatype");
			return TRUE;
	}
	
	return FALSE;
}

#if 0
//---- MediaSignalArguments -----------------------------------------------

static int check_signal_arguments(void *_object)
{
	return THIS_ARG->param_values == NULL;
}

BEGIN_METHOD(MediaSignalArguments_get, GB_INTEGER index)

	int index = VARG(index);
	
	if (index < 0 || index >= THIS_ARG->n_param_values)
	{
		GB.Error(GB_ERR_BOUND);
		return;
	}
	
	return_value(&THIS_ARG->param_values[index]);

END_METHOD
#endif

//---- MediaControl -------------------------------------------------------

DECLARE_EVENT(EVENT_State);
//DECLARE_EVENT(EVENT_Signal);

typedef
	struct {
		char *klass;
		char *type;
	}
	MEDIA_TYPE;

static MEDIA_TYPE _types[] =
{
	{ "MediaContainer", "bin" },
	{ "MediaPipeline", "pipeline" },
	{ "Media", "pipeline" },
	{ "MediaPlayer", "playbin2" },
	//{ "MediaDecoder", "decodebin2" },
	{ NULL, NULL }
};

static void cb_pad_added(GstElement *element, GstPad *pad, CMEDIACONTROL *_object)
{
	char *name;
	//GstPad *sinkpad;
	
	//fprintf(stderr, "cb_pad_added: %s\n", gst_element_factory_get_klass(gst_element_get_factory(ELEMENT)));

	if (!THIS->dest)
		return;
	
	/* We can now link this pad with the vorbis-decoder sink pad */
	//sinkpad = gst_element_get_static_pad (decoder, "sink");
	//gst_pad_link (pad, sinkpad);
	
	name = gst_pad_get_name(pad);
	gst_element_link_pads(ELEMENT, name, ((CMEDIACONTROL *)THIS->dest)->elt, NULL);
	g_free(name);
	
	//GB.Unref(POINTER(&THIS->dest));
	//gst_object_unref (sinkpad);
}

BEGIN_METHOD(MediaControl_new, GB_OBJECT parent; GB_STRING type)

	char *type;
	CMEDIACONTAINER *parent;
	MEDIA_TYPE *mtp;
	GB_CLASS klass;
	
	if (MISSING(type))
	{
		klass = GB.GetClass(THIS);
		type = NULL;
		
		for (mtp = _types; mtp->klass; mtp++)
		{
			if (GB.FindClass(mtp->klass) == klass)
			{
				type = mtp->type;
				break;
			}
		}
		
		if (!type)
		{
			GB.Error("The type must be specified");
			return;
		}
	}
	else
		type = GB.ToZeroString(ARG(type));
	
	THIS->state = GST_STATE_NULL;
	THIS->type = GB.NewZeroString(type);
	
	ELEMENT = gst_element_factory_make(type, NULL);
	if (!ELEMENT)
	{
		GB.Error("Unable to create media control");
		return;
	}
	
	gst_object_ref(GST_OBJECT(ELEMENT));
	g_object_set_data(G_OBJECT(ELEMENT), "gambas-control", THIS);
	
	parent = VARGOPT(parent, NULL);
	if (parent)
	{
		gst_bin_add(GST_BIN(parent->elt), ELEMENT);
		gst_element_sync_state_with_parent(ELEMENT);
	}
	else if (!GST_IS_PIPELINE(ELEMENT))
		GB.CheckObject(parent);
		
END_METHOD

BEGIN_METHOD_VOID(MediaControl_free)

	GB.Unref(POINTER(&THIS->dest));
	GB.FreeString(&THIS->type);
	
	if (ELEMENT)
	{
		gst_element_set_state(ELEMENT, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ELEMENT));
	}
	
END_METHOD

BEGIN_PROPERTY(MediaControl_Type)

	GB.ReturnString(THIS->type); //NewZeroString(gst_element_factory_get_klass(gst_element_get_factory(ELEMENT)));
	
END_PROPERTY

BEGIN_PROPERTY(MediaControl_Name)

	if (READ_PROPERTY)
		GB.ReturnNewZeroString(gst_element_get_name(ELEMENT));
	else
		gst_element_set_name(ELEMENT, GB.ToZeroString(PROP(GB_STRING)));

END_PROPERTY

static bool set_state(void *_object, int state)
{
	GstStateChangeReturn status;

	status = gst_element_set_state(ELEMENT, state);
	
	if (status == GST_STATE_CHANGE_ASYNC)
		status = gst_element_get_state(ELEMENT, NULL, NULL, GST_SECOND * 3);
	
	if (status == GST_STATE_CHANGE_FAILURE)
	{
		GB.Error("Cannot set status");
		return TRUE;
	}
	
	return FALSE;
}

BEGIN_PROPERTY(MediaControl_State)

	if (READ_PROPERTY)
	{
		GB.ReturnInteger(THIS->state);
		/*GstState state;
		
		status = gst_element_get_state(ELEMENT, &state, NULL, GST_SECOND * 3);
		
		if (status != GST_STATE_CHANGE_SUCCESS)
			GB.ReturnInteger(-1);
		else
			GB.ReturnInteger(state);*/
	}
	else
	{
		set_state(THIS, VPROP(GB_INTEGER));
	}

END_PROPERTY

static GParamSpec *get_property(GstElement *element, char *property)
{
	GParamSpec *desc;
	
	desc = g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(element)), property);
	if (!desc)
		GB.Error("Unknown property: '&1'", property);
	
	return desc;
}

BEGIN_METHOD(MediaControl_get, GB_STRING property)

	char *property = GB.ToZeroString(ARG(property));
	GParamSpec *desc;
	GValue value = G_VALUE_INIT;
	
	desc = get_property(ELEMENT, property);
	if (!desc)
		return;
	
	//fprintf(stderr, "type = %s\n", g_type_name(desc->value_type));
	g_value_init(&value, desc->value_type);
	g_object_get_property(G_OBJECT(ELEMENT), property, &value);
	return_value(&value);
	g_value_unset(&value);
	
END_METHOD

BEGIN_METHOD(MediaControl_put, GB_VARIANT value; GB_STRING property)

	char *property = GB.ToZeroString(ARG(property));
	GParamSpec *desc;
	GValue value = G_VALUE_INIT;
	GB_VALUE *v = (GB_VALUE *)ARG(value);
	
	desc = get_property(ELEMENT, property);
	if (!desc)
		return;
	
	g_value_init(&value, desc->value_type);
	if (set_value(&value, v))
		return;
	
	g_object_set_property(G_OBJECT(ELEMENT), property, &value);
	g_value_unset(&value);
	
END_METHOD

BEGIN_METHOD(MediaControl_LinkTo, GB_OBJECT dest; GB_STRING output; GB_STRING input)

	CMEDIACONTROL *dest = (CMEDIACONTROL *)VARG(dest);
	char *output;
	char *input;
	
	if (GB.CheckObject(dest))
		return;

	output = MISSING(output) ? NULL : GB.ToZeroString(ARG(output));
	if (output && !*output) output = NULL;
	input = MISSING(input) ? NULL : GB.ToZeroString(ARG(input));
	if (input && !*input) input = NULL;
	
	gst_element_link_pads(ELEMENT, output, dest->elt, input);

END_METHOD

BEGIN_METHOD(MediaControl_LinkLaterTo, GB_OBJECT dest)

	CMEDIACONTROL *dest = (CMEDIACONTROL *)VARG(dest);
	
	if (GB.CheckObject(dest))
		return;

	GB.Unref(POINTER(&THIS->dest));
	GB.Ref(dest);
	THIS->dest = dest;
	g_signal_connect(ELEMENT, "pad-added", G_CALLBACK(cb_pad_added), THIS);

END_METHOD

static void fill_pad_list(GB_ARRAY array, GstIterator *iter)
{
	bool done = FALSE;
	GstPad *pad;
	char *name;
	
	while (!done) 
	{
		switch (gst_iterator_next(iter, (gpointer *)&pad)) 
		{
			case GST_ITERATOR_OK:
				name = gst_pad_get_name(pad);
				*((char **)GB.Array.Add(array)) = GB.NewZeroString(name);
				g_free(name);
				gst_object_unref(pad);
				break;
			case GST_ITERATOR_RESYNC:
				gst_iterator_resync(iter);
				break;
			case GST_ITERATOR_ERROR:
			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}
	
	gst_iterator_free(iter);
}

BEGIN_PROPERTY(MediaControl_Inputs)

	GstIterator *iter;
	GB_ARRAY array;
	
	GB.Array.New(&array, GB_T_STRING, 0);
	iter = gst_element_iterate_sink_pads(ELEMENT);
	fill_pad_list(array, iter);
	GB.ReturnObject(array);

END_PROPERTY

BEGIN_PROPERTY(MediaControl_Outputs)

	GstIterator *iter;
	GB_ARRAY array;
	
	GB.Array.New(&array, GB_T_STRING, 0);
	iter = gst_element_iterate_src_pads(ELEMENT);
	fill_pad_list(array, iter);
	GB.ReturnObject(array);

END_PROPERTY

#if 0
static void closure_marshal(GClosure     *closure,
                            GValue       *return_value,
                            guint         n_param_values,
                            const GValue *param_values,
                            gpointer      invocation_hint,
                            gpointer      marshal_data)
{
	GObject *src;
	CMEDIACONTROL *_object;
	CMEDIASIGNALARGUMENTS *arg;
	
	src = g_value_peek_pointer (param_values + 0);
	_object = get_control_from_element(src);
	
	arg = GB.New(GB.FindClass("MediaSignalArguments"), NULL, NULL);
	arg->n_param_values = n_param_values;
	arg->param_values = param_values;
	
	GB.Ref(arg);
	GB.Raise(THIS, EVENT_Signal, 1, GB_T_OBJECT, arg);
	MEDIA_raise_event_arg(THIS, EVENT_Signal, );
	
	arg->n_param_values = 0;
	arg->param_values = NULL;
	GB.Unref(POINTER(&arg));
}

static GClosure *get_closure()
{
	static GClosure *closure = NULL;
	
	if (!closure)
	{
		closure = g_closure_new_simple(sizeof(GClosure), NULL);
		g_closure_set_marshal(closure, closure_marshal);
	}
	
	return closure;
}

BEGIN_METHOD(MediaControl_Activate, GB_STRING signal)

	char *signal = GB.ToZeroString(ARG(signal));
	GClosure *closure = get_closure();
	
	if (g_signal_handler_find(ELEMENT, G_SIGNAL_MATCH_CLOSURE | G_SIGNAL_MATCH_ID, g_signal_lookup(signal, G_OBJECT_TYPE(ELEMENT)), (GQuark)0, closure, NULL, NULL))
	{
		GB.Error("Signal is already activated");
		return;
	}
		
	g_signal_connect_closure(ELEMENT, GB.ToZeroString(ARG(signal)), closure, FALSE);

END_METHOD
#endif

//---- MediaContainer -----------------------------------------------------

static bool add_input_output(void *_object, CMEDIACONTROL *child, char *name, int direction, const char *dir_error, const char *unknown_error)
{
	GstPad *pad;
	GstIterator *iter;
	GstIteratorResult res;
	
	if (GB.CheckObject(child))
		return TRUE;
	
	if (!name)
	{
		if (direction == GST_PAD_SINK)
			iter = gst_element_iterate_sink_pads(child->elt);
		else
			iter = gst_element_iterate_src_pads(child->elt);
		
		for(;;)
		{
			res = gst_iterator_next(iter, (gpointer *)&pad);
			if (res == GST_ITERATOR_RESYNC)
				gst_iterator_resync(iter);
			else
				break;
		}
		
		gst_iterator_free(iter);
		
		if (res != GST_ITERATOR_OK)
		{
			GB.Error(unknown_error);
			return TRUE;
		}
	}
	else
	{
		pad = gst_element_get_static_pad(child->elt, name);
		if (!pad)
		{
			GB.Error(unknown_error);
			return TRUE;
		}
		
		if (gst_pad_get_direction(pad) != direction)
		{
			gst_object_unref (GST_OBJECT(pad));
			GB.Error(dir_error);
			return TRUE;
		}
	}
	
	gst_element_add_pad(ELEMENT, gst_ghost_pad_new(name, pad));
	gst_object_unref(GST_OBJECT(pad));
	
	return FALSE;
}

BEGIN_METHOD(MediaContainer_AddInput, GB_OBJECT child; GB_STRING name)

	add_input_output(THIS, (CMEDIACONTROL *)VARG(child), MISSING(name) ? NULL : GB.ToZeroString(ARG(name)), GST_PAD_SINK, "Not an input", "Unknown input");

END_METHOD

BEGIN_METHOD(MediaContainer_AddOutput, GB_OBJECT child; GB_STRING name)

	add_input_output(THIS, (CMEDIACONTROL *)VARG(child), MISSING(name) ? NULL : GB.ToZeroString(ARG(name)), GST_PAD_SRC, "Not an output", "Unknown output");

END_METHOD

//---- MediaPipeline ------------------------------------------------------

DECLARE_EVENT(EVENT_End);
DECLARE_EVENT(EVENT_Message);
DECLARE_EVENT(EVENT_Tag);
DECLARE_EVENT(EVENT_Buffering);
DECLARE_EVENT(EVENT_Duration);
DECLARE_EVENT(EVENT_Progress);

static int cb_message(CMEDIAPIPELINE *_object)
{
	GstMessage *msg;
	GstMessageType type;
	int msg_type;
	GstBus *bus;
	CMEDIACONTROL *control;
	
	bus = gst_pipeline_get_bus(PIPELINE);
	
	while((msg = gst_bus_pop(bus)) != NULL) 
	{
		type = GST_MESSAGE_TYPE(msg);
		control = MEDIA_get_control_from_element(GST_MESSAGE_SRC(msg));
		
		if (type == GST_MESSAGE_APPLICATION)
		{
			//CMEDIACONTROL *target = (CMEDIACONTROL *)g_value_get_pointer(gst_structure_get_value(gst_message_get_structure(msg), "control"));
			int event = g_value_get_int(gst_structure_get_value(gst_message_get_structure(msg), "event"));
			GB.Raise(control, event, 0);
		}
		else
		if (type == GST_MESSAGE_STATE_CHANGED && control)
		{
			GstState old_state, new_state;

			gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
			control->state = new_state;
			if (new_state == GST_STATE_NULL)
				control->error = FALSE;
			GB.Raise(control, EVENT_State, 0);
		}
		else //if (GST_MESSAGE_SRC(msg) == GST_OBJECT(PIPELINE))
		{
			switch (type)
			{
				case GST_MESSAGE_EOS: 
					GB.Raise(THIS, EVENT_End, 0); 
					break;
				
				case GST_MESSAGE_ERROR: 
				case GST_MESSAGE_WARNING: 
				case GST_MESSAGE_INFO: 
				{
					gchar *debug;
					GError *error;
					
					if (type == GST_MESSAGE_ERROR)
					{
						gst_message_parse_error(msg, &error, &debug);
						msg_type = 2;
						control->error = TRUE;
						THIS->error = TRUE;
					}
					else if (type == GST_MESSAGE_WARNING)
					{
						gst_message_parse_warning(msg, &error, &debug);
						msg_type = 1;
					}
					else
					{
						gst_message_parse_info(msg, &error, &debug);
						msg_type = 0;
					}
					
					g_free(debug);
					
					GB.Ref(control);
					GB.Raise(THIS, EVENT_Message, 3, GB_T_OBJECT, control, GB_T_INTEGER, msg_type, GB_T_STRING, error->message, -1);
					g_error_free(error);
					GB.Unref(POINTER(&control));
					
					break;
				}
				
				case GST_MESSAGE_TAG: GB.Raise(THIS, EVENT_Tag, 0); break;
				case GST_MESSAGE_BUFFERING: GB.Raise(THIS, EVENT_Buffering, 0); break;
				case GST_MESSAGE_DURATION: GB.Raise(THIS, EVENT_Duration, 0); break;
				case GST_MESSAGE_PROGRESS: GB.Raise(THIS, EVENT_Progress, 0); break;
				default: break;
			}
		}
		
		gst_message_unref(msg);
	}
	
	gst_object_unref(bus);
	
	return FALSE;
}

BEGIN_METHOD_VOID(MediaPipeline_new)
	
	THIS->watch = GB.Every(250, (GB_TIMER_CALLBACK)cb_message, (intptr_t)THIS);

END_METHOD

BEGIN_METHOD_VOID(MediaPipeline_free)

	GB.Unref(POINTER(&THIS->watch));

END_METHOD

BEGIN_METHOD_VOID(MediaPipeline_Play)

	set_state(THIS, GST_STATE_PLAYING);

END_METHOD

BEGIN_METHOD_VOID(MediaPipeline_Stop)

	set_state(THIS, GST_STATE_READY);

END_METHOD

BEGIN_METHOD_VOID(MediaPipeline_Close)

	set_state(THIS, GST_STATE_NULL);

END_METHOD

BEGIN_METHOD_VOID(MediaPipeline_Pause)

	set_state(THIS, GST_STATE_PAUSED);

END_METHOD

BEGIN_PROPERTY(MediaPipeline_Position)

	if (READ_PROPERTY)
	{
		GstFormat format = GST_FORMAT_TIME;
		gint64 pos;
		
		if (THIS->state == GST_STATE_NULL || THIS->state == GST_STATE_READY || THIS->error || !gst_element_query_position(ELEMENT, &format, &pos) || format != GST_FORMAT_TIME)
			GB.ReturnFloat(0);
		else
			GB.ReturnFloat((double)(pos / 1000) / 1E6);
	}
	else
	{
		guint64 pos = VPROP(GB_FLOAT) * 1E9;
		
		if (pos < 0) 
			pos = 0;
		
		gst_element_seek_simple(ELEMENT, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, pos);
	}

END_PROPERTY

BEGIN_PROPERTY(MediaPipeline_Duration)

	GstFormat format = GST_FORMAT_TIME;
	gint64 dur;
	
	if (THIS->state == GST_STATE_NULL || THIS->error || !gst_element_query_duration(ELEMENT, &format, &dur) || format != GST_FORMAT_TIME)
		GB.ReturnFloat(0);
	else
		GB.ReturnFloat((double)(dur / 1000) / 1E6);

END_PROPERTY

//---- Media --------------------------------------------------------------

BEGIN_METHOD(Media_Link, GB_OBJECT controls)

	GB_OBJECT *controls = ARG(controls);
	int i;
	CMEDIACONTROL *c1, *c2;
	
	if (GB.CheckObject(VARG(controls)))
		return;
	
	// GB.NParam() returns 0 when MediaPipeline_Link has just two arguments
	
	for (i = 0; i <= GB.NParam(); i++)
	{
		c1 = VALUE(&controls[i]);
		c2 = VALUE(&controls[i + 1]);
		if (GB.CheckObject(c1))
			return;
		gst_element_link(c1->elt, c2->elt);
	}

END_METHOD

BEGIN_METHOD(Media_Time, GB_FLOAT second)

	GB.ReturnLong(VARG(second) * 1E9);

END_METHOD

BEGIN_METHOD(Media_URL, GB_STRING path)

	char *path = GB.RealFileName(STRING(path), LENGTH(path));
	
	path = g_filename_to_uri(path, NULL, NULL);
	GB.ReturnNewZeroString(path);
	g_free(path);

END_METHOD

//-------------------------------------------------------------------------

#if 0
GB_DESC MediaSignalArgumentsDesc[] = 
{
	GB_DECLARE("MediaSignalArguments", sizeof(CMEDIASIGNALARGUMENTS)),
	GB_NOT_CREATABLE(), GB_HOOK_CHECK(check_signal_arguments),
	
	//GB_METHOD("_new", NULL, MediaControl_new, "[(Type)s(Parent)MediaContainer;]"),
	//GB_METHOD("_free", NULL, MediaSignalArguments_free, NULL),
	
	//GB_METHOD("_put", NULL, MediaSignal_put, "(Value)v(Property)s"),
	GB_METHOD("_get", "v", MediaSignalArguments_get, "(Name)s"),
	
	GB_END_DECLARE
};
#endif

GB_DESC MediaControlDesc[] = 
{
	GB_DECLARE("MediaControl", sizeof(CMEDIACONTROL)),
	
	GB_METHOD("_new", NULL, MediaControl_new, "[(Parent)MediaContainer;(Type)s]"),
	GB_METHOD("_free", NULL, MediaControl_free, NULL),
	
	GB_PROPERTY("Name", "s", MediaControl_Name),
	GB_PROPERTY_READ("Type", "s", MediaControl_Type),
	GB_PROPERTY("State", "i", MediaControl_State),
			
	GB_METHOD("_put", NULL, MediaControl_put, "(Value)v(Property)s"),
	GB_METHOD("_get", "v", MediaControl_get, "(Property)s"),
	
	GB_METHOD("LinkTo", NULL, MediaControl_LinkTo, "(Destination)MediaControl;[(Output)s(Input)s]"),
	GB_METHOD("LinkLaterTo", NULL, MediaControl_LinkLaterTo, "(Destination)MediaControl;"),
	
	GB_PROPERTY_READ("Inputs", "String[]", MediaControl_Inputs),
	GB_PROPERTY_READ("Outputs", "String[]", MediaControl_Outputs),
	
	//GB_METHOD("Activate", NULL, MediaControl_Activate, "(Signal)s"),
	
	GB_EVENT("State", NULL, NULL, &EVENT_State),
	//GB_EVENT("Signal", NULL, "(Arg)MediaSignalArguments", &EVENT_Signal),
	
	GB_END_DECLARE
};

GB_DESC MediaContainerDesc[] = 
{
	GB_DECLARE("MediaContainer", sizeof(CMEDIACONTAINER)),
	GB_INHERITS("MediaControl"),
	
	GB_METHOD("AddInput", NULL, MediaContainer_AddInput, "(Child)MediaControl;[(Name)s]"),
	GB_METHOD("AddOutput", NULL, MediaContainer_AddOutput, "(Child)MediaControl;[(Name)s]"),
	
	GB_END_DECLARE
};

GB_DESC MediaPipelineDesc[] = 
{
	GB_DECLARE("MediaPipeline", sizeof(CMEDIAPIPELINE)),
	GB_INHERITS("MediaContainer"),
	
	GB_METHOD("_new", NULL, MediaPipeline_new, NULL),
	GB_METHOD("_free", NULL, MediaPipeline_free, NULL),
	
	GB_CONSTANT("Null", "i", GST_STATE_NULL),
	GB_CONSTANT("Ready", "i", GST_STATE_READY),
	GB_CONSTANT("Paused", "i", GST_STATE_PAUSED),
	GB_CONSTANT("Playing", "i", GST_STATE_PLAYING),

	GB_CONSTANT("Info", "i", 0),
	GB_CONSTANT("Warning", "i", 1),
	GB_CONSTANT("Error", "i", 2),
	
	GB_PROPERTY("Position", "f", MediaPipeline_Position),
	GB_PROPERTY_READ("Duration", "f", MediaPipeline_Duration),
	GB_PROPERTY_READ("Length", "f", MediaPipeline_Duration),
	
	GB_METHOD("Play", NULL, MediaPipeline_Play, NULL),
	GB_METHOD("Stop", NULL, MediaPipeline_Stop, NULL),
	GB_METHOD("Pause", NULL, MediaPipeline_Pause, NULL),
	GB_METHOD("Close", NULL, MediaPipeline_Close, NULL),
	
	GB_EVENT("End", NULL, NULL, &EVENT_End),
	GB_EVENT("Message", NULL, "(Source)MediaControl;(Type)i(Message)s", &EVENT_Message),
	GB_EVENT("Tag", NULL, NULL, &EVENT_Tag),
	GB_EVENT("Buffering", NULL, NULL, &EVENT_Buffering),
	GB_EVENT("Duration", NULL, NULL, &EVENT_Duration),
	GB_EVENT("Progress", NULL, NULL, &EVENT_Progress),
	
	GB_END_DECLARE
};

GB_DESC MediaDesc[] = 
{
	GB_DECLARE("Media", sizeof(CMEDIAPIPELINE)),
	GB_INHERITS("MediaPipeline"),
	
	GB_CONSTANT("Null", "i", GST_STATE_NULL),
	GB_CONSTANT("Unknown", "i", -1),
	GB_CONSTANT("Ready", "i", GST_STATE_READY),
	GB_CONSTANT("Paused", "i", GST_STATE_PAUSED),
	GB_CONSTANT("Playing", "i", GST_STATE_PLAYING),

	GB_CONSTANT("Info", "i", 0),
	GB_CONSTANT("Warning", "i", 1),
	GB_CONSTANT("Error", "i", 2),
	
	GB_STATIC_METHOD("Link", NULL, Media_Link, "(FirstControl)MediaControl;(SecondControl)MediaControl;."),
	GB_STATIC_METHOD("Time", "l", Media_Time, "(Seconds)f"),
	GB_STATIC_METHOD("URL", "s", Media_URL, "(Path)s"),
	
	GB_END_DECLARE
};

