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

static void
mach_join(T_AutoMach *m1, T_AutoMach *m2)
{
	if(m1->s_first == -1){
		m1->s_first = m2->s_first;
		m1->s_last  = m2->s_last;
	}else{
		m1->s_first = T_MIN(m1->s_first, m2->s_first);
		m1->s_last  = T_MAX(m1->s_last,  m2->s_last);
	}

	if(m1->e_first == -1){
		m1->e_first = m2->e_first;
		m1->e_last  = m2->e_last;
	}else{
		m1->e_first = T_MIN(m1->e_first, m2->e_first);
		m1->e_last  = T_MAX(m1->e_last,  m2->e_last);
	}
}

static T_Result
mach_dup(T_Auto *aut, T_AutoMach *morg, T_AutoMach *mdup)
{
	int soff, eoff;
	T_ID sorg_id, sdup_id;
	T_ID eorg_id, edup_id;

	t_auto_mach_init(mdup);

	if(morg->s_first == -1)
		return T_OK;

	soff = aut->states.nmem - morg->s_first;
	eoff = aut->edges.nmem - morg->e_first;

	sorg_id = morg->s_first;
	while(1){
		T_AutoState *sorg;
		T_AutoState sdup;

		sorg = state_array_element(aut, sorg_id);
		sdup.data  = sorg->data;
		sdup.edges = (sorg->edges == -1) ? -1 : sorg->edges + eoff;

		if((sdup_id = state_array_add(aut, &sdup)) < 0)
			return sdup_id;

		if(sorg_id == morg->s_last)
			break;
		sorg_id++;
	}

	eorg_id = morg->e_first;
	while(1){
		T_AutoEdge *eorg;
		T_AutoEdge edup;

		eorg = edge_array_element(aut, eorg_id);
		edup.dest   = eorg->dest + soff;
		edup.symbol = eorg->symbol;
		edup.next   = (eorg->next == -1) ? -1 : eorg->next + eoff;

		if((edup_id = edge_array_add(aut, &edup)) < 0)
			return edup_id;

		if(eorg_id == morg->e_last)
			break;
		eorg_id++;
	}

	mdup->s_begin = morg->s_begin + soff;
	mdup->s_end   = morg->s_end + soff;
	mdup->s_first = morg->s_first + soff;
	mdup->s_last  = morg->s_last + soff;
	mdup->e_first = morg->e_first + eoff;
	mdup->e_last  = morg->e_last + eoff;

	return T_OK;
}

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
t_auto_mach_init(T_AutoMach *mach)
{
	T_ASSERT(mach);

	mach->s_begin = -1;
	mach->s_end   = -1;
	mach->s_first = -1;
	mach->s_last  = -1;
	mach->e_first = -1;
	mach->e_last  = -1;
}

T_ID
t_auto_mach_add_state(T_Auto *aut, T_AutoMach *mach, T_AutoData data)
{
	T_ID s;

	T_ASSERT(aut && mach);

	s = t_auto_add_state(aut, data);
	if(s < 0)
		return s;

	if(mach->s_begin == -1){
		mach->s_begin = s;
		mach->s_end   = s;
	}

	if(mach->s_first == -1)
		mach->s_first = s;

	mach->s_last = s;

	return s;
}

T_ID
t_auto_mach_add_edge(T_Auto *aut, T_AutoMach *mach, T_ID from, T_ID to, T_AutoSymbol symbol)
{
	T_ID e;

	T_ASSERT(aut && mach);

	e = t_auto_add_edge(aut, from, to, symbol);
	if(e < 0)
		return e;

	if(mach->e_first == -1)
		mach->e_first = e;

	mach->e_last = e;

	return e;
}

