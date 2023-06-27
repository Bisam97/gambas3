/***************************************************************************

  gbx_signal.c

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

#define __GBX_SIGNAL_C

#include "gb_alloc.h"
#include "gb_error.h"
#include "gb_array.h"
#include "gbx_api.h"
#include "gbx_c_process.h"
#include "gbx_c_task.h"
#include "gbx_signal.h"

//#define DEBUG_ME 1

uint64_t SIGNAL_check_mask = 0;

static SIGNAL_HANDLER *_handlers = NULL;
static int _pipe[2] = { -1, -1 };
static volatile int _count = 0;
static int _raising_callback = 0;

enum { CB_NONE = 0, CB_HANDLER = 1, CB_ACTION = 2 };

static int get_callback(struct sigaction *action, void (**callback)())
{
	if (action->sa_handler != SIG_DFL && action->sa_handler != SIG_IGN)
	{
		if (action->sa_flags & SA_SIGINFO)
		{
			*callback = (void *)action->sa_sigaction;
			return CB_ACTION;
		}
		else
		{
			*callback = (void *)action->sa_handler;
			return CB_HANDLER;
		}
	}
	else
	{
		*callback = NULL;
		return CB_NONE;
	}
}

void SIGNAL_install(SIGNAL_HANDLER *handler, int signum, void (*callback)(int, siginfo_t *, void *))
{
	struct sigaction action;
	sigset_t sig;
	
	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_install: %d %p\n", signum, callback);
	#endif
	
	handler->signum = signum;
	
	action.sa_flags = SA_SIGINFO;
	// According to manpage, the emitting signal is blocked by default.
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = callback;

	if (sigaction(signum, NULL, &handler->old_action) != 0 || sigaction(signum, &action, NULL) != 0)
		ERROR_panic("Cannot install signal handler: %s", strerror(errno));

	// Ensure that the signal is not blocked
	sigemptyset(&sig);
	sigaddset(&sig, signum);
	sigprocmask(SIG_UNBLOCK, &sig, NULL);

}

void SIGNAL_uninstall(SIGNAL_HANDLER *handler, int signum)
{
	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_uninstall: %d\n", signum);
	#endif
	
	if (sigaction(signum, &handler->old_action, NULL) != 0)
		ERROR_panic("Cannot uninstall signal handler");

	while (handler->callbacks)
		SIGNAL_unregister(handler->signum, handler->callbacks);
}

void SIGNAL_previous(SIGNAL_HANDLER *handler, int signum, siginfo_t *info, void *context)
{
	void (*cb)();
	
	switch (get_callback(&handler->old_action, &cb))
	{
		case CB_ACTION:
			(*cb)(signum, info, context);
			break;
		
		case CB_HANDLER:
			(*cb)(signum);
			break;
		
		default:
			; /* do nothing */
	}
}

static SIGNAL_HANDLER *find_handler(int signum)
{
	int i;
	
	for (i = 0; i < ARRAY_count(_handlers); i++)
	{
		if (_handlers[i].signum == signum)
			return &_handlers[i];
	}
	
	return NULL;
}

static SIGNAL_HANDLER *add_handler(void)
{
	if (!_handlers)
		ARRAY_create_inc(&_handlers, 1);
	
	return ARRAY_add_void(&_handlers);
}

static void handle_signal(int signum, siginfo_t *info, void *context)
{
	char buffer;
	int save_errno;
	static volatile int64_t lock = 0;

	if (lock & (1 << signum))
		return;

	save_errno = errno;
	lock |= 1 << signum;

	#if DEBUG_ME	
	char digit;
	write(2, "[SIGNAL:", 8);
	digit = '0' + (signum / 10);
	write(2, &digit, 1);
	digit = '0' + (signum % 10);
	write(2, &digit, 1);
	write(2, "]\n", 2);
	#endif
	
	if (_count)
	{
		buffer = signum;
		for(;;)
		{
			if (write(_pipe[1], &buffer, 1) == 1)
				break;
			
			if (errno != EINTR)
			{
				ERROR_warning("cannot write signal #%d into signal pipe: %s", signum, strerror(errno));
				break;
			}
		}
	}
	
	SIGNAL_previous(find_handler(signum), signum, info, context);
	
	errno = save_errno;
	lock &= ~(1 << signum);
}

