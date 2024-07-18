/***************************************************************************

  gbc.c

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

#define __GBC_C

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>
#include <signal.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gb_str.h"
#include "gb_file.h"
#include "gb_common_buffer.h"
#include "gb_system.h"

#include "gbc_compile.h"

#include "gb_reserved.h"
#include "gbc_read.h"
#include "gbc_form.h"
#include "gbc_trans.h"
#include "gbc_header.h"
#include "gbc_output.h"

#include "gb_system_temp.h"

typedef
	void (*BACKGROUND_TASK)(const char *);

typedef
	struct {
		const char *option;
		bool *variable;
	}
	COMPILER_FLAGS;
	
static bool main_debug = FALSE;
static bool main_exec = FALSE;
static bool main_compile_all = FALSE;
static bool main_trans = FALSE;
static bool main_warnings = FALSE;
static bool main_swap = FALSE;
static bool main_quiet = FALSE;

static bool _opt_no_old_read_syntax = FALSE;
static bool _opt_check_prefix = FALSE;
static bool _opt_public_module = FALSE;
static bool _opt_public_control = FALSE;

static char *_convert_form = NULL;

#if HAVE_GETOPT_LONG
static struct option _long_options[] =
{
	{ "debug", 0, NULL, 'g' },
	{ "version", 0, NULL, 'V' },
	{ "help", 0, NULL, 'h' },
	{ "license", 0, NULL, 'L' },
	{ "verbose", 0, NULL, 'v' },
	{ "translate", 0, NULL, 't' },
	{ "swap", 0, NULL, 's' },
	{ "all", 0, NULL, 'a' },
	{ "translate-errors", 0, NULL, 'e' },
	{ "warnings", 0, NULL, 'w' },
	{ "quiet", 0, NULL, 'q' },
	
	{ "jobs", 1, NULL, 'j' },
	{ "root", 1, NULL, 'r' },
	{ "bytecode-version", 1, NULL, 'b' },
	{ "default-namespace", 1, NULL, 'n' },
	{ "form", 1, NULL, 'F' },

	//{ "no-old-read-write-syntax", 0, &_opt_no_old_read_syntax, 1 },
	//{ "check-prefix", 0, &_opt_check_prefix, 1},
	
	{ 0 }
};
#endif

static COMPILER_FLAGS _compiler_flags[] = {
	{ "no-old-read-write-syntax", &_opt_no_old_read_syntax },
	{ "check-prefix", &_opt_check_prefix },
	{ "public-module", &_opt_public_module },
	{ "public-control", &_opt_public_control },
	{ NULL }
};

//static char *main_class_file = NULL;

static char **_files = NULL;
static bool make_test = FALSE;

static uint _ntask_max = 0;
static uint _ntask = 0;
static bool _child = FALSE;

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
	printf("\nGambas compiler version ");
	print_version();
}

static void get_arguments(int argc, char **argv)
{
	const char *dir;
	int opt;
	#if HAVE_GETOPT_LONG
	int index = 0;
	#endif
	COMPILER_FLAGS *cf;

	for(;;)
	{
		#if HAVE_GETOPT_LONG
			opt = getopt_long(argc, argv, "gxvaVhj:Lwtser:f:n:F:b:q", _long_options, &index);
		#else
			opt = getopt(argc, argv, "gxvaVhj:Lwtser:f:n:F:b:q");
		#endif
		if (opt < 0) break;

		switch (opt)
		{
			case 'V':
				
				print_version();
				exit(0);

			case 'g':
				
				main_debug = TRUE;
				break;

			case 'x':
				
				main_exec = TRUE;
				break;

			case 'v':
				
				COMP_verbose = TRUE;
				break;

			case 'a':
				
				main_compile_all = TRUE;
				break;

			case 't':
				
				main_trans = TRUE;
				break;

			case 'w':
				
				main_warnings = TRUE;
				break;
				
			case 'q':
				
				main_quiet = TRUE;
				break;

			case 's':
				
				main_swap = TRUE;
				break;

			case 'r':
				
				if (COMP_root)
					ERROR_fail("option '-r' already specified.");
				COMP_root = STR_copy(optarg);
				break;

			case 'b':
				if (COMP_bytecode)
					ERROR_fail("option '-b' already specified.");
				if (strcmp(optarg, "3.8") && strcmp(optarg, "3.15") && strcmp(optarg, "3.18") && strcmp(optarg, "3.19") && strcmp(optarg,"last"))
					ERROR_fail("option '-b' takes one of the following values: '3.8', '3.15', '3.18', '3.19' or 'last'.");
				COMP_bytecode = STR_copy(optarg);
				break;

			case 'e':
				
				ERROR_translate = TRUE;
				break;
				
			case 'L':
				
				print_title();
				printf(COPYRIGHT);
				exit(0);

			case 'h': case '?':
				
				print_title();
				
				printf(
					"\nCompile Gambas projects into architecture-independent bytecode.\n"
					"\n    gbc" GAMBAS_VERSION_STRING " [options] [<project directory>]\n"
					"\nConvert a form file into Gambas code.\n"
					"\n    gbc" GAMBAS_VERSION_STRING " -F <form file>\n\n"
					"Options:"
					#if HAVE_GETOPT_LONG
					"\n\n"
					"  -a --all                              compile all files\n"
					"  -b --bytecode-version                 force a specific bytecode version\n"
					"  -e --translate-errors                 display translatable error messages\n"
					"  -F --form <form file>                 convert a form file into code and print it\n"
					"  -g --debug                            add debugging information\n"
					"  -h --help                             display this help\n"
					"  -j --jobs <count>                     number of background jobs (default: %d)\n"
					"  -L --license                          display license\n"
					"  -n --default-namespace <namespace>    default namespace for exported classes\n"
					"  -q --quiet                            do not print anything if compilation succeeds\n"
					"  -r --root <directory>                 gives the gambas installation directory\n"
					"  -s --swap                             swap endianness\n"
					"  -t --translate                        output translation files and compile them if needed\n"
					"  -v --verbose                          verbose output\n"
					"  -V --version                          display version\n"
					"  -w --warnings                         display warnings\n"
					"  -x --exec                             executable mode (define the 'Exec' preprocessor constant and remove assertions)\n"
					"\nCompiler flags:\n\n"
					"  -f check-prefix                       check the prefix of variables if warnings are enable\n"
					"  -f public-module                      module symbols are public by default\n"
					"  -f public-control                     form controls are public\n"
					#else
					" (no long options on this system)\n\n"
					"  -a                   compile all files\n"
					"  -b                   force a specific bytecode version\n"
					"  -e                   display translatable error messages\n"
					"  -F <form file>       convert a form file into code and print it\n"
					"  -g                   add debugging information\n"
					"  -h                   display this help\n"
					"  -j <count>           number of background jobs (default: %d)\n"
					"  -L                   display license\n"
					"  -n <namespace>       default namespace for exported classes\n"
					"  -q                   do not print anything if compilation succeeds\n"
					"  -r <directory>       gives the gambas installation directory\n"
					"  -s                   swap endianness\n"
					"  -t                   output translation files and compile them if needed\n"
					"  -v                   verbose output\n"
					"  -V                   display version\n"
					"  -w                   display warnings\n"
					"  -x                   executable mode (define the 'Exec' preprocessor constant and remove assertions)\n"
					"\nCompiler flags:\n\n"
					"  -f check-prefix      check the prefix of variables if warnings are enable\n"
					"  -f public-module     module symbols are public by default\n"
					"  -f public-control    form controls are public\n"
					#endif
					"\n",
					SYSTEM_get_cpu_count());

				exit(0);
				
			case 'j':
				
				_ntask_max = atoi(optarg);
				if (_ntask_max < 0 || _ntask_max > 16)
					ERROR_fail("Incorrect number of jobs.");
				break;
				
			case 'f':
				
				cf = _compiler_flags;
				while (cf->option)
				{
					if (strcmp(optarg, cf->option) == 0)
					{
						*cf->variable = TRUE;
						break;
					}
					cf++;
				}
				
				break;
				
			case 'n':
				
				if (COMP_default_namespace)
					ERROR_fail("option '-n' already specified.");
				COMP_default_namespace = STR_copy(optarg);
				break;
				
			case 'F':
				
				if (_convert_form)
					ERROR_fail("option '-F' already specified.");
				_convert_form = STR_copy(optarg);
				break;

			default:
				exit(1);

		}
	}

	if (optind < (argc - 1))
		ERROR_fail("too many arguments");

	if (_convert_form)
		return;
	
	if (optind < argc)
		FILE_chdir(argv[optind]);

	dir = FILE_get_current_dir();
	if (!dir)
		ERROR_fail("no current directory");

	COMP_dir = STR_copy(dir);
	COMP_project = STR_copy(FILE_cat(COMP_dir, ".project", NULL));

	if (!FILE_exist(COMP_project))
		ERROR_fail("project file not found: %s", COMP_project);
}


static void compile_file(const char *file)
{
	int i, lock;
	time_t time_src, time_form, time_pot, time_output;
	char *source;

	COMPILE_begin(file, main_trans, main_debug);

	if (!main_compile_all)
	{
		if (FILE_exist(JOB->output))
		{
			time_src = FILE_get_time(JOB->name);
			time_output = FILE_get_time(JOB->output);

			if (JOB->form)
				time_form = FILE_get_time(JOB->form);
			else
				time_form = time_src;

			if (main_trans)
				time_pot = FILE_get_time(JOB->tname);
			else
				time_pot = time_src;

			if (time_src <= time_output && time_src <= time_pot && time_form <= time_output)
				goto _FIN;
		}
	}

	COMPILE_alloc();
	
	JOB->exec = main_exec;
	JOB->warnings = main_warnings;
	JOB->check_prefix = main_warnings && _opt_check_prefix;
	JOB->swap = main_swap;
	JOB->public_module = _opt_public_module;
	JOB->no_old_read_syntax = _opt_no_old_read_syntax;
	//JOB->class_file = main_class_file;

	if (COMP_verbose)
	{
		fputc('\n', stderr);
		for (i = 1; i <= 9; i++)
			fprintf(stderr, "--------");
		fprintf(stderr, "\nCompiling %s...\n", FILE_get_name(JOB->name));
	}

	JOB->first_line = 1;

	if (JOB->form)
	{
		JOB->first_line = FORM_FIRST_LINE;
		BUFFER_add(&JOB->source, "#Line " FORM_FIRST_LINE_STRING "\n", -1);

		BUFFER_create(&source);
		BUFFER_load_file(&source, JOB->form);
		BUFFER_add(&source, "\n\0", 2);

		FORM_set_target(&JOB->source);
		
		switch (JOB->family->type)
		{
			case FORM_WEBPAGE:
				FORM_webpage(source);
				break;

			case FORM_NORMAL:
			default:
				FORM_do(source, FALSE, _opt_public_control);
				break;
		}

		BUFFER_delete(&source);

		BUFFER_add(&JOB->source, "#Line 1\n", -1);
	}

	COMPILE_load();
	BUFFER_add(&JOB->source, "\n\0", 2);

	JOB->step = JOB_STEP_READ;
	READ_do();

	#ifdef DEBUG
	TABLE_print(JOB->class->table, TRUE);
	#endif

	JOB->step = JOB_STEP_CODE;
	HEADER_do();
	TRANS_code();

	#ifdef DEBUG
	TABLE_print(JOB->class->string, FALSE);
	#endif

	JOB->step = JOB_STEP_OUTPUT;
	OUTPUT_do(main_swap);
	
	lock = COMPILE_lock_file(".gbc.lock");
	CLASS_export();
	COMPILE_unlock_file(lock);
	
	COMPILE_free();
	
_FIN:
	COMPILE_end();
}


static void wait_for_task(void)
{
	int status;
	pid_t pid;
	
	if (COMP_verbose)
		fprintf(stderr, "gbc" GAMBAS_VERSION_STRING ": wait for tasks...\n");
	
	pid = wait(&status);
	
	if (pid < 0)
		ERROR_fail("wait() fails: &1", strerror(errno));
	
	if (!WIFEXITED(status))
	{
		fprintf(stderr, "gbc" GAMBAS_VERSION_STRING ": the background job %d has crashed with signal %d", pid, WTERMSIG(status));
		exit(status);
	}
	
	if (WEXITSTATUS(status))
	{
		while (_ntask > 0)
		{
			wait(&status);
			_ntask--;
		}
		exit(1);
	}
	
	if (COMP_verbose)
		fprintf(stderr, "gbc" GAMBAS_VERSION_STRING ": end task %d\n", pid);
	_ntask--;
}


static void wait_for_all_task(void)
{
	if (_ntask_max > 1)
	{
		while (_ntask > 0)
			wait_for_task();
	}
}


/*static void kill_tasks(void)
{
	if (_ntask > 0)
	{
		if (COMP_verbose)
			fprintf(stderr, "gbc" GAMBAS_VERSION_STRING ": kill pending tasks\n");
		kill(-getpid(), SIGTERM);
	}
}*/

