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

#if !defined(T_ALLOC_TYPE)
	#error T_ALLOC_TYPE has not been defined
#endif

#ifndef T_ALLOC_ARGS_DECL
	#define T_ALLOC_ARGS_DECL    void
#endif

#ifndef T_ALLOC_ARGS
	#define T_ALLOC_INIT_ARGS
#else
	#define T_ALLOC_INIT_ARGS , T_ALLOC_ARGS
#endif

#ifndef T_ALLOC_FUNC
	#define T_ALLOC_FUNC(name) name
#endif

#ifndef T_ALLOC_MALLOC
	#ifdef T_ALLOC_REALLOC
		#define T_ALLOC_MALLOC(size) T_ALLOC_REALLOC(NULL, size)
	#else
		#define T_ALLOC_MALLOC(size) malloc(size)
	#endif
#endif

#ifndef T_ALLOC_FREE
	#ifdef T_ALLOC_REALLOC
		#define T_ALLOC_FREE(ptr)    T_ALLOC_REALLOC(ptr, 0)
	#else
		#define T_ALLOC_FREE(ptr)    free(ptr)
	#endif
#endif

#ifndef T_ALLOC_INIT
	#define T_ALLOC_INIT(p...)   T_OK
#endif

#ifndef T_ALLOC_DEINIT
	#define T_ALLOC_DEINIT(p)
#endif

T_ALLOC_TYPE*
T_ALLOC_FUNC(create)(T_ALLOC_ARGS_DECL)
{
	T_ALLOC_TYPE *type;

	type = (T_ALLOC_TYPE*)T_ALLOC_MALLOC(sizeof(T_ALLOC_TYPE));
	if(!type)
		return NULL;

	if(T_ALLOC_INIT(type T_ALLOC_INIT_ARGS) < 0){
		T_ALLOC_FREE(type);
		return NULL;
	}

	return type;
}

void
T_ALLOC_FUNC(destroy)(T_ALLOC_TYPE *type)
{
	T_ASSERT(type);

	T_ALLOC_DEINIT(type);
	T_ALLOC_FREE(type);
}

#undef T_ALLOC_TYPE
#undef T_ALLOC_ARGS_DECL
#ifdef T_ALLOC_ARGS
	#undef T_ALLOC_ARGS
#endif
#undef T_ALLOC_INIT_ARGS
#undef T_ALLOC_FUNC
#undef T_ALLOC_MALLOC
#undef T_ALLOC_FREE
#ifdef T_ALLOC_REALLOC
	#undef T_ALLOC_REALLOC
#endif
#undef T_ALLOC_INIT
#undef T_ALLOC_DEINIT
