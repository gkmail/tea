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

#ifndef _T_RE_H_
#define _T_RE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef T_Auto T_Re;

#define T_RE_EPSILON 0xFFFF
#define T_RE_BOL     0xFFFE
#define T_RE_EOL     0xFFFD

#define T_RE_CHAR_MIN 0
#define T_RE_CHAR_MAX 0xFFFF

typedef int T_ReFlags;

#define T_RE_FL_IGNORE_CASE 1
#define T_RE_FL_NO_BOL      2

typedef struct{
	T_ID   s_begin;
	T_ID   s_end;
	T_ID   s_first;
	T_ID   s_last;
	T_ID   e_first;
	T_ID   e_last;
}T_ReMach;

extern T_Result   t_re_to_auto(T_Re *re, const char *str, int len, T_AutoData data, T_ReFlags flags, int *errcol);
extern T_Re*      t_re_create(const char *str, int len, T_ReFlags flags, int *errcol);
extern void       t_re_destroy(T_Re *re);
extern T_Result   t_re_match(T_Re *re, const char *str, int len, int *start, int *end);

extern void       t_re_mach_init(T_ReMach *mach);
extern T_ID       t_re_mach_add_state(T_Re *re, T_ReMach *mach, T_AutoData data);
extern T_ID       t_re_mach_add_edge(T_Re *re, T_ReMach *mach, T_ID from, T_ID to, T_AutoSymbol symbol);
extern T_Result   t_re_mach_or(T_Re *re, T_ReMach *mach, T_ReMach *mor);
extern T_Result   t_re_mach_link(T_Re *re, T_ReMach *mach, T_ReMach *ml);
extern T_Result   t_re_mach_repeat(T_Re *re, T_ReMach *mach, int min, int max);

extern T_Result   t_re_to_dfa(T_Re *nfa, T_Re *dfa);

#ifdef __cplusplus
}
#endif

#endif
