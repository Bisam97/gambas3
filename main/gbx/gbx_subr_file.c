/***************************************************************************

	gbx_subr_file.c

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

#include "gb_common.h"
#include "gb_common_buffer.h"

#include "gbx_subr.h"
#include "gb_file.h"
#include "gb_list.h"
#include "gbx_stream.h"
#include "gbx_archive.h"
#include "gbx_api.h"
#include "gbx_local.h"
#include "gbx_regexp.h"
#include "gbx_string.h"
#include "gbx_c_file.h"
#include "gbx_math.h"
#include "gbx_project.h"

#include <sys/file.h>

typedef
	struct _stream {
		struct _stream *next;
		CSTREAM *stream;
		}
	CSTREAM_NODE;

static void *_default_in = NULL;
static void *_default_out = NULL;
static void *_default_err = NULL;

static void **_default_list[3] = { &_default_in, &_default_out, &_default_err };

static GB_ARRAY _result;
static char *_pattern;
static int _len_pattern;
static int _ignore;

static STREAM *_stream;

static bool _use_debug = FALSE;

static bool _init_log_func = FALSE;
static GB_FUNCTION _log_func;
static bool _use_log_func;
static bool _inside_log = FALSE;


static void push_stream(void **list, CSTREAM *stream)
{
	CSTREAM_NODE *slot;

	ALLOC(&slot, sizeof(CSTREAM_NODE));
	slot->stream = stream;
	//OBJECT_REF(stream);

	slot->next = *list;
	*list = slot;
}


static CSTREAM *pop_stream(void **list)
{
	CSTREAM *stream;
	CSTREAM_NODE *slot;

	if (!*list)
		return NULL;

	stream = ((CSTREAM_NODE *)*list)->stream;
	slot = *list;
	*list = slot->next;
	FREE(&slot);

	return stream;
}


static STREAM *get_default(int val)
{
	STREAM *stream = NULL;

	switch(val)
	{
		case CFILE_IN:
			if (_default_in)
				stream = CSTREAM_TO_STREAM(((CSTREAM_NODE *)_default_in)->stream);
			else
				stream = CSTREAM_TO_STREAM(CFILE_get_standard_stream(CFILE_IN));
			break;
		case CFILE_OUT:
			if (_default_out)
				stream = CSTREAM_TO_STREAM(((CSTREAM_NODE *)_default_out)->stream);
			else
				stream = CSTREAM_TO_STREAM(CFILE_get_standard_stream(CFILE_OUT));
			break;
		case CFILE_ERR:
			if (_default_err)
				stream = CSTREAM_TO_STREAM(((CSTREAM_NODE *)_default_err)->stream);
			else
			{
				if (!_init_log_func && PROJECT_class)
				{
					_init_log_func = TRUE;
				  GB_GetFunction(&_log_func, PROJECT_class, "Application_Log", "ss", "");
				}

				if (!_inside_log && GB_FUNCTION_IS_VALID(&_log_func))
				{
					_use_log_func = TRUE;
					return NULL;
				}

				stream = CSTREAM_TO_STREAM(CFILE_get_standard_stream(CFILE_ERR));
			}
			break;
	}

	if (!stream)
		THROW(E_CLOSED);
	
	return stream;
}

NORETURN static void throw_bad_stream(VALUE *value)
{
	if (VALUE_is_null(value))
		THROW(E_NULL);
	else
		THROW_TYPE((TYPE)CLASS_Stream, (TYPE)value->type);
}

static inline STREAM *_get_stream(VALUE *value, bool can_default)
{
	STREAM *stream;
	
	VARIANT_undo(value);
	
	if ((can_default) && TYPE_is_integer(value->type) && value->_integer.value >= 0 && value->_integer.value <= 3)
		stream = get_default(value->_integer.value);
	else
	{
		if (TYPE_is_object(value->type) && value->_object.object && OBJECT_class(value->_object.object)->is_stream)
			stream = CSTREAM_TO_STREAM(value->_object.object);
		else
			throw_bad_stream(value);
	}
	
	return stream;
}

#define get_stream(_value, _can_default) \
({ \
	STREAM *_stream = _get_stream(_value, _can_default); \
	\
	if (!_stream || STREAM_is_closed(_stream)) \
		THROW(E_CLOSED); \
	\
	_stream; \
})

#define get_stream_for_writing(_value, _can_default) \
({ \
	STREAM *_stream = _get_stream(_value, _can_default); \
	\
	if (!_stream || STREAM_is_closed_for_writing(_stream)) \
		THROW(E_CLOSED); \
	\
	_stream; \
})

static char *get_path(VALUE *param)
{
	char *name;
	int len;

	SUBR_get_string_len(param, &name, &len);

	return STRING_conv_file_name(name, len);
}

void SUBR_open(ushort code)
{
	CFILE *file;
	STREAM stream;
	int mode;
	void *addr;

	SUBR_ENTER_PARAM(2);

	SUBR_check_integer(&PARAM[1]);
	mode = PARAM[1]._integer.value ^ GB_ST_BUFFERED;

	if (code & 0x3F)
	{
		if (TYPE_is_pointer(PARAM->type))
			addr = (void *)PARAM->_pointer.value;
		else
			THROW_TYPE(T_POINTER, PARAM->type);
		
		STREAM_open(&stream, (char *)addr, mode | GB_ST_MEMORY);
	}
	else if (mode & GB_ST_STRING)
	{
		char *str;

		STREAM_open(&stream, NULL, mode);

		if (!VALUE_is_null(PARAM))
		{
			str = SUBR_get_string(PARAM);
			
			if (mode & GB_ST_WRITE)
			{
				stream.string.buffer = STRING_new(str, STRING_length(str));
			}
			else
			{
				stream.string.buffer = str;
				STRING_ref(str);
			}
			stream.string.size = STRING_length(str);
		}
	}
	else
	{
		STREAM_open(&stream, get_path(PARAM), mode);
	}

	file = CFILE_create(&stream, mode);

	OBJECT_put(RETURN, file);

	SUBR_LEAVE();
}


static void do_lock(bool wait)
{
	SUBR_ENTER_PARAM(wait ? 2 : 1);
	STREAM stream;
	CFILE *file;
	const char *path = get_path(PARAM);
	double next, timer;

	if (FILE_is_relative(path))
		THROW(E_BADPATH);

	if (wait)
	{
		DATE_timer(&next, FALSE);
		next += SUBR_get_float(&PARAM[1]);
	}

	for(;;)
	{
		STREAM_open(&stream, path, GB_ST_LOCK);

		if (!STREAM_lock_all(&stream) && FILE_exist(path))
			break;

		STREAM_close(&stream);

		if (wait)
		{
			DATE_timer(&timer, FALSE);
			if (timer < next)
			{
				usleep(10000);
				continue;
			}
		}

		THROW(E_LOCK);
	}

	file = CFILE_create(&stream, GB_ST_LOCK);
	OBJECT_put(RETURN, file);
	SUBR_LEAVE();
}

void SUBR_close(ushort code)
{
	static void *jump[] = { &&__CLOSE, &&__FLUSH, &&__INP_OUT, &&__INP_OUT, &&__INP_OUT, &&__LINE_INPUT, &&__LOCK, &&__UNLOCK, &&__LOCK };

	STREAM *stream;
	CSTREAM *cstream;
	void **where;
	char *addr;

	SUBR_ENTER_PARAM(1);

	code &= 0x1F;
	goto *jump[code];

__CLOSE:

	stream = get_stream(PARAM, FALSE);

	if (stream->type == &STREAM_string)
	{
		char *buffer = stream->string.buffer;

		RETURN->type = T_STRING;
		RETURN->_string.addr = buffer;
		RETURN->_string.start = 0;
		RETURN->_string.len = stream->string.size;

		STRING_ref(buffer);
		STREAM_close(stream);
		STRING_free_later(buffer);

		//fprintf(stderr, "buffer ref = %d\n", STRING_from_ptr(buffer)->ref);

		SUBR_LEAVE();
	}
	else
	{
		STREAM_close(stream);
		SUBR_LEAVE_VOID();
	}

	return;

__FLUSH:

	STREAM_flush(get_stream(PARAM, TRUE));

	SUBR_LEAVE_VOID();
	return;

__INP_OUT:

	where = _default_list[code - 2];

	if (VALUE_is_null(PARAM))
	{
		cstream = pop_stream(where);
		if (cstream)
			OBJECT_UNREF(cstream);
		return;
	}

	VALUE_conv_object(PARAM, (TYPE)CLASS_Stream);

	cstream = PARAM->_object.object;
	OBJECT_REF(cstream);

	push_stream(where, cstream);

	SUBR_LEAVE_VOID();
	return;

__LINE_INPUT:

	stream = get_stream(&SP[-1], TRUE);

	addr = STREAM_line_input(stream, NULL);

	SP--;
	if (!TYPE_is_integer(SP->type))
		RELEASE_OBJECT(SP);

	SP->type = T_STRING;
	SP->_string.addr = addr;
	SP->_string.start = 0;
	SP->_string.len = STRING_length(addr);

	SP++;
	return;

__UNLOCK:

	STREAM_close(get_stream(PARAM, FALSE));
	SUBR_LEAVE_VOID();
	return;

__LOCK:

	do_lock(code == 8);
	return;
}


void SUBR_flush(void)
{
	SUBR_close(1);
}


/*static void print_it(char *addr, long len)
{
	STREAM_write(_stream, addr, len);
}*/