static void run_task(BACKGROUND_TASK func, const char *arg)
{
	pid_t pid;
	
	if (_ntask_max <= 1)
	{
		(*func)(arg);
		return;
	}
	
	while (_ntask >= _ntask_max)
		wait_for_task();
	
	pid = fork();
	
	if (pid < 0)
		THROW("Failed to run child process: &1", strerror(errno));
	
	if (pid == 0)
	{
		_child = TRUE;
		_ntask = 0;
		
		if (setpgid(0, getppid()) < 0)
			ERROR_fail("[%d] setpgid to %d failed: %s", getpid(), getppid(), strerror(errno));
		
		(*func)(arg);
		exit(0);
	}
	else
	{
		if (COMP_verbose)
			fprintf(stderr, "gbc" GAMBAS_VERSION_STRING ": start task: [%d] '%s'\n", pid, arg);
		_ntask++;
	}
}


static int compare_path(char **a, char **b)
{
	return strcmp(*a, *b);
}

/*static bool check_cvs_directory(const char *dir)
{
	int len;
	char *buffer;
	struct stat info;
	
	len = strlen(dir);
	buffer = alloca(len + 32);
	strcpy(buffer, dir);
	
	strcpy(&buffer[len], "Root");
	if (stat(buffer, &info))
		return FALSE;
	
	strcpy(&buffer[len], "Entries");
	if (stat(buffer, &info))
		return FALSE;

	strcpy(&buffer[len], "Repository");
	if (stat(buffer, &info))
		return FALSE;

	return TRUE;
}*/

