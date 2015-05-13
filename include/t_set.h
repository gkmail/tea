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

#if !defined(T_SET_TYPE) || !defined(T_SET_ELEM_TYPE) || !defined(T_SET_NAME)
	#error T_SET_TYPE, T_SET_ELEM_TYPE or T_SET_NAME have not been defined
#endif

#ifndef T_SET_CMP
#endif

#define T_ARRAY_TYPE       T_SET_TYPE
#define T_ARRAY_ELEM_TYPE  T_SET_ELEM_TYPE
#define T_ARRAY_NAME       T_SET_NAME

#ifdef T_SET_FUNC
	#define T_ARRAY_FUNC   T_SET_FUNC
#endif

#ifdef T_SET_REALLOC
	#define T_ARRAY_REALLOC T_SET_REALLOC
#endif

#ifdef T_SET_MALLOC
	#define T_ARRAY_MALLOC T_SET_MALLOC
#elif defined(T_SET_REALLOC)
	#define T_ARRAY_MALLOC(size) T_SET_REALLOC(NULL, size)
#endif

#ifdef T_SET_FREE
	#define T_ARRAY_FREE T_SET_FREE
#elif defined(T_SET_REALLOC)
	#define T_ARRAY_FREE(ptr) T_SET_REALLOC(ptr, 0)
#endif

#ifdef T_SET_ELEM_DEINIT
	#define T_ARRAY_ELEM_DEINIT T_SET_ELEM_DEINIT
#endif

#include "t_array.h"

#define T_SORT_TYPE T_SET_ELEM_TYPE

#ifdef T_SET_FUNC
	#define T_SORT_FUNC    T_SET_FUNC
#endif

#ifdef T_SET_CMP
	#define T_SORT_CMP     T_SET_CMP
#endif

#include "t_sort.h"

#define T_BSEARCH_TYPE T_SET_ELEM_TYPE

#ifdef T_SET_FUNC
	#define T_BSEARCH_FUNC T_SET_FUNC
#endif

#ifdef T_SET_CMP
	#define T_BSEARCH_CMP  T_SET_CMP
#endif

#include "t_bsearch.h"

static T_INLINE T_Result
T_SET_FUNC(set_init)(T_SET_TYPE *set)
{
	return T_SET_FUNC(array_init)(set);
}

static T_INLINE void
T_SET_FUNC(set_deinit)(T_SET_TYPE *set)
{
	T_SET_FUNC(array_deinit)(set);
}

static T_INLINE void
T_SET_FUNC(set_reinit)(T_SET_TYPE *set)
{
	T_SET_FUNC(array_reinit)(set);
}

static T_INLINE T_Result
T_SET_FUNC(set_dup)(T_SET_TYPE *dst, T_SET_TYPE *src)
{
	return T_SET_FUNC(array_dup)(dst, src);
}

static T_INLINE void
T_SET_FUNC(set_clear)(T_SET_TYPE *set)
{
	T_SET_FUNC(array_clear)(set);
}

static T_INLINE int
T_SET_FUNC(set_length)(T_SET_TYPE *set)
{
	return T_SET_FUNC(array_length)(set);
}

static T_INLINE T_SET_ELEM_TYPE*
T_SET_FUNC(set_element)(T_SET_TYPE *set, T_ID id)
{
	return T_SET_FUNC(array_element)(set, id);
}

static T_INLINE T_ID
T_SET_FUNC(set_add_not_check)(T_SET_TYPE *set, T_SET_ELEM_TYPE *elem)
{
	return T_SET_FUNC(array_add)(set, elem);
}

static T_INLINE void
T_SET_FUNC(set_sort)(T_SET_TYPE *set)
{
	return T_SET_FUNC(sort)(set->T_SET_NAME.buff, set->T_SET_NAME.nmem);
}

static T_ID
T_SET_FUNC(set_search)(T_SET_TYPE *set, T_SET_ELEM_TYPE *elem)
{
	T_ASSERT(set && elem);

	return T_SET_FUNC(bsearch)(set->T_SET_NAME.buff, set->T_SET_NAME.nmem, elem);
}