void SUBR_print(ushort code)
{
	int i;
	char *addr;
	int len;
	const char *prefix;
	char *result;
	bool use_log;
	bool use_debug;

	SUBR_ENTER();

	if (NPARAM < 1)
		THROW(E_NEPARAM);

	_use_log_func = FALSE;
	_stream = get_stream_for_writing(PARAM, TRUE);
	use_log = _use_log_func;
	use_debug = _use_debug;

	if (use_debug)
	{
		_use_debug = FALSE;

		if (!use_log)
		{
			prefix = DEBUG_get_current_position();
	    STREAM_write(_stream, (void *)prefix, strlen(prefix));
	    STREAM_write(_stream, ": ", 2);
		}
	}
	else
		prefix = NULL;

	if (use_log)
		STRING_start();

	for (i = 1; i < NPARAM; i++)
	{
		PARAM++;
		VALUE_to_local_string(PARAM, &addr, &len);
		if (use_log)
			STRING_make(addr, len);
		else if (len == 1 && *addr == '\n')
			STREAM_write_eol(_stream);
		else
			STREAM_write(_stream, addr, len);
	}

	if (use_log)
	{
		result = STRING_end_temp();
		if (use_debug)
			prefix = STRING_new_temp_zero(DEBUG_get_current_position());
		else
			prefix = NULL;
		GB_Push(2, T_STRING, result, STRING_length(result), T_STRING, prefix, STRING_length(prefix));
		_inside_log = TRUE;
		GB_Call(&_log_func, 2, FALSE);
		_inside_log = FALSE;
	}

	SUBR_LEAVE_VOID();
}