static void fill_files(const char *root, bool recursive)
{
	DIR *dir;
	char *path;
	struct dirent *dirent;
	char *file_name;
	const char *file;
	struct stat info;
	const char *ext;

	path = STR_copy(root);

	dir = opendir(path);
	if (!dir)
		ERROR_fail("cannot browse directory: %s", path);

	while ((dirent = readdir(dir)) != NULL)
	{
		file_name = dirent->d_name;
		if (*file_name == '.')
			continue;

		file = FILE_cat(path, file_name, NULL);

		if (stat(file, &info))
		{
			ERROR_warning("cannot stat file: %s", file);
			continue;
		}

		if (S_ISDIR(info.st_mode))
		{
			if (recursive)
			{
				if (*file_name == 'C')
				{
					if (strcmp(file_name, "CVS") == 0) // && check_cvs_directory(file))
						continue;
					if (strcmp(file_name, "CVSROOT") == 0)
						continue;
				}
				fill_files(file, TRUE);
			}
		}
		else
		{
			ext = FILE_get_ext(file);

			if ((strcmp(ext, "module") == 0)
					|| (strcmp(ext, "class") == 0))
			{
				*((char **)ARRAY_add(&_files)) = STR_copy(file);
			}
			else if (strcmp(ext, "test") == 0)
			{
				*((char **)ARRAY_add(&_files)) = STR_copy(file);
				make_test = TRUE;
			}
		}
	}

	closedir(dir);
	STR_free(path);
}


