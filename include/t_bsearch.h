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

#if !defined(T_BSEARCH_TYPE)
	#error T_BSEARCH_TYPE has not been defined
#endif

#ifndef T_BSEARCH_FUNC
	#define T_BSEARCH_FUNC(name) name
#endif

#ifndef T_BSEARCH_CMP
	#define T_BSEARCH_CMP(p1, p2) ((int)(*(p1) - *(p2)))
#endif

static T_ID
T_BSEARCH_FUNC(bsearch)(T_BSEARCH_TYPE *aa, int n, T_BSEARCH_TYPE *key)
{
	int begin = 0;
	int end = n;
	int diff;
	int mid;
	int v;

	if(!aa || !n)
		return -1;

	T_ASSERT(aa && (n >= 0) && key);

	while((diff = end - begin) >= 2){
		mid = begin + diff / 2;

		v = T_BSEARCH_CMP(key, aa + mid);
		if(v == 0)
			return mid;

		if(v < 0)
			end = mid;
		else
			begin = mid;
	}

	if(T_BSEARCH_CMP(key, aa + begin) == 0)
		return begin;

	return -1;
}

#undef T_BSEARCH_TYPE
#undef T_BSEARCH_FUNC
#undef T_BSEARCH_CMP
