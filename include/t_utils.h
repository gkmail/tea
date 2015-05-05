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

#ifndef _T_UTILS_H_
#define _T_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define T_INLINE     inline
#define T_ASSERT(a)  assert(a)

#define T_MACRO_BEGIN do{
#define T_MACRO_END   }while(0)

#define T_MAX(a,b) ((a)>(b)?(a):(b))
#define T_MIN(a,b) ((a)<(b)?(a):(b))

#define T_ABS(a)   ((a)>=0?(a):-(a))

#define T_ALLOC(type)\
	((type*)malloc(sizeof(type)))
#define T_ALLOC_N(type, n)\
	((type*)malloc(sizeof(type) * n))

#define T_ALLOC0(type)\
	({\
		type *ptr;\
		ptr = T_ALLOC(type);\
		if(ptr)\
			memset(ptr, 0, sizeof(type));\
		ptr;\
	})
#define T_ALLOC0_N(type, n)\
	({\
		type *ptr;\
		ptr = T_ALLOC_N(type);\
		if(ptr)\
			memset(ptr, 0, sizeof(type) * n);\
		ptr;\
	})

#ifdef __cplusplus
}
#endif

#endif
