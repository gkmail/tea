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

#ifndef _T_AUTO_H_
#define _T_AUTO_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int T_AutoData;
typedef int T_AutoSymbol;

#define T_AUTO_EPSILON -1
#define T_AUTO_BOL     -2
#define T_AUTO_EOL     -3
#define T_AUTO_REDUCE  -4

typedef struct{
	T_ID       edges;
	T_AutoData data;
}T_AutoState;

typedef struct{
	T_ID         dest;
	T_ID         next;
	T_AutoSymbol symbol;
}T_AutoEdge;

typedef struct{
	T_ID   s_begin;
	T_ID   s_end;
	T_ID   s_first;
	T_ID   s_last;
	T_ID   e_first;
	T_ID   e_last;
}T_AutoMach;

typedef struct{
	T_ARRAY_DECL(T_AutoState, states);
	T_ARRAY_DECL(T_AutoEdge,  edges);
}T_Auto;

typedef int T_AutoFlags;
#define T_AUTO_FL_IGNORE_CASE 1
#define T_AUTO_FL_NO_BOL      2

extern T_Result t_auto_init(T_Auto *aut);
extern void     t_auto_deinit(T_Auto *aut);
extern T_Auto*  t_auto_create(void);
extern void     t_auto_destroy(T_Auto *aut);
extern T_ID     t_auto_add_state(T_Auto *aut, T_AutoData data);
extern T_ID     t_auto_add_edge(T_Auto *aut, T_ID from, T_ID to, T_AutoSymbol symbol);
extern T_AutoState* t_auto_get_state(T_Auto *aut, T_ID sid);
extern T_AutoEdge*  t_auto_get_edge(T_Auto *aut, T_ID eid);
extern T_Result     t_auto_to_dfa(T_Auto *nfa, T_Auto *dfa);
extern void     t_auto_dump(T_Auto *aut);

extern void     t_auto_mach_init(T_AutoMach *mach);
extern T_ID     t_auto_mach_add_state(T_Auto *aut, T_AutoMach *mach, T_AutoData data);
extern T_ID     t_auto_mach_add_edge(T_Auto *aut, T_AutoMach *mach, T_ID from, T_ID to, T_AutoSymbol symbol);
extern T_Result t_auto_mach_or(T_Auto *aut, T_AutoMach *mach, T_AutoMach *mor);
extern T_Result t_auto_mach_link(T_Auto *aut, T_AutoMach *mach, T_AutoMach *ml);
extern T_Result t_auto_mach_repeat(T_Auto *aut, T_AutoMach *mach, int min, int max);
extern T_Result t_auto_mach_add_symbol(T_Auto *aut, T_AutoMach *mach, T_AutoSymbol symbol, T_AutoFlags flags);
extern T_Result t_auto_mach_add_symbols(T_Auto *aut, T_AutoMach *mach, T_AutoSymbol *symbol, int len, T_AutoFlags flags);
extern T_Result t_auto_mach_add_symbol_range(T_Auto *aut, T_AutoMach *mach, T_AutoSymbol min, T_AutoSymbol max, T_AutoFlags flags);

#ifdef __cplusplus
}
#endif

#endif