static T_Result
T_SET_FUNC(set_add)(T_SET_TYPE *set, T_SET_ELEM_TYPE *elem, T_ID *rid)
{
	T_ID id;

	T_ASSERT(set && elem);

	id = T_SET_FUNC(bsearch)(set->T_SET_NAME.buff, set->T_SET_NAME.nmem, elem);
	if(id >= 0){
		if(rid)
			*rid = id;
		return 0;
	}

	id = T_SET_FUNC(array_add)(set, elem);
	if(id < 0)
		return id;

	T_SET_FUNC(sort)(set->T_SET_NAME.buff, set->T_SET_NAME.nmem);
	if(rid)
		*rid = id;
	return 1;
}

static T_Result
T_SET_FUNC(set_union)(T_SET_TYPE *s1, T_SET_TYPE *s2)
{
	T_SET_ELEM_TYPE *e, *eend;
	T_ID id;
	int cnt;

	T_ASSERT(s1 && s2);

	e    = s2->T_SET_NAME.buff;
	eend = e + s2->T_SET_NAME.nmem;
	cnt  = s1->T_SET_NAME.nmem;

	while(e < eend){
		id = T_SET_FUNC(bsearch)(s1->T_SET_NAME.buff, cnt, e);
		if(id < 0){
			id = T_SET_FUNC(array_add)(s1, e);
			if(id < 0)
				return id;
		}
		e++;
	}

	T_SET_FUNC(sort)(s1->T_SET_NAME.buff, s1->T_SET_NAME.nmem);
	return T_OK;
}

static T_Result
T_SET_FUNC(set_diff)(T_SET_TYPE *s1, T_SET_TYPE *s2)
{
	T_SET_ELEM_TYPE *e, *eend;
	T_SET_TYPE ns;
	T_ID id;
	int cnt;

	T_ASSERT(s1 && s2);

	T_SET_FUNC(set_init)(&ns);

	e    = s1->T_SET_NAME.buff;
	eend = e + s1->T_SET_NAME.nmem;
	cnt  = s2->T_SET_NAME.nmem;

	while(e < eend){
		id = T_SET_FUNC(bsearch)(s2->T_SET_NAME.buff, cnt, e);
		if(id < 0){
			id = T_SET_FUNC(array_add)(&ns, e);
			if(id < 0){
				T_SET_FUNC(set_deinit)(&ns);
				return id;
			}
		}
		e++;
	}

	T_SET_FUNC(set_deinit)(s1);

	s1->T_SET_NAME.buff = ns.T_SET_NAME.buff;
	s1->T_SET_NAME.nmem = ns.T_SET_NAME.nmem;
	s1->T_SET_NAME.size = ns.T_SET_NAME.size;

	return T_OK;
}

static T_INLINE void
T_SET_FUNC(set_iter_first)(T_SET_TYPE *set, T_SetIter *iter)
{
	T_SET_FUNC(array_iter_first)(set, iter);
}

static T_INLINE T_SET_ELEM_TYPE*
T_SET_FUNC(set_iter_data)(T_SetIter *iter)
{
	return T_SET_FUNC(array_iter_data)(iter);
}

static T_INLINE void
T_SET_FUNC(set_iter_next)(T_SetIter *iter)
{
	T_SET_FUNC(array_iter_next)(iter);
}

static T_INLINE T_Bool
T_SET_FUNC(set_iter_last)(T_SetIter *iter)
{
	return T_SET_FUNC(array_iter_last)(iter);
}

#undef T_SET_TYPE
#undef T_SET_ELEM_TYPE
#undef T_SET_NAME
#undef T_SET_FUNC
#ifdef T_SET_REALLOC
	#undef T_SET_REALLOC
#endif
#ifdef T_SET_MALLOC
	#undef T_SET_MALLOC
#endif
#ifdef T_SET_FREE
	#undef T_SET_FREE
#endif
#ifdef T_SET_ELEM_DEINIT
	#undef T_SET_ELEM_DEINIT
#endif

