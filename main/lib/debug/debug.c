/***************************************************************************

	debug.c

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

#define __DEBUG_C

// Do not include gbx_debug.h
#define __GBX_DEBUG_H

#include "gb_common.h"
#include "gambas.h"

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "gb_error.h"
#include "gbx_type.h"
#include "gb_limit.h"
#include "gbx_stack.h"
#include "gbx_class.h"
#include "gbx_exec.h"
#include "gbx_local.h"
#include "gbx_object.h"

#include "gbx_eval.h"

#include "print.h"

#include "debug.h"

//#define DEBUG_ME

typedef
	struct {
		int id;
		FUNCTION *func;
		PCODE *addr;
		CLASS *class;
		ushort line;
		VALUE *bp;
		FUNCTION *fp;
		}
	DEBUG_BREAK;

typedef 
	struct {
		int id;
		EXPRESSION *expr;
		VALUE value;
		unsigned changed : 1;
	}
	DEBUG_WATCH;

typedef
	struct {
		char *pattern;
		DEBUG_TYPE type;
		void (*func)(char *);
		bool loop;
		}
	DEBUG_COMMAND;



DEBUG_INFO DEBUG_info = { 0 };
GB_DEBUG_INTERFACE *DEBUG_interface;
char DEBUG_buffer[DEBUG_BUFFER_MAX + 1];
char *DEBUG_fifo = NULL;

static DEBUG_BREAK *_breakpoints;
static char *_error = NULL;

static DEBUG_WATCH *_watches;

static EVAL_INTERFACE EVAL;

static int _fdr;
static int _fdw;
static FILE *_out = NULL;
static FILE *_in = NULL;
static bool _fifo;
static bool _debug = FALSE;

#define EXEC_current (*(STACK_CONTEXT *)GB_DEBUG.GetExec())

#ifdef DEBUG_ME
#define WARNING(_msg, ...) fprintf(stderr, "W\t" _msg "\n", ##__VA_ARGS__)
#define INFO(_msg, ...) fprintf(stderr, "I\t" _msg "\n", ##__VA_ARGS__)
#else
#define WARNING(_msg, ...) _out && fprintf(_out, "W\t" _msg "\n", ##__VA_ARGS__)
#define INFO(_msg, ...) _out && fprintf(_out, "I\t" _msg "\n", ##__VA_ARGS__)
#endif

static void init_eval_interface()
{
	static bool init = FALSE;

	if (!init)
	{
		GB.GetInterface("gb.eval", EVAL_INTERFACE_VERSION, &EVAL);
		init = TRUE;
	}
}

void DEBUG_break_on_next_line(void)
{
	DEBUG_info.stop = TRUE;
	DEBUG_info.leave = FALSE;
	DEBUG_info.fp = NULL;
	DEBUG_info.bp = NULL;
	DEBUG_info.pp = NULL;
}


static void signal_user(int sig)
{
	signal(SIGUSR2, signal_user);

	#ifdef DEBUG_ME
	fprintf(stderr, "Got SIGUSR2\n");
	#endif

	/*CAPP_got_signal();*/
	DEBUG_break_on_next_line();
}


bool DEBUG_calc_line_from_position(CLASS *class, FUNCTION *func, PCODE *addr, ushort *line)
{
	int lo, hi;
	int mid;
	ushort pos = addr - func->code;
	ushort *post;

	if (func->debug)
	{
		post = func->debug->pos;
		
		lo = 0;
		hi = func->debug->nline - 1;
		
		while (lo < hi)
		{
			mid = (lo + hi) >> 1;
			if (pos < post[mid])
			{
				hi = mid;
			}
			else if (pos >= post[mid + 1])
			{
				lo = mid + 1;
			}
			else
			{
				*line = mid + func->debug->line;
				return FALSE;
			}
		}
	}

	return TRUE;
}


static bool calc_position_from_line(CLASS *class, ushort line, FUNCTION **function, PCODE **addr)
{
	int i;
	ushort pos, pos_after;
	FUNCTION *func = NULL;
	FUNC_DEBUG *debug = NULL;

	// Look at the first functions last, because global variables can be declared and 
	// initialized everywhere in the file, and initialization function is the first one.
	
	for(i = class->load->n_func - 1; i >= 0; i--)
	{
		func = &class->load->func[i];
		debug = func->debug;
		//fprintf(stderr, "calc_position_from_line: %s (%d -> %d) / %d\n", debug->name, debug->line, debug->line + debug->nline - 1, line);
		if (debug && line >= debug->line && line < (debug->line + debug->nline))
			break;
	}

	if (i < 0)
		return TRUE;

	//fprintf(stderr, "calc_position_from_line: %s OK\n", debug->name);

	line -= debug->line;

	for(;;)
	{
		pos = debug->pos[line];
		pos_after = debug->pos[line + 1];
		if (pos != pos_after)
			break;

		line++;
		if (line >= debug->nline)
			return TRUE;
	}

	*function = func;
	*addr = &func->code[pos];

	/*printf("%s.%d -> %04X\n", class->name, line + debug->line, **addr);*/

	return FALSE;
}


