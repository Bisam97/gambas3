/***************************************************************************

	gbx_c_timer.c

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

#define __GBX_C_TIMER_C

#include "gbx_info.h"

#ifndef GBX_INFO

#include "gb_common.h"
#include "gbx_watch.h"
#include "gbx_api.h"
#include "gbx_exec.h"
#include "gbx_event.h"
#include "gbx_c_timer.h"

#define EXT(_ob) ((CTIMER_EXT *)((CTIMER *)_ob)->ext)

int CTIMER_active_count = 0;


DECLARE_EVENT(EVENT_Timer);


static void enable_timer(CTIMER *_object, bool on)
{
	if (on == (THIS->id != 0))
		return;
	
	HOOK_DEFAULT(timer, WATCH_timer)((GB_TIMER *)THIS, on);
	
	if (on && (THIS->id == 0))
		GB_Error("Too many active timers");
	
	if (!THIS->ignore)
	{
		CTIMER_active_count += on ? 1 : -1;
		/*if (!on)
			fprintf(stderr, "disable_timer: %d -> %d\n", THIS->delay, CTIMER_active_count);*/
	}
}


CTIMER *CTIMER_every(int delay, GB_TIMER_CALLBACK callback, intptr_t param)
{
	CTIMER *timer;

	timer = OBJECT_create(CLASS_Timer, NULL, NULL, 0);
	OBJECT_REF(timer);
	
	timer->delay = delay;
	timer->task = EXEC_task;
	timer->ignore = TRUE;

	ALLOC_ZERO(&timer->ext, sizeof(CTIMER_EXT));
	EXT(timer)->callback = callback;
	EXT(timer)->tag = param;

	enable_timer(timer, TRUE);

	return timer;
}


void CTIMER_raise(void *_object)
{
	if (THIS->task == EXEC_task)
	{
		if (THIS_EXT && THIS_EXT->callback)
		{
			if (!(*(THIS_EXT->callback))(THIS_EXT->tag))
				return;
		}
		else
		{
			void *parent = OBJECT_parent(THIS);
			if (parent && OBJECT_is_valid(parent) && !GB_Raise(THIS, EVENT_Timer, 0))
				return;
		}
	}

	enable_timer(THIS, FALSE);
}


BEGIN_METHOD(Timer_new, GB_INTEGER delay)

	int delay;
	THIS->id = 0;
	
	delay = VARGOPT(delay, -1);
	if (delay < 0)
		delay = 1000;
	
	THIS->task = EXEC_task;
	THIS->delay = delay;
	if (!MISSING(delay))
		enable_timer(THIS, TRUE);

END_METHOD


BEGIN_METHOD_VOID(Timer_free)

	enable_timer(THIS, FALSE);
	
	if (THIS->ext)
		IFREE(THIS->ext);

END_METHOD


BEGIN_METHOD_VOID(Timer_Start)

	enable_timer(THIS, TRUE);

END_METHOD


BEGIN_METHOD_VOID(Timer_Stop)

	enable_timer(THIS, FALSE);
	THIS->triggered = FALSE;

END_METHOD


BEGIN_METHOD_VOID(Timer_Restart)

	enable_timer(THIS, FALSE);
	enable_timer(THIS, TRUE);

END_METHOD


BEGIN_PROPERTY(Timer_Enabled)

	if (READ_PROPERTY)
		GB_ReturnBoolean(THIS->id != 0);
	else
		enable_timer(THIS, !!VPROP(GB_BOOLEAN));

END_PROPERTY


BEGIN_PROPERTY(Timer_Delay)

	if (READ_PROPERTY)
		GB_ReturnInteger(THIS->delay);
	else
	{
		int delay = VPROP(GB_INTEGER);
		bool enabled = THIS->id != 0;

		if (delay < 0 || delay >= (1<<30))
		{
			GB_Error(GB_ERR_ARG);
			return;
		}
		
		if (enabled)
			HOOK_DEFAULT(timer, WATCH_timer)((GB_TIMER *)THIS, FALSE);

		THIS->delay = delay;

		if (enabled)
			HOOK_DEFAULT(timer, WATCH_timer)((GB_TIMER *)THIS, TRUE);
	}

END_PROPERTY

static void trigger_timer(void *_object)
{
	if (THIS->triggered)
	{
		THIS->triggered = FALSE;
		GB_Raise(THIS, EVENT_Timer, 0);
	}
	
	OBJECT_UNREF(_object);
}

BEGIN_METHOD_VOID(Timer_Trigger)

	if (THIS->triggered)
		return;

	THIS->triggered = TRUE;
	OBJECT_REF(THIS);
	EVENT_post(trigger_timer, (intptr_t)THIS);

END_METHOD

BEGIN_PROPERTY(Timer_Ignore)

	if (READ_PROPERTY)
		GB_ReturnBoolean(THIS->ignore);
	else
	{
		bool v = VPROP(GB_BOOLEAN);
		
		if (THIS->ignore == v)
			return;
		
		THIS->ignore = v;
		if (THIS->id)
			CTIMER_active_count += THIS->ignore ? -1 : 1;
	}

END_PROPERTY

#endif

GB_DESC NATIVE_Timer[] =
{
	GB_DECLARE("Timer", sizeof(CTIMER)),

	GB_METHOD("_new", NULL, Timer_new, "[(Delay)i]"),
	GB_METHOD("_free", NULL, Timer_free, NULL),

	GB_PROPERTY("Enabled", "b", Timer_Enabled),
	GB_PROPERTY("Delay", "i", Timer_Delay),
	GB_PROPERTY("Ignore", "b", Timer_Ignore),
	//GB_PROPERTY_READ("Timeout", "f", Timer_Timeout),

	GB_METHOD("Start", NULL, Timer_Start, NULL),
	GB_METHOD("Stop", NULL, Timer_Stop, NULL),
	GB_METHOD("Restart", NULL, Timer_Restart, NULL),
	GB_METHOD("Trigger", NULL, Timer_Trigger, NULL),

	GB_CONSTANT("_IsControl", "b", TRUE),
	GB_CONSTANT("_IsVirtual", "b", TRUE),
	GB_CONSTANT("_Group", "s", "Special"),
	GB_CONSTANT("_Properties", "s", "Enabled,Delay{Range:0;86400000;10;ms}=1000,Ignore"),
	GB_CONSTANT("_DefaultEvent", "s", "Timer"),

	GB_EVENT("Timer", NULL, NULL, &EVENT_Timer),

	GB_END_DECLARE
};