static void init_files(const char *first)
{
	bool recursive;
	const char *name;
	const char *ext;
	int i, n;
	bool has_test;

	ARRAY_create(&_files);

	if (*first)
		FILE_chdir(first);

	recursive = chdir(".src") == 0;
	fill_files(FILE_get_current_dir(), recursive);
	if (recursive) FILE_chdir("..");

	// Sort paths
	n = ARRAY_count(_files);
	qsort(_files, n, sizeof(*_files), (int (*)(const void *, const void *))compare_path);

	// Add the classes to the list of classes
	has_test = FALSE;
	for (i = 0; i < n; i++)
	{
		if (!has_test)
		{
			ext = FILE_get_ext(_files[i]);
			if (strcmp(ext, "test") == 0)
			{
				has_test = TRUE;
				COMPILE_add_component("gb.test");
			}
		}
		name = FILE_get_basename(_files[i]);
		COMPILE_add_class(name, strlen(name));
	}

	// End the list of classes
	COMPILE_end_class();
}


static void exit_files(void)
{
	int i;

	for (i = 0; i < ARRAY_count(_files); i++)
		STR_free(_files[i]);

	ARRAY_delete(&_files);
}


static void compile_lang(const char *file_po)
{
	const char *file_log;
	char *file_mo;
	time_t time_po, time_mo;
	char *cmd;
	int ret;
	
	time_po = FILE_get_time(file_po);
	
	if (time_po == ((time_t)-1))
		return;
	
	file_mo = (char *)FILE_set_ext(file_po, "mo");
		
	if (!main_compile_all)
	{
		time_mo = FILE_get_time(file_mo);
		if (time_mo >= time_po)
			return;
	}
	
	file_mo = STR_copy(file_mo);
	file_log = FILE_set_ext(file_po, "log");
	unlink(file_log);
	unlink(file_mo);
	
	// Shell "msgfmt -o " & Shell$(sPath) & " " & Shell(sTrans) Wait
	if (COMP_verbose)
	{
		cmd = STR_print("msgfmt -o %s %s 2>&1", file_mo, file_po);
		fprintf(stderr, "running: %s\n", cmd);
	}
	else
		cmd = STR_print("msgfmt -o %s %s > %s 2>&1", file_mo, file_po, file_log);
	
	ret = system(cmd);
	
	if (!WIFEXITED(ret) || WEXITSTATUS(ret))
		ERROR_warning("unable to compile translation file with 'msgfmt': %s", file_po);
	
	if (FILE_get_size(file_log) == 0)
		unlink(file_log);
	
	STR_free(cmd);
	STR_free(file_mo);
}


