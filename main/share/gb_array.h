/***************************************************************************

  gb_array.h

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

#ifndef __GB_ARRAY_H
#define __GB_ARRAY_H

typedef
  struct {
    uint count;
    uint max;
    uint size;
    uint inc;
    }
  ARRAY;

typedef
  int (*ARRAY_COMP_FUNC)(const void *, const void *);

#define DATA_TO_ARRAY(_data) ((ARRAY *)(_data) - 1)
#define ARRAY_TO_DATA(_array) ((char *)((ARRAY *)(_array) + 1))

#define ARRAY_create(_data) ARRAY_create_with_size((_data), sizeof(**(_data)), 32)
#define ARRAY_create_inc(_data, _inc) ARRAY_create_with_size((_data), sizeof(**(_data)), (_inc))

void ARRAY_create_with_size(void *p_data, size_t size, uint inc);
void ARRAY_delete(void *p_data);

#define ARRAY_set_inc(_data, _inc) (DATA_TO_ARRAY(_data)->inc = _inc)

#define ARRAY_size(_data) (DATA_TO_ARRAY(_data)->size)
#define ARRAY_count(_data) ((_data) ? DATA_TO_ARRAY(_data)->count : 0)

void ARRAY_realloc(void *p_data);

void *ARRAY_add_data(void *p_data, uint num, bool zero);
void *ARRAY_add_data_one(void *p_data, bool zero);

#define ARRAY_add_one(_pdata, _zero) \
({ \
	ARRAY *__array = DATA_TO_ARRAY(*(_pdata)); \
	__typeof__(*(_pdata)) ptr; \
	int old_count = __array->count; \
	\
	__array->count++; \
	\
	if (__array->count <= __array->max) \
	{ \
		ptr = *(_pdata) + old_count; \
	} \
	else \
	{ \
		ARRAY_realloc(_pdata); \
		ptr = *(_pdata) + old_count; \
	} \
	if (_zero) memset(ptr, 0, sizeof(*ptr)); \
	ptr; \
})

#define ARRAY_add_data_one(_pdata, _zero) \
({ \
	ARRAY *__array = DATA_TO_ARRAY(*(_pdata)); \
	int size = __array->size; \
	char *ptr; \
	int old_count = __array->count; \
	\
	__array->count++; \
	\
	if (__array->count <= __array->max) \
	{ \
		ptr = (char *)*(_pdata) + size * old_count; \
	} \
	else \
	{ \
		ARRAY_realloc(_pdata); \
		ptr = (char *)*(_pdata) + size * old_count; \
	} \
	if (_zero) memset(ptr, 0, size); \
	(void *)ptr; \
})

#define ARRAY_add(_pdata) ARRAY_add_one(_pdata, FALSE)
#define ARRAY_add_void(_pdata) ARRAY_add_one(_pdata, TRUE)
#define ARRAY_add_size(_pdata) ARRAY_add_data_one(_pdata, FALSE)
#define ARRAY_add_void_size(_pdata) ARRAY_add_data_one(_pdata, TRUE)

#define ARRAY_add_many(_pdata, _num) ARRAY_add_data(_pdata, _num, FALSE)
#define ARRAY_add_many_void(_pdata, _num) ARRAY_add_data(_pdata, _num, TRUE)

//PUBLIC void *ARRAY_get(void *data, int pos);
#define ARRAY_get(_data, _pos) ((char *)(_data) + DATA_TO_ARRAY(_data)->size * (_pos))

void *ARRAY_insert_many(void *p_data, int pos, uint count);
#define ARRAY_insert(_pdata, _pos) ARRAY_insert_many(_pdata, _pos, 1);
void ARRAY_remove_many(void *p_data, int pos, uint count);
#define ARRAY_remove(_pdata, _pos) ARRAY_remove_many(_pdata, _pos, 1);

void ARRAY_remove_last(void *p_data);

void ARRAY_qsort(void *data, ARRAY_COMP_FUNC cmp);

#endif