void SUBR_linput(void)
{
	SUBR_close(5);
}


void SUBR_input(ushort code)
{
	static STREAM *stream = NULL;
	char *addr;

	SUBR_ENTER();

	if (NPARAM == 1)
		stream = get_stream(PARAM, TRUE);

	if (stream)
		addr = STREAM_input(stream);
	else
		addr = NULL;
		
	if (NPARAM == 1)
	{
		SP--;
		if (!TYPE_is_integer(SP->type))
			RELEASE_OBJECT(SP);
	}
	
	if (addr)
	{
		//VALUE_from_string(SP, addr, STRING_length(addr));
		SP->type = T_STRING;
		SP->_string.addr = addr;
		SP->_string.start = 0;
		SP->_string.len = STRING_length(addr);
	}
	else
		STRING_void_value(SP);
		
	SP++;
}


void SUBR_eof(ushort code)
{
	STREAM *stream;
	bool eof;

	SUBR_ENTER();

	if (NPARAM == 1)
	{
		stream = get_stream(PARAM, FALSE);
		eof = STREAM_eof(stream);
		RELEASE_OBJECT(PARAM);
		SP--;
	}
	else
	{
		stream = get_default(0);
		eof = STREAM_eof(stream);
	}

	SP->type = T_BOOLEAN;
	SP->_boolean.value = (-eof);
	SP++;
}


