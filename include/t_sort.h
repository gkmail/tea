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

#if !defined(T_SORT_TYPE)
	#error T_SORT_TYPE has not been defined
#endif

#ifndef T_SORT_FUNC
	#define T_SORT_FUNC(name) name
#endif

#ifndef T_SORT_CMP
	#define T_SORT_CMP(p1, p2) ((int)(*(p1) - *(p2)))
#endif

static T_INLINE void
T_SORT_FUNC(swap)(T_SORT_TYPE *a, T_SORT_TYPE *b)
{
	T_SORT_TYPE t = *a;
	*a = *b;
	*b = t;
}

static T_INLINE void
T_SORT_FUNC(vecswap)(T_SORT_TYPE *a, T_SORT_TYPE *b, int n)
{
	while(n--){
		T_SORT_TYPE t = *a;
		*a++ = *b;
		*b++ = t;
	}
}

static T_INLINE T_SORT_TYPE*
T_SORT_FUNC(med3)(T_SORT_TYPE *a, T_SORT_TYPE *b, T_SORT_TYPE *c)
{
	return T_SORT_CMP(a, b) < 0 ?
	       (T_SORT_CMP(b, c) < 0 ? b : (T_SORT_CMP(a, c) < 0 ? c : a ))
              :(T_SORT_CMP(b, c) > 0 ? b : (T_SORT_CMP(a, c) < 0 ? a : c ));
}

static void
T_SORT_FUNC(sort)(T_SORT_TYPE *aa, int n)
{
	T_SORT_TYPE *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swap_cnt;
	T_SORT_TYPE *a = aa;

	if(!aa && !n)
		return;

	T_ASSERT(aa && (n >= 0));

loop:
	swap_cnt = 0;
	if (n < 7) {
		for (pm = a + 1; pm < a + n; pm ++)
			for (pl = pm; pl > a && T_SORT_CMP(pl - 1, pl) > 0;
			     pl --)
				T_SORT_FUNC(swap)(pl, pl - 1);
		return;
	}
	pm = a + (n / 2);
	if (n > 7) {
		pl = a;
		pn = a + n - 1;
		if (n > 40) {
			d = n / 8;
			pl = T_SORT_FUNC(med3)(pl, pl + d, pl + 2 * d);
			pm = T_SORT_FUNC(med3)(pm - d, pm, pm + d);
			pn = T_SORT_FUNC(med3)(pn - 2 * d, pn - d, pn);
		}
		pm = T_SORT_FUNC(med3)(pl, pm, pn);
	}
	T_SORT_FUNC(swap)(a, pm);

	pa = pb = a + 1;
	pc = pd = a + n - 1;
	for (;;) {
		while (pb <= pc && (r = T_SORT_CMP(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				T_SORT_FUNC(swap)(pa, pb);
				pa ++;
			}
			pb ++;
		}
		while (pb <= pc && (r = T_SORT_CMP(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				T_SORT_FUNC(swap)(pc, pd);
				pd --;
			}
			pc --;
		}
		if (pb > pc)
			break;
		T_SORT_FUNC(swap)(pb, pc);
		swap_cnt = 1;
		pb ++;
		pc --;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = a + 1; pm < a + n; pm++)
			for (pl = pm; pl > a && T_SORT_CMP(pl - 1, pl) > 0;
			     pl --)
				T_SORT_FUNC(swap)(pl, pl - 1);
		return;
	}

	pn = a + n;
	r = T_MIN(pa - a, pb - pa);
	T_SORT_FUNC(vecswap)(a, pb - r, r);
	r = T_MIN(pd - pc, pn - pd - 1);
	T_SORT_FUNC(vecswap)(pb, pn - r, r);
	if ((r = pb - pa) > 1)
		T_SORT_FUNC(sort)(a, r);
	if ((r = pd - pc) > 1) {
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r;
		goto loop;
	}
}

#undef T_SORT_TYPE
#undef T_SORT_FUNC
#undef T_SORT_CMP