T_Result
t_auto_mach_or(T_Auto *aut, T_AutoMach *mach, T_AutoMach *mor)
{
	T_ID sb, se, e;

	T_ASSERT(aut && mach && mor);

	if((sb = t_auto_mach_add_state(aut, mach, -1)) < 0)
		return sb;

	if((se = t_auto_mach_add_state(aut, mach, -1)) < 0)
		return se;

	if(mach->s_begin != -1){
		if((e = t_auto_mach_add_edge(aut, mach, sb, mach->s_begin, T_AUTO_EPSILON)) < 0)
			return e;
	}

	if(mor->s_begin != -1){
		if((e = t_auto_mach_add_edge(aut, mach, sb, mor->s_begin, T_AUTO_EPSILON)) < 0)
			return e;
	}

	if(mach->s_end != -1){
		if((e = t_auto_mach_add_edge(aut, mach, mach->s_end, se, T_AUTO_EPSILON)) < 0)
			return e;
	}

	if(mor->s_end != -1){
		if((e = t_auto_mach_add_edge(aut, mach, mor->s_end, se, T_AUTO_EPSILON)) < 0)
			return e;
	}

	if((mach->s_begin == -1) || (mor->s_begin == -1)){
		if((e = t_auto_mach_add_edge(aut, mach, sb, se, T_AUTO_EPSILON)) < 0)
			return e;
	}

	mach_join(mach, mor);

	mach->s_begin = sb;
	mach->s_end   = se;

	return T_OK;
}

T_Result
t_auto_mach_link(T_Auto *aut, T_AutoMach *mach, T_AutoMach *ml)
{
	T_ID e;

	T_ASSERT(aut && mach && ml);

	if(ml->s_begin == -1)
		return T_OK;

	if(mach->s_begin == -1){
		*mach = *ml;
		return T_OK;
	}

	if((e = t_auto_mach_add_edge(aut, mach, mach->s_end, ml->s_begin, T_AUTO_EPSILON)) < 0)
		return e;

	mach_join(mach, ml);

	mach->s_end = ml->s_end;

	return T_OK;
}

T_Result
t_auto_mach_repeat(T_Auto *aut, T_AutoMach *mach, int min, int max)
{
	T_AutoMach mret;
	T_ID s, e;
	int cnt;
	T_Result r;

	T_ASSERT(aut && mach && (min >= 0));

	if((min == 0) && (max == 0)){
		if((s = t_auto_mach_add_state(aut, mach, -1)) < 0)
			return s;

		mach->s_begin = s;
		mach->s_end   = s;
		return T_OK;
	}

	cnt = min;
	if(max >= 0)
		cnt += max;
	else
		cnt++;

	{
		T_AutoMach mdup[cnt];
		T_AutoMach mempty;
		int i;

		mdup[0] = *mach;
		for(i = 1; i < cnt; i++){
			if((r = mach_dup(aut, mach, &mdup[i])) < 0)
				return r;
		}

		t_auto_mach_init(&mret);

		for(i = 0; i < min; i++){
			if((r = t_auto_mach_link(aut, &mret, &mdup[i])) < 0)
				return r;
		}

		t_auto_mach_init(&mempty);

		if(max < 0)
			cnt--;

		for(; i < cnt; i++){
			if((r = t_auto_mach_or(aut, &mdup[i], &mempty)) < 0)
				return r;
			if((r = t_auto_mach_link(aut, &mret, &mdup[i])) < 0)
				return r;
		}

		if(max < 0){
			s = mret.s_end;
			if(s == -1){
				if((s = t_auto_mach_add_state(aut, &mret, -1)) < 0)
					return s;
			}

			if((e = t_auto_mach_add_edge(aut, &mret, s, mdup[cnt].s_begin, T_AUTO_EPSILON)) < 0)
				return e;
			if((e = t_auto_mach_add_edge(aut, &mret, mdup[cnt].s_end, s, T_AUTO_EPSILON)) < 0)
				return e;
		}
	}

	*mach = mret;
	return T_OK;
}

T_Result
t_auto_mach_add_symbol(T_Auto *aut, T_AutoMach *mach, T_AutoSymbol symbol)
{
	T_ID s, e;

	T_ASSERT(aut && mach);

	if(mach->s_begin == -1){
		if((s = t_auto_mach_add_state(aut, mach, -1)) < 0)
			return s;
	}

	if((s = t_auto_mach_add_state(aut, mach, -1)) < 0)
		return s;

	if((e = t_auto_mach_add_edge(aut, mach, mach->s_end, s, symbol)) < 0)
		return e;

	mach->s_end = s;

	return T_OK;
}

