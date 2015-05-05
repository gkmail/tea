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

extern T_Result   t_re_to_auto(T_Auto *aut, const char *str, int len, T_AutoData data, int *errcol);
extern T_Re*      t_re_create(const char *str, int len, int *errcol);
extern void       t_re_destroy(T_Re *re);
extern T_Result   t_re_match(T_Re *re, const char *str, int len, int *start, int *end);

#ifdef __cplusplus
}
#endif

#endif