void SUBR_lof(ushort code)
{
	STREAM *stream;

	SUBR_ENTER();

	if (NPARAM == 1)
		stream = get_stream(PARAM, FALSE);
	else
		stream = get_default(0);

	RETURN->type = T_LONG;
	STREAM_lof(stream, &(RETURN->_long.value));

	SUBR_LEAVE();
}


void SUBR_seek(ushort code)
{
	STREAM *stream;
	int64_t pos;
	int64_t len;
	int whence = SEEK_SET;

	SUBR_ENTER();

	stream = get_stream(PARAM, FALSE);

	if (NPARAM >= 2)
	{
		VALUE_conv(&PARAM[1], T_LONG);
		pos = PARAM[1]._long.value;

		if (NPARAM == 3)
		{
			VALUE_conv_integer(&PARAM[2]);
			whence = PARAM[2]._integer.value;
			if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
				THROW_ARG();
		}
		else
		{
			if (pos < 0)
			{
				STREAM_lof(stream, &len);
				pos += len;
			}
		}

		STREAM_seek(stream, pos, (int)whence);
		RETURN->type = T_VOID;
	}
	else
	{
		RETURN->type = T_LONG;
		RETURN->_long.value = STREAM_tell(stream);
	}

	SUBR_LEAVE();
}

void SUBR_read(ushort code)
{
	STREAM *stream;
	char *data;
	int len = 0;
	int eff;

	SUBR_ENTER_PARAM(2);

	stream = get_stream(PARAM, TRUE);
	
	if (code & 0x3F)
	{
		VALUE_conv_integer(&PARAM[1]);
		len = PARAM[1]._integer.value;
		
		if (len == 0)
		{
			STRING_void_value(RETURN);
		}
		else if (len > 0)
		{
			data = STRING_new_temp(NULL, len);
			
			if ((code & 0x3F) == 2)
				STREAM_peek(stream, data, len);
			else
				STREAM_read(stream, data, len);
			
			RETURN->type = T_STRING;
			RETURN->_string.addr = data;
			RETURN->_string.start = 0;
			RETURN->_string.len = len;
		}
		else
		{
			len = (-len);
			
			data = STRING_new(NULL, len);
			
			eff = STREAM_read_max(stream, data, len);
			
			if (eff == 0)
			{
				STRING_void_value(RETURN);
				STRING_free(&data);
			}
			else
			{
				if (eff < len)
				{
					data = STRING_extend(data, eff);
					len = eff;
				}
				
				STRING_extend_end(data);
				
				RETURN->type = T_STRING;
				RETURN->_string.addr = data;
				RETURN->_string.start = 0;
				RETURN->_string.len = len;
			}
		}
	}
	else
	{
		TYPE type = SUBR_get_type(&PARAM[1]);
		STREAM_read_type(stream, type, RETURN);
	}

	SUBR_LEAVE();
}