T_Result
t_auto_mach_add_symbols(T_Auto *aut, T_AutoMach *mach, T_AutoSymbol *symbol, int len)
{
	T_AutoSymbol *sym, *send;
	T_ID sb, se, e;

	T_ASSERT(aut && mach && (len >= 0));

	if(len == 0)
		return T_OK;

	if(mach->s_end == -1){
		if((sb = t_auto_mach_add_state(aut, mach, -1)) < 0)
			return sb;
	}else{
		sb = mach->s_end;
	}

	if((se = t_auto_mach_add_state(aut, mach, -1)) < 0)
		return se;

	sym  = symbol;
	send = sym + len;

	while(sym < send){
		if((e = t_auto_mach_add_edge(aut, mach, sb, se, *sym)) < 0)
			return e;

		sym++;
	}

	mach->s_end = se;

	return T_OK;
}

T_Result
t_auto_mach_add_symbol_range(T_Auto *aut, T_AutoMach *mach, T_AutoSymbol min, T_AutoSymbol max)
{
	T_AutoSymbol sym;
	T_ID sb, se, e;

	T_ASSERT(aut && mach && (max >= min));

	if(mach->s_end == -1){
		if((sb = t_auto_mach_add_state(aut, mach, -1)) < 0)
			return sb;
	}else{
		sb = mach->s_end;
	}

	if((se = t_auto_mach_add_state(aut, mach, -1)) < 0)
		return se;

	for(sym = min; sym <= max; sym++){
		if((e = t_auto_mach_add_edge(aut, mach, sb, se, sym)) < 0)
			return e;
	}

	mach->s_end = se;

	return T_OK;
}

typedef struct{
	T_SET_DECL(T_ID, s);
}SIDSet;
#define T_SET_TYPE       SIDSet
#define T_SET_ELEM_TYPE  T_ID
#define T_SET_NAME       s
#define T_SET_FUNC(name) sid_##name
#include <t_set.h>

typedef struct{
	T_QUEUE_DECL(T_ID, q);
}SIDQueue;
#define T_QUEUE_TYPE       SIDQueue
#define T_QUEUE_ELEM_TYPE  T_ID
#define T_QUEUE_NAME       q
#define T_QUEUE_FUNC(name) sid_##name
#include <t_queue.h>

static void
auto_dump(T_Auto *aut)
{
#ifdef T_ENABLE_DEBUG
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
			fprintf(stderr, "%d->%d ", e->symbol, e->dest);
			eid = e->next;
		}
		fprintf(stderr, "\n");
	}
#endif
}

static T_Result
nfa_clear_epsilon(T_Auto *nfa)
{
	SIDSet eps_set;
	SIDSet full_set;
	SIDQueue q;
	T_AutoState *s, *eps;
	T_AutoEdge *e;
	T_ID sid, dest_sid, eps_sid, data_sid, eid;
	T_Result r;

	sid_set_init(&eps_set);
	sid_set_init(&full_set);
	sid_queue_init(&q);

	sid = 0;
	if((r = sid_queue_push_back(&q, &sid)) < 0)
		goto end;
	if((r = sid_set_add(&full_set, &sid, NULL)) < 0)
		goto end;

	do{
		T_SetIter iter;
		
		sid_queue_pop_front(&q, &sid);
		s = state_array_element(nfa, sid);
		data_sid = sid;

		T_DEBUG_I("check state %d", sid);

		/*Find epsilon states.*/
		eid = s->edges;
		while(eid != -1){
			e = edge_array_element(nfa, eid);
			dest_sid = e->dest;

			if(e->symbol == T_AUTO_EPSILON){
				T_DEBUG_I("add eps %d", dest_sid);
				if((r = sid_set_add(&eps_set, &dest_sid, NULL)) < 0)
					goto end;
			}else{
				if((r = sid_set_add(&full_set, &dest_sid, NULL)) < 0)
					goto end;
				if(r > 0){
					T_DEBUG_I("add real %d", dest_sid);
					if((r = sid_queue_push_back(&q, &dest_sid)) < 0)
						goto end;
				}
			}

			eid = e->next;
		}

		/*Relink edges.*/
		sid_set_iter_first(&eps_set, &iter);
		while(!sid_set_iter_last(&iter)){

			eps_sid = *sid_set_iter_data(&iter);
			eps = state_array_element(nfa, eps_sid);

			T_DEBUG_I("eps %d", eps_sid);

			eid = eps->edges;
			while(eid != -1){
				e = edge_array_element(nfa, eid);
				if(e->symbol != T_AUTO_EPSILON){
					dest_sid = e->dest;

					if((r = sid_set_add(&full_set, &dest_sid, NULL)) < 0)
						goto end;
					if(r > 0){
						T_DEBUG_I("add real %d", dest_sid);
						if((r = sid_queue_push_back(&q, &dest_sid)) < 0)
							goto end;
					}

					T_DEBUG_I("relink %d %d %d", sid, dest_sid, e->symbol);
					if((r = t_auto_add_edge(nfa, sid, dest_sid, e->symbol)) < 0)
						goto end;
				}

				eid = edge_array_element(nfa, eid)->next;
			}

			if((eps->data != -1) && ((s->data == -1) || (data_sid > eps_sid))){
				s->data = eps->data;
				data_sid = eps_sid;
			}

			sid_set_iter_next(&iter);
		}

		sid_set_reinit(&eps_set);
	}while(!sid_queue_empty(&q));

	r = T_OK;
end:
	sid_set_deinit(&eps_set);
	sid_set_deinit(&full_set);
	sid_queue_deinit(&q);

	return r;
}

