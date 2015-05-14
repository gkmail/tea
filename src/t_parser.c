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

#define T_PARSER_RULE_SIZE  64

#define T_PARSER_RDATA_NT_FLAG       0x80000000
#define T_PARSER_RDATA_IS_NT(v)      ((v) & T_PARSER_RDATA_NT_FLAG)
#define T_PARSER_RDATA_RID(v)        ((v) & 0xFFF)
#define T_PARSER_RDATA_TID(v)        (((v) >> 12) & 0xFFF)
#define T_PARSER_RDATA_POP(v)        (((v) >> 24) & 0x7F)
#define T_PARSER_RDATA_MAKE(rid, nt, pop) ((((unsigned int)(pop))<<24)|(((unsigned int)(nt))<<12)|(rid))

#define T_ARRAY_TYPE        T_ParserExpr
#define T_ARRAY_ELEM_TYPE   T_ParserToken
#define T_ARRAY_NAME        tokens
#define T_ARRAY_FUNC(name)  token_##name
#include <t_array.h>

#define T_ARRAY_TYPE        T_ParserRule
#define T_ARRAY_ELEM_TYPE   T_ParserExpr
#define T_ARRAY_ELEM_INIT   token_array_init
#define T_ARRAY_ELEM_DEINIT token_array_deinit
#define T_ARRAY_NAME        exprs
#define T_ARRAY_FUNC(name)  expr_##name
#include <t_array.h>

#define T_ARRAY_TYPE        T_ParserDecl
#define T_ARRAY_ELEM_TYPE   T_ParserRule
#define T_ARRAY_ELEM_INIT   expr_array_init
#define T_ARRAY_ELEM_DEINIT expr_array_deinit
#define T_ARRAY_NAME        rules
#define T_ARRAY_FUNC(name)  rule_##name
#include <t_array.h>

#define T_ARRAY_TYPE        T_ParserDecl
#define T_ARRAY_ELEM_TYPE   T_ParserState
#define T_ARRAY_NAME        states
#define T_ARRAY_FUNC(name)  state_##name
#include <t_array.h>

#define T_ARRAY_TYPE        T_ParserDecl
#define T_ARRAY_ELEM_TYPE   T_ParserEdge
#define T_ARRAY_NAME        edges
#define T_ARRAY_FUNC(name)  edge_##name
#include <t_array.h>

static T_ID
decl_add_state(T_ParserDecl *decl)
{
	T_ParserState s;

	s.shifts = -1;
	s.jumps  = -1;
	s.reduce = -1;

	return state_array_add(decl, &s);
}

static T_ID
decl_add_shift(T_ParserDecl *decl, T_ID from, T_ID to, int symbol)
{
	T_ParserEdge e;
	T_ID eid;

	e.symbol = symbol;
	e.dest   = to;
	e.next   = state_array_element(decl, from)->shifts;

	if((eid = edge_array_add(decl, &e)) < 0)
		return eid;

	state_array_element(decl, from)->shifts = eid;

	return eid;
}

static T_ID
decl_add_jump(T_ParserDecl *decl, T_ID from, T_ID to, int symbol)
{
	T_ParserEdge e;
	T_ID eid;

	e.symbol = symbol;
	e.dest   = to;
	e.next   = state_array_element(decl, from)->jumps;

	if((eid = edge_array_add(decl, &e)) < 0)
		return eid;

	state_array_element(decl, from)->jumps = eid;

	return eid;
}

T_Result
t_parser_decl_init(T_ParserDecl *decl)
{
	T_ASSERT(decl);

	rule_array_init(decl);
	state_array_init(decl);
	edge_array_init(decl);

	return T_OK;
}

void
t_parser_decl_deinit(T_ParserDecl *decl)
{
	edge_array_deinit(decl);
	state_array_deinit(decl);
	rule_array_deinit(decl);
}