DEBUG_INFO *DEBUG_init(GB_DEBUG_INTERFACE *debug)
{
	DEBUG_interface = debug;

	GB.NewArray(&_breakpoints, sizeof(DEBUG_BREAK), 16);
	GB.NewArray(&_watches, sizeof(DEBUG_WATCH), 0);
	
	return &DEBUG_info;
}

bool DEBUG_init_fifo(bool fifo, const char *fifo_name)
{
	char *env;
	char path[DEBUG_FIFO_PATH_MAX];

	_fifo = fifo;

	if (_fifo)
	{
		DEBUG_fifo = GB.NewZeroString(fifo_name);
		
		snprintf(path, sizeof(path), "%sin", fifo_name);
		
		for(;;)
		{
			_fdw = open(path, O_WRONLY | O_CLOEXEC);
			if (_fdw >= 0)
				break;
			if (errno != EAGAIN && errno != EINTR)
			{
				fprintf(stderr, "gb.debug: unable to open input fifo: %s: %s\n", strerror(errno), path);
				return TRUE;
			}
		}
		
		//fprintf(stderr, "gb.debug: open input fifo: %d\n", _fdw);
		_out = fdopen(_fdw, "w");

		if (!_out)
		{
			fprintf(stderr, "gb.debug: unable to create stream on input fifo: %s: %s\n", strerror(errno), path);
			return TRUE;
		}
	}
	else
	{
		_out = stdout;
	}

	//ARRAY_create(&_breakpoints);
	signal(SIGUSR2, signal_user);
	signal(SIGPIPE, SIG_IGN);

	setlinebuf(_out);

	env = getenv("GB_DEBUG_DEBUG");
	if (env && env[0] == '1' && env[1] == 0)
		_debug = TRUE;

	return FALSE;
}

void DEBUG_exit(void)
{
	int i;
	
	GB.FreeArray(&_breakpoints);
	
	if (_watches)
	{
		for (i = 0; i < GB.Count(_watches); i++)
			EVAL.Free(POINTER(&_watches[i].expr));
		
		GB.FreeArray(&_watches);
	}
	
	GB.FreeString(&DEBUG_fifo);
	GB.FreeString(&_error);

	// Don't do it, it blocks!
	
	/*if (EXEC_fifo)
	{
		fclose(_in);
		fclose(_out);
	}
	*/
}


static int find_free_breakpoint(void)
{
	int i;
	char used[MAX_BREAKPOINT];

	memset(used, FALSE, MAX_BREAKPOINT);

	for (i = 0; i < ARRAY_count(_breakpoints); i++)
		used[_breakpoints[i].id - 1] = TRUE;

	for (i = 0; i < MAX_BREAKPOINT; i++)
		if (!used[i])
			return (i + 1);

	return 0;
}

static bool init_breakpoint(DEBUG_BREAK *brk)
{
	PCODE *addr = NULL;
	FUNCTION *func;

	//fprintf(stderr, "init_breakpoint: id = %d\n", brk->id);

	if (brk->addr || !CLASS_is_loaded(brk->class))
	{
		WARNING("breakpoint is pending");
		return TRUE;
	}

	if (CLASS_is_native(brk->class) || !brk->class->debug)
	{
		WARNING("Cannot set breakpoint: no debugging information");
		return TRUE;
	}

	if (calc_position_from_line(brk->class, brk->line, &func, &addr))
	{
		WARNING("Cannot set breakpoint: cannot calculate position");
		//fprintf(_out, "Cannot calc position from line number\n");
		return TRUE;
	}

	if (!PCODE_is_breakpoint(*addr))
	{
		//WARNING("Cannot set breakpoint: Not a line beginning: %04d: %04X", addr - func->code, *addr);
		WARNING("Cannot set breakpoint: Not a line beginning");
		//fprintf(_out, "Not a line beginning ?\n");
		return TRUE;
	}

	if (*addr & 0xFF)
	{
		WARNING("breakpoint already set");
		//fprintf(_out, "_breakpoints already set\n");
		return FALSE;
	}

	brk->addr = addr;
	*addr = PCODE_BREAKPOINT(brk->id);

	//fprintf(stderr, "init_breakpoint: OK\n");

	#ifdef DEBUG_ME
	fprintf(stderr, "init_breakpoint: %s.%d\n", brk->class->name, brk->line);
	#endif

	INFO("breakpoint set: %s.%d", brk->class->name, brk->line);
	return FALSE;
}


static bool set_breakpoint(CLASS *class, ushort line)
{
	DEBUG_BREAK *brk;
	int id;

	if (GB.Count(_breakpoints) >= MAX_BREAKPOINT)
	{
		WARNING("Too many breakpoints");
		return TRUE;
	}

	id = find_free_breakpoint();
	if (id == 0)
	{
		WARNING("Cannot create breakpoint");
		return TRUE;
	}

	brk = (DEBUG_BREAK *)GB.Add(&_breakpoints);

	brk->id = id;
	brk->addr = NULL;
	brk->class = class;
	brk->line = line;

	#ifdef DEBUG_ME
	fprintf(stderr, "set_breakpoint: %s.%d\n", class->name, line);
	#endif

	init_breakpoint(brk);

	return FALSE;
}


