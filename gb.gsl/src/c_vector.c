/***************************************************************************

	c_vector.c

	gb.gsl component

	(c) 2012 Benoît Minisini <gambas@users.sourceforge.net>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 1, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA 02110-1301, USA.

***************************************************************************/

#define __C_VECTOR_C

#include "c_complex.h"
#include "c_vector.h"

#define THIS ((CVECTOR *)_object)
#define VEC(_v) ((gsl_vector *)(_v)->vector)
#define CVEC(_v) ((gsl_vector_complex *)(_v)->vector)
#define SIZE(_v) ((int)(VEC(_v)->size))
#define COMPLEX(_v) ((_v)->complex)

//---- Utility functions ----------------------------------------------

/*int gsl_vector_has_zero(gsl_vector *a)
{
	int i;
	int size = (int)a->size;
	
	for (i = 0; i < size; i++)
	{
		if (gsl_vector_get(a, i) == 0.0)
			return TRUE;
	}
	
	return FALSE;
}

int gsl_vector_complex_has_zero(gsl_vector *a)
{
	int i;
	int size = (int)a->size;
	gsl_complex *p:
	
	for (i = 0; i < size; i++)
	{
		p = gsl_vector_complex_ptr((gsl_vector_complext *)a, i);
		if (p->dat[0] == 0.0 && p->dat[1] == 0.0)
			return TRUE;
	}
	
	return FALSE;
}

void gsl_vector_inverse(gsl_vector *a)
{
	int i;
	int size = (int)a->size;
	double *p;
	
	for (i = 0; i < size; i++)
	{
		p = gsl_vector_ptr(a, i);
		*p = 1.0 / *p;
	}
}

void gsl_vector_complex_inverse(gsl_vector *a)
{
	int i;
	int size = (int)a->size;
	gsl_complex *p;
	
	for (i = 0; i < size; i++)
	{
		p = gsl_vector_complex_ptr((gsl_vector_complex *)a, i);
		*p = gsl_complex_inverse(*p);
	}
}

void gsl_vector_negative(gsl_vector *a)
{
	int i;
	int size = (int)a->size;
	double *p;
	
	for (i = 0; i < size; i++)
	{
		p = gsl_vector_ptr(a, i);
		*p = (- *p);
	}
}

void gsl_vector_complex_negative(gsl_vector *a)
{
	int i;
	int size = (int)a->size;
	gsl_complex *p;
	
	for (i = 0; i < size; i++)
	{
		p = gsl_vector_complex_ptr((gsl_vector_complext *)a, i);
		*p = gsl_complex_negative(*p);
	}
}*/

//---- Vector creation ------------------------------------------------------

//static bool _do_not_init = FALSE;

static CVECTOR *VECTOR_create(int size, bool complex, bool init)
{
	GB.Push(2, GB_T_INTEGER, size, GB_T_BOOLEAN, complex);
	return (CVECTOR *)GB.New(CLASS_Vector, NULL, (void *)(intptr_t)2);
}

static CVECTOR *VECTOR_copy(CVECTOR *_object)
{
	CVECTOR *copy = VECTOR_create(SIZE(THIS), COMPLEX(THIS), FALSE);
	if (COMPLEX(THIS))
		gsl_vector_memcpy(VEC(copy), VEC(THIS));
	else
		gsl_vector_complex_memcpy(CVEC(copy), CVEC(THIS));
	
	return copy;
}

static CVECTOR *VECTOR_convert_to_complex(CVECTOR *_object)
{
	CVECTOR *v = VECTOR_create(SIZE(THIS), TRUE, FALSE);
	int i;
	
	for (i = 0; i < SIZE(THIS); i++)
		gsl_vector_complex_set((gsl_vector_complex *)v->vector, i, gsl_complex_rect(gsl_vector_get(VEC(THIS), i), 0));
	
	return v;
}

static void ensure_complex(CVECTOR *_object)
{
	gsl_vector_complex *v;
	int size = SIZE(THIS);
	int i;
	
	if (COMPLEX(THIS))
		return;
	
	v = gsl_vector_complex_alloc(size);
	for (i = 0; i < size; i++)
		gsl_vector_complex_set(v, i, gsl_complex_rect(gsl_vector_get(VEC(THIS), i), 0));
	
	gsl_vector_free(VEC(THIS));
	THIS->vector = v;
	THIS->complex = TRUE;
}