void SUBR_write(ushort code)
{
	STREAM *stream;

	SUBR_ENTER_PARAM(3);

	stream = get_stream_for_writing(PARAM, TRUE);

	if (code & 0x3F)
	{
		char *str;
		int len;
		int lenw;
		
		VALUE_conv_integer(&PARAM[2]);
		lenw = PARAM[2]._integer.value;
		
		if (TYPE_is_pointer(PARAM[1].type))
		{
			if (lenw < 0)
				lenw = 0;
			len = lenw;
			str = (char *)PARAM[1]._pointer.value;
		}
		else
		{
			SUBR_get_string_len(&PARAM[1], &str, &len);
			if (lenw < 0)
				lenw = len;
		}
		
		if (lenw > 0)
		{
			STREAM_write(stream, str, Min(len, lenw));
			if (lenw > len)
				STREAM_write_zeros(stream, lenw - len);
		}
	}
	else
	{
		TYPE type;
		type = SUBR_get_type(&PARAM[2]);
		VALUE_conv(&PARAM[1], type);
		STREAM_write_type(stream, type, &PARAM[1]);
	}
	
	SUBR_LEAVE_VOID();
}


void SUBR_stat(ushort code)
{
	const char *path;
	CSTAT *cstat;
	FILE_STAT info;
	bool follow = FALSE;

	SUBR_ENTER();

	path = get_path(PARAM);

	if (NPARAM == 2)
		follow = SUBR_get_boolean(&PARAM[1]);
	
	if (FILE_stat(path, &info, follow))
		THROW_SYSTEM(errno, path);

	cstat = OBJECT_new(CLASS_Stat, NULL, NULL);
	OBJECT_UNREF_KEEP(cstat);
	cstat->info = info;
	cstat->path = STRING_new_zero(path);

	RETURN->_object.class = CLASS_Stat;
	RETURN->_object.object = cstat;

	SUBR_LEAVE();
}


void SUBR_exist(ushort code)
{
	bool exist;
	const char *path;
	bool follow = FALSE;

	SUBR_ENTER();
	
	if (!NPARAM)
	{
		NPARAM++;
		PARAM--;
	}

	path = get_path(PARAM);

	if (NPARAM == 2)
		follow = SUBR_get_boolean(&PARAM[1]);

	exist = FILE_exist_follow(path, follow);

	RETURN->type = T_BOOLEAN;
	RETURN->_integer.value = exist ? -1 : 0;

	SUBR_LEAVE();
}


void SUBR_dir(ushort code)
{
	GB_ARRAY array;
	const char *path;
	char *pattern;
	int len_pattern;
	char *str;
	int attr = 0;

	SUBR_ENTER();

	path = get_path(PARAM);

	if (NPARAM >= 2)
	{
		pattern = SUBR_get_string(&PARAM[1]);
		if (NPARAM == 3)
			attr = SUBR_get_integer(&PARAM[2]);
	}
	else
		pattern = NULL;

	FILE_dir_first(path, pattern, attr);

	GB_ArrayNew(&array, T_STRING, 0);

	while (!FILE_dir_next(&pattern, &len_pattern))
	{
		if (!LOCAL_is_UTF8)
		{
			if (STRING_conv(&str, pattern, len_pattern, LOCAL_encoding, SC_UTF8, FALSE))
				str = STRING_new(pattern, len_pattern);
			else
				STRING_ref(str);
		}
		else
			str = STRING_new(pattern, len_pattern);

		*((char **)GB_ArrayAdd(array)) = str;
	}

	RETURN->_object.class = OBJECT_class(array);
	RETURN->_object.object = array;

	SUBR_LEAVE();
}


static void found_file(const char *path)
{
	char *str;
	int len;

	path += _ignore;
	len = strlen(path);

	if (_pattern && !REGEXP_match(_pattern, _len_pattern, path, len))
		return;

	if (!LOCAL_is_UTF8)
	{
		if (STRING_conv(&str, path, len, LOCAL_encoding, SC_UTF8, FALSE))
			str = STRING_new(path, len);
		else
			STRING_ref(str);
	}
	else
		str = STRING_new(path, len);

	if (!_result)
		GB_ArrayNew(&_result, T_STRING, 0);

	*((char **)GB_ArrayAdd(_result)) = str;
}