static bool _must_purge_callbacks = FALSE;
static int _purge_signum;
static SIGNAL_HANDLER *_purge_handler;

static void purge_callbacks(void)
{
	SIGNAL_CALLBACK *cb, *next_cb;

	_raising_callback--;
	if (_raising_callback)
		return;
	
	#if DEBUG_ME
	fprintf(stderr, ">> purge_callbacks\n");
	#endif

	while (_must_purge_callbacks)
	{
		_must_purge_callbacks = FALSE;

		cb = _purge_handler->callbacks;
		while (cb)
		{
			#if DEBUG_ME
			fprintf(stderr, "purge_callbacks: cb = %p\n", cb);
			#endif
			next_cb = cb->next;

			if (!cb->callback)
				SIGNAL_unregister(_purge_signum, cb);
			
			cb = next_cb;
		}
	}

	#if DEBUG_ME
	fprintf(stderr, "<< purge_callbacks\n");
	#endif
}

void SIGNAL_raise_callbacks(int fd, int type, void *data)
{
	SIGNAL_HANDLER *handler;
	SIGNAL_CALLBACK *cb;
	char signum;
	int ret;

	/*old = signal(SIGCHLD, signal_child);*/
	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_raise_callbacks: fd = %d blocking = %d\n", fd, (fcntl(fd, F_GETFL) & O_NONBLOCK) == 0);
	#endif

	for(;;)
	{
		ret = read(fd, &signum, 1);
		if (ret != 1)
		{
			#if DEBUG_ME
			fprintf(stderr, "SIGNAL_raise_callbacks: read -> %d / errno = %d\n", ret, errno);
			#endif
			return;
		}
		
		#if DEBUG_ME
		fprintf(stderr, "SIGNAL_raise_callbacks: signum = %d\n", signum);
		#endif

		handler = find_handler(signum);
		if (!handler)
		{
			#if DEBUG_ME
			fprintf(stderr, "SIGNAL_raise_callbacks: no handler\n");
			#endif
			return;
		}
		
		#if DEBUG_ME
		fprintf(stderr, ">> SIGNAL_raise_callbacks (%d)\n", _raising_callback);
		#endif

		_raising_callback++;
		_purge_signum = signum;
		_purge_handler = handler;
		
		ON_ERROR(purge_callbacks)
		{
			cb = handler->callbacks;
			while (cb)
			{
				#if DEBUG_ME
				fprintf(stderr, "SIGNAL_raise_callbacks: cb = %p cb->callback = %p\n", cb, cb->callback);
				#endif
				if (cb->callback)
					(*cb->callback)((int)signum, cb->data);
				
				cb = cb->next;
			}
		}
		END_ERROR
		
		#if DEBUG_ME
		fprintf(stderr, "SIGNAL_raise_callbacks: purge_callbacks\n");
		#endif
		purge_callbacks();

		#if DEBUG_ME
		fprintf(stderr, "<< SIGNAL_raise_callbacks (%d)\n", _raising_callback);
		#endif
	}
}