static bool unset_breakpoint(CLASS *class, ushort line)
{
	int i;
	DEBUG_BREAK *brk;

	for (i = 0; i < GB.Count(_breakpoints); i++)
	{
		brk = &_breakpoints[i];
		if (brk->class == class && brk->line == line)
		{
			if (brk->addr)
				*(brk->addr) = PCODE_BREAKPOINT(0);
			GB.Remove(&_breakpoints, i, 1);

			#ifdef DEBUG_ME
			fprintf(stderr, "unset_breakpoint: %s.%d\n", class->name, line);
			#endif

			INFO("breakpoint removed");
			return FALSE;
		}
	}

	WARNING("Unknown breakpoint");
	return TRUE;
}


static void unset_all_breakpoints()
{
	int i;
	DEBUG_BREAK *brk;

	for (i = 0; i < GB.Count(_breakpoints); i++)
	{
		brk = &_breakpoints[i];
		if (brk->addr)
			*(brk->addr) = PCODE_BREAKPOINT(0);
	}

	GB.Remove(&_breakpoints, 0, GB.Count(_breakpoints));
}


void DEBUG_init_breakpoints(CLASS *class)
{
	int i;
	DEBUG_BREAK *brk;

	#ifdef DEBUG_ME
	fprintf(stderr, "DEBUG_init_breakpoints: %p %s\n", class, class->name);
	#endif
	
	for (i = 0; i < GB.Count(_breakpoints); i++)
	{
		brk = &_breakpoints[i];
		if (brk->class == class)
		{
			//fprintf(stderr, "DEBUG_init_breakpoints: %s\n", class->name);
			init_breakpoint(brk);
		}
	}
}


static void print_local()
{
	int i;
	FUNCTION *fp;
	LOCAL_SYMBOL *lp;

	fp = DEBUG_info.fp;
	
	if (!fp || !fp->debug)
		return;
		
	for (i = 0; i < fp->debug->n_local; i++)
	{
		lp = &fp->debug->local[i];
		fprintf(_out, "%.*s ", lp->sym.len, lp->sym.name);
	}
}


static void print_symbol(GLOBAL_SYMBOL *gp, bool is_static, bool is_public)
{
	if (CTYPE_get_kind(gp->ctype) != TK_VARIABLE && CTYPE_get_kind(gp->ctype) != TK_CONST)
		return;

	if (CTYPE_is_static(gp->ctype) && !is_static)
		return;

	if (!CTYPE_is_static(gp->ctype) && is_static)
		return;

	if (CTYPE_is_public(gp->ctype) && !is_public)
		return;

	if (!CTYPE_is_public(gp->ctype) && is_public)
		return;

	fprintf(_out, "%.*s ", gp->sym.len, gp->sym.name);
}


static void print_object()
{
	int i;
	GLOBAL_SYMBOL *gp;
	CLASS *cp = DEBUG_info.cp;
	void *op = DEBUG_info.op;

	if (!cp || !cp->load)
		return;

	fprintf(_out, "S: ");

	for (i = 0; i < cp->load->n_global; i++)
	{
		gp = &cp->load->global[i];
		print_symbol(gp, TRUE, TRUE);
	}

	fprintf(_out, "s: ");

	for (i = 0; i < cp->load->n_global; i++)
	{
		gp = &cp->load->global[i];
		print_symbol(gp, TRUE, FALSE);
	}

	if (op)
	{
		fprintf(_out, "D: ");

		for (i = 0; i < cp->load->n_global; i++)
		{
			gp = &cp->load->global[i];
			print_symbol(gp, FALSE, TRUE);
		}

		fprintf(_out, "d: ");

		for (i = 0; i < cp->load->n_global; i++)
		{
			gp = &cp->load->global[i];
			print_symbol(gp, FALSE, FALSE);
		}
	}
}

static void command_quit(char *cmd)
{
	exit(1);
}

static void command_hold(char *cmd)
{
	GB_DEBUG.DebugHold();
}

static void command_go(char *cmd)
{
	GB.Component.Signal(GB_SIGNAL_DEBUG_CONTINUE, 0);

	DEBUG_info.stop = FALSE;
	DEBUG_info.leave = FALSE;
	DEBUG_info.fp = NULL;
	DEBUG_info.bp = NULL;
	DEBUG_info.pp = NULL;
}

static void command_step(char *cmd)
{
	GB.Component.Signal(GB_SIGNAL_DEBUG_FORWARD, 0);
	DEBUG_break_on_next_line();
}

