/***************************************************************************

  gbc_compile.c

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

#define __GBC_COMPILE_C

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gb_str.h"
#include "gb_file.h"
#include "gb_component.h"

#include "gbc_compile.h"
#include "gb_reserved.h"
#include "gbc_read.h"
#include "gbc_trans.h"
#include "gbc_header.h"
#include "gb_code.h"
#include "gbc_output.h"
#include "gbc_form.h"
#include "gbc_chown.h"
#include "gb_arch.h"

/*#define DEBUG*/

bool COMP_verbose = FALSE;

char *COMP_root = NULL;
char *COMP_dir;
char *COMP_project;
char *COMP_project_name;
char *COMP_info_path;
char *COMP_lib_path;
char *COMP_classes = NULL;
COMPILE COMP_current;
char *COMP_bytecode = NULL;
uint COMP_version = GAMBAS_PCODE_VERSION;
char *COMP_default_namespace = NULL;
bool COMP_do_not_lock = TRUE;

#define STARTUP_MAX_LINE 256

static FILE *_file = NULL;
static const char *_file_name = NULL;

const FORM_FAMILY COMP_form_families[] =
{
	{ "form", FORM_NORMAL },
	{ "report", FORM_NORMAL },
	{ "webpage", FORM_WEBPAGE },
	{ "webform", FORM_NORMAL },
	{ "termform", FORM_NORMAL },
	{ NULL }
};

static bool read_line(FILE *f, char *dir, int max)
{
	char *p;
	int c;

	p = dir;

	for(;;)
	{
		max--;

		c = fgetc(f);

		if (c == EOF || c == '\n' || max == 0)
		{
			*p = 0;
			return c == EOF;
		}

		*p++ = (char)c;
	}
}

static void add_memory_list(char *p, int size)
{
	char *pe = p + size;
	char *p2;
	int len;

	for(;;)
	{
		if (p >= pe)
			break;

		p2 = p;
		while (p2 < pe && *p2 != '\n')
			p2++;

		if (p2 >= pe)
			break;

		len = p2 - p;
		/*if (len > 2 && p[len - 1] == '?')
			len--;*/

		COMPILE_add_class(p, len);

		p = p2 + 1;
	}
}

static bool add_file_list(const char *path)
{
	FILE *f;
	char line[256];
	int len;

	f = fopen(path, "r");

	if (!f)
		return TRUE;

	for(;;)
	{
		if (read_line(f, line, sizeof(line)))
			break;

		len = strlen(line);
		/*if (len > 2 && line[len - 1] == '?')
			len--;*/

		COMPILE_add_class(line, len);
	}
	
	fclose(f);
	return FALSE;
}

static void add_library_list_file(const char *path, bool ref)
{
	ARCH *arch;
	ARCH_FIND find;
	const char *name;
	char *rpath = NULL;
	bool is_file = FALSE;

	if (*path == ':')
	{
		name = &path[1];

		rpath = STR_cat(FILE_cat(COMP_lib_path, name, NULL), ".gambas", NULL);
		if (!FILE_exist(rpath))
		{
			STR_free(rpath);
			rpath = STR_cat(FILE_cat(COMP_root, "lib/gambas" GAMBAS_VERSION_STRING, name, NULL), ".gambas", NULL);
		}

		if (!FILE_exist(rpath))
			path = NULL;
		else
			path = rpath;
	}
	else
	{
		name = path;
		if (!FILE_exist(path))
			path = NULL;
		/*
		{
			is_file = TRUE;
			path = FILE_cat(FILE_get_dir(path), ".list", NULL);
			if (!FILE_exist(path))
				path = NULL;
		}
		*/
	}

	if (path)
	{
		if (is_file)
		{
			add_file_list(path);
		}
		else
		{
			arch = ARCH_open(path);

			if (!ARCH_find(arch, ".list", 0, &find))
				add_memory_list(&arch->addr[find.pos], find.len);

			ARCH_close(arch);
		}
	}
	else
		ERROR_warning((ref ? "cannot find reference: %s" : "cannot find library: %s"), name);

	if (rpath)
		STR_free(rpath);
}


void COMPILE_add_component(const char *name)
{
	char *path;

	if (COMP_verbose)
		fprintf(stderr, "Loading information from component '%s'\n", name);
	
	path = (char *)FILE_cat(COMP_info_path, name, NULL);
	strcat(path, ".list");

	if (add_file_list(path))
	{
		// Do not raise an error if a component self-reference is not found
		if (strcmp(name, COMP_project_name))
			//fprintf(stderr, "warning: cannot read component list file: %s.list\n", name);
			THROW("Component not found: &1", name);
		return;
	}
}

static void add_components(char *list)
{
	char *comp;

	comp = strtok(list, ",");
	while (comp)
	{
		COMPILE_add_component(comp);
		comp = strtok(NULL, ",");
	}
}

