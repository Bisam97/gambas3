/***************************************************************************

  gbx.c

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

#define __GBX_C

#include "config.h"

//#define USE_PROFILE 1

#include "gb_common.h"
#include "gb_alloc.h"
#include "gb_error.h"

#include <dlfcn.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "gbx_class.h"
#include "gbx_exec.h"
#include "gbx_stack.h"
#include "gbx_debug.h"
#include "gb_file.h"
#include "gbx_component.h"
#include "gbx_project.h"
#include "gbx_local.h"
#include "gbx_watch.h"
#include "gbx_event.h"
#include "gbx_extern.h"
#include "gbx_eval.h"
#include "gbx_subr.h"
#include "gbx_math.h"
#include "gb_common_buffer.h"
#include "gbx_api.h"
#include "gbx_signal.h"
#include "gbx_jit.h"

#if USE_PROFILE
#include "gbx_profile.h"
#endif

#include "gbx_c_file.h"
#include "gbx_c_application.h"
#include "gbx.h"

extern void _exit(int) NORETURN;
FILE *log_file;

static bool _welcome = FALSE;
static bool _quit_after_main = FALSE;
static bool _eval = FALSE;
static const char *_tests = NULL;

static void NORETURN do_exit(int ret, bool immediate)
{
	fflush(NULL);

	if (FLAG.debug_hold)
	{
		if (ret) ERROR_warning("The process returned %d", ret);
		sleep(86400);
	}
	
	if (immediate || FLAG.has_forked)
		_exit(ret);
	else
		exit(ret);
}

static void NORETURN my_exit(int ret)
{
	LOCAL_exit();
	COMPONENT_exit();
	EXTERN_exit();
	//fclose(log_file);
	do_exit(ret, FALSE);
}

static void init(const char *file, int argc, char **argv)
{
	COMPONENT_init();
	FILE_init();
	JIT_init();
	EXEC_init();
	CLASS_init();
	WATCH_init();
	MATH_init();
	PROJECT_init(file);

	LOCAL_init();
	STACK_init();

	if (file)
	{
		PROJECT_load();
		DEBUG_init();

		if (PROJECT_run_httpd)
			COMPONENT_exec("gb.httpd", argc, argv);

		PROJECT_load_finish();
	}
	else
		DEBUG_init();


	if (FLAG.debug)
	{
		DEBUG.Welcome();
		DEBUG.Main(FALSE);
	}
	_welcome = TRUE;
}


void main_exit(bool silent)
{
	silent |= FLAG.task;
	
	// If the stack has not been initialized because the project could not be started, do it now
	if (!SP)
		STACK_init();

	TRY
	{
		JIT_abort();
		SIGNAL_exit();
		EXTERN_release();
		STREAM_exit();
		OBJECT_exit();
		CFILE_exit();

		CLASS_clean_up(silent);

		SUBR_exit();
		DEBUG_exit();
		WATCH_exit();
		#if USE_PROFILE
		PROFILE_exit();
		#endif
		CLASS_exit();
		COMPONENT_exit();
		EXTERN_exit();
		PROJECT_exit();
		LOCAL_exit();
		EVENT_exit();
		FILE_exit();
		STACK_exit();
		JIT_exit();
		ERROR_exit();
	}
	CATCH
	{
		if (!silent)
			ERROR_print_at(stderr, _eval, TRUE);
		do_exit(1, TRUE);
	}
	END_TRY

	STRING_exit();
}


void NORETURN MAIN_exit(bool silent, int ret)
{
	main_exit(silent);
	do_exit(ret, TRUE);
}


static bool is_option(const char *arg, char option)
{
	return (arg[0] == '-' && arg[1] == option && arg[2] == 0);
}

static bool is_long_option(const char *arg, char option, const char *long_option)
{
	if (is_option(arg, option))
		return TRUE;
	else
		return (arg[0] == '-' && arg[1] == '-' && !strcmp(&arg[2], long_option));
}

static bool is_option_arg(char **argv, int argc, int *i, char option, const char *long_option, const char **param)
{
	if (long_option)
	{
		if (!is_long_option(argv[*i], option, long_option))
			return FALSE;
	}
	else
	{
		if (!is_option(argv[*i], option))
			return FALSE;
	}
	
	if (*i < (argc - 1) && *argv[*i + 1] != '-')
	{
		*param = argv[*i + 1];
		(*i)++;
	}
	else
	{
		if (long_option)
			ERROR_fatal("--%s option requires an argument", long_option);
		else
			ERROR_fatal("-%c option requires an argument", option);
	}

	return TRUE;
}

static void print_version()
{
#ifdef TRUNK_VERSION
	printf(VERSION " " TRUNK_VERSION "\n");
#else /* no TRUNK_VERSION */
	printf(VERSION "\n");