static void create_pipe(void)
{
	if (pipe(_pipe) != 0)
		ERROR_panic("Cannot create signal handler pipes: %s", strerror(errno));

	if (_pipe[0] == 0)
		BREAKPOINT();
	
	fcntl(_pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(_pipe[1], F_SETFD, FD_CLOEXEC);
	// Allows to read the signal pipe without blocking
	fcntl(_pipe[0], F_SETFL, fcntl(_pipe[0], F_GETFL) | O_NONBLOCK);

	GB_Watch(_pipe[0], GB_WATCH_READ, (void *)SIGNAL_raise_callbacks, 0);
	
	#if DEBUG_ME
	fprintf(stderr, "create_pipe: fd = %d\n", _pipe[0]);
	#endif
}

static void delete_pipe(void)
{
	#if DEBUG_ME
	fprintf(stderr, "delete_pipe: fd = %d\n", _pipe[0]);
	#endif
	GB_Watch(_pipe[0], GB_WATCH_NONE, NULL, 0);
	close(_pipe[0]);
	close(_pipe[1]);
	_pipe[0] = -1;
	_pipe[1] = -1;
}

SIGNAL_CALLBACK *SIGNAL_register(int signum, void (*callback)(int, intptr_t), intptr_t data)
{
	SIGNAL_HANDLER *handler;
	SIGNAL_CALLBACK *cb;
	
	if (!_count)
		create_pipe();
	
	_count++;
	
	handler = find_handler(signum);
	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_register: find_handler(%d) -> %p\n", signum, handler);
	#endif
	
	if (!handler)
	{
		handler = add_handler();
		SIGNAL_install(handler, signum, handle_signal);
	}
	
	ALLOC(&cb, sizeof(SIGNAL_CALLBACK));
	
	cb->prev = NULL;
	cb->next = handler->callbacks;
	cb->callback = callback;
	cb->data = data;
	
	if (cb->next)
		cb->next->prev = cb;
	handler->callbacks = cb;

	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_register: %d -> %p (%p)\n", signum, cb, cb->callback);
	#endif
	
	#if DEBUG_ME
	fprintf(stderr, "handler->callbacks %p:", handler);
	SIGNAL_CALLBACK *save = cb;
	cb = handler->callbacks;
	while (cb)
	{
		fprintf(stderr, " -> %p (%p)", cb, cb->callback);
		cb = cb->next;
	}
	fprintf(stderr, "\n");
	cb = save;
	#endif

	return cb;
}

void SIGNAL_unregister(int signum, SIGNAL_CALLBACK *cb)
{
	SIGNAL_HANDLER *handler = find_handler(signum);
	
	if (!handler)
		return;
	
	if (_raising_callback)
	{
		#if DEBUG_ME
		fprintf(stderr, "SIGNAL_unregister: disable %d %p (%p)\n", signum, cb, cb->callback);
		#endif
		cb->callback = NULL;
		_must_purge_callbacks = TRUE;
		return;
	}
	
	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_unregister: remove %d %p (%p)\n", signum, cb, cb->callback);
	#endif
	
	if (cb->prev)
		cb->prev->next = cb->next;
	
	if (cb->next)
		cb->next->prev = cb->prev;
	
	if (cb == handler->callbacks)
		handler->callbacks = cb->next;
	
	IFREE(cb);
	
	_count--;
	
	if (_count == 0)
		delete_pipe();
	
	#if DEBUG_ME
	fprintf(stderr, "handler->callbacks %p:", handler);
	cb = handler->callbacks;
	while (cb)
	{
		fprintf(stderr, " -> %p (%p)", cb, cb->callback);
		cb = cb->next;
	}
	fprintf(stderr, "\n");
	#endif
}

void SIGNAL_exit(void)
{
	int i;
	SIGNAL_HANDLER *handler;
	
	if (_handlers)
	{
		_raising_callback = 0;
		for (i = 0; i < ARRAY_count(_handlers); i++)
		{
			handler = &_handlers[i];
			SIGNAL_uninstall(handler, handler->signum);
		}
		
		ARRAY_delete(&_handlers);
	}
}

int SIGNAL_get_fd(void)
{
	return _pipe[0];
}

void SIGNAL_has_forked(void)
{
	if (!_count)
		return;
	
	GB_Watch(_pipe[0], GB_WATCH_NONE, NULL, 0);
	close(_pipe[0]);
	close(_pipe[1]);
	create_pipe();
}

void SIGNAL_do_check(int signum)
{
	struct sigaction action;
	SIGNAL_HANDLER *handler = find_handler(signum);
	void (*cb)();
	
	if (!handler)
		return;
	
	sigaction(signum, NULL, &action);
	get_callback(&action, &cb);
	
	#if DEBUG_ME
	fprintf(stderr, "SIGNAL_check: %d -> %d (action.sa_sigaction = %p)\n", signum, cb == handle_signal, cb);
	if (cb != handle_signal)
		BREAKPOINT();
	#endif
	
	if (cb == handle_signal)
		return;
	
	SIGNAL_install(handler, signum, handle_signal);

	if (signum == SIGCHLD)
	{
		CPROCESS_callback_child();
		CTASK_callback_child();
	}
}

void SIGNAL_must_check(int signum)
{
	SIGNAL_check_mask |= (1 << signum);
}