static void command_next(char *cmd)
{
	GB.Component.Signal(GB_SIGNAL_DEBUG_FORWARD, 0);
	DEBUG_info.stop = TRUE;
	DEBUG_info.leave = FALSE;
	DEBUG_info.fp = FP;
	DEBUG_info.bp = BP;
	DEBUG_info.pp = PP;
}

static void command_from(char *cmd)
{
	STACK_CONTEXT *sc = GB_DEBUG.GetStack(0); //STACK_get_current();

	if (sc && sc->pc)
	{
		GB.Component.Signal(GB_SIGNAL_DEBUG_FORWARD, 0);
		DEBUG_info.stop = TRUE;
		DEBUG_info.leave = FALSE;
		DEBUG_info.fp = sc->fp;
		DEBUG_info.bp = sc->bp;
		DEBUG_info.pp = sc->pp;
	}
	else
	{
		GB.Component.Signal(GB_SIGNAL_DEBUG_FORWARD, 0);
		DEBUG_info.stop = TRUE;
		DEBUG_info.leave = TRUE;
		DEBUG_info.fp = FP;
		DEBUG_info.bp = BP;
		DEBUG_info.pp = PP;
	}
}

void DEBUG_set_breakpoint(bool set, char *cmd)
{
	char *comp = NULL;
	char class_name[256];
	ushort line;
	char *p;

	if (!set && cmd[0] == '*' && cmd[1] == 0)
	{
		unset_all_breakpoints();
		return;
	}

	if (*cmd == '[')
	{
		p = index(cmd, ']');
		if (p && p[1] == '.')
		{
			comp = &cmd[1];
			*p = 0;
			cmd = p + 2;

			if (comp[0] == '$' && !comp[1])
				comp = NULL;
		}
	}

	if (sscanf(cmd, "%256[^.].%hu", class_name, &line) != 2)
		WARNING("Cannot %s breakpoint: syntax error", set ? "add" : "remove");
	else if (set)
		set_breakpoint((CLASS *)GB_DEBUG.FindClass(comp, class_name), line);
	else
		unset_breakpoint((CLASS *)GB_DEBUG.FindClass(comp, class_name), line);
}

static void command_breakpoint(char *cmd)
{
	DEBUG_set_breakpoint(*cmd == '+', &cmd[1]);
}

void DEBUG_set_breakpoints_from_file(const char *path)
{
	int fd;
	struct stat info;
	char *data;
	int len, lenr;
	char *p;
	char *cmd;
	
	fd = open(path, O_RDONLY);
	if (fd < 0) 
		return;
	
	if (fstat(fd, &info))
	{
		close(fd);
		return;
	}
	
	len = info.st_size;
	GB.Alloc(POINTER(&data), len + 1);
	
	p = data;
	
	for(;;)
	{
		lenr = read(fd, p, len);
		if (lenr < 0)
		{
			close(fd);
			return;
		}
		
		if (lenr == len)
			break;
			
		p += lenr;
		len -= lenr;
	}

	close(fd);
	
	data[len] = 0;
	p = data;
	
	cmd = strtok(data, ",");
	for (;;)
	{
		if (!cmd)
			break;
		if (*cmd == '+')
			DEBUG_set_breakpoint(TRUE, &cmd[1]);
		cmd = strtok(NULL, ",");
	}
	
	GB.Free(POINTER(&data));
}

void DEBUG_backtrace(FILE *out)
{
	int i, n;
	STACK_CONTEXT *context;

	fprintf(out, "%s", DEBUG_get_current_position());

	//for (i = 0; i < (STACK_frame_count - 1); i++)
	n = 0;
	for (i = 0;; i++)
	{
		context = GB_DEBUG.GetStack(i); //&STACK_frame[i];
		if (!context)
			break;

		n += fprintf(out, " %s", DEBUG_get_position(context->cp, context->fp, context->pc));

		/*if (context->pc)
		{
			line = 0;
			if (DEBUG_calc_line_from_position(context->cp, context->fp, context->pc, &line))
				n += fprintf(out, " %s.?.?", context->cp->name);
			else
				n += fprintf(out, " %s.%s.%d", context->cp->name, context->fp->debug->name, line);
		}
		else if (context->cp)
			n += fprintf(out, " ?");*/
		
		if (n >= (DEBUG_OUTPUT_MAX_SIZE / 2))
		{
			fprintf(out, " ...");
			break;
		}
	}
}


static void debug_info(bool frame)
{
	const char *p;
	char c;
	int i;
	DEBUG_WATCH *watch;
	
	fprintf(_out, "%c[%d]\t", frame ? '@' : '*', getpid());
	
	if (_error)
	{
		p = _error;
		for(;;)
		{
			c = *p++;
			if (!c)
				break;
			
			if (c == '\n' || c == '\r' || c == '\t')
				c = ' ';
			fputc(c, _out);
		}
	}
	
	fprintf(_out, "\t");
	
	DEBUG_backtrace(_out);
	fprintf(_out, "\t");
	
	print_local();
	fprintf(_out, "\t");
	
	print_object();
	fprintf(_out, "\t");
	
	for (i = 0; i < GB.Count(_watches); i++)
	{
		watch = &_watches[i];
		if (watch->changed)
			fprintf(_out, "%d ", watch->id);
	}
	
	fputc('\n', _out);
}


