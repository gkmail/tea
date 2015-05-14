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

#ifndef _T_DEBUG_H_
#define _T_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef T_ENABLE_DEBUG
	#ifndef T_GLOBAL_DEBUG
		#undef T_ENABLE_DEBUG
	#endif
#endif

#ifdef T_ENABLE_DEBUG
#define T_DEBUG(t, a...)\
	T_MACRO_BEGIN\
		fprintf(stderr, "%s", t);\
		fprintf(stderr, "/\"%s\" %d: ", __FILE__, __LINE__);\
		fprintf(stderr, a);\
		fprintf(stderr, "\n");\
	T_MACRO_END
#else
#define T_DEBUG(t, a...)
#endif

#define T_DEBUG_I(a...) T_DEBUG("I", a)
#define T_DEBUG_E(a...) T_DEBUG("E", a)
#define T_DEBUG_W(a...) T_DEBUG("W", a)

#ifdef __cplusplus
}
#endif

#endif