#define T_ALLOC_TYPE   T_ParserDecl
#define T_ALLOC_INIT   t_parser_decl_init
#define T_ALLOC_DEINIT t_parser_decl_deinit
#define T_ALLOC_FUNC(name) t_parser_decl_##name
#include <t_alloc.h>

T_Result
t_parser_decl_add_rule(T_ParserDecl *decl, T_ParserNonterm nonterm, ...)
{
	int rule[T_PARSER_RULE_SIZE];
	int len = 0;
	int v;
	va_list ap;

	T_ASSERT(decl);

	va_start(ap, nonterm);
	while(1){
		v = va_arg(ap, int);
		if(v == -1)
			break;

		rule[len++] = v;
	}
	va_end(ap);

	return t_parser_decl_add_rulev(decl, nonterm, rule, len);
}

T_Result
t_parser_decl_add_rulev(T_ParserDecl *decl, T_ParserNonterm nonterm, int *tokens, int len)
{
	T_ParserRule *rule;
	T_ParserToken tok;
	T_ParserExpr expr;
	T_ID rid;
	T_Bool reduce;
	int i;
	T_Result r;

	T_ASSERT(decl && tokens && (len > 0));

	if(!T_PARSER_IS_NONTERM(nonterm))
		return T_ERR_ARG;

	rid = T_PARSER_NONTERM_ID(nonterm);
	if(!(rule = rule_array_element_resize(decl, rid)))
		return T_ERR_NOMEM;

	token_array_init(&expr);
	expr.prio   = T_PARSER_PRIO_MIN;

	reduce = T_FALSE;
	for(i = 0; i < len; i++){
		int tid = tokens[i];

		if(T_PARSER_IS_REDUCE(tid)){
			if(reduce){
				T_DEBUG_E("duplicate reduce actions");
				return T_ERR_SYNTAX;
			}

			tok = tid;
			if((r = token_array_add(&expr, &tok)) < 0)
				goto error;

			reduce = T_TRUE;
		}else if(T_PARSER_IS_TERM(tid)){
			tok = tid;
			if((r = token_array_add(&expr, &tok)) < 0)
				goto error;

			reduce = T_FALSE;
		}else if(T_PARSER_IS_NONTERM(tid)){
			tok = tid;
			if((r = token_array_add(&expr, &tok)) < 0)
				goto error;

			reduce = T_FALSE;
		}else if(T_PARSER_IS_PRIO(tid)){
			expr.prio = tid;
		}else{
			r = T_ERR_SYNTAX;
		}
	}

	if(!reduce){
		T_DEBUG_W("reduce action has not been defined");

		tok = T_PARSER_REDUCE_NONE;
		if((r = token_array_add(&expr, &tok)) < 0)
			goto error;
	}

	if((r = expr_array_add(rule, &expr)) < 0)
		goto error;

	return T_OK;
error:
	token_array_deinit(&expr);
	return r;
}

typedef struct{
	int rule_id;
	int expr_id;
	int dot;
}Prod;

typedef struct{
	T_SET_DECL(Prod, prods);
	int sid;
}Closure;

#define T_SET_TYPE        Closure
#define T_SET_ELEM_TYPE   Prod
#define T_SET_NAME        prods
#define T_SET_CMP(p1, p2) memcmp(p1, p2, sizeof(Prod))
#define T_SET_FUNC(name)  prod_##name
#include <t_set.h>

static T_Bool
clos_equal(Closure *c1, Closure *c2)
{
	if(prod_array_length(c1) != prod_array_length(c2))
		return T_FALSE;

	if(memcmp(c1->prods.buff, c2->prods.buff, sizeof(Prod) * prod_array_length(c1)))
		return T_FALSE;

	return T_TRUE;
}

typedef struct{
	T_ARRAY_DECL(Closure, a);
}ClosArray;

#define T_ARRAY_TYPE       ClosArray
#define T_ARRAY_ELEM_TYPE  Closure
#define T_ARRAY_NAME       a
#define T_ARRAY_EQUAL      clos_equal
#define T_ARRAY_ELEM_DEINIT prod_array_deinit
#define T_ARRAY_FUNC(name) clos_##name
#include <t_array.h>