static void set_info(STACK_CONTEXT *context)
{
	if (context)
	{
		DEBUG_info.bp = context->bp;
		DEBUG_info.pp = context->pp;
		DEBUG_info.fp = context->fp;
		DEBUG_info.op = context->op;
		DEBUG_info.cp = context->cp;
	}
	else
	{
		DEBUG_info.bp = BP;
		DEBUG_info.pp = PP;
		DEBUG_info.fp = FP;
		DEBUG_info.op = OP;
		DEBUG_info.cp = CP;
	}
}


static void command_frame(char *cmd)
{
	int i;
	int frame;
	STACK_CONTEXT *context = NULL;
	
	if (cmd) 
	{
		frame = atoi(&cmd[1]);
		//fprintf(_out, "switching to frame %d\n", frame);
	
		if (frame > 0)
		{
			for (i = 0;; i++)
			{
				context = GB_DEBUG.GetStack(i);
				if (!context)
					break;
				if (!context->pc && !context->cp)
					continue;
				
				frame--;
				if (!frame)
					break;
			}
		}
	}

	set_info(context);
	debug_info(cmd != NULL);
}


static void command_eval(char *cmd)
{
	EXPRESSION *expr;
	ERROR_INFO save_error = { 0 };
	ERROR_INFO save_last = { 0 };
	DEBUG_INFO save_debug;
	VALUE *val;
	VALUE result;
	int start, len;
	FILE *out;
	const char *name;
	int ret;

	init_eval_interface();

	//out = *cmd == '!' ? stdout : _out;
	out = _out;

	len = strlen(cmd);
	for (start = 0; start < len; start++)
	{
		if (cmd[start] == '\t')
			break;
		//if (*cmd != '!')
			fputc(cmd[start], _out);
	}
	
	if (start >= len)
		return;

	//if (*cmd != '!')
		fprintf(_out, "\t");

	GB_DEBUG.SaveError(&save_error, &save_last);
	save_debug = DEBUG_info;

	start++;
	EVAL.New(POINTER(&expr), &cmd[start], len - start);

	if (EVAL.Compile(expr, *cmd == '='))
	{
		//if (*cmd != '!')
			fprintf(_out, "!");
		fputs(expr->error, out);
		goto __END;
	}
	
	GB_DEBUG.EnterEval();
	val = (VALUE *)EVAL.Run(expr, GB_DEBUG.GetValue);
	GB_DEBUG.LeaveEval();
	if (!val)
		goto __ERROR;

	result = *val;
	GB.BorrowValue((GB_VALUE *)&result);
	
	switch(*cmd)
	{		  
		case '?':
			PRINT_value(out, val, TRUE);
			break;
			
		case '!':
			PRINT_value(out, val, FALSE);
			break;
			
		case '#':
			PRINT_object(out, val);
			break;
			
		case '=':
			if (!EVAL.GetAssignmentSymbol(expr, &name, &len))
			{
				ret = GB_DEBUG.SetValue(name, len, val);
				if (ret == GB_DEBUG_SET_ERROR)
				{
					GB.ReleaseValue((GB_VALUE *)&result);
					goto __ERROR;
				}
				else if (ret == GB_DEBUG_SET_READ_ONLY)
				{
					GB.ReleaseValue((GB_VALUE *)&result);
					fprintf(out, "!%.*s is read-only", len, name);
					goto __END;
				}
			}
			fprintf(out, "OK");
			break;
	}

	GB.ReleaseValue((GB_VALUE *)&result);
	goto __END;

__ERROR:

	fprintf(out, "!");
	fputs(GB_DEBUG.GetErrorMessage(), out);

__END:

	EVAL.Free(POINTER(&expr));
	DEBUG_info = save_debug; //.cp = NULL;
	GB_DEBUG.RestoreError(&save_error, &save_last);
	
	fputc('\n', out);
	fflush(out);
}

static int find_watch(int id)
{
	int i;
	DEBUG_WATCH *watch;
	
	for (i = 0; i < GB.Count(_watches); i++)
	{
		watch = &_watches[i];
		if (watch->id == id)
			return i;
	}
	
	return -1;
}

static int add_watch(int id, EXPRESSION *expr, VALUE *value)
{
	DEBUG_WATCH *watch = (DEBUG_WATCH *)GB.Add(&_watches);
	watch->id = id;
	watch->expr = expr;
	if (value)
		watch->value = *value;
	return GB.Count(_watches) - 1;
}


static void remove_watch(int index)
{
	DEBUG_WATCH *watch = &_watches[index];
	
	EVAL.Free(POINTER(&watch->expr));
	GB.Remove(&_watches, index, 1);
}

