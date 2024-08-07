/***************************************************************************

  gbx_c_application.c

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

#define __GBX_C_APPLICATION_C

#include "gambas.h"
#include "gbx_info.h"

#ifndef GBX_INFO

#include "config.h"

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gbx_api.h"
#include "gbx_class.h"
#include "gbx_project.h"
#include "gbx_local.h"
#include "gbx_event.h"
#include "gbx_string.h"
#include "gbx_exec.h"
#include "gbx_extern.h"
#include "gbx_object.h"

#include "gbx_c_application.h"

extern char **environ;

static bool _daemon = FALSE;

//-------------------------------------------------------------------------

BEGIN_PROPERTY(Application_Path)

	if (PROJECT_override)
		GB_ReturnNewZeroString(FILE_get_dir(PROJECT_override));
	else
		GB_ReturnString(PROJECT_path);

END_PROPERTY


BEGIN_PROPERTY(Application_Name)

	GB_ReturnConstZeroString(PROJECT_name);

END_PROPERTY


BEGIN_PROPERTY(Application_Title)

  GB_ReturnConstZeroString(LOCAL_gettext(PROJECT_title));

END_PROPERTY


BEGIN_PROPERTY(Application_Handle)

  GB_ReturnInt(getpid());

END_PROPERTY


BEGIN_PROPERTY(Application_ParentHandle)

  GB_ReturnInt(getppid());

END_PROPERTY


BEGIN_PROPERTY(Application_Dir)

  GB_ReturnString(PROJECT_oldcwd);

END_PROPERTY


BEGIN_PROPERTY(Application_Version)

  GB_ReturnConstZeroString(PROJECT_version);

END_PROPERTY


//-------------------------------------------------------------------------

BEGIN_PROPERTY(Application_Args_Count)

  GB_ReturnInt(PROJECT_argc);

END_PROPERTY

BEGIN_PROPERTY(Application_Args_Max)

  GB_ReturnInt(PROJECT_argc - 1);

END_PROPERTY


BEGIN_METHOD(Application_Args_get, GB_INTEGER index)

  int index = VARG(index);

  if (index < 0)
  {
    GB_Error((char *)E_ARG);
    return;
  }

  if (index >= PROJECT_argc)
    GB_ReturnVoidString();
  else if (index == 0 && PROJECT_override)
    GB_ReturnNewZeroString(FILE_get_name(PROJECT_override));
	else
    GB_ReturnConstZeroString(PROJECT_argv[index]);

END_METHOD


BEGIN_METHOD_VOID(Application_Args_next)

  int *index = (int*)GB_GetEnum();

  if (*index >= PROJECT_argc)
    GB_StopEnum();
  else
  {
    GB_ReturnConstZeroString(PROJECT_argv[*index]);
    (*index)++;
  }

END_METHOD


BEGIN_PROPERTY(Application_Args_Copy)

	GB_ARRAY array;
	int i;
	char *arg;

	GB_ArrayNew(&array, GB_T_STRING, PROJECT_argc);
	for (i = 0; i < PROJECT_argc; i++)
	{
		arg = PROJECT_argv[i];
		if (arg && *arg)
			*(char **)GB_ArrayGet(array, i) = STRING_new_zero(arg);
	}

  GB_ReturnObject(array);

END_PROPERTY

/*BEGIN_PROPERTY(Application_Args_Name)

	if (READ_PROPERTY)
    GB_ReturnConstZeroString(PROJECT_argv[0]);
	else
	{
		GB_StoreString(PROP(GB_STRING), &PROJECT_argname);
		PROJECT_argv[0] = PROJECT_argname;
	}

END_PROPERTY*/

//-------------------------------------------------------------------------

static int get_environ_count()
{
	int n = 0;

	while(environ[n])
		n++;
	
	return n;
}

BEGIN_PROPERTY(Application_Env_Count)

  GB_ReturnInteger(get_environ_count());

END_PROPERTY


BEGIN_PROPERTY(Application_Env_Max)

  GB_ReturnInteger(get_environ_count() - 1);

END_PROPERTY


BEGIN_METHOD(Application_Env_get, GB_STRING key)

  GB_ReturnNewZeroString(getenv(GB_ToZeroString(ARG(key))));

END_METHOD


BEGIN_METHOD(Application_Env_put, GB_STRING value; GB_STRING key)

  setenv(GB_ToZeroString(ARG(key)), GB_ToZeroString(ARG(value)), 1);

END_METHOD


BEGIN_METHOD_VOID(Application_Env_next)

  int *index = (int*)GB_GetEnum();
  char *pos;
  char *key;

  if (environ[*index] == NULL)
    GB_StopEnum();
  else
  {
    key = environ[*index];
    pos = strchr(key, '=');
    if (!pos)
      GB_ReturnVoidString();
    else
      GB_ReturnConstString(key, pos - key);
    (*index)++;
  }

END_METHOD