typedef struct{
	T_QUEUE_DECL(Prod, q);
}ProdQueue;

#define T_QUEUE_TYPE      ProdQueue
#define T_QUEUE_ELEM_TYPE Prod
#define T_QUEUE_NAME      q
#define T_QUEUE_FUNC(name) prod_##name
#include <t_queue.h>

T_HASH_ENTRY_DECL(int, Closure, SymClosHashEntry);

typedef struct{
	T_HASH_DECL(SymClosHashEntry, h);
}SymClosHash;

#define T_HASH_TYPE       SymClosHash
#define T_HASH_ENTRY_TYPE SymClosHashEntry
#define T_HASH_KEY_TYPE   int
#define T_HASH_ELEM_TYPE  Closure
#define T_HASH_NAME       h
#define T_HASH_ELEM_DEINIT prod_array_deinit
#define T_HASH_FUNC(name) sym_clos_##name
#include <t_hash.h>

static T_Result
add_rule_prods(T_ParserDecl *decl, Closure *prods, int id, ProdQueue *q, T_Bool *has_eps)
{
	T_ParserRule *rule;
	T_ParserExpr *expr;
	T_ArrayIter iter;
	Prod prod;
	T_Bool eps = T_FALSE;
	T_Result r;

	if((id < 0) || (id >= rule_array_length(decl))){
		T_DEBUG_E("undefined non terminated token %08x", T_PARSER_NONTERM(id));
		return T_ERR_UNDEF;
	}

	rule = rule_array_element(decl, id);

	prod.rule_id = id;
	prod.expr_id = 0;
	prod.dot = 0;

	expr_array_iter_first(rule, &iter);
	while(!expr_array_iter_last(&iter)){
		int len;

		expr = expr_array_iter_data(&iter);

		if((r = prod_set_add(prods, &prod, NULL)) < 0)
			return r;

		if(r > 0){
			T_DEBUG_I("add prod rule:%d expr:%d dot:%d", prod.rule_id, prod.expr_id, prod.dot);
			if((r = prod_queue_push_back(q, &prod)) < 0)
				return r;
		}

		len = token_array_length(expr);
		if(len == 1)
			eps = T_TRUE;

		expr_array_iter_next(&iter);
		prod.expr_id++;
	}

	if(has_eps)
		*has_eps = eps;

	return T_OK;
}

static T_Result
add_clos_prods(T_ParserDecl *decl, Closure *prods, Closure *clos, ProdQueue *q)
{
	T_SetIter iter;
	Prod *prod;
	T_Result r;

	prod_set_iter_first(clos, &iter);
	while(!prod_set_iter_last(&iter)){
		prod = prod_set_iter_data(&iter);

		if((r = prod_set_add(prods, prod, NULL)) < 0)
			return r;

		if(r > 0){
			T_DEBUG_I("add prod rule:%d expr:%d dot:%d", prod->rule_id, prod->expr_id, prod->dot);
			if((r = prod_queue_push_back(q, prod)) < 0)
				return r;
		}

		prod_set_iter_next(&iter);
	}

	return T_OK;
}

static void
prod_dump(T_ParserDecl *decl, Prod *p)
{
	T_ParserRule *rule;
	T_ParserExpr *expr;
	int tid;

	rule = rule_array_element(decl, p->rule_id);
	expr = expr_array_element(rule, p->expr_id);

	fprintf(stderr, "%08x: ", T_PARSER_NONTERM(p->rule_id));

	for(tid = 0; tid < token_array_length(expr); tid++){
		T_ParserToken tok = *token_array_element(expr, tid);

		if(tid == p->dot)
			fprintf(stderr, ". ");

		fprintf(stderr, "%08x ", tok);
	}

	fprintf(stderr, "\n");
}

