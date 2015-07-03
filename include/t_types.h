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

#ifndef _T_TYPES_H_
#define _T_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char T_Bool;
#define T_TRUE  1
#define T_FALSE 0

typedef enum{
	T_OK = 0,
	T_ERR_NOMEM    = -1000,
	T_ERR_SYNTAX   = -1001,
	T_ERR_ARG      = -1002,
	T_ERR_NOTINIT  = -1003,
	T_ERR_REINIT   = -1004,
	T_ERR_IO       = -1005,
	T_ERR_UNDEF    = -1006,
	T_ERR_SYSTEM   = -1007,
	T_ERR_EMPTY    = -1008
}T_Result;

typedef int    T_ID;
typedef size_t T_Size;

typedef int8_t    T_Int8;
typedef int16_t   T_Int16;
typedef int32_t   T_Int32;
typedef int64_t   T_Int64;
typedef uint8_t   T_UInt8;
typedef uint16_t  T_UInt16;
typedef uint32_t  T_UInt32;
typedef uint64_t  T_UInt64;
typedef intptr_t  T_IntPtr;
typedef uintptr_t T_UIntPtr;

#define T_ARRAY_DECL(TYPE, NAME)\
	struct{\
		int   size;\
		int   nmem;\
		TYPE *buff;\
	}NAME

typedef struct{
	int   index;
	void *array;
}T_ArrayIter;

#define T_QUEUE_DECL(TYPE, NAME)\
	struct{\
		int   size;\
		int   nmem;\
		int   head;\
		TYPE *buff;\
	}NAME

#define T_SET_DECL(TYPE, NAME) T_ARRAY_DECL(TYPE, NAME)

typedef T_ArrayIter T_SetIter;

#define T_HASH_ENTRY_DECL(KTYPE, DTYPE, NAME)\
	typedef struct NAME NAME;\
	struct NAME{\
		NAME *next;\
		KTYPE key;\
		DTYPE data;\
	};

#define T_HASH_DECL(ENTRY, NAME)\
	struct{\
		int     nmem;\
		int     bucket;\
		ENTRY **entries;\
	}NAME

typedef struct{
	void *hash;
	void *entry;
	int   pos;
}T_HashIter;

#define T_SLIST_DECL(TYPE, NAME)\
	TYPE *NAME

#ifdef __cplusplus
}
#endif

#endif
