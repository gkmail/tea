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

#ifndef T_ARRAY_MALLOC
	#ifdef T_ARRAY_REALLOC
		#define T_ARRAY_MALLOC(size) T_ARRAY_REALLOC(NULL, size) 
	#else
		#define T_ARRAY_MALLOC(size) malloc(size)
	#endif
#endif

#ifndef T_ARRAY_FREE
	#ifdef T_ARRAY_REALLOC
		#define T_ARRAY_FREE(ptr) T_ARRAY_REALLOC(ptr, 0) 
	#else
		#define T_ARRAY_FREE(ptr) free(ptr)
	#endif
#endif

#ifndef T_ARRAY_REALLOC
	#define T_ARRAY_REALLOC(ptr, size) realloc(ptr, size)
#endif

#ifndef T_ARRAY_EQUAL
	#ifdef T_ARRAY_CMP
		#define T_ARRAY_EQUAL(p1, p2) (T_ARRAY_CMP(p1, p2) == 0)
	#endif
#endif

#ifdef T_ARRAY_ELEM_INIT
static T_INLINE T_Result
T_ARRAY_FUNC(array_element_init)(T_ARRAY_TYPE *array, T_ID from, int cnt)
{
	T_ARRAY_ELEM_TYPE *elem, *eend;
	T_Result r;

	elem = array->T_ARRAY_NAME.buff + from;
	eend = elem + cnt;
	while(elem < eend){
		if((r = T_ARRAY_ELEM_INIT(elem)) < 0)
			return r;
		elem++;
	}

	return T_OK;
}
#else
static T_INLINE T_Result
T_ARRAY_FUNC(array_element_init)(T_ARRAY_TYPE *array, T_ID from, int cnt)
{
	return T_OK;
}
#endif

#ifdef T_ARRAY_ELEM_DEINIT
static T_INLINE void
T_ARRAY_FUNC(array_element_destroy)(T_ARRAY_TYPE *array)
{
	T_ARRAY_ELEM_TYPE *elem, *eend;

	elem = array->T_ARRAY_NAME.buff;
	eend = elem + array->T_ARRAY_NAME.nmem;

	while(elem < eend){
		T_ARRAY_ELEM_DEINIT(elem);
		elem++;
	}
}
#else
static T_INLINE void
T_ARRAY_FUNC(array_element_destroy)(T_ARRAY_TYPE *array)
{
}
#endif