static int
prio_check(T_ParserDecl *decl, Prod *p1, Prod *p2)
{
	T_ParserRule *r1, *r2;
	T_ParserExpr *e1, *e2;
	int v1, v2, d1, d2;

	r1 = rule_array_element(decl, p1->rule_id);
	r2 = rule_array_element(decl, p2->rule_id);
	e1 = expr_array_element(r1, p1->expr_id);
	e2 = expr_array_element(r2, p2->expr_id);

	v1 = T_PARSER_PRIO_VALUE(e1->prio);
	v2 = T_PARSER_PRIO_VALUE(e2->prio);
	d1 = T_PARSER_PRIO_DIR(e1->prio);
	d2 = T_PARSER_PRIO_DIR(e2->prio);

	if(v1 != v2)
		return v1 - v2;

	if((d1 == T_PARSER_PRIO_RIGHT) && (d2 == T_PARSER_PRIO_RIGHT))
		return -1;

	return 1;
}

static T_Result
clos_build(T_ParserDecl *decl, T_ID clos_id, ClosArray *c_array)
{
	T_ParserRule *rule;
	T_ParserExpr *expr;
	T_ParserToken tok;
	SymClosHash c_hash;
	ProdQueue p_queue;
	Closure prods, *clos;
	T_SetIter s_iter;
	T_HashIter h_iter;
	T_Result r;
	T_ParserReduce r_tok;
	Prod r_prod;

	clos = clos_array_element(c_array, clos_id);

	T_DEBUG_I("build closure %d", clos->sid);

	r_tok = -1;
	r_prod.rule_id = -1;

	sym_clos_hash_init(&c_hash);
	prod_queue_init(&p_queue);
	prod_set_init(&prods);

	/*Collect productions.*/
	T_DEBUG_I("init prod data");

	if((r = add_clos_prods(decl, &prods, clos, &p_queue)) < 0)
		goto end;

	T_DEBUG_I("analyze prod queue");
	while(!prod_queue_empty(&p_queue)){
		Prod prod;
		int dot;

		prod_queue_pop_front(&p_queue, &prod);

		T_DEBUG_I("check prod rule:%d expr:%d dot:%d", prod.rule_id, prod.expr_id, prod.dot);

		rule = rule_array_element(decl, prod.rule_id);
		expr = expr_array_element(rule, prod.expr_id);
		dot  = prod.dot;

		while(dot < token_array_length(expr)){
			tok = *token_array_element(expr, dot);

			if(T_PARSER_IS_NONTERM(tok) && (tok != T_PARSER_NONTERM_ERR)){
				int tid = T_PARSER_NONTERM_ID(tok);
				T_Bool has_eps;

				if((r = add_rule_prods(decl, &prods, tid, &p_queue, &has_eps)) < 0)
					goto end;

				if(has_eps){
					dot++;
					continue;
				}
			}

			break;
		}
	}

	/*Analyze the next input tokens*/
	T_DEBUG_I("check next input tokens");
	prod_set_iter_first(&prods, &s_iter);
	while(!prod_set_iter_last(&s_iter)){
		Prod *prod = prod_set_iter_data(&s_iter);
		rule = rule_array_element(decl, prod->rule_id);
		expr = expr_array_element(rule, prod->expr_id);

		if(prod->dot < token_array_length(expr)){
			Prod nprod;
			SymClosHashEntry *hent;
			Closure *nclos;

			tok = *token_array_element(expr, prod->dot);

			if(prod->dot + 1 < token_array_length(expr)){
				/*Add the next production.*/
				nprod.rule_id = prod->rule_id;
				nprod.expr_id = prod->expr_id;
				nprod.dot     = prod->dot + 1;

				T_DEBUG_I("symbol:%08x -> prod rule:%d expr:%d dot:%d", tok, nprod.rule_id, nprod.expr_id, nprod.dot);

				if((r = sym_clos_hash_add_entry(&c_hash, &tok, &hent)) < 0)
					goto end;

				nclos = &hent->data;

				if(r > 0){
					prod_set_init(nclos);
					nclos->sid = 0;
				}

				if((r = prod_set_add(nclos, &nprod, NULL)) < 0)
					goto end;
			}

			if(T_PARSER_IS_REDUCE(tok)){
				/*Reduce*/
				if(r_prod.rule_id != -1){
					T_DEBUG_W("reduce/reduce conflict");
					prod_dump(decl, &r_prod);
					prod_dump(decl, prod);
				}else{
					r_prod = *prod;
					r_tok  = tok;
				}
			}
		}

		prod_set_iter_next(&s_iter);
	}

	/*Solve shift/reduce conflict.*/
	T_DEBUG_I("solve shift/reduce conflict ");

	if(r_prod.rule_id != -1){
		T_ParserState *s = state_array_element(decl, clos->sid);
		T_ParserRule *rule = rule_array_element(decl, r_prod.rule_id);
		T_ParserExpr *expr = expr_array_element(rule, r_prod.expr_id);
		T_ParserToken tok = *token_array_element(expr, r_prod.dot);
		int reduce, pop, ntid, rid, flag;

		sym_clos_hash_iter_first(&c_hash, &h_iter);
		while(!sym_clos_hash_iter_last(&h_iter)){
			int sym = *sym_clos_hash_iter_key(&h_iter);
			Closure *pclos = sym_clos_hash_iter_data(&h_iter);

			if(T_PARSER_IS_REDUCE(sym) && (r_tok != sym)){
				pclos->sid = -1;
			}else if(T_PARSER_IS_TERM(sym)){
				Prod *sprod = prod_set_element(pclos, 0);

				if(prio_check(decl, &r_prod, sprod) > 0){
					pclos->sid = -1;
				}
			}

			sym_clos_hash_iter_next(&h_iter);
		}

		if(r_prod.dot == token_array_length(expr) - 1){
			rid  = T_PARSER_REDUCE_ID(tok);
			ntid = r_prod.rule_id;
			pop  = r_prod.dot;
			flag = T_PARSER_RDATA_NT_FLAG;
		}else{
			rid  = T_PARSER_REDUCE_ID(tok);
			ntid = 0;
			pop  = r_prod.dot;
			flag = 0;
		}

		reduce = T_PARSER_RDATA_MAKE(rid, ntid, pop) | flag;

		T_DEBUG_I("reduce %08x", reduce);
		s->reduce = reduce;
	}

	/*Create DFA states and edges.*/
	sym_clos_hash_iter_first(&c_hash, &h_iter);
	while(!sym_clos_hash_iter_last(&h_iter)){
		int sym = *sym_clos_hash_iter_key(&h_iter);
		Closure *pclos = sym_clos_hash_iter_data(&h_iter);
		T_ID cid, eid;

		if(pclos->sid != -1){
			if((r = clos_array_add_unique(c_array, pclos, &cid)) < 0)
				goto end;

			if(r > 0)
				prod_set_init(pclos);

			clos  = clos_array_element(c_array, clos_id);
			pclos = clos_array_element(c_array, cid);
			if(r > 0){
				pclos->sid = decl_add_state(decl);
				if(pclos->sid < 0){
					r = pclos->sid;
					goto end;
				}

				T_DEBUG_I("create state: %d", pclos->sid);
			}

			if(T_PARSER_IS_TERM(sym)){
				T_DEBUG_I("shift %d -> %08x -> %d", clos->sid, sym, pclos->sid);
				eid = decl_add_shift(decl, clos->sid, pclos->sid, sym);
				if(eid < 0){
					r = eid;
					goto end;
				}
			}else if(T_PARSER_IS_NONTERM(sym) || T_PARSER_IS_REDUCE(sym)){
				T_DEBUG_I("goto %d -> %08x -> %d", clos->sid, sym, pclos->sid);
				eid = decl_add_jump(decl, clos->sid, pclos->sid, sym);
				if(eid < 0){
					r = eid;
					goto end;
				}
			}
		}

		sym_clos_hash_iter_next(&h_iter);
	}

	r = T_OK;
end:
	prod_set_deinit(&prods);
	sym_clos_hash_deinit(&c_hash);
	prod_queue_deinit(&p_queue);
	return r;
}