/*State group.*/
typedef struct{
	T_SET_DECL(T_ID, s);
}SGroup;

#define T_SET_TYPE         SGroup
#define T_SET_ELEM_TYPE    T_ID
#define T_SET_NAME         s
#define T_SET_FUNC(name)   sgrp_##name
#include <t_set.h>

/*State hash table. Match NFA state group to DFA state ID.*/
static unsigned int
sgrp_key(SGroup *sgrp)
{
	T_ID id;

	id = *sgrp_set_element(sgrp, 0);
	return (unsigned int)id;
}

static T_Bool
sgrp_equal(SGroup *g1, SGroup *g2)
{
	int i, len1, len2;

	len1 = sgrp_set_length(g1);
	len2 = sgrp_set_length(g2);

	if(len1 != len2)
		return T_FALSE;

	for(i = 0; i < len1; i++){
		if(*sgrp_set_element(g1, i) != *sgrp_set_element(g2, i))
			return T_FALSE;
	}

	return T_TRUE;
}

T_HASH_ENTRY_DECL(SGroup, T_ID, SIDHashEntry);
typedef struct{
	T_HASH_DECL(SIDHashEntry, h);
}SIDHash;
#define T_HASH_TYPE        SIDHash
#define T_HASH_ENTRY_TYPE  SIDHashEntry
#define T_HASH_NAME        h
#define T_HASH_KEY_TYPE    SGroup
#define T_HASH_ELEM_TYPE   T_ID
#define T_HASH_KEY         sgrp_key
#define T_HASH_EQUAL       sgrp_equal
#define T_HASH_KEY_DESTROY sgrp_set_deinit
#define T_HASH_FUNC(name)  sid_##name
#include <t_hash.h>

/*Symbol hash table. Match symbol to state group.*/
T_HASH_ENTRY_DECL(T_AutoSymbol, SGroup, SymbolHashEntry);
typedef struct{
	T_HASH_DECL(SymbolHashEntry, h);
}SymbolHash;
#define T_HASH_TYPE        SymbolHash
#define T_HASH_ENTRY_TYPE  SymbolHashEntry
#define T_HASH_NAME        h
#define T_HASH_KEY_TYPE    T_AutoSymbol
#define T_HASH_ELEM_TYPE   SGroup
#define T_HASH_ELEM_DESTROY sgrp_set_deinit
#define T_HASH_FUNC(name)   sym_##name
#include <t_hash.h>

/*State group queue.*/
typedef struct{
	T_QUEUE_DECL(SGroup, q);
}SGroupQueue;
#define T_QUEUE_TYPE       SGroupQueue
#define T_QUEUE_ELEM_TYPE  SGroup
#define T_QUEUE_NAME       q
#define T_QUEUE_FUNC(name) sgrp_##name
#include <t_queue.h>