static T_INLINE T_Result
T_ARRAY_FUNC(array_init)(T_ARRAY_TYPE *array)
{
	T_ASSERT(array);

	array->T_ARRAY_NAME.size = 0;
	array->T_ARRAY_NAME.nmem = 0;
	array->T_ARRAY_NAME.buff = NULL;

	return T_OK;
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

	T_ASSERT(array && data);

	if(array->T_ARRAY_NAME.nmem >= array->T_ARRAY_NAME.size){
		T_ARRAY_ELEM_TYPE *buf;
		int size = T_MAX(array->T_ARRAY_NAME.size * 2, 8);

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

static void
T_ARRAY_FUNC(array_clear)(T_ARRAY_TYPE *array)
{
	T_ASSERT(array);
	T_ARRAY_FUNC(array_deinit)(array);
	T_ARRAY_FUNC(array_init)(array);
}

static T_Result
T_ARRAY_FUNC(array_dup)(T_ARRAY_TYPE *dst, T_ARRAY_TYPE *src)
{
	T_ASSERT(dst && src);

	dst->T_ARRAY_NAME.nmem = src->T_ARRAY_NAME.nmem;
	dst->T_ARRAY_NAME.size = src->T_ARRAY_NAME.nmem;
	dst->T_ARRAY_NAME.buff = (T_ARRAY_ELEM_TYPE*)T_ARRAY_MALLOC(dst->T_ARRAY_NAME.size * sizeof(T_ARRAY_ELEM_TYPE));
	if(!dst->T_ARRAY_NAME.buff)
		return T_ERR_NOMEM;

	memcpy(dst->T_ARRAY_NAME.buff, src->T_ARRAY_NAME.buff, dst->T_ARRAY_NAME.nmem * sizeof(T_ARRAY_ELEM_TYPE));
	return T_OK;
}

static T_INLINE T_ARRAY_ELEM_TYPE*
T_ARRAY_FUNC(array_element)(T_ARRAY_TYPE *array, T_ID index)
{
	T_ASSERT(array && (index >= 0) && (index < array->T_ARRAY_NAME.nmem));

	return &array->T_ARRAY_NAME.buff[index];
}

static T_INLINE T_ARRAY_ELEM_TYPE*
T_ARRAY_FUNC(array_element_resize)(T_ARRAY_TYPE *array, T_ID index)
{
	T_ASSERT(array && (index >= 0));

	if(index >= array->T_ARRAY_NAME.nmem){
		int add;

		if(index >= array->T_ARRAY_NAME.size){
			T_ARRAY_ELEM_TYPE *buf;
			int size = T_MAX(array->T_ARRAY_NAME.size * 2, index + 1);

			size = T_MAX(size, 16);

			buf = (T_ARRAY_ELEM_TYPE*)T_ARRAY_REALLOC(array->T_ARRAY_NAME.buff, size * sizeof(T_ARRAY_ELEM_TYPE));
			if(!buf)
				return NULL;

			array->T_ARRAY_NAME.buff = buf;
			array->T_ARRAY_NAME.size = size;
		}

		add = index + 1 - array->T_ARRAY_NAME.nmem;
		if(T_ARRAY_FUNC(array_element_init)(array, array->T_ARRAY_NAME.nmem, add) < 0)
			return NULL;

		array->T_ARRAY_NAME.nmem = index + 1;
	}

	return &array->T_ARRAY_NAME.buff[index];
}

static T_Result
T_ARRAY_FUNC(array_insert)(T_ARRAY_TYPE *array, T_ID pos, T_ARRAY_ELEM_TYPE *v)
{
	T_ARRAY_ELEM_TYPE *elem;
	int len, left;

	T_ASSERT(array && v && (pos >= 0));

	len = T_ARRAY_FUNC(array_length)(array);

	if(pos < len){
		elem = T_ARRAY_FUNC(array_element_resize)(array, len);
		if(!elem)
			return T_ERR_NOMEM;

		left = len - pos;
		if(left > 0)
			memmove(array->T_ARRAY_NAME.buff + pos + 1, array->T_ARRAY_NAME.buff + pos, left * sizeof(T_ARRAY_ELEM_TYPE));

		array->T_ARRAY_NAME.buff[pos] = *v;
	}else{
		elem = T_ARRAY_FUNC(array_element_resize)(array, pos);
		if(!elem)
			return T_ERR_NOMEM;

		*elem = *v;
	}

	return T_OK;
}

#ifdef T_ARRAY_EQUAL
static T_ID
T_ARRAY_FUNC(array_find)(T_ARRAY_TYPE *array, T_ARRAY_ELEM_TYPE *v)
{
	T_ID id;

	T_ASSERT(array && v);

	for(id = 0; id < array->T_ARRAY_NAME.nmem; id++){
		if(T_ARRAY_EQUAL(v, &array->T_ARRAY_NAME.buff[id]))
			return id;
	}

	return -1;
}

static T_Result
T_ARRAY_FUNC(array_add_unique)(T_ARRAY_TYPE *array, T_ARRAY_ELEM_TYPE *v, T_ID *rid)
{
	T_Result r;
	T_ID id;

	T_ASSERT(array && v);

	if ((id = T_ARRAY_FUNC(array_find)(array, v)) < 0){
		if((id = T_ARRAY_FUNC(array_add)(array, v)) < 0)
			return id;

		r = 1;
	}else{
		r = 0;
	}

	if(rid)
		*rid = id;
	return r;
}

#endif /*T_ARRAY_EQUAL*/

#ifdef T_ARRAY_CMP
static T_ID
T_ARRAY_FUNC(array_search)(T_ARRAY_TYPE *array, T_ARRAY_ELEM_TYPE *v)
{
	T_ID id, min_id;
	int cv, min_cv;

	T_ASSERT(array && v);

	min_id = -1;

	for(id = 0; id < array->T_ARRAY_NAME.nmem; id++){
		cv = T_ARRAY_CMP(v, &array->T_ARRAY_NAME.buff[id]);
		if(cv == 0)
			return id;
		if((cv > 0) && ((min_id == -1) || (cv < min_cv))){
			min_id = id;
			min_cv = cv;
		}
	}

	return min_id;
}
#endif /*T_ARRAY_CMP*/

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
#undef T_ARRAY_MALLOC
#undef T_ARRAY_FREE
#undef T_ARRAY_ELEM_INIT
#undef T_ARRAY_ELEM_DEINIT
#ifdef T_ARRAY_CMP
	#undef T_ARRAY_CMP
#endif
#ifdef T_ARRAY_EQUAL
	#undef T_ARRAY_EQUAL
#endif