T_Result
t_parser_decl_build(T_ParserDecl *decl)
{
	Closure clos;
	ClosArray c_array;
	T_Result r;
	Prod prod;
	T_ID id;

	T_ASSERT(decl);

	if(state_array_length(decl) > 0)
		return T_ERR_REINIT;

	/*Add S rule.*/
	if((r = t_parser_decl_add_rule(decl, T_PARSER_NONTERM_S, T_PARSER_NONTERM_BEGIN, T_PARSER_REDUCE_NONE, -1)) < 0)
		return r;

	T_DEBUG_I("parser decl build, rules:%d", rule_array_length(decl));

	prod_set_init(&clos);
	clos_array_init(&c_array);

	/*Create the first state.*/
	if((clos.sid = decl_add_state(decl)) < 0){
		r = clos.sid;
		goto end;
	}

	prod_set_init(&clos);

	prod.rule_id = 0;
	prod.expr_id = 0;
	prod.dot     = 0;

	if((r = prod_set_add(&clos, &prod, NULL)) < 0)
		goto end;

	if((r = clos_array_add(&c_array, &clos)) < 0)
		goto end;

	prod_set_init(&clos);

	/*Create all the closures.*/
	id = 0;
	do{
		if((r = clos_build(decl, id, &c_array)) < 0)
			goto end;

		id++;
	}while(id < clos_array_length(&c_array));

#ifdef T_ENABLE_DEBUG
	t_parser_decl_dump(decl);
#endif

	r = T_OK;
end:
	prod_set_deinit(&clos);
	clos_array_deinit(&c_array);

	return r;
}