static FILE *open_project_file()
{
	FILE *fp = fopen(COMP_project, "r");
	if (!fp)
		THROW(E_OPEN, COMP_project);
	return fp;
}

static bool line_begins_with(const char *line, const char *key, int len)
{
	if (len < 0)
		len = strlen(key);

	return strncmp(line, key, len) == 0;
}

static void find_lines(const char *key, const char *def, void (*print_func)(const char *line))
{
	FILE *fp;
	char line[256];
	int len = strlen(key);
	bool print = FALSE;

	fp = open_project_file();

	for(;;)
	{
		if (read_line(fp, line, sizeof(line)))
			break;

		if (line_begins_with(line, key, len))
		{
			(*print_func)(&line[len]);
			print = TRUE;
		}
	}

	fclose(fp);

	if (!print && def)
		(*print_func)(def);
}

static char *find_version_in_file(void)
{
	char *dir, *pdir;
	FILE *fv;
	char line[256];
	const char *path;
	int len;

	dir = STR_copy(COMP_project);

	for(;;)
	{
		pdir = STR_copy(FILE_get_dir(dir));
		STR_free(dir);
		dir = pdir;

		if (dir[0] == '/' && dir[1] == 0)
		{
			STR_free(dir);
			return NULL;
		}

		path = FILE_cat(dir, "VERSION", NULL);

		if (FILE_exist(path))
		{
			STR_free(dir);
			break;
		}
	}

	fv = fopen(path, "r");
	if (!fv)
		return NULL;

	len = fread(line, 1, sizeof(line) - 1, fv);
	while (len > 0 && isspace(line[len - 1]))
		len--;
	line[len] = 0;
	return STR_copy(line);
}

static void print_version()
{
	FILE *fp;
	char line[256];
	char *version = NULL;
	char *branch = NULL;
	bool add_branch = FALSE;
	const char *path;

	fp = open_project_file();

	for(;;)
	{
		if (read_line(fp, line, sizeof(line)))
			break;

		if (line_begins_with(line, "Version=", 8))
		{
			version = STR_copy(&line[8]);
			branch = index(version, ' ');
		}
		else if (line_begins_with(line, "VersionFile=", 12))
		{
			if (line[12] == '1')
			{
				version = find_version_in_file();
				add_branch = TRUE;
			}
		}
	}

	fclose(fp);

	if (!add_branch)
	{
		path = FILE_cat(COMP_dir, ".version", NULL);
		fp = fopen(path, "r");
		if (fp)
		{
			read_line(fp, line, sizeof(line));
			version = STR_copy(line);
		}
	}

	if (version)
	{
		fputs(version, _file);
		if (add_branch && branch)
			fputs(branch, _file);
		fputc('\n', _file);
		STR_free(version);
	}
	else
		fputs("0.0.0\n", _file);
}


static void create_file(const char *file)
{
	const char *path = FILE_cat(FILE_get_dir(COMP_project), file, NULL);

	_file_name = file;
	_file = fopen(path, "w");

	if (!_file)
		THROW("Cannot create '&1' file", _file_name);

	FILE_set_owner(path, COMP_project);
}


static void close_file()
{
	const char *path = FILE_cat(FILE_get_dir(COMP_project), _file_name, NULL);

	if (fclose(_file))
		THROW("Cannot create '&1' file", _file_name);

	_file = NULL;
	_file_name = NULL;

	if (FILE_get_size(path) == 0)
		FILE_unlink(path);
}


static void print_line(const char *line)
{
	fputs(line, _file);
	fputc('\n', _file);
}


static void print_environment(const char *line)
{
	if (*line && *line != ' ')
		print_line(line);
}


static void create_startup_file()
{
	create_file(".startup");

	find_lines("Startup=", "", print_line);
	find_lines("Title=", "", print_line);

	fputc('#', _file);
	fputs(COMP_project_name, _file);
	fputc('\n', _file);

	//startup_print(fs, "Stack=", "0");
	find_lines("StackTrace=", "0", print_line);

	print_version();

	fputc('\n', _file);
	find_lines("Component=", NULL, print_line);
	find_lines("Library=", NULL, print_line);
	fputc('\n', _file);

	close_file();

	create_file(".environment");

	find_lines("StartupEnvironment=", NULL, print_environment);

	close_file();
}

#undef isdigit

static int read_version_digits(const char **pstr)
{
	const char *p = *pstr;
	int n;
	int i;

	if (!isdigit(*p))
		return -1;

	n = 0;

	for (i = 0; i < 4; i++)
	{
		n = (n << 4) + *p++ - '0';
		if (!isdigit(*p))
			break;
	}

	*pstr = p;
	return n;
}