static void command_watch(char *cmd)
{
	EXPRESSION *expr;
	int start, len;
	int id;
	int index;
	VALUE *value;
	ERROR_INFO save_error = { 0 };
	ERROR_INFO save_last = { 0 };
	DEBUG_INFO save_debug;

	init_eval_interface();

	len = strlen(cmd);
	for (start = 0; start < len; start++)
	{
		if (cmd[start] == '\t')
			break;
	}
	
	cmd[start] = 0;
	id = atoi(&cmd[1]);
	if (id == 0)
		return;
	
	expr = NULL;
	value = NULL;
	
	if (len > start)
	{
		GB_DEBUG.SaveError(&save_error, &save_last);
		save_debug = DEBUG_info;

		start++;
		EVAL.New(POINTER(&expr), &cmd[start], len - start);

		if (EVAL.Compile(expr, FALSE))
		{
			fputs(expr->error, _out);
			EVAL.Free(POINTER(&expr));
		}
		else
		{
			GB_DEBUG.EnterEval();
			value = (VALUE *)EVAL.Run(expr, GB_DEBUG.GetValue);
			GB_DEBUG.LeaveEval();
		}

		DEBUG_info = save_debug;
		GB_DEBUG.RestoreError(&save_error, &save_last);
		
		if (!expr)
			return;
	}

	index = find_watch(id);
	if (index >= 0)
		remove_watch(index);
	if (expr)
		add_watch(id, expr, value);
	
	DEBUG_info.watch = GB.Count(_watches) > 0;
}


static void command_symbol(char *cmd)
{
	int start, len;
	ERROR_INFO save_error = { 0 };
	ERROR_INFO save_last = { 0 };
	DEBUG_INFO save_debug;

	GB_DEBUG.SaveError(&save_error, &save_last);
	save_debug = DEBUG_info;

	len = strlen(cmd);
	for (start = 0; start < len; start++)
	{
		if (cmd[start] == '\t')
			break;
		fputc(cmd[start], _out);
	}
	
	if (start >= len)
		return;

	fprintf(_out, "\t");

	/*DEBUG_info.bp = BP;
	DEBUG_info.fp = FP;
	DEBUG_info.op = OP;
	DEBUG_info.cp = CP;*/
	
	start++;
	PRINT_symbol(_out, &cmd[start], len - start);
	
	fputc('\n', _out);
	fflush(_out);
	
	DEBUG_info = save_debug;
	GB_DEBUG.RestoreError(&save_error, &save_last);
}


static void command_option(char *cmd)
{
	bool on;
	
	if (!cmd[1] || !cmd[2])
		return;
	
	on = cmd[2] == '+';
	
	switch(cmd[1])
	{
		case 'b':
			
			GB_DEBUG.BreakOnError(on);
			break;
			
		case 'g':
			GB_DEBUG.DebugInside(on);
			break;
			
		default:
			;
	}
}


void DEBUG_breakpoint(int id)
{
	DEBUG_main(FALSE);
}


const char *DEBUG_get_position(CLASS *cp, FUNCTION *fp, PCODE *pc)
{
	const char *comp_name;
	const char *class_name;
	const char *func_name;
	ushort line = 0;

	if (!cp)
		return "?";

	class_name = cp->name;

	while (*class_name == '^')
		class_name++;

	if (cp->component)
		comp_name = cp->component->name;
	else
		comp_name = "$";

	if (fp && fp->debug)
	{
		func_name = fp->debug->name;
		if (pc)
			DEBUG_calc_line_from_position(cp, fp, pc, &line);
	}
	else
		func_name = "?";

	snprintf(DEBUG_buffer, sizeof(DEBUG_buffer), "[%s].%s.%s.%d", comp_name, class_name, func_name, line);
	return DEBUG_buffer;
}

const char *DEBUG_get_profile_position(CLASS *cp, FUNCTION *fp, PCODE *pc)
{
	static uint prof_index = 0;
	const char *name;
	const char *func;
	char buffer[16], buffer2[16];
	
	func = "?";
	
	if (cp)
	{
		if (cp->load && cp->load->prof)
		{
			if (cp->load->prof[0] == 0)
			{
				prof_index++;
				cp->load->prof[0] = prof_index;
				name = cp->name;
			}
			else
			{
				sprintf(buffer, "%u", cp->load->prof[0]);
				name = buffer;
			}

			if (fp && fp->debug)
			{
				int i = fp->debug->index + 1;
				if (cp->load->prof[i] == 0)
				{
					prof_index++;
					cp->load->prof[i] = prof_index;
					func = fp->debug->name;
				}
				else
				{
					sprintf(buffer2, "%u", cp->load->prof[i]);
					func = buffer2;
				}
			}
			else
				func = "?";
		}
		else
			name = cp->name;
	}
	else
		name = "?";
	
	if (pc)
	{
		ushort line = 0;

		if (fp != NULL && fp->debug)
			DEBUG_calc_line_from_position(cp, fp, pc, &line);

		snprintf(DEBUG_buffer, sizeof(DEBUG_buffer), "%.64s.%.64s.%d", name, func, line);
	}
	else
	{
		snprintf(DEBUG_buffer, sizeof(DEBUG_buffer), "%.64s.%.64s", name, func);
	}

	return DEBUG_buffer;
}