/*static bool ensure_not_complex(CVECTOR *_object)
{
	gsl_vector *v;
	int size = SIZE(THIS);
	int i;
	gsl_complex c;
	
	if (!COMPLEX(THIS))
		return FALSE;
	
	for (i = 0; i < size; i++)
	{
		c = gsl_vector_complex_get(CVEC(THIS), i);
		if (GSL_IMAG(c) != 0.0)
			return TRUE;
	}
	
	v = gsl_vector_alloc(size);
	
	for (i = 0; i < size; i++)
		gsl_vector_set(v, i, GSL_REAL(gsl_vector_complex_get(CVEC(THIS), i)));
	
	gsl_vector_complex_free(CVEC(THIS));
	THIS->vector = v;
	THIS->complex = FALSE;
	return FALSE;
}*/

//---- Conversions ----------------------------------------------------------

static char *_to_string(CVECTOR *_object, bool local)
{
	char *result = NULL;
	int i;
	int size = SIZE(THIS);
	char *str;
	int len;
	
	result = GB.AddChar(result, '[');
	
	for (i = 0; i < size; i++)
	{
		if (i)
			result = GB.AddChar(result, ' ');
		
		if (!COMPLEX(THIS))
		{
			GB.NumberToString(local, gsl_vector_get(VEC(THIS), i), NULL, &str, &len);
			result = GB.AddString(result, str, len);
		}
		else
		{
			str = COMPLEX_to_string(gsl_vector_complex_get(CVEC(THIS), i), local);
			result = GB.AddString(result, str, GB.StringLength(str));
			GB.FreeString(&str);
		}
	}
	
	result = GB.AddChar(result, ']');
	
	return result;
}

static bool _convert(CVECTOR *_object, GB_TYPE type, GB_VALUE *conv)
{
	if (THIS)
	{
		if (!COMPLEX(THIS))
		{
			switch (type)
			{
				case GB_T_FLOAT:
					conv->_float.value = gsl_blas_dnrm2(VEC(THIS));
					return FALSE;
					
				case GB_T_SINGLE:
					conv->_single.value = gsl_blas_dnrm2(VEC(THIS));
					return FALSE;
					
				case GB_T_INTEGER:
				case GB_T_SHORT:
				case GB_T_BYTE:
					conv->_integer.value = gsl_blas_dnrm2(VEC(THIS));
					return FALSE;
					
				case GB_T_LONG:
					conv->_long.value = gsl_blas_dnrm2(VEC(THIS));
					return FALSE;
					
				case GB_T_STRING:
				case GB_T_CSTRING:
					conv->_string.value.addr = _to_string(THIS, type == GB_T_CSTRING);
					conv->_string.value.start = 0;
					conv->_string.value.len = GB.StringLength(conv->_string.value.addr);
					return FALSE;
					
				default:
					return TRUE;
			}
		}
		else
		{
			switch (type)
			{
				case GB_T_FLOAT:
					conv->_float.value = gsl_blas_dznrm2(CVEC(THIS));
					return FALSE;
					
				case GB_T_SINGLE:
					conv->_single.value = gsl_blas_dznrm2(CVEC(THIS));
					return FALSE;
					
				case GB_T_INTEGER:
				case GB_T_SHORT:
				case GB_T_BYTE:
					conv->_integer.value = gsl_blas_dznrm2(CVEC(THIS));
					return FALSE;
					
				case GB_T_LONG:
					conv->_long.value = gsl_blas_dznrm2(CVEC(THIS));
					return FALSE;
					
				case GB_T_STRING:
				case GB_T_CSTRING:
					conv->_string.value.addr = _to_string(THIS, type == GB_T_CSTRING);
					conv->_string.value.start = 0;
					conv->_string.value.len = GB.StringLength(conv->_string.value.addr);
					return FALSE;
					
				default:
					return TRUE;
			}
		}
	}
	else if (type >= GB_T_OBJECT)
	{
		if (GB.Is(conv->_object.value, GB.FindClass("Array")))
		{
			GB_ARRAY array = (GB_ARRAY)conv->_object.value;
			int size = GB.Array.Count(array);
			CVECTOR *v;
			int i;
			GB_VALUE temp;
			void *data;
			GB_TYPE atype = GB.Array.Type(array);
			
			if (atype > GB_T_BOOLEAN && atype <= GB_T_FLOAT)
			{
				v = VECTOR_create(size, FALSE, FALSE);
				
				for (i = 0; i < size; i++)
				{
					data = GB.Array.Get(array, i);
					GB.ReadValue(&temp, data, atype);
					GB.Conv(&temp, GB_T_FLOAT);
					gsl_vector_set(VEC(v), i, temp._float.value);
				}
				
				conv->_object.value = v;
				return FALSE;
			}
			else if (atype == GB_T_VARIANT)
			{
				CCOMPLEX *c;
				v = VECTOR_create(size, TRUE, FALSE);
				
				for (i = 0; i < size; i++)
				{
					GB.ReadValue(&temp, GB.Array.Get(array, i), atype);
					GB.BorrowValue(&temp);
					GB.Conv(&temp, CLASS_Complex);
					c = temp._object.value;
					if (c)
						gsl_vector_complex_set(CVEC(v), i, c->number);
					else
						gsl_vector_complex_set(CVEC(v), i, COMPLEX_zero);
					GB.ReleaseValue(&temp);
				}
				
				conv->_object.value = v;
				return FALSE;
			}
			else if (atype == CLASS_Complex)
			{
				CCOMPLEX *c;
				v = VECTOR_create(size, TRUE, FALSE);
				
				for (i = 0; i < size; i++)
				{
					c = *((CCOMPLEX **)GB.Array.Get(array, i));
					if (c)
						gsl_vector_complex_set((gsl_vector_complex *)v->vector, i, c->number);
					else
						gsl_vector_complex_set((gsl_vector_complex *)v->vector, i, gsl_complex_rect(0, 0));
				}
				
				conv->_object.value = v;
				return FALSE;
			}
		}
		else if (type == CLASS_Complex)
		{
			CCOMPLEX *c = (CCOMPLEX *)conv->_object.value;
			CVECTOR *v = VECTOR_create(1, TRUE, FALSE);
			gsl_vector_complex_set(CVEC(v), 0, c->number);
			conv->_object.value = v;
			return FALSE;
		}
	}
	
	return TRUE;
}