static void compile_all_lang(void)
{
	DIR *dir;
	char *path;
	struct dirent *dirent;
	char *file_name;
	int i;
	char c;

	path = STR_copy(FILE_cat(COMP_dir, ".lang", NULL));
	FILE_chdir(path);
	
	dir = opendir(".");
	if (!dir)
	{
		ERROR_warning("cannot browse directory: %s", path);
		return;
	}

	while ((dirent = readdir(dir)) != NULL)
	{
		file_name = dirent->d_name;
		if (*file_name == '.')
			continue;

		if (strcmp(FILE_get_ext(file_name), "po"))
			continue;
		
		for (i = 0; i < strlen(file_name); i++)
		{
			c = file_name[i];
			if (c == '.')
				break;
			if (!isalnum(c))
				continue;
		}
		
		run_task(compile_lang, file_name);
	}
	
	wait_for_all_task();

	closedir(dir);
	STR_free(path);
}


int main(int argc, char **argv)
{
	int i;

	MEMORY_init();
	COMMON_init();

	TRY
	{
		get_arguments(argc, argv);
		
		if (_convert_form)
		{
			FORM_convert(_convert_form);
			STR_free(_convert_form);
		}
		else
		{
			if (_ntask_max == 0)
				_ntask_max = SYSTEM_get_cpu_count() + 1;
			
			if (_ntask_max >= 2)
			{
				if (setpgid(0, 0))
					ERROR_fail("setpgid() fails: %s", strerror(errno));
			}

			COMPILE_init();
			COMP_do_not_lock = FALSE;

			// Remove information files if we are compiling everything

			if (main_compile_all)
			{
				if (COMP_verbose)
					fputs("Removing .info and .list files", stderr);
				FILE_chdir(COMP_dir);
				FILE_unlink(".info");
				FILE_unlink(".list");
			}

			init_files(COMP_dir);

			for (i = 0; i < ARRAY_count(_files); i++)
				run_task(compile_file, _files[i]);

			wait_for_all_task();
			
			COMPILE_remove_lock(".gbc.lock");
			COMPILE_remove_lock(".gbc.stderr");

			exit_files();
			
			if (main_trans)
				compile_all_lang();
			
			COMPILE_exit(_ntask_max == 1);
			FILE_exit();

			if (!main_quiet)
				puts("OK");
		}
	}
	CATCH
	{
		//wait_for_all_task();
		
		fflush(NULL);
		
		COMPILE_print(MSG_ERROR, -1, NULL);
		ERROR_print();
		exit(1);
	}
	END_TRY

	return 0;
}

