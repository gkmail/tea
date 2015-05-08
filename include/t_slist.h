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

#if !defined(T_SLIST_TYPE) || !defined(T_SLIST_ELEM_TYPE) || !defined(T_SLIST_NAME)
	#error T_SLIST_TYPE, T_SLIST_ELEM_TYPE or T_SLIST_NAME have not been defined
#endif

#ifndef T_SLIST_FUNC
	#define T_SLIST_FUNC(name) name
#endif

#ifndef T_SLIST_ELEM_DEINIT
	#define T_SLIST_ELEM_DEINIT(ptr)
#endif

static T_INLINE T_Result
T_SLIST_FUNC(slist_init)(T_SLIST_TYPE *slist)
{
	slist->T_SLIST_NAME = NULL;
	return T_OK;
}

static T_INLINE void
T_SLIST_FUNC(slist_deinit)(T_SLIST_TYPE *slist)
{
	T_SLIST_ELEM_TYPE *elem, *enext;

	T_ASSERT(slist);

	elem = slist->T_SLIST_NAME;
	while(elem){
		enext = elem->T_SLIST_NAME;
		T_SLIST_ELEM_DEINIT(elem);
		elem = enext;
	}
}

static T_INLINE void
T_SLIST_FUNC(slist_push)(T_SLIST_TYPE *slist, T_SLIST_ELEM_TYPE *elem)
{
	T_ASSERT(slist && elem);

	elem->T_SLIST_NAME = slist->T_SLIST_NAME;
	slist->T_SLIST_NAME = elem;
}

static T_INLINE T_SLIST_ELEM_TYPE*
T_SLIST_FUNC(slist_top)(T_SLIST_TYPE *slist)
{
	T_ASSERT(slist);
	return slist->T_SLIST_NAME;
}

static T_INLINE void
T_SLIST_FUNC(slist_pop)(T_SLIST_TYPE *slist)
{
	T_SLIST_ELEM_TYPE *elem;

	T_ASSERT(slist);

	elem = slist->T_SLIST_NAME;
	if(elem){
		slist->T_SLIST_NAME = elem->T_SLIST_NAME;
		T_SLIST_ELEM_DEINIT(elem);
	}
}

#undef T_SLIST_TYPE
#undef T_SLIST_ELEM_TYPE
#undef T_SLIST_ELEM_DEINIT
#undef T_SLIST_NAME
#undef T_SLIST_FUNC