static void init_version(void)
{
	const char *ver;
	int n, v;

	ver = COMP_bytecode;
	if (!ver)
		ver = getenv("GB_PCODE_VERSION");

	if (ver && *ver)
	{
		if (strcmp(ver, "last") == 0)
			return;

		v = 0;
		n = read_version_digits(&ver);
		if (n <= 0 || n > GAMBAS_VERSION)
			return;

		v = n << 24;

		if (*ver++ != '.')
			return;

		n = read_version_digits(&ver);
		if (n < 0 || n > 0x99)
			return;

		v |= n << 16;

		if (*ver++ == '.')
		{
			n = read_version_digits(&ver);
			if (n > 0)
			{
				if (n > 0x9999)
					return;

				v |= n;
			}
		}

		COMP_version = v;
	}
}

void COMPILE_init(void)
{
	FILE *fp;
	char line[256];
	char *env;

	RESERVED_init();

	if (!COMP_root)
		COMP_root = STR_copy(FILE_get_dir(FILE_get_dir(FILE_find_gambas())));

	// Component directory

	COMP_info_path = STR_copy(FILE_cat(COMP_root, "share/gambas" GAMBAS_VERSION_STRING "/info", NULL));

	// Local libraries directory

	env = getenv("XDG_DATA_HOME");
	if (env && *env)
		COMP_lib_path = STR_copy(FILE_cat(env, "gambas3/lib", NULL));
	else
		COMP_lib_path = STR_copy(FILE_cat(FILE_get_home(), ".local/share/gambas3/lib", NULL));

	// Project name

	COMP_project_name = STR_copy(FILE_get_name(FILE_get_dir(COMP_project)));

	// Bytecode version

	init_version();

	// Project classes

	BUFFER_create(&COMP_classes);

	COMPILE_add_component("gb");

	fp = open_project_file();

	for(;;)
	{
		if (read_line(fp, line, sizeof(line)))
			break;

		/*printf("%s\n", line);*/

		if (strncmp(line, "Component=", 10) == 0)
			COMPILE_add_component(&line[10]);
		else if (strncmp(line, "Library=", 8) == 0)
			add_library_list_file(&line[8], FALSE);
		else if (strncmp(line, "Reference=", 10) == 0)
			add_library_list_file(&line[10], TRUE);
		else if (strncmp(line, "ManualComponents=", 17) == 0)
			add_components(&line[17]);
	}

	fclose(fp);
	
	// Add local ".list" file
	// Not possible at the moment.
	// add_file_list(FILE_cat(FILE_get_dir(COMP_project), ".list", NULL));

	// Startup file

	create_startup_file();

	// Adds a separator to make the difference between classes from components
	// (they must be searched in the global symbol table) and classes from the
	// project (they must be searched in the project symbol table)

	COMPILE_add_class("-", 1);

	/*
	dir = opendir(FILE_get_dir(COMP_project));
	if (dir)
	{
		while ((dirent = readdir(dir)) != NULL)
		{
			name = dirent->d_name;
			if (*name == '.')
				continue;

			if ((strcasecmp(FILE_get_ext(name), "module") == 0)
					|| (strcasecmp(FILE_get_ext(name), "class") == 0))
			{
				name = FILE_get_basename(name);
				BUFFER_add(&COMP_classes, name, strlen(name));
				BUFFER_add(&COMP_classes, "\n", 1);
			}
		}

		closedir(dir);
	}

	BUFFER_add(&COMP_classes, "\n", 1);
	*/
}


void COMPILE_begin(const char *file, bool trans, bool debug)
{
	CLEAR(JOB);

	JOB->name = STR_copy(file);
	JOB->debug = debug;
	JOB->form = FORM_get_file_family(JOB->name, &JOB->family);
	JOB->output = OUTPUT_get_file(JOB->name);

	if (trans)
	{
		JOB->trans = TRUE;
		JOB->tname = OUTPUT_get_trans_file(JOB->name);
	}
}


void COMPILE_alloc()
{
	struct stat info;
	off_t size;

	BUFFER_create(&JOB->source);
	CLASS_create(&JOB->class);

	JOB->default_library = NO_SYMBOL;

	size = 0;

	if (stat(JOB->name, &info))
		ERROR_warning("cannot stat file: %s", JOB->name);
	else
		size += info.st_size;

	if (JOB->form)
	{
		if (stat(JOB->form, &info))
			ERROR_warning("cannot stat file: %s", JOB->form);
		else
			size += info.st_size * 2;
	}

	ALLOC(&JOB->pattern, sizeof(PATTERN) * (16 + size));
	ALLOC(&JOB->pattern_pos, sizeof(int) * (16 + size));
	JOB->pattern_count = 0;
	
	JOB->current = NULL;
}


void COMPILE_load(void)
{
	if (BUFFER_load_file(&JOB->source, JOB->name))
		THROW("Cannot load source file: &1", strerror(errno));

	//BUFFER_add(&JOB->source, "\n", 1);
}


