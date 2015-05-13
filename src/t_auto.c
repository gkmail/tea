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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define T_ENABLE_DEBUG
#include <tea.h>

#define T_ARRAY_TYPE       T_Auto
#define T_ARRAY_ELEM_TYPE  T_AutoState
#define T_ARRAY_NAME       states
#define T_ARRAY_FUNC(name) state_##name
#include <t_array.h>

#define T_ARRAY_TYPE       T_Auto
#define T_ARRAY_ELEM_TYPE  T_AutoEdge
#define T_ARRAY_NAME       edges
#define T_ARRAY_FUNC(name) edge_##name
#include <t_array.h>

T_Result
t_auto_init(T_Auto *aut)
{
	T_AutoState s;
	T_ID r;

	state_array_init(aut);
	edge_array_init(aut);

	s.edges = -1;
	s.data  = -1;
	if((r = state_array_add(aut, &s)) < 0){
		state_array_deinit(aut);
		edge_array_deinit(aut);
		return r;
	}

	return T_OK;
}

void
t_auto_deinit(T_Auto *aut)
{
	state_array_deinit(aut);
	edge_array_deinit(aut);
}

#define T_ALLOC_TYPE   T_Auto
#define T_ALLOC_INIT   t_auto_init
#define T_ALLOC_DEINIT t_auto_deinit
#define T_ALLOC_FUNC(name) t_auto_##name
#include <t_alloc.h>

T_ID
t_auto_add_state(T_Auto *aut, T_AutoData data)
{
	T_AutoState s;

	T_ASSERT(aut);

	s.data  = data;
	s.edges = -1;

	return state_array_add(aut, &s);
}

T_ID
t_auto_add_state_data(T_Auto *aut, T_AutoState *s)
{
	T_ASSERT(aut && s);

	return state_array_add(aut, s);
}

T_ID
t_auto_add_edge(T_Auto *aut, T_ID from, T_ID to, T_AutoSymbol symbol)
{
	T_AutoState *s;
	T_AutoEdge e;
	T_ID id;

	T_ASSERT(aut && (from >= 0) && (to >= 0));

	s = state_array_element(aut, from);

	e.dest   = to;
	e.symbol = symbol;
	e.next   = s->edges;

	id = edge_array_add(aut, &e);
	if(id < 0)
		return id;

	s->edges = id;
	return id;
}

T_ID
t_auto_add_edge_data(T_Auto *aut, T_AutoEdge *e)
{
	T_ASSERT(aut && e);

	return edge_array_add(aut, e);
}

T_AutoState*
t_auto_get_state(T_Auto *aut, T_ID sid)
{
	return state_array_element(aut, sid);
}

T_AutoEdge*
t_auto_get_edge(T_Auto *aut, T_ID eid)
{
	return edge_array_element(aut, eid);
}

void
t_auto_dump(T_Auto *aut)
{
	T_AutoState *s;
	T_AutoEdge *e;
	T_ID sid, eid;

	fprintf(stderr, "auto states:%d edges:%d\n", aut->states.nmem, aut->edges.nmem);

	for(sid = 0; sid < state_array_length(aut); sid++){
		s = state_array_element(aut, sid);
		fprintf(stderr, "state %d(%d): ", sid, s->data);

		eid = s->edges;
		while(eid != -1){
			e = edge_array_element(aut, eid);
			fprintf(stderr, "%08x->%d ", e->symbol, e->dest);
			eid = e->next;
		}
		fprintf(stderr, "\n");
	}
}