void
t_parser_decl_dump(T_ParserDecl *decl)
{
	T_ID rid, eid, tid, sid;

	T_ASSERT(decl);

	fprintf(stderr, "parser decl rules:%d\n", rule_array_length(decl));

	for(rid = 0; rid < rule_array_length(decl); rid++){
		T_ParserRule *rule = rule_array_element(decl, rid);

		fprintf(stderr, "rule %d:\n", rid);

		for(eid = 0; eid < expr_array_length(rule); eid++){
			T_ParserExpr *expr = expr_array_element(rule, eid);

			fprintf(stderr, "\texpr %d: ", eid);

			for(tid = 0; tid < token_array_length(expr); tid++){
				T_ParserToken tok = *token_array_element(expr, tid);
				fprintf(stderr, "%08x ", tok);
			}

			fprintf(stderr, "\n");
		}
	}

	for(sid = 0; sid < state_array_length(decl); sid++){
		T_ParserState *s = state_array_element(decl, sid);
		T_ParserEdge *e;
		
		fprintf(stderr, "state %d:\n", sid);

		fprintf(stderr, "\tshift: ");
		eid = s->shifts;
		while(eid != -1){
			e = edge_array_element(decl, eid);
			fprintf(stderr, "%08x->%d ", e->symbol, e->dest);
			eid = e->next;
		}
		fprintf(stderr, "\n");

		if(s->reduce != T_PARSER_REDUCE_NONE){
			fprintf(stderr, "\treduce: %08x\n", s->reduce);
		}

		fprintf(stderr, "\tgoto: ");
		eid = s->jumps;
		while(eid != -1){
			e = edge_array_element(decl, eid);
			fprintf(stderr, "%08x->%d ", e->symbol, e->dest);
			eid = e->next;
		}
		fprintf(stderr, "\n");
	}
}

#define T_QUEUE_TYPE       T_Parser
#define T_QUEUE_ELEM_TYPE  T_ParserValue
#define T_QUEUE_NAME       stack
#define T_QUEUE_FUNC(name) value_##name
#include <t_queue.h>