BEGIN_PROPERTY(Application_Env_Copy)

	GB_ARRAY array;
	int i, n;
	char *arg;

	n = get_environ_count();
	GB_ArrayNew(&array, GB_T_STRING, n - 1);
	for (i = 0; i < n; i++)
	{
		arg = environ[i];
		if (arg && *arg)
			*(char **)GB_ArrayGet(array, i) = STRING_new_zero(environ[i]);
	}

  GB_ReturnObject(array);

END_PROPERTY

//-------------------------------------------------------------------------

static void init_again(pid_t old_pid)
{
	char old[PATH_MAX];

	FILE_remove_temp_file();
	snprintf(old, sizeof(old), FILE_TEMP_DIR, (int)getuid(), (int)old_pid);
	rename(old, FILE_make_temp(NULL, NULL));
	FILE_chdir(PROJECT_path);
}

BEGIN_PROPERTY(Application_Daemon)

	pid_t old_pid;

	if (READ_PROPERTY)
		GB_ReturnBoolean(_daemon);
	else
	{
		if (!_daemon && VPROP(GB_BOOLEAN))
		{
			old_pid = getpid();
			if (daemon(FALSE, FALSE))
				THROW_SYSTEM(errno, NULL);
			// Argh! daemon() changes the current process id...
			_daemon = TRUE;
			init_again(old_pid);
		}
	}

END_PROPERTY


BEGIN_PROPERTY(Application_Startup)

	GB_ReturnObject(PROJECT_class);

END_PROPERTY


BEGIN_PROPERTY(Application_Priority)

	int pr;

	if (READ_PROPERTY)
	{
		errno = 0;
		pr = getpriority(PRIO_PROCESS, 0);
		if (pr == -1 && errno > 0)
			THROW_SYSTEM(errno, NULL);
		GB_ReturnInteger(pr);
	}
	else
	{
		pr = VPROP(GB_INTEGER);

		if (pr < -20)
			pr = -20;
		else if (pr > 19)
			pr = 19;

		if (setpriority(PRIO_PROCESS, 0, pr) < 0)
			THROW_SYSTEM(errno, NULL);
	}

END_PROPERTY


BEGIN_PROPERTY(Application_Task)

  GB_ReturnBoolean(FLAG.task);

END_PROPERTY


BEGIN_PROPERTY(Application_TempDir)

	GB_ReturnNewZeroString(FILE_make_temp(NULL, NULL));

END_PROPERTY

#endif

//-------------------------------------------------------------------------

GB_DESC NATIVE_AppArgs[] =
{
  GB_DECLARE_STATIC("Args"),

  GB_STATIC_PROPERTY_READ("Count", "i", Application_Args_Count),
  GB_STATIC_PROPERTY_READ("Max", "i", Application_Args_Max),
  GB_STATIC_PROPERTY_READ("All", "String[]", Application_Args_Copy),
  GB_STATIC_METHOD("Copy", "String[]", Application_Args_Copy, NULL),
  GB_STATIC_METHOD("_get", "s", Application_Args_get, "(Index)i"),
  GB_STATIC_METHOD("_next", "s", Application_Args_next, NULL),
  //GB_STATIC_PROPERTY("Name", "s", Application_Args_Name),

  GB_END_DECLARE
};


GB_DESC NATIVE_AppEnv[] =
{
  GB_DECLARE_STATIC("Env"),

  GB_STATIC_PROPERTY_READ("Count", "i", Application_Env_Count),
  GB_STATIC_PROPERTY_READ("Max", "i", Application_Env_Max),
  GB_STATIC_METHOD("Copy", "String[]", Application_Env_Copy, NULL),
  GB_STATIC_METHOD("_get", "s", Application_Env_get, "(Key)s"),
  GB_STATIC_METHOD("_put", NULL, Application_Env_put, "(Value)s(Key)s"),
  GB_STATIC_METHOD("_next", "s", Application_Env_next, NULL),

  GB_END_DECLARE
};


GB_DESC NATIVE_App[] =
{
  GB_DECLARE_STATIC("Application"),

  GB_STATIC_PROPERTY_SELF("Args", "Args"),
  GB_STATIC_PROPERTY_SELF("Env", "Env"),
  GB_STATIC_PROPERTY_READ("Path", "s", Application_Path),
  GB_STATIC_PROPERTY_READ("Name", "s", Application_Name),
  GB_STATIC_PROPERTY_READ("Title", "s", Application_Title),
  GB_STATIC_PROPERTY_READ("Id", "i", Application_Handle),
  GB_STATIC_PROPERTY_READ("Handle", "i", Application_Handle),
  GB_STATIC_PROPERTY_READ("ParentHandle", "i", Application_ParentHandle),
  GB_STATIC_PROPERTY_READ("Version", "s", Application_Version),
  GB_STATIC_PROPERTY_READ("Dir", "s", Application_Dir),
  GB_STATIC_PROPERTY_READ("TempDir", "s", Application_TempDir),
  GB_STATIC_PROPERTY("Daemon", "b", Application_Daemon),
  GB_STATIC_PROPERTY_READ("Startup", "Class", Application_Startup),
	GB_STATIC_PROPERTY("Priority", "i", Application_Priority),
  GB_STATIC_PROPERTY_READ("Task", "b", Application_Task),

  GB_END_DECLARE
};