void COMPILE_free(void)
{
	CLASS_delete(&JOB->class);
	BUFFER_delete(&JOB->source);
	FREE(&JOB->pattern);
	FREE(&JOB->pattern_pos);

	if (JOB->help)
		ARRAY_delete(&JOB->help);
}

void COMPILE_end(void)
{
	STR_free(JOB->name);
	STR_free(JOB->form);
	STR_free(JOB->output);
	if (JOB->trans)
		STR_free(JOB->tname);
	if (JOB->help)
		STR_free(JOB->hname);
}


void COMPILE_exit(bool can_dump_count)
{
	/*if (COMP_verbose && can_dump_count)
		PCODE_dump_count(stdout);*/

	RESERVED_exit();
	BUFFER_delete(&COMP_classes);
	STR_free(COMP_project_name);
	STR_free(COMP_project);
	STR_free(COMP_info_path);
	STR_free(COMP_dir);
	STR_free(COMP_root);
	STR_free(COMP_default_namespace);
	STR_free(COMP_bytecode);
}

static void add_class(const char *name, int len)
{
	unsigned char clen = (unsigned char)len;
	
	if (clen != len)
		ERROR_panic("Class name is too long");

	//fprintf(stderr, "add_class: %.*s\n", len, name);
	
	BUFFER_add(&COMP_classes, &clen, 1);
	BUFFER_add(&COMP_classes, name, len);
}

void COMPILE_add_class(const char *name, int len)
{
	char *p;

	p = memchr(name, ' ', len);
	if (!p)
		add_class(name, len);
	else
		add_class(name, p - name);
}

void COMPILE_end_class(void)
{
	unsigned char clen = 0;
	BUFFER_add(&COMP_classes, &clen, 1);
}

int COMPILE_lock_file(const char *name)
{
	const char *path;
	int fd;
	
	if (COMP_do_not_lock)
		return -1;
	
	path = FILE_cat(COMP_dir, name, NULL);
	
	fd = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0666);
	if (fd < 0)
		goto __ERROR;
	if (lockf(fd, F_LOCK, 0) < 0)
		goto __ERROR;
	
	return fd;
		
__ERROR:

	ERROR_fail("unable to lock file: %s: %s", path, strerror(errno));
}


void COMPILE_unlock_file(int fd)
{
	if (!COMP_do_not_lock)
		close(fd);
}


void COMPILE_remove_lock(const char *name)
{
	const char *path;
	
	if (COMP_do_not_lock)
		return;
	
	path = FILE_cat(COMP_dir, name, NULL);
	if (FILE_exist(path))
		FILE_unlink(path);
}


void COMPILE_print(int type, int line, const char *msg, ...)
{
	int i;
  va_list args;
	const char *arg[4];
	int col = -1;
	int lock;

	if (!JOB->warnings && type == MSG_WARNING)
		return;

	lock = COMPILE_lock_file(".gbc.stderr");
	
  va_start(args, msg);

	if (line < 0)
	{
		if (JOB->step == JOB_STEP_READ)
		{
			line = JOB->line;
			col = READ_get_column();
		}
		else if (JOB->step == JOB_STEP_TREE)
		{
			col = TRANS_get_column(&line);
		}
		else if (JOB->step == JOB_STEP_CODE)
		{
			line = JOB->line;
			if (JOB->current)
				col = COMPILE_get_column(JOB->current);
		}
	}

	if (JOB->name)
	{
		const char *name = FILE_get_name(JOB->name);
		if (line)
		{
			if (line > JOB->max_line && JOB->form)
			{
				name = FILE_get_name(JOB->form);
				fprintf(stderr, "%s:%d: ", name, line - FORM_FIRST_LINE + 1);
			}
			else
			{
				if (col >= 0)
					fprintf(stderr, "%s:%d:%d: ", name, line, col + 1);
				else
					fprintf(stderr, "%s:%d: ", name, line);
			}
		}
		else
			fprintf(stderr, "%s: ", name);
	}
	else
		fprintf(stderr, "gbc" GAMBAS_VERSION_STRING ": ");

	fprintf(stderr, "%s: ", type ? "warning" : "error");

	if (msg)
	{
		for (i = 0; i < 4; i++)
			arg[i] = va_arg(args, const char *);

		ERROR_define(msg, arg);
		fputs(ERROR_info.msg, stderr);
		putc('\n', stderr);
	}
	
	COMPILE_unlock_file(lock);

	va_end(args);
}


void COMPILE_create_file(FILE **fw, const char *file)
{
	if (!*fw)
	{
		*fw = fopen(file, "w");
		if (!*fw)
			THROW("Cannot create file: &1: &2", FILE_cat(FILE_get_dir(COMP_project), file, NULL), strerror(errno));
	}
}

