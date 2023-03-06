/***************************************************************************

	CDebug.c

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

#define __CDEBUG_C

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "main.h"
#include "gb_limit.h"
#include "gb.debug.h"
#include "debug.h"
#include "CDebug.h"

/*#define DEBUG*/

DECLARE_EVENT(EVENT_Read);

static int _started = FALSE;
static int _fdr = -1;
static int _fdw = -1;
static CDEBUG *_debug_object = NULL;

#define BUFFER_SIZE DEBUG_OUTPUT_MAX_SIZE
static char *_buffer = NULL;
static int _buffer_left;

#if 0
static void callback_read(int fd, int type, intptr_t param)
{
	int n;
	
	fcntl(_fdr, F_SETFL, fcntl(_fdr, F_GETFL) | O_NONBLOCK);

	n = read(_fdr, _buffer, 4096);
	if (n > 0)
	{
		GB.Raise(_debug_object, EVENT_Read, 1, GB_T_STRING, _buffer, n);
	}
	else
	{
		if (n == 0 || (errno != EINTR && errno != EAGAIN))
			GB.Watch(fd, GB_WATCH_NONE, (void *)callback_read, 0);
	}
	
	fcntl(_fdr, F_SETFL, fcntl(_fdr, F_GETFL) & ~O_NONBLOCK);
}
#endif

static void callback_read(int fd, int type, intptr_t param)
{
	int n, i, p;

	//fprintf(stderr, "callback_read\n");

	for(;;)
	{
		fcntl(_fdr, F_SETFL, fcntl(_fdr, F_GETFL) | O_NONBLOCK);

		if (_buffer_left)
		{
			n = read(_fdr,  &_buffer[_buffer_left], BUFFER_SIZE - _buffer_left);
			if (n > 0)
			{
				n += _buffer_left;
				_buffer_left = 0;
			}
		}
		else
			n = read(_fdr, _buffer, BUFFER_SIZE);

		//fprintf(stderr, "n = %d\n", n);

		if (n <= 0)
		{
			if (n == 0 || (errno != EINTR && errno != EAGAIN))
			{
				//fprintf(stderr, "stop watch\n");
				GB.Watch(fd, GB_WATCH_NONE, (void *)callback_read, 0);
			}

			break; // try again
		}

		p = 0;

		for (i = 0; i < n; i++)
		{
			if (_buffer[i] == '\n')
			{
				/*fprintf(stderr, "CDEBUG_read: <<< %.*s >>>\n", i - p, &_buffer[p]);*/
				GB.Raise(_debug_object, EVENT_Read, 1, GB_T_STRING, i <= p ? NULL : &_buffer[p], i - p + 1);
				if (!_buffer)
					break;
				p = i + 1;
			}
		}

		if (!_buffer)
			break;

		if (p == 0 && n >= BUFFER_SIZE)
		{
			GB.Raise(_debug_object, EVENT_Read, 1, GB_T_STRING, _buffer, BUFFER_SIZE);
			if (!_buffer)
				break;
			_buffer_left = 0;
		}
		else
		{
			_buffer_left = n - p;
			if (p && n > p)
				memmove(_buffer, &_buffer[p], _buffer_left);
		}
	}

	fcntl(_fdr, F_SETFL, fcntl(_fdr, F_GETFL) & ~O_NONBLOCK);
}


static char *fifo_path(char *path, const char *direction)
{
	sprintf(path, DEBUG_FIFO_PATTERN, getuid(), getpid(), direction ? direction : "");
	return path;
}

static void open_write_fifo()
{
	int i;
	char path[PATH_MAX];
	
	fifo_path(path, "out");
	
	for(i = 0; i < 4; i++)
	{
		_fdw = open(path, O_WRONLY);
		if (_fdw >= 0)
			break;
		if (errno != EAGAIN && errno != EINTR)
			break;
		usleep(20000);
	}

	if (_fdw < 0)
	{
		GB.Error("Unable to open fifo: &1: &2", path, strerror(errno));
		return;
	}

	//fprintf(stderr, "open_write_fifo: %d\n", _fdw);
}