const char *DEBUG_get_current_position(void)
{
	return DEBUG_get_position(CP, FP, PC);
}


void DEBUG_where(void)
{
	fprintf(_out ? _out : stderr, "%s: ", DEBUG_get_current_position());
}


void DEBUG_welcome(void)
{
	if (!_fifo)
		fprintf(_out, DEBUG_WELCOME);
}

static bool compare_values(VALUE *a, VALUE *b)
{
	void *jump[] = {
		&&__VOID, &&__BOOLEAN, &&__BYTE, &&__SHORT, &&__INTEGER, &&__LONG, &&__SINGLE, &&__FLOAT, 
		&&__DATE, &&__STRING, &&__STRING, &&__POINTER, &&__VARIANT, &&__FUNCTION, &&__CLASS, &&__NULL
	};
	
	void *vjump[] = {
		&&__VOID, &&__VBOOLEAN, &&__VBYTE, &&__VSHORT, &&__VINTEGER, &&__VLONG, &&__VSINGLE, &&__VFLOAT, 
		&&__VDATE, &&__VSTRING, &&__VSTRING, &&__VPOINTER, &&__VOID, &&__VOID, &&__VOID, &&__VOID
	};
	
	if (a->type != b->type)
		return TRUE;
	
	if (TYPE_is_object(a->type))
		goto __OBJECT;
	else
		goto *jump[a->type];
	
__VOID:
__NULL:
	return FALSE;
	
__BOOLEAN:
__BYTE:
__SHORT:
__INTEGER:
	return a->_integer.value != b->_integer.value;
	
__LONG:
__DATE:
	return a->_long.value != b->_long.value;

__SINGLE:
	return a->_single.value != b->_single.value;
	
__FLOAT:
	return a->_float.value != b->_float.value;

__STRING:
	return a->_string.addr != b->_string.addr || a->_string.start != b->_string.start || a->_string.len != b->_string.len;
	
__POINTER:
__OBJECT:
__CLASS:
	return a->_pointer.value != b->_pointer.value;
	
__FUNCTION:
	return a->_function.class != b->_function.class || a->_function.object != b->_function.object || a->_function.index != b->_function.index;
	
__VARIANT:
	if (a->_variant.vtype != b->_variant.vtype)
		return TRUE;
	
	if (TYPE_is_object(a->_variant.vtype))
		goto __VOBJECT;
	else
		goto *vjump[a->_variant.vtype];
	
__VBOOLEAN: return a->_variant.value._boolean != b->_variant.value._boolean;
__VBYTE: return a->_variant.value._byte != b->_variant.value._byte;
__VSHORT: return a->_variant.value._short != b->_variant.value._short;
__VINTEGER: return a->_variant.value._integer != b->_variant.value._integer;
__VDATE: __VLONG: return a->_variant.value._long != b->_variant.value._long;
__VSINGLE: return a->_variant.value._single!= b->_variant.value._single;
__VFLOAT: return a->_variant.value._float != b->_variant.value._float;
__VPOINTER: __VOBJECT: __VSTRING: return a->_variant.value._string != b->_variant.value._string;
}


bool DEBUG_check_watches(void)
{
	int i;
	DEBUG_WATCH *watch;
	VALUE *value;
	bool changed = FALSE;
	ERROR_INFO save_error = { 0 };
	ERROR_INFO save_last = { 0 };
	DEBUG_INFO save_debug;
	
	set_info(NULL);
	
	GB_DEBUG.SaveError(&save_error, &save_last);
	save_debug = DEBUG_info;
	
	for (i = 0; i < GB.Count(_watches); i++)
	{
		watch = &_watches[i];
		watch->changed = FALSE;
		
		GB_DEBUG.EnterEval();
		value = (VALUE *)EVAL.Run(watch->expr, GB_DEBUG.GetValue);
		GB_DEBUG.LeaveEval();
		if (!value)
			continue;
		
		if (compare_values(&watch->value, value))
		{
			if (watch->value.type != T_VOID)
				changed = TRUE;
			watch->value = *value;
			watch->changed = TRUE;
		}
	}
	
	DEBUG_info = save_debug;
	GB_DEBUG.RestoreError(&save_error, &save_last);

	if (!changed)
		return FALSE;
	
	DEBUG_main(FALSE);
	return TRUE;
}


static void open_read_fifo()
{
	char path[DEBUG_FIFO_PATH_MAX];

	if (_fifo)
	{
		//fprintf(stderr, "open_read_fifo\n");

		snprintf(path, sizeof(path), "%sout", DEBUG_fifo);
		
		for(;;)
		{
			_fdr = open(path, O_RDONLY | O_CLOEXEC);
			if (_fdr >= 0)
				break;
			if (errno != EAGAIN && errno != EINTR)
			{
				fprintf(stderr, "gb.debug: unable to open output fifo: %s: %s\n", strerror(errno), path);
				return;
			}
			usleep(20000);
		}
		
		//fprintf(stderr, "gb.debug: open read fifo: %d\n", _fdr);
		_in = fdopen(_fdr, "r");

		if (!_in)
		{
			fprintf(stderr, "gb.debug: unable to open stream on output fifo: %s: %s\n", strerror(errno), path);
			return;
		}

		setlinebuf(_in);
	}
	else
	{
		_in = stdin;
	}
}