static T_Result
nfa_to_dfa(T_Auto *nfa, T_Auto *dfa)
{
	SIDHash sid_hash;
	SIDHashEntry *sid_hent;
	SymbolHash sym_hash;
	SymbolHashEntry *sym_hent;
	T_HashIter h_iter;
	T_SetIter s_iter;
	SGroupQueue q;
	T_ID sid, eid, dest, dfa_sid, orig_dfa_sid, data_sid;
	T_AutoState *s;
	T_AutoEdge *e;
	SGroup *pgrp, grp;
	T_AutoSymbol sym;
	T_AutoData data;
	T_Result r;

	sgrp_set_init(&grp);
	sid_hash_init(&sid_hash);
	sym_hash_init(&sym_hash);
	sgrp_queue_init(&q);

	sid = 0;
	if((r = sgrp_set_add(&grp, &sid, NULL)) < 0)
		goto end;
	dfa_sid = 0;
	if((r = sid_hash_add(&sid_hash, &grp, &dfa_sid)) < 0){
		sgrp_set_deinit(&grp);
		goto end;
	}
	if((r = sgrp_queue_push_back(&q, &grp)) < 0)
		goto end;

	do{
		sgrp_queue_pop_front(&q, &grp);

		T_DEBUG_I("check state group");

		r = sid_hash_lookup(&sid_hash, &grp, &orig_dfa_sid);
		T_ASSERT(r > 0);

		sgrp_set_iter_first(&grp, &s_iter);
		while(!sgrp_set_iter_last(&s_iter)){

			sid = *sgrp_set_iter_data(&s_iter);
			s = state_array_element(nfa, sid);

			T_DEBUG_I("check state %d", sid);

			/*Generate symbol set.*/
			eid = s->edges;
			while(eid != -1){
				e = edge_array_element(nfa, eid);
				sym = e->symbol;

				if(sym != T_AUTO_EPSILON){
					dest = e->dest;

					if((r = sym_hash_add_entry(&sym_hash, &sym, &sym_hent)) < 0)
						goto end;

					pgrp = &sym_hent->data;
					if(r > 0){
						sgrp_set_init(pgrp);
					}

					if((r = sgrp_set_add(pgrp, &dest, NULL)) < 0)
						goto end;
				}

				eid = e->next;
			}

			sgrp_set_iter_next(&s_iter);
		}

		/*Create DFA states.*/
		sym_hash_iter_first(&sym_hash, &h_iter);
		while(!sym_hash_iter_last(&h_iter)){

			sym  = *sym_hash_iter_key(&h_iter);
			pgrp = sym_hash_iter_data(&h_iter);

			T_DEBUG_I("symbol %d", sym);

			if((r = sid_hash_add_entry(&sid_hash, pgrp, &sid_hent)) < 0)
				goto end;

			if(r == 0){
				dfa_sid = sid_hent->data;
			}else{
				/*Get the data.*/
				data_sid = -1;
				data = -1;

				sgrp_set_iter_first(pgrp, &s_iter);
				while(!sgrp_set_iter_last(&s_iter)){
					T_ID t_id = *sgrp_set_iter_data(&s_iter);
					T_AutoState *t_s = state_array_element(nfa, t_id);

					if((t_s->data != -1) && ((data_sid == -1) || (data_sid > t_id))){
						data_sid = t_id;
						data = t_s->data;
					}

					sgrp_set_iter_next(&s_iter);
				}
				
				/*Add a new DFA state.*/
				if((dfa_sid = t_auto_add_state(dfa, data)) < 0){
					r = dfa_sid;
					goto end;
				}
				sid_hent->data = dfa_sid;

				if((r = sgrp_queue_push_back(&q, pgrp)) < 0)
					goto end;

				sgrp_set_init(pgrp);
			}

			T_DEBUG_I("dfa edge %d->%d->%d", orig_dfa_sid, sym, dfa_sid);

			if((r = t_auto_add_edge(dfa, orig_dfa_sid, dfa_sid, sym)) < 0)
				goto end;

			sym_hash_iter_next(&h_iter);
		}

		sym_hash_reinit(&sym_hash);
	}while(!sgrp_queue_empty(&q));

	r = T_OK;
end:
	sgrp_queue_deinit(&q);
	sid_hash_deinit(&sid_hash);
	sym_hash_deinit(&sym_hash);

	return r;
}

T_Result
t_auto_to_dfa(T_Auto *nfa, T_Auto *dfa)
{
	T_Result r;

	T_ASSERT(nfa && dfa);

	T_DEBUG_I("NFA:");
	auto_dump(nfa);

	if((r = nfa_clear_epsilon(nfa)) < 0)
		return r;

	T_DEBUG_I("NFA after clear epsilon:");
	auto_dump(nfa);

	if((r = nfa_to_dfa(nfa, dfa)) < 0)
		return r;

	T_DEBUG_I("DFA:");
	auto_dump(dfa);

	return T_OK;
}