#endif
}

static void print_title()
{
	printf("\nGambas interpreter version ");
	print_version();
}

int main(int argc, char *argv[])
{
	CLASS_DESC_METHOD *startup = NULL;
	int i, n;
	char *file = NULL;
	int ret = 0;
	const char *redirect_stderr = NULL;

	//char log_path[256];
	//sprintf(log_path, "/tmp/gambas-%d.log", getuid());
	//log_file = freopen(log_path, "w+", stderr);
	//fprintf(stderr, "Fichier log Gambas\n");

	/*struct rlimit rl = { 64000000, 64000000 };
	if (setrlimit(RLIMIT_CORE, &rl))
		perror(strerror(errno));*/

	MEMORY_init();
	COMMON_init();
	//STRING_init();

	FLAG.arch = (strcmp(FILE_get_name(argv[0]), "gbr" GAMBAS_VERSION_STRING) == 0);

	if (argc == 2)
	{
		if (is_long_option(argv[1], 'h', "help"))
		{
			print_title();
			
			if (FLAG.arch)
			{
				printf(
					"\nExecute a Gambas executable.\n"
					"\n    gbr" GAMBAS_VERSION_STRING " [options] <executable file> [<arguments>]\n\n"
					);
			}
			else
			{
				printf(
					"\nExecute a Gambas project.\n"
					"\n    gbx" GAMBAS_VERSION_STRING " [options] [<project directory>] [-- <arguments>]\n"
					"\nEvaluate a Gambas expression.\n"
					"\n    gbx" GAMBAS_VERSION_STRING " -e <expression>\n\n"
					);
			}

			printf("Options:\n\n");
			
			printf(
				"  -a <path>       override application path\n"
			);
			
			if (!FLAG.arch)
				printf("  -e              evaluate an expression\n");
			
			printf(
				"  -g              enter debugging mode\n"
				"  -h --help       display this help\n"
				"  -H --httpd      run through an embedded http server\n"
				"  -j              disable just-in-time compiler\n"
				"  -k              do not unload shared libraries\n"
				"  -L --license    display license\n"
				"  -p <path>       activate profiling and debugging mode\n"
				"  -r <path>       redirect standard error output\n"
				"  -s <class>      override startup class\n"
				"  -t --trace      dump the position of each executed line of code\n"
				"  -T              list all test modules\n"
				"  -T <tests>      run the specified test modules\n"
				"  -V --version    display version\n"
				"\n"
				);

			my_exit(0);
		}
		else if (is_long_option(argv[1], 'V', "version"))
		{
			print_version();
			my_exit(0);
		}
		else if (is_long_option(argv[1], 'L', "license"))
		{
			print_title();
			printf(COPYRIGHT);
			my_exit(0);
		}
	}

	if (!FLAG.arch && argc >= 2 && is_option(argv[1], 'e'))
	{
		if (argc < 3)
			ERROR_fatal("-e option requires an expression.");

		_eval = TRUE;

		TRY
		{
			init(NULL, argc, argv);
			EVAL_string(argv[2]);
		}
		CATCH
		{
			if (ERROR_current->info.code && ERROR_current->info.code != E_ABORT)
				ERROR_print_at(stderr, TRUE, TRUE);
			MAIN_exit(TRUE, 1);
		}
		END_TRY

		MAIN_exit(FALSE, 0);
	}

	for (i = 1; i < argc; i++)
	{
		if (is_option(argv[i], 'g'))
		{
			FLAG.debug = TRUE;
		}
		else if (is_option_arg(argv, argc, &i, 'p', NULL, &EXEC_profile_path))
		{
			FLAG.debug = TRUE;
			FLAG.profile = TRUE;
		}
		else if (is_long_option(argv[i], 't', "trace"))
		{
			FLAG.trace = TRUE;
		}
		else if (is_option_arg(argv, argc, &i, 'f', NULL, &EXEC_fifo_name))
		{
			FLAG.fifo = TRUE;
		}
		else if (is_option_arg(argv, argc, &i, 's', NULL, &PROJECT_startup))
		{
			continue;
		}
		else if (is_option_arg(argv, argc, &i, 'r', NULL, &redirect_stderr))
		{
			int fd = open(redirect_stderr, O_WRONLY | O_CLOEXEC);
			if (fd < 0)
				ERROR_fatal("cannot redirect stderr.");
			dup2(fd, STDERR_FILENO);
		}
		else if (is_option(argv[i], 'k'))
		{
			FLAG.keep_library = TRUE;
		}
		else if (is_option(argv[i], 'q'))
		{
			_quit_after_main = TRUE;
		}
		else if (is_long_option(argv[i], 'H', "httpd"))
		{
			PROJECT_run_httpd = TRUE;
		}
		else if (is_option(argv[i], 'j'))
		{
			JIT_disabled = TRUE;
		}
		else if (is_option_arg(argv, argc, &i, 'T', "test", &_tests))
		{
			PROJECT_run_tests = TRUE;
		}
		else if (is_option_arg(argv, argc, &i, 'a', NULL, &PROJECT_override))
		{
		}
		else if (is_option(argv[i], '-'))
		{
			i++;
			break;
		}
		else
		{
			if (file)
				ERROR_fatal("too many %s.", FLAG.arch ? "executable files" : "project directories");

			file = argv[i];

			if (FLAG.arch)
			{
				i++;
				break;
			}
		}
	}
	
	n = i;
	if (!file)
		file = ".";

	if (FLAG.arch)
		argv[0] = file;

	for (i = 1; i <= (argc - n); i++)
		argv[i] = argv[i + n - 1];

	argc -= n - 1;

	TRY
	{
		init(file, argc, argv);

		if (!FLAG.arch)
			argv[0] = PROJECT_name;

		HOOK(main)(&argc, &argv);
		FLAG.main_hook_done = TRUE;

		/* Startup class */
		
		CLASS_load(PROJECT_class);
		startup = (CLASS_DESC_METHOD *)CLASS_get_symbol_desc_kind(PROJECT_class, "main", CD_STATIC_METHOD, 0, T_ANY);
		if (startup == NULL)
			THROW(E_MAIN);

		//CAPP_init(); /* needs startup class */
		CFILE_init_watch();

		PROJECT_argc = argc;
		PROJECT_argv = argv;
	}
	CATCH
	{
		ERROR_hook();

		if (FLAG.debug && DEBUG_is_init())
		{
			if (!_welcome)
				DEBUG.Main(TRUE);
			DEBUG.Main(TRUE);
			MAIN_exit(FALSE, 0);
		}
		else
		{
			if (ERROR->info.code && ERROR->info.code != E_ABORT)
				ERROR_print(FALSE);
			MAIN_exit(TRUE, 1);
		}
	}
	END_TRY

	TRY
	{
		if (PROJECT_run_tests)
		{
			GB_Push(1, T_STRING, _tests, -1);
			EXEC_public_desc(PROJECT_class, NULL, startup, 1);
		}
		else
		{
			EXEC_public_desc(PROJECT_class, NULL, startup, 0);
		}

		if (TYPE_is_boolean(startup->type))
			ret = RP->_boolean.value ? 1 : 0;
		else if (TYPE_is_integer(startup->type))
			ret = RP->_integer.value & 0xFF;

		EXEC_release_return_value();

		if (_quit_after_main)
			MAIN_exit(TRUE, 0);

		if (!ret)
		{
			DEBUG_enter_event_loop();
			HOOK_DEFAULT(loop, WATCH_loop)();
			DEBUG_leave_event_loop();
			EVENT_check_post();
		}
	}
	CATCH
	{
		ret = EXEC_quit_value;

		if (ERROR->info.code)
		{
			if (ERROR->info.code == E_ABORT)
			{
				FILE_exit();
				do_exit(ret, TRUE);
			}

			ERROR_hook();

			if (FLAG.debug)
			{
				DEBUG.Main(TRUE);
				MAIN_exit(TRUE, 0);
			}
			else
			{
				ERROR_print(FALSE);
				MAIN_exit(TRUE, 1);
			}
		}
	}
	END_TRY

	main_exit(FALSE);

	if (!FLAG.task)
		MEMORY_exit();

	do_exit(ret, FALSE);
}

