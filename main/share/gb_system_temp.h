/***************************************************************************

	gb_system_temp.h

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

#include "config.h"
#include "gb_system.h"

#include <stdio.h>

#if OS_LINUX || defined(OS_CYGWIN) || defined(__GLIBC__)

#include <sys/sysinfo.h>

int SYSTEM_get_cpu_count(void)
{
	return get_nprocs();
}

#elif OS_BSD

#include <sys/types.h>
#include <sys/sysctl.h>

int SYSTEM_get_cpu_count(void)
{
	int mib[2], ncpus;
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ncpus);
	sysctl(mib, 2, &ncpus, &len, NULL, 0);
	
	return ncpus;
}

#else

int SYSTEM_get_cpu_count(void)
{
	fprintf(stderr, "gbx" GAMBAS_VERSION_STRING ": warning: don't know how to return cpu count\n");
	return 1;
}

#endif