void DEBUG_main(bool error)
{
	static DEBUG_TYPE last_command = TC_NONE;

	static DEBUG_COMMAND Command[] =
	{
		// "p" and "i" are reserved for remote debugging.
		
		{ "q", TC_NONE, command_quit, FALSE },
		{ "n", TC_NEXT, command_next, FALSE },
		{ "s", TC_STEP, command_step, FALSE },
		{ "f", TC_FROM, command_from, FALSE },
		{ "g", TC_GO, command_go, FALSE },
		{ "+", TC_NONE, command_breakpoint, TRUE },
		{ "-", TC_NONE, command_breakpoint, TRUE },
		{ "&", TC_NONE, command_symbol, TRUE },
		{ "?", TC_NONE, command_eval, TRUE },
		{ "!", TC_NONE, command_eval, TRUE },
		{ "#", TC_NONE, command_eval, TRUE },
		{ "=", TC_NONE, command_eval, TRUE },
		{ "@", TC_NONE, command_frame, TRUE },
		{ "o", TC_NONE, command_option, TRUE },
		{ "w", TC_NONE, command_watch, TRUE },
		{ "h", TC_NONE, command_hold, TRUE },

		{ NULL }
	};

	static bool first = TRUE;
	char *cmd = NULL;
	char cmdbuf[256];
	int len;
	DEBUG_COMMAND *tc = NULL;
	int save_errno = errno;
	int try = 0;

	GB.FreeString(&_error);
	if (error)
		_error = GB.NewZeroString(GB_DEBUG.GetErrorMessage());

	fflush(_out);

	#ifdef DEBUG_ME
	fprintf(stderr, "DEBUG_main {\n");
	#endif

	if (_fifo)
	{
		fprintf(_out, first ? "!!\n" : "!\n");
		fflush(_out);
		first = FALSE;
	}

	command_frame(NULL);
	
	if (!_in)
		open_read_fifo();

	do
	{
		/*if (CP == NULL)
			printf("[]:");
		else
			printf("[%s%s]:", DEBUG_get_current_position(), _error ? "*" : "");*/

		GB.Component.Signal(GB_SIGNAL_DEBUG_BREAK, 0);

		if (!_fifo)
		{
			fprintf(_out, "> ");
			fflush(_out);
		}

		GB.FreeString(&cmd);

	__TRY_AGAIN:

		for(;;)
		{
			*cmdbuf = 0;
			errno = 0;
			if (fgets(cmdbuf, sizeof(cmdbuf), _in) == NULL && errno != EINTR)
				break;
			if (!*cmdbuf)
				continue;
			cmd = GB.AddString(cmd, cmdbuf, 0);
			if (cmd[GB.StringLength(cmd) - 1] == '\n')
				break;
		}

		len = GB.StringLength(cmd);
		
		// A null string command means an I/O error or an end-of-file
		// We try to reopen the fifo.
		if (len == 0)
		{
			if (errno)
				fprintf(stderr, "gb.debug: warning: unable to read debugger input: %s\n", strerror(errno));
			else
			{
				if (_debug)
					fprintf(stderr, "gb.debug: input has been closed, reopen it.\n");
				usleep(1000);
			}

			fclose(_in);
			open_read_fifo();
			try++;
			if (try < 3)
				goto __TRY_AGAIN;
			_exit(1);
		}
		
		if (len > 0 && cmd[len - 1] == '\n')
		{
			len--;
			cmd[len] = 0;
		}
		
		#ifdef DEBUG_ME
		fprintf(stderr, "--> %s\n", cmd);
		#endif

		if (len == 0)
		{
			if (last_command == TC_NONE || _fifo)
				continue;

			for (tc = Command; tc->pattern; tc++)
			{
				if (tc->type == last_command)
				{
					(*tc->func)(cmd);
					break;
				}
			}
		}
		else
		{
			for (tc = Command; tc->pattern; tc++)
			{
				if (strncasecmp(tc->pattern, cmd, strlen(tc->pattern)) == 0)
				{
					if (tc->type != TC_NONE)
						last_command = tc->type;
					(*tc->func)(cmd);
					break;
				}
			}
		}

		if (tc->pattern == NULL)
			WARNING("Unknown command: %s", cmd);

		fflush(_out);
	}
	while (last_command == TC_NONE || tc->pattern == NULL || tc->loop);

	GB.FreeString(&cmd);

	errno = save_errno;

	#ifdef DEBUG_ME
	fprintf(stderr, "} DEBUG_main\n");
	#endif
}