//---------------------------------------------------------------------------

BEGIN_METHOD(Vector_new, GB_INTEGER size; GB_BOOLEAN complex)

	bool complex = VARGOPT(complex, FALSE);
	int size = VARGOPT(size, 1);
	
	if (size < 1) size = 1;
	
	THIS->complex = complex;
	
	if (!complex)
		THIS->vector = gsl_vector_calloc(size);
	else
		THIS->vector = gsl_vector_complex_calloc(size);
	
END_METHOD


BEGIN_METHOD_VOID(Vector_free)

	if (!COMPLEX(THIS))
		gsl_vector_free(VEC(THIS));
	else
		gsl_vector_complex_free(CVEC(THIS));

END_METHOD


BEGIN_PROPERTY(Vector_Count)

	GB.ReturnInteger(SIZE(THIS));

END_PROPERTY


BEGIN_METHOD_VOID(Vector_Copy)

	GB.ReturnObject(VECTOR_copy(THIS));

END_METHOD


BEGIN_METHOD(Vector_get, GB_INTEGER index)

	int size = SIZE(THIS);
	int index = VARG(index);
	
	if (index < 0 || index >= size)
	{
		GB.Error(GB_ERR_ARG);
		return;
	}
	
	if (!COMPLEX(THIS))
		GB.ReturnFloat(gsl_vector_get(VEC(THIS), index));
	else
		GB.ReturnObject(COMPLEX_create(gsl_vector_complex_get(CVEC(THIS), index)));

	GB.ReturnConvVariant();

END_METHOD


BEGIN_METHOD(Vector_put, GB_VARIANT value; GB_INTEGER index)

	int index = VARG(index);
	int size = SIZE(THIS);
	GB_VALUE *value = (GB_VALUE *)ARG(value);
	int type;
	gsl_complex z;
	double x;
	
	if (index < 0 || index > size)
	{
		GB.Error(GB_ERR_ARG);
		return;
	}
	
	type = COMPLEX_get_value(value, &x, &z);
	
	if (type == CGV_ERR)
		return;

	if (type == CGV_COMPLEX)
	{
		ensure_complex(THIS);
		gsl_vector_complex_set(CVEC(THIS), index, z);
	}
	else
	{
		if (COMPLEX(THIS))
			gsl_vector_complex_set(CVEC(THIS), index, z);
		else
			gsl_vector_set(VEC(THIS), index, x);
	}
	
END_METHOD


BEGIN_METHOD(Vector_Scale, GB_VALUE value)

	GB_VALUE *value = (GB_VALUE *)ARG(value);
	int type;
	gsl_complex z;
	double x;
	
	type = COMPLEX_get_value(value, &x, &z);
	
	if (type == CGV_ERR)
		return;

	if (type == CGV_COMPLEX)
	{
		ensure_complex(THIS);
		gsl_vector_complex_scale(CVEC(THIS), z);
	}
	else
	{
		if (COMPLEX(THIS))
			gsl_vector_complex_scale(CVEC(THIS), z);
		else
			gsl_vector_scale(VEC(THIS), x);
	}
	
	GB.ReturnObject(THIS);
		