BEGIN_METHOD_VOID(Debug_Begin)

	char path[PATH_MAX];

	signal(SIGPIPE, SIG_IGN);

	unlink(fifo_path(path, "in"));
	if (mkfifo(path, 0600))
	{
		GB.Error("Cannot create input fifo in /tmp: &1", strerror(errno));
		return;
	}

	unlink(fifo_path(path, "out"));
	if (mkfifo(path, 0600))
	{
		GB.Error("Cannot create output fifo in /tmp: &1", strerror(errno));
		return;
	}

	GB.ReturnNewZeroString(fifo_path(path, ""));

END_METHOD


BEGIN_METHOD_VOID(Debug_Start)

	char path[DEBUG_FIFO_PATH_MAX];

	if (_started)
		return;

	_fdr = open(fifo_path(path, "in"), O_RDONLY | O_NONBLOCK);
	fcntl(_fdr, F_SETFL, fcntl(_fdr, F_GETFL) & ~O_NONBLOCK);

	_debug_object = GB.New(GB.FindClass("Debug"), "Debug", NULL);
	GB.Ref(_debug_object);

	GB.Alloc(POINTER(&_buffer), BUFFER_SIZE);
	_buffer_left = 0;

	//fprintf(stderr, "watch debugger on %d\n", _fdr);
	GB.Watch(_fdr, GB_WATCH_READ, (void *)callback_read, 0);

	_started = TRUE;

END_METHOD


BEGIN_METHOD_VOID(Debug_Stop)

	if (!_started)
		return;

	GB.Watch(_fdr, GB_WATCH_NONE, (void *)callback_read, 0);
	GB.Free(POINTER(&_buffer));

	GB.Unref(POINTER(&_debug_object));

	if (_fdw >= 0)
	{
		close(_fdw);
		_fdw = -1;
	}
	
	close(_fdr);
 _fdr = -1;
 
	_started = FALSE;

END_METHOD


BEGIN_METHOD_VOID(Debug_End)

	char path[DEBUG_FIFO_PATH_MAX];

	CALL_METHOD_VOID(Debug_Stop);

	unlink(fifo_path(path, "in"));
	unlink(fifo_path(path, "out"));

	signal(SIGPIPE, SIG_DFL);

END_METHOD


BEGIN_METHOD(Debug_Write, GB_STRING data)

	int try = 0;
	const char *data = STRING(data);
	int len = LENGTH(data);

__TRY_AGAIN:

	if (_fdw < 0)
		open_write_fifo();

	if (data && len > 0)
	{
		if (write(_fdw, data, len) != len)
			goto __ERROR;
	}
	if (write(_fdw, "\n", 1) != 1)
		goto __ERROR;

	return;

__ERROR:

	close(_fdw);
	_fdw = -1;
	try++;
	if (try < 3)
	{
		usleep(1000);
		goto __TRY_AGAIN;
	}

	fprintf(stderr, "gb.debug: warning: unable to send data to the debugger: %s\n", strerror(errno));

END_METHOD


BEGIN_METHOD(Debug_GetSignal, GB_INTEGER signal)

	GB.ReturnNewZeroString(strsignal(VARG(signal)));

END_METHOD


BEGIN_PROPERTY(Debug_Fifo)

	GB.ReturnString(DEBUG_fifo);

END_PROPERTY

BEGIN_METHOD(Debug_Signal, GB_INTEGER pid)

	kill(VARG(pid), SIGUSR2);

END_METHOD


GB_DESC CDebugDesc[] =
{
	GB_DECLARE("Debug", sizeof(CDEBUG)),
	GB_NOT_CREATABLE(),

	GB_STATIC_METHOD("_exit", NULL, Debug_End, NULL),

	GB_STATIC_METHOD("Begin", "s", Debug_Begin, NULL),
	GB_STATIC_METHOD("End", NULL, Debug_End, NULL),
	GB_STATIC_METHOD("Start", NULL, Debug_Start, NULL),
	GB_STATIC_METHOD("Stop", NULL, Debug_Stop, NULL),

	GB_STATIC_METHOD("GetSignal", "s", Debug_GetSignal, "(Signal)i"),

	GB_STATIC_METHOD("Write", NULL, Debug_Write, "(Data)s"),
	
	GB_STATIC_METHOD("Signal", NULL, Debug_Signal, "(ProcessId)i"),
	
	GB_STATIC_PROPERTY_READ("Fifo", "s", Debug_Fifo),
	
	GB_EVENT("Read", NULL, "(Data)s", &EVENT_Read),

	GB_END_DECLARE
};