T_Result
t_parser_init(T_Parser *parser, T_ParserDecl *decl, T_Lex *lex)
{
	T_Result r;

	T_ASSERT(parser && decl && lex);

	if(rule_array_length(decl) <= 0)
		return T_ERR_NOTINIT;

	parser->decl = decl;
	parser->lex  = lex;
	parser->user_data = NULL;
	parser->value     = NULL;
	parser->reduce    = NULL;
	parser->error     = NULL;
	parser->fetched   = T_FALSE;
	parser->reduce_count = 0;

	parser->fetched_v.loc.user_data    = NULL;
	parser->fetched_v.loc.first_lineno = 0;
	parser->fetched_v.loc.first_column = 0;
	parser->fetched_v.loc.last_lineno  = 0;
	parser->fetched_v.loc.last_column  = 0;

	if((r = value_queue_init(parser)) < 0)
		return r;

	return T_OK;
}

void
t_parser_deinit(T_Parser *parser)
{
	T_ASSERT(parser);

	value_queue_deinit(parser);
}

void
t_parser_set_func(T_Parser *parser, T_ParserValueFunc val, T_ParserReduceFunc reduce, T_ParserErrorFunc err, void *user_data)
{
	T_ASSERT(parser);

	parser->value  = val;
	parser->reduce = reduce;
	parser->error  = err;
	parser->user_data = user_data;
}

#define T_ALLOC_TYPE       T_Parser
#define T_ALLOC_INIT       t_parser_init
#define T_ALLOC_ARGS_DECL  T_ParserDecl *decl, T_Lex *lex
#define T_ALLOC_ARGS       decl, lex
#define T_ALLOC_DEINIT     t_parser_deinit
#define T_ALLOC_FUNC(name) t_parser_##name
#include <t_alloc.h>

static void
stack_dump(T_Parser *parser)
{
	int pos;

	fprintf(stderr, "stack size %d\n", value_queue_length(parser));
	pos = 0;

	while(pos < value_queue_length(parser)){
		T_ParserValue *pv = value_queue_back(parser, pos);
		fprintf(stderr, "\tstack %d: %d %p\n", pos, pv->sid, pv->value);
		pos++;
	}
}