void SUBR_rdir(ushort code)
{
	const char *path;
	int attr = 0;
	bool follow = FALSE;

	SUBR_ENTER();

	path = get_path(PARAM);

	if (NPARAM >= 2)
	{
		SUBR_get_string_len(&PARAM[1], &_pattern, &_len_pattern);
		if (NPARAM >= 3)
		{
			attr = SUBR_get_integer(&PARAM[2]);
			if (NPARAM == 4)
				follow = SUBR_get_boolean(&PARAM[3]);
		}
	}
	else
		_pattern = NULL;

	_result = NULL;

	if (!path || *path == 0)
		path = ".";
	_ignore = strlen(path);
	if (_ignore > 0 && path[_ignore - 1] != '/')
		_ignore++;

	FILE_recursive_dir(path, found_file, NULL, attr, follow);

	if (!_result)
		GB_ArrayNew(&_result, T_STRING, 0);

	RETURN->_object.class = OBJECT_class(_result);
	RETURN->_object.object = _result;

	SUBR_LEAVE();
}


void SUBR_kill(ushort code)
{
	SUBR_ENTER_PARAM(1);

	switch(code & 0xFF)
	{
		case 0:
			FILE_unlink(get_path(PARAM));
			break;
			
		case 1:
			FILE_mkdir_mode(get_path(PARAM), CFILE_default_dir_auth);
			break;
			
		case 2:
			FILE_rmdir(get_path(PARAM));
			break;

		default:
			THROW_ILLEGAL();
	}

	SUBR_LEAVE_VOID();
}


void SUBR_mkdir(ushort code)
{
	SUBR_ENTER_PARAM(1);
	
	switch (code & 0xFF)
	{
		case 0: // Deprecated Mkdir
			SUBR_kill(1);
			return;
			
		case 1: // Even
			VALUE_conv(PARAM, T_LONG);
			PARAM->type = GB_T_BOOLEAN;
			PARAM->_boolean.value = (PARAM->_long.value & 1) == 0 ? -1 : 0;
			break;

		case 2: // Odd
			VALUE_conv(PARAM, T_LONG);
			PARAM->type = GB_T_BOOLEAN;
			PARAM->_boolean.value = (PARAM->_long.value & 1) != 0 ? -1 : 0;
			break;
		
		default:
			THROW_ILLEGAL();
	}
}


void SUBR_rmdir(ushort code)
{
	SUBR_ENTER();
	int min, max;

	switch (code & 0x3F)
	{
		case 0: // Deprecated RmDir
			SUBR_kill(2);
			return;

		default: // Rand

			if (NPARAM == 1)
			{
				VALUE_conv_integer(PARAM);
				min = 0;
				max = PARAM->_integer.value;
			}
			else
			{
				VALUE_conv_integer(PARAM);
				VALUE_conv_integer(&PARAM[1]);
				min = PARAM->_integer.value;
				max = PARAM[1]._integer.value;
				if (max < min)
				{
					int temp = max;
					max = min;
					min = temp;
				}
			}

			RETURN->type = T_INTEGER;
			RETURN->_integer.value = min + (int)(rnd() * (max + 1 - min));
			break;
	}

	SUBR_LEAVE();
}


