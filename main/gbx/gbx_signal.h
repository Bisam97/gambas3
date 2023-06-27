/***************************************************************************

  gbx_signal.h

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

#ifndef __GBX_SIGNAL_H
#define __GBX_SIGNAL_H

#include <unistd.h>
#include <signal.h>

#ifndef __GBX_SIGNAL_C
extern uint64_t SIGNAL_check_mask;
#endif

typedef
	struct SIGNAL_CALLBACK {
		struct SIGNAL_CALLBACK *prev;
		struct SIGNAL_CALLBACK *next;
		void (*callback)(int, intptr_t);
		intptr_t data;
	}
	SIGNAL_CALLBACK;

typedef
	struct {
		int signum;
		struct sigaction old_action;
		SIGNAL_CALLBACK *callbacks;
	}
	SIGNAL_HANDLER;
	
void SIGNAL_install(SIGNAL_HANDLER *handler, int signum, void (*callback)(int, siginfo_t *, void *));
void SIGNAL_uninstall(SIGNAL_HANDLER *handler, int signum);
void SIGNAL_previous(SIGNAL_HANDLER *handler, int signum, siginfo_t *info, void *context);
void SIGNAL_do_check(int signum);
void SIGNAL_must_check(int signum);

#define SIGNAL_check(_signum) { if (SIGNAL_check_mask & (1 << (_signum))) SIGNAL_do_check(_signum); }

SIGNAL_CALLBACK *SIGNAL_register(int signum, void (*callback)(int, intptr_t), intptr_t data);
void SIGNAL_unregister(int signum, SIGNAL_CALLBACK *cb);

int SIGNAL_get_fd(void);
void SIGNAL_raise_callbacks(int fd, int type, void *data);
void SIGNAL_exit(void);
void SIGNAL_has_forked(void);

#endif

 
