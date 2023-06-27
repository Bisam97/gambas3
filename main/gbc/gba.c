/***************************************************************************

	gba.c

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

#define __GBA_C

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>

#include "gb_common.h"
#include "gb_error.h"
#include "gb_str.h"
#include "gb_file.h"
#include "gb_array.h"
#include "gb_common_buffer.h"

#include "gbc_archive.h"

#if HAVE_GETOPT_LONG
static struct option Long_options[] =
{
	{ "version", 0, NULL, 'V' },
	{ "help", 0, NULL, 'h' },
	{ "swap", 0, NULL, 's' },
	{ "verbose", 0, NULL, 'v' },
	{ "output", 1, NULL, 'o' },
	{ "extract", 1, NULL, 'x' },
	{ "ignore-public", 0, NULL, 'p'},
	{ 0 }
};
#endif

static char **path_list;
static int path_current;

static const char *_allowed_hidden_files[] = { ".gambas", ".info", ".list", ".test", ".lang", ".action", ".connection", ".component", ".public", NULL };
#define PUBLIC_DIR _allowed_hidden_files[8]

static const char *remove_ext_lang[] = { "pot", "po", NULL };

static bool _extract = FALSE;
static char *_extract_file = NULL;

static bool _list_all = FALSE;

static char *_archive;

static bool _ignore_public = FALSE;

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
	printf("\nGambas archiver version ");
	print_version();
}

static void get_arguments(int argc, char **argv)
{
	int opt;
	#if HAVE_GETOPT_LONG
	int index = 0;
	#endif

	for(;;)
	{
		#if HAVE_GETOPT_LONG
			opt = getopt_long(argc, argv, "vVLhso:x:l:p", Long_options, &index);
		#else
			opt = getopt(argc, argv, "vVLhso:x:l:p");
		#endif

		if (opt < 0) break;

		switch (opt)
		{
			case 'V':
				print_version();
				exit(0);

			case 'v':
				ARCH_verbose = TRUE;
				break;

			case 's':
				ARCH_swap = TRUE;
				break;

			case 'o':
				ARCH_define_output(optarg);
				break;
				
			case 'x':
				_archive = optarg;
				_extract = TRUE;
				break;

			case 'l':
				_archive = optarg;
				_list_all = TRUE;
				break;

			case 'p':
				_ignore_public = TRUE;
				break;
				
			case 'L':
				print_version();
				printf(COPYRIGHT);
				exit(0);

			case 'h': case '?':
				print_title();
				printf(
					"\nCreate a standalone one-file executable from a Gambas project.\n"
					"\n    gba" GAMBAS_VERSION_STRING " [options] [<project directory>]\n"
					"\nExtract a specific file from a Gambas executable (-x option).\n"
					"\n    gba" GAMBAS_VERSION_STRING " -x <archive path> <file>\n"
					"\nList all files included in a Gambas executable (-l option).\n"
					"\n    gba" GAMBAS_VERSION_STRING " -l <archive path>\n\n"
					"Options:"
					#if HAVE_GETOPT_LONG
					"\n\n"
					"  -h  --help                 display this help\n"
					"  -L  --license              display license\n"
					"  -o  --output=ARCHIVE       archive path [<project directory>/<project name>.gambas]\n"
					"  -s  --swap                 swap endianness\n"
					"  -v  --verbose              verbose output\n"
					"  -V  --version              display version\n"
					"  -x  --extract=ARCHIVE      extract a specific file from the archive\n"
					"  -l  --list=ARCHIVE         list archive files\n"
					"  -p  --ignore-public        ignore '.public' directory\n"
					#else
					" (no long options on this system)\n\n"
					"  -h                     display this help\n"
					"  -L                     display license\n"
					"  -o=ARCHIVE             archive path [<project directory>/<project name>.gambas]\n"
					"  -s                     swap endianness\n"
					"  -v                     verbose output\n"
					"  -V                     display version\n"
					"  -x=ARCHIVE             extract a specific file from the archive\n"
					"  -l=ARCHIVE             list archie files\n"
					"  -p                     ignore '.public' directory\n"
					#endif
					"\n"
					);

				exit(0);

			default:
				exit(1);

		}
	}

	if (optind < (argc - 1))
	{
		fprintf(stderr, "gba: too many arguments.\n");
		exit(1);
	}

	if (_extract)
	{
		if (optind == argc)
		{
			fprintf(stderr, "gba: not enough arguments.\n");
			exit(1);
		}
		_extract_file = argv[optind];
	}
	else if (_list_all)
	{
		// everything is ok
	}
	else
	{
		if (optind == argc)
			ARCH_define_project(NULL);
		else
			ARCH_define_project(argv[optind]);

		if (!FILE_exist(ARCH_project))
		{
			fprintf(stderr, "gba: project file not found: %s\n", ARCH_project);
			exit(1);
		}
	}
}


static void print_resolved_path_rec(ARCH *arch, int n)
{
	static char buffer[16];

	const char *name = arch->symbol[n].sym.name;
	int len = arch->symbol[n].sym.len;
	int nrec;
	const char *p = NULL;

	if (*name == '/')
	{
		p = index(name, ':');
		if (p)
		{
			nrec = p - name - 1;
			if (nrec > 0 && nrec < 16)
			{
				strncpy(buffer, &name[1], nrec);
				buffer[nrec] = 0;
				nrec = atoi(buffer);
				if (nrec > 0 && nrec < arch->header.n_symbol)
				{
					print_resolved_path_rec(arch, nrec);
					putchar('/');
				}
			}
		}
	}

	if (p)
		fwrite(p + 1, sizeof(char), len - (p - name) - 1, stdout);
	else
		fwrite(name, sizeof(char), len, stdout);
}


static void print_resolved_path(ARCH *arch, int n)
{
	print_resolved_path_rec(arch, n);
	putchar('\n');
}


static void path_add(const char *path)
{
	*((char **)ARRAY_add(&path_list)) = STR_copy(path);
}


static void path_init(const char *first)
{
	ARRAY_create(&path_list);

	if (*first)
		FILE_chdir(first);

	path_add(FILE_get_current_dir());

	path_current = 0;
}


static void path_exit(void)
{
	ARRAY_delete(&path_list);
}


static int path_count(void)
{
	return ARRAY_count(path_list);
}


int main(int argc, char **argv)
{
	const char *path;
	int nfile;
	struct dirent **filelist;
	struct dirent *dirent;
	char *file_name;
	const char *file;
	struct stat info;
	const char *ext;
	int len;
	const char **p;
	int len_prefix;
	const char **remove_ext;
	ARCH *arch;
	ARCH_FIND find;
	int i;

	get_arguments(argc, argv);
	COMMON_init();

	TRY
	{
		if (_extract) // Extract a file from an archive
		{
			arch = ARCH_open(_archive);
			
			if (ARCH_find(arch, _extract_file, 0, &find))
				fprintf(stderr, "gba: file not found in archive\n");
			else
				fwrite(&arch->addr[find.pos], sizeof(char), find.len, stdout);
			
			ARCH_close(arch);
		}
		else if (_list_all)
		{
			arch = ARCH_open(_archive);

			for (i = 0; i < arch->header.n_symbol; i++)
				print_resolved_path(arch, i);

			ARCH_close(arch);
		}
		else // Create an archive
		{
			ARCH_init();

			file = FILE_get_dir(ARCH_project);
			len_prefix = strlen(file);
			path_init(file);

			// '.startup', '.project' and '.environment' files are always first!
			
			path = FILE_cat(FILE_get_dir(ARCH_project), ".startup", NULL);
			if (FILE_exist(path)) ARCH_add_file(path);

			path = FILE_cat(FILE_get_dir(ARCH_project), ".project", NULL);
			if (FILE_exist(path)) ARCH_add_file(path);

			path = FILE_cat(FILE_get_dir(ARCH_project), ".environment", NULL);
			if (FILE_exist(path)) ARCH_add_file(path);

			for(;;)
			{
				if (path_current >= path_count())
					break;

				path = path_list[path_current++];
				
				if (chdir(path) != 0)
				{
					fprintf(stderr, "gba: warning: cannot change to directory: %s\n", path);
					goto _NEXT_PATH;
				}

				filelist = NULL;
				nfile = scandir(path, &filelist, NULL, alphasort);

				if (nfile < 0)
				{
					fprintf(stderr, "gba: warning: cannot scan directory: %s\n", path);
					goto _NEXT_PATH;
				}
				
				for (i = 0; i < nfile; i++)
				{
					dirent = filelist[i];
					
					file_name = dirent->d_name;
					len = strlen(file_name);

					file = FILE_cat(path, file_name, NULL);

					if (*file_name == '.')
					{
						// hidden files are allowed only on the root of the project
						if (path_current != 1)
							continue;

						for (p = _allowed_hidden_files; *p; p++)
						{
							if (strcmp(file_name, *p) == 0)
								break;
						}

						if (*p == NULL)
							continue;

						if (*p == PUBLIC_DIR && _ignore_public)
							continue;
					}

					if (file_name[len - 1] == '~')
						continue;

					//if (strcmp(file_name, ARCH_project_name) == 0)
					//  continue;
						
					if ((len == 4) && (strncmp(file_name, "core", 4) == 0))
						continue;

					if ((len > 5) && (strncmp(file_name, "core.", 5) == 0))
						continue;

					if ((len > 7) && (strncmp(file_name, "vgcore.", 5) == 0))
						continue;

					if ((len > 10) && (strncmp(file_name, "callgrind.", 5) == 0))
						continue;

					// Do not put the archive file inside itself.
					if (!strcmp(file, ARCH_output))
						continue;

					if (stat(file_name, &info))
					{
						fprintf(stderr, "gba: warning: cannot stat file: %s\n", file);
						continue;
					}

					if (S_ISDIR(info.st_mode))
					{
						if (strcmp(file_name, "CVS") == 0)
							continue;
							
						path_add(file);
						ARCH_add_file(file);
					}
					else
					{
						ext = FILE_get_ext(file_name);

						//printf("path = %s\n", &path[len_prefix]);

						//if (path[len_prefix] == 0)
						//	remove_ext = remove_ext_root;
						if (strcmp(&path[len_prefix], "/.lang") == 0)
							remove_ext = remove_ext_lang;
						else
							remove_ext = 0;
						
						if (remove_ext)
						{
							for (p = remove_ext; *p; p++)
							{
								if (strcasecmp(ext, *p) == 0)
									break;
							}
						
							if (*p != NULL)
								continue;
						}
						
						if (strcmp(ext, "gambas") == 0)
							continue;

						ARCH_add_file(file);
					}
					
					free(dirent);
				}

	_NEXT_PATH:
				if (filelist != NULL) 
					free(filelist);
					
				FREE((char **)&path);
			}

			path_exit();

			ARCH_exit();
			/*MEM_check();*/
		}
	}
	CATCH
	{
		fflush(NULL);
		fprintf(stderr, "gba: ERROR: ");
		ERROR_print();
		exit(1);
	}
	END_TRY

	return 0;
}

