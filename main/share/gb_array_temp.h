/***************************************************************************

	gb_array_temp.h

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

#define __ARRAY_C

#include "gb_common.h"
#include "gb_error.h"
#include "gb_alloc.h"
#include "gb_array.h"

void ARRAY_create_with_size(void *p_data, size_t size, uint inc)
{
	ARRAY *array;

	ALLOC(&array, sizeof(ARRAY));

	array->count = 0;
	array->max = 0;
	array->size = (uint)size;
	if (size > 2 && (size & 3))
		fprintf(stderr, "WARNING: ARRAY_create_with_size: size = %zi\n", size);
	array->inc = inc;

	*((void **)p_data) = ARRAY_TO_DATA(array);
}


void ARRAY_delete(void *p_data)
{
	void **data = (void **)p_data;
	ARRAY *alloc = DATA_TO_ARRAY(*data);

	if (!*data)
		return;

	FREE(&alloc);

	*data = NULL;
}


static inline ARRAY *array_realloc(ARRAY *array)
{
	ARRAY *new_array;
	
	array->max = array->inc + ((array->count + array->inc) / array->inc) * array->inc;
	new_array = array;
	REALLOC(&new_array, sizeof(ARRAY) + array->max * array->size);
	return new_array;
}


void *ARRAY_add_data(void *p_data, uint num, bool zero)
{
	void **data = (void **)p_data;
	ARRAY *array = DATA_TO_ARRAY(*data);
	char *ptr;

	array->count += num;

	if (array->count > array->max)
	{
		/*array->max = array->inc + ((array->count + array->inc) / array->inc) * array->inc;
		new_array = array;
		REALLOC(&new_array, sizeof(ARRAY) + array->max * array->size);
		array = new_array;*/
		array = array_realloc(array);
		*data = ARRAY_TO_DATA(array);
	}

	ptr = (char *)array + sizeof(ARRAY) + array->size * (array->count - num);

	if (zero) memset(ptr, 0, array->size * num);

	return ptr;
}


void ARRAY_realloc(void *p_data)
{
	void **data = (void **)p_data;
	ARRAY *array = DATA_TO_ARRAY(*data);
	
	*data = ARRAY_TO_DATA(array_realloc(array));
}


/*void *ARRAY_add_data_one(void *p_data, bool zero)
{
	void **data = (void **)p_data;
	register ARRAY *array = DATA_TO_ARRAY(*data);
	int size = array->size;
	char *ptr;

	array->count++;

	if (array->count > array->max)
	{
		array = array_realloc(array);
		*data = ARRAY_TO_DATA(array);
	}

	ptr = (char *)array + sizeof(ARRAY) + size * (array->count - 1);

	if (zero) memset(ptr, 0, size);

	return ptr;
}*/


void ARRAY_remove_last(void *p_data)
{
	void **data = (void **)p_data;
	ARRAY *array = DATA_TO_ARRAY(*data);

	if (!array->count)
		return;

	array->count--;
}


void *ARRAY_insert_many(void *p_data, int pos, uint count)
{
	void **data;
	ARRAY *array;
	char *addr;
	int len;
	uint apos;

	data = (void **)p_data;
	array = DATA_TO_ARRAY(*data);

	if ((pos < 0) || (pos > array->count))
		apos = array->count;
	else
		apos = pos;

	ARRAY_add_many(p_data, count);
	array = DATA_TO_ARRAY(*data);

	addr = ((char *)(*data)) + array->size * apos;
	len = (array->count - apos - count) * array->size;

	if (len > 0)
		memmove(addr + array->size * count, addr, len);

	memset(addr, 0, array->size * count);

	return addr;
}


void ARRAY_remove_many(void *p_data, int pos, uint count)
{
	void **data = (void **)p_data;
	ARRAY *array = DATA_TO_ARRAY(*data);
	char *addr;
	int len;
	uint apos;

	if ((pos < 0) || (pos >= array->count))
		return;
	
	apos = pos;

	if (count < 0 || count > (array->count - apos))
		count = array->count - apos;

	addr = ((char *)(*data)) + array->size * apos;
	len = (array->count - apos - count) * array->size;

	if (len > 0)
		memmove(addr, addr + array->size * count, len);

	array->count -= count;

	if (array->max <= array->inc)
		return;

	if (array->count > (array->max / 2))
		return;

	array->max = ((array->count + array->inc) / array->inc) * array->inc;
	REALLOC(&array, sizeof(ARRAY) + array->max * array->size);
	*data = ARRAY_TO_DATA(array);
}


void ARRAY_qsort(void *data, ARRAY_COMP_FUNC cmp)
{
	ARRAY *array = DATA_TO_ARRAY(data);

	if (array->count)
		qsort(data, array->count, array->size, cmp);
}