T_Result
t_parser_parse(T_Parser *parser)
{
	T_ParserDecl *decl;
	T_Lex *lex;
	T_ParserValueFunc vfunc;
	T_ParserReduceFunc rfunc;
	void *udata;
	T_ParserValue pv;
	T_Result r;
	T_LexToken fetched_tok = T_LEX_EOF;
	T_Bool error = T_FALSE;

	T_ASSERT(parser);

	decl = parser->decl;
	lex  = parser->lex;
	vfunc = parser->value;
	rfunc = parser->reduce;
	udata = parser->user_data;

	if(state_array_length(decl) == 0)
		return T_ERR_NOTINIT;

	memset(&pv, 0, sizeof(pv));

	if((r = value_queue_push_back(parser, &pv)) < 0)
		return r;

	while(1){
		T_ParserValue *top;
		T_ParserState *s;
		T_ParserEdge *e;
		T_ID eid;

next_state:

		top = value_queue_back(parser, 0);

		if(!parser->fetched){
			if((fetched_tok = t_lex_lex(lex, &parser->fetched_v.loc)) < T_LEX_EOF)
				return fetched_tok;

			parser->fetched_v.value = NULL;

			if(vfunc){
				if((r = vfunc(udata, parser, fetched_tok, &parser->fetched_v.value)) < 0)
					return r;
			}

			parser->fetched = T_TRUE;
		}

		T_DEBUG_I("input token %08x", fetched_tok);

#ifdef T_ENABLE_DEBUG
		stack_dump(parser);
#endif

		/*Shift check.*/
		s = state_array_element(decl, top->sid);
		eid = s->shifts;
		while(eid != -1){
			e = edge_array_element(decl, eid);
			if(e->symbol == fetched_tok){
				parser->fetched_v.sid   = e->dest;
				if((r = value_queue_push_back(parser, &parser->fetched_v)) < 0)
					return r;
				parser->fetched = T_FALSE;

				T_DEBUG_I("shift to state %d", e->dest);
				goto next_state;
			}
			eid = e->next;
		}

		/*Reduce check.*/
		if(s->reduce != -1){
			int rid;
			int tid;
			int pop;
			T_ParserReduce reduce;
			T_ParserToken tok;

			error = T_FALSE;

			rid = T_PARSER_RDATA_RID(s->reduce);
			tid = T_PARSER_RDATA_TID(s->reduce);
			pop = T_PARSER_RDATA_POP(s->reduce);

			parser->reduce_count = pop;

			if(T_PARSER_RDATA_IS_NT(s->reduce)){
				tok = T_PARSER_NONTERM(tid);
			}else{
				tok = T_PARSER_REDUCE(tid);
				pop = 0;
			}

			reduce = T_PARSER_REDUCE(rid);

			T_DEBUG_I("reduce %d use rule %08x", pop, reduce);

			if(rfunc){
				if((r = rfunc(udata, parser, reduce, &pv.value)) < 0)
					return r;
			}

			if(pop){
				T_ParserValue *bot = value_queue_back(parser, pop - 1);

				pv.loc.user_data    = bot->loc.user_data;
				pv.loc.first_lineno = bot->loc.first_lineno;
				pv.loc.first_column = bot->loc.first_column;
				pv.loc.last_lineno  = bot->loc.last_lineno;
				pv.loc.last_column  = bot->loc.last_column;
			}else{
				pv.loc = top->loc;
			}

			value_queue_pop_back_n(parser, pop);

			top = value_queue_back(parser, 0);

			s = state_array_element(decl, top->sid);
			eid = s->jumps;
			while(eid != -1){
				e = edge_array_element(decl, eid);
				if(e->symbol == tok){
					pv.sid   = e->dest;
					if((r = value_queue_push_back(parser, &pv)) < 0)
						return r;

					T_DEBUG_I("goto state %d", e->dest);
					goto next_state;
				}
				eid = e->next;
			}
		}

		if((value_queue_length(parser) == 1) && (fetched_tok == T_LEX_EOF))
			return T_OK;

		if(!error){
			T_DEBUG_E("parser error");

			if(parser->error)
				parser->error(udata, parser);

			error = T_TRUE;
		}

		if(fetched_tok != T_LEX_EOF){
			parser->fetched = T_FALSE;

			while(!value_queue_empty(parser)){
				top = value_queue_back(parser, 0);
				s = state_array_element(decl, top->sid);

				eid = s->jumps;
				while(eid != -1){
					e = edge_array_element(decl, eid);
					if(e->symbol == T_PARSER_NONTERM_ERR){
						pv.sid   = e->dest;
						pv.value = NULL;
						pv.loc   = parser->fetched_v.loc;

						if((r = value_queue_push_back(parser, &pv)) < 0)
							return r;

						T_DEBUG_I("goto state %d", e->dest);
						goto next_state;
					}
					eid = e->next;
				}

				value_queue_pop_back_n(parser, 1);
			}
		}

		return T_ERR_SYNTAX;
	}

	return T_OK;
}

T_Result
t_parser_get_loc(T_Parser *parser, int n, T_LexLoc *loc)
{
	T_ParserValue *pv;

	T_ASSERT(parser && loc && (n >= 0) && (n < parser->reduce_count));

	if(n >= value_queue_length(parser))
		return T_ERR_ARG;

	pv = value_queue_back(parser, parser->reduce_count - n - 1);
	*loc = pv->loc;

	return T_OK;
}

T_Result
t_parser_get_value(T_Parser *parser, int n, void **value)
{
	T_ParserValue *pv;

	T_ASSERT(parser && value && (n >= 0) && (n < parser->reduce_count));

	if(n >= value_queue_length(parser))
		return T_ERR_ARG;

	pv = value_queue_back(parser, parser->reduce_count - n - 1);
	*value= pv->value;

	return T_OK;
}