void SUBR_move(ushort code)
{
	char *path;
	char *auth;
	FILE_STAT info;
	
	SUBR_ENTER_PARAM(2);

	path = get_path(&PARAM[0]);
	
	switch (code & 0xFF)
	{
		case 0: // Move
			
			FILE_rename(path, get_path(&PARAM[1]));
			break;
			
		case 1: // Copy
			
			FILE_copy(path, get_path(&PARAM[1]));
			break;
			
		case 2: // Link
			
			/* Parameters are NOT inverted ANYMORE! */
			FILE_link(path, get_path(&PARAM[1]));
			break;
			
		case 3: // Chmod
			
			auth = SUBR_get_string(&PARAM[1]);
			FILE_stat(path, &info, TRUE);
			FILE_chmod(path, FILE_mode_from_string(info.mode, auth));
			break;
			
		case 4: // Chown
			
			auth = SUBR_get_string(&PARAM[1]);
			FILE_chown(path, auth);
			break;
			
		case 5: // Chgrp
			
			auth = SUBR_get_string(&PARAM[1]);
			FILE_chgrp(path, auth);
			break;
			
		case 6: // Move DownTo
			
			FILE_rename_unlink(path, get_path(&PARAM[1]));
			break;
			
		default:
			THROW_ILLEGAL();
	}

	SUBR_LEAVE_VOID();
}


void SUBR_link(ushort code)
{
	SUBR_ENTER_PARAM(1);
	
	switch (code & 0xFF)
	{
		case 0: // Deprecated Link
			SUBR_move(2);
			return;
			
		case 1: // IsNan
			VALUE_conv(PARAM, T_FLOAT);
			PARAM->type = GB_T_BOOLEAN;
			PARAM->_boolean.value = isnan(PARAM->_float.value) ? -1 : 0;
			break;

		case 2: // IsInf
			VALUE_conv(PARAM, T_FLOAT);
			PARAM->type = GB_T_INTEGER;
			PARAM->_integer.value = isinf(PARAM->_float.value);
			break;
		
		default:
			THROW_ILLEGAL();
	}
}


void SUBR_temp(ushort code)
{
	char *temp;
	int len;

	SUBR_ENTER();

	if (NPARAM == 0)
		temp = FILE_make_temp(&len, NULL);
	else
		temp = FILE_make_temp(&len, SUBR_get_string(PARAM));

	STRING_new_temp_value(RETURN, temp, len);

	SUBR_LEAVE();
}


void SUBR_isdir(void)
{
	bool isdir;
	const char *path;

	SUBR_ENTER_PARAM(1);

	path = get_path(PARAM);

	isdir = FILE_is_dir(path);

	RETURN->type = T_BOOLEAN;
	RETURN->_integer.value = isdir ? -1 : 0;

	SUBR_LEAVE();
}


void SUBR_access(ushort code)
{
	int access;

	SUBR_ENTER();

	if (NPARAM == 1)
		access = GB_ST_READ;
	else
	{
		VALUE_conv_integer(&PARAM[1]);
		access = PARAM[1]._integer.value;
	}

	RETURN->type = T_BOOLEAN;
	RETURN->_integer.value = FILE_access(get_path(PARAM), access) ? -1 : 0;

	SUBR_LEAVE();
}


void SUBR_lock(ushort code)
{
	SUBR_close((code & 0x1F) + 6);
}


void SUBR_inp_out(ushort code)
{
	SUBR_close(2 + (code & 0x1F));
}

static void free_list(void **list)
{
	CSTREAM *stream;

	for(;;)
	{
		stream = pop_stream(list);
		if (!stream)
			return;
		OBJECT_UNREF(stream);
	}
}

void SUBR_exit_inp_out(void)
{
	free_list((void **)&_default_in);
	free_list((void **)&_default_out);
	free_list((void **)&_default_err);
}


void SUBR_dfree(void)
{
	SUBR_ENTER_PARAM(1);

	RETURN->type = T_LONG;
	RETURN->_long.value = FILE_free(get_path(PARAM));

	SUBR_LEAVE();
}

void SUBR_debug(ushort code)
{
	SUBR_ENTER();

	if (NPARAM == 0)
	{
		_use_debug = TRUE;
		RETURN->type = T_INTEGER;
		RETURN->_integer.value = 2;
	}
	else if (NPARAM == 1)
	{
		VALUE_conv_boolean(PARAM);
		if (PARAM->_boolean.value == 0)
			THROW(E_ASSERT);
	}
	
	SUBR_LEAVE();
}