END_METHOD

static void do_dot(CVECTOR *_object, CVECTOR *v, bool conj)
{
	bool ca, cb;
	
	if (GB.CheckObject(v))
		return;
	
	ca = !COMPLEX(THIS);
	cb = !COMPLEX(v);
	
	if (ca && cb)
	{
		double result;
		gsl_blas_ddot(VEC(THIS), v->vector, &result);
		GB.ReturnFloat(result);
	}
	else
	{
		CVECTOR *a, *b;
		gsl_complex result;
		
		if (ca)
			a = VECTOR_convert_to_complex(THIS);
		else
			a = THIS;
		
		if (cb)
			b = VECTOR_convert_to_complex(v);
		else
			b = v;
		
		if (conj)
			gsl_blas_zdotc(CVEC(a), CVEC(b), &result);
		else
			gsl_blas_zdotu(CVEC(a), CVEC(b), &result);
		
		GB.ReturnObject(COMPLEX_create(result));
		
		if (ca) GB.Unref(POINTER(&a));
		if (cb) GB.Unref(POINTER(&b));
	}

	GB.ReturnConvVariant();
}

BEGIN_METHOD(Vector_Dot, GB_OBJECT vector)

	do_dot(THIS, VARG(vector), FALSE);

END_METHOD

BEGIN_METHOD(Vector_ConjDot, GB_OBJECT vector)

	do_dot(THIS, VARG(vector), TRUE);
	
END_METHOD

BEGIN_METHOD_VOID(Vector_Norm)

	if (!COMPLEX(THIS))
		GB.ReturnFloat(gsl_blas_dnrm2(VEC(THIS)));
	else
		GB.ReturnFloat(gsl_blas_dznrm2(CVEC(THIS)));

END_METHOD

BEGIN_METHOD(Vector_Equal, GB_OBJECT vector)

	CVECTOR *v = VARG(vector);
	bool ca, cb;
	
	if (GB.CheckObject(v))
		return;
	
	if (SIZE(THIS) != SIZE(v))
	{
		GB.ReturnBoolean(FALSE);
		return;
	}
	
	ca = !COMPLEX(THIS);
	cb = !COMPLEX(v);
	
	if (ca && cb)
	{
		GB.ReturnBoolean(gsl_vector_equal(VEC(THIS), VEC(v)));
	}
	else
	{
		CVECTOR *a, *b;
		
		if (ca)
			a = VECTOR_convert_to_complex(THIS);
		else
			a = THIS;
		
		if (cb)
			b = VECTOR_convert_to_complex(v);
		else
			b = v;
		
		GB.ReturnBoolean(gsl_vector_complex_equal(CVEC(a), CVEC(b)));
		
		if (ca) GB.Unref(POINTER(&a));
		if (cb) GB.Unref(POINTER(&b));
	}


END_METHOD


BEGIN_PROPERTY(Vector_Handle)

	GB.ReturnPointer(THIS->vector);
	
END_PROPERTY


GB_DESC VectorDesc[] =
{
	GB_DECLARE("Vector", sizeof(CVECTOR)),
	
	GB_METHOD("_new", NULL, Vector_new, "[(Size)i(Complex)b]"),
	GB_METHOD("_free", NULL, Vector_free, NULL),
	//GB_STATIC_METHOD("_call", "Vector", Vector_call, "(Value)f."),
	GB_METHOD("Copy", "Vector", Vector_Copy, NULL),
	
	GB_PROPERTY_READ("Count", "i", Vector_Count),
	GB_PROPERTY_READ("Handle", "p", Vector_Handle),
	
	GB_METHOD("_get", "v", Vector_get, "(Index)i"),
	GB_METHOD("_put", NULL, Vector_put, "(Value)v(Index)i"),

	GB_METHOD("Scale", "Vector", Vector_Scale, "(Value)v"),
	GB_METHOD("Dot", "v", Vector_Dot, "(Vector)Vector"),
	GB_METHOD("ConjDot", "v", Vector_ConjDot, "(Vector)Vector"),
	GB_METHOD("Norm", "f", Vector_Norm, NULL),
	GB_METHOD("Equal", "b", Vector_Equal, "(Vector)Vector"),
	
	GB_INTERFACE("_convert", &_convert),
	
	GB_END_DECLARE
};
