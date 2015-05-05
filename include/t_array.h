/*****************************************************************************
 * TEA: a simple C tool library.
 *----------------------------------------------------------------------------
 * Copyright (C) 2015  L+#= +0=1 <gkmail@sina.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#if !defined(T_ARRAY_TYPE) || !defined(T_ARRAY_ELEM_TYPE) || !defined(T_ARRAY_NAME)
	#error T_ARRAY_TYPE, T_ARRAY_ELEM_TYPE or T_ARRAY_NAME have not been defined
#endif

#ifndef T_ARRAY_FUNC
	#define T_ARRAY_FUNC(name) name
#endif

#ifndef T_ARRAY_REALLOC
	#define T_ARRAY_REALLOC(ptr, size) realloc(ptr, size)
#endif

#ifndef T_ARRAY_FREE
	#define T_ARRAY_FREE(ptr) free(ptr)
#endif

#ifdef T_ARRAY_ELEM_DESTROY
static T_INLINE void
T_ARRAY_FUNC(array_element_destroy)(T_ARRAY_TYPE *array)
{
	T_ARRAY_ELEM_TYPE *elem, *eend;

	elem = array->T_ARRAY_NAME.buff;
	eend = elem + array->T_ARRAY_NAME.nmem;

	while(elem < eend){
		T_ARRAY_ELEM_DESTROY(elem);
		elem++;
	}
}
#else
static T_INLINE void
T_ARRAY_FUNC(array_element_destroy)(T_ARRAY_TYPE *array)
{
}
#endif

static T_INLINE void
T_ARRAY_FUNC(array_init)(T_ARRAY_TYPE *array)
{
	T_ASSERT(array);

	array->T_ARRAY_NAME.size = 0;
	array->T_ARRAY_NAME.nmem = 0;
	array->T_ARRAY_NAME.buff = NULL;
}

static T_INLINE int
T_ARRAY_FUNC(array_length)(T_ARRAY_TYPE *array)
{
	T_ASSERT(array);
	return array->T_ARRAY_NAME.nmem;
}

static T_INLINE T_ID
T_ARRAY_FUNC(array_add)(T_ARRAY_TYPE *array, T_ARRAY_ELEM_TYPE *data)
{
	T_ID id;

	T_ASSERT(array);

	if(array->T_ARRAY_NAME.nmem >= array->T_ARRAY_NAME.size){
		T_ARRAY_ELEM_TYPE *buf;
		T_ID size = T_MAX(array->T_ARRAY_NAME.size * 2, 8);

		buf = (T_ARRAY_ELEM_TYPE*)T_ARRAY_REALLOC(array->T_ARRAY_NAME.buff, size * sizeof(T_ARRAY_ELEM_TYPE));
		if(!buf)
			return T_ERR_NOMEM;

		array->T_ARRAY_NAME.buff = buf;
		array->T_ARRAY_NAME.size = size;
	}

	id = array->T_ARRAY_NAME.nmem;
	array->T_ARRAY_NAME.buff[array->T_ARRAY_NAME.nmem++] = *data;

	return id;
}

static void
T_ARRAY_FUNC(array_deinit)(T_ARRAY_TYPE *array)
{
	T_ASSERT(array);

	if(array->T_ARRAY_NAME.buff){
		T_ARRAY_FUNC(array_element_destroy)(array);
		T_ARRAY_FREE(array->T_ARRAY_NAME.buff);
	}
}

static void
T_ARRAY_FUNC(array_reinit)(T_ARRAY_TYPE *array)
{
	T_ASSERT(array);

	T_ARRAY_FUNC(array_element_destroy)(array);
	array->T_ARRAY_NAME.nmem = 0;
}

static T_INLINE T_ARRAY_ELEM_TYPE*
T_ARRAY_FUNC(array_element)(T_ARRAY_TYPE *array, T_ID index)
{
	T_ASSERT(array && (index >= 0) && (index < array->T_ARRAY_NAME.nmem));

	return &array->T_ARRAY_NAME.buff[index];
}

static T_INLINE void
T_ARRAY_FUNC(array_iter_first)(T_ARRAY_TYPE *array, T_ArrayIter *iter)
{
	iter->array = array;
	iter->index = 0;
}

static T_INLINE T_ARRAY_ELEM_TYPE*
T_ARRAY_FUNC(array_iter_data)(T_ArrayIter *iter)
{
	T_ARRAY_TYPE *array = iter->array;
	return &array->T_ARRAY_NAME.buff[iter->index];
}

static T_INLINE void
T_ARRAY_FUNC(array_iter_next)(T_ArrayIter *iter)
{
	iter->index++;
}

static T_INLINE T_Bool
T_ARRAY_FUNC(array_iter_last)(T_ArrayIter *iter)
{
	T_ARRAY_TYPE *array = iter->array;
	return iter->index >= array->T_ARRAY_NAME.nmem;
}

#undef T_ARRAY_TYPE
#undef T_ARRAY_ELEM_TYPE
#undef T_ARRAY_NAME
#undef T_ARRAY_FUNC
#undef T_ARRAY_REALLOC
#undef T_ARRAY_FREE
#undef T_ARRAY_ELEM_DESTROY

