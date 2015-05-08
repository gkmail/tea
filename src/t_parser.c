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

T_Result
t_parser_decl_init(T_ParserDecl *decl)
{
	T_Result r;

	T_ASSERT(decl);

	if((r = rule_array_init(decl)) < 0)
		return r;

	if((r = t_auto_init(&decl->dfa)) < 0){
		rule_array_deinit(decl);
		return r;
	}

	return T_OK;
}

void
t_parser_decl_deinit(T_ParserDecl *decl)
{
	rule_array_deinit(decl);
	t_auto_deinit(&decl->dfa);
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
	int i;
	T_Result r;

	T_ASSERT(decl && tokens && (len > 0));

	if(!T_PARSER_IS_NONTERM(nonterm))
		return T_ERR_ARG;

	rid = T_PARSER_NONTERM_ID(nonterm);
	if(!(rule = rule_array_element_resize(decl, rid)))
		return T_ERR_NOMEM;

	token_array_init(&expr);
	expr.prio = T_PARSER_PRIO_MIN;

	for(i = 0; i < len; i++){
		int tid = tokens[i];

		if(T_PARSER_IS_TERM(tid)){
			tok.token = tid;

			if((i + 1 < len) && T_PARSER_IS_REDUCE(tokens[i + 1])){
				i++;
				tok.reduce = tokens[i];
			}else{
				tok.reduce = -1;
			}

			if((r = token_array_add(&expr, &tok)) < 0)
				goto error;
		}else if(T_PARSER_IS_NONTERM(tid)){
			tok.token  = tid;
			tok.reduce = -1;

			if((r = token_array_add(&expr, &tok)) < 0)
				goto error;
		}else if(T_PARSER_IS_PRIO(tid)){
			expr.prio = tid;
		}else{
			r = T_ERR_SYNTAX;
		}
	}

	if((r = expr_array_add(rule, &expr)) < 0)
		goto error;

	return T_OK;
error:
	token_array_deinit(&expr);
	return r;
}

typedef struct{
	int  rule_id;
	int  expr_id;
	int  dot;
	int  reduce_sid;
}Prod;

typedef struct{
	T_ARRAY_DECL(Prod, a);
	T_ID dfa_sid;
}Closure;
#define T_ARRAY_TYPE       Closure
#define T_ARRAY_ELEM_TYPE  Prod
#define T_ARRAY_NAME       a
#define T_ARRAY_CMP(p1, p2) memcmp(p1, p2, sizeof(Prod))
#define T_ARRAY_FUNC(name) prod_##name
#include <t_array.h>

typedef struct{
	T_ARRAY_DECL(Closure, a);
}ClosArray;
#define T_ARRAY_TYPE        ClosArray
#define T_ARRAY_ELEM_TYPE   Closure
#define T_ARRAY_NAME        a
#define T_ARRAY_ELEM_DEINIT prod_array_deinit
#define T_ARRAY_FUNC(name)  clos_##name
#include <t_array.h>

typedef struct{
	T_QUEUE_DECL(T_ID, q);
}CIDQueue;
#define T_QUEUE_TYPE        CIDQueue
#define T_QUEUE_ELEM_TYPE   T_ID
#define T_QUEUE_NAME        q
#define T_QUEUE_FUNC(name)  cid_##name
#include <t_queue.h>

typedef struct{
	T_ID cid;
	struct{
		Prod prod;
		int  prio;
		int  dfa_sid;
	}shift;
	struct{
		Prod prod;
		int  prio;
		int  dfa_sid;
	}reduce;
}Action;

T_HASH_ENTRY_DECL(int, Action, ActHashEntry);
typedef struct{
	T_HASH_DECL(ActHashEntry, h);
}ActHash;

#define T_HASH_TYPE       ActHash
#define T_HASH_ENTRY_TYPE ActHashEntry
#define T_HASH_KEY_TYPE   int
#define T_HASH_ELEM_TYPE  Action
#define T_HASH_NAME       h
#define T_HASH_FUNC(name) act_##name
#include <t_hash.h>

static T_Result
clos_add_rule(T_ParserDecl *decl, Closure *clos, T_ParserNonterm nt, T_ID reduce_sid)
{
	T_ID tid = T_PARSER_NONTERM_ID(nt);
	T_ParserRule *rule = rule_array_element(decl, tid);
	Prod prod;
	T_ID eid;
	T_Result r;

	prod.rule_id    = tid;
	prod.dot        = 0;
	prod.reduce_sid = reduce_sid;

	for(eid = 0; eid < rule->exprs.nmem; eid++){
		prod.expr_id = eid;

		T_DEBUG_I("add prod rule:%d expr:%d dot:%d reduce_sid:%d to closure %d",
					prod.rule_id, prod.expr_id, prod.dot, prod.reduce_sid,
					clos->dfa_sid);

		if((r = prod_array_add_unique(clos, &prod, NULL)) < 0)
			return r;
	}

	return T_OK;
}

static T_Result
solve_shift_reduce(T_ParserDecl *decl, Action *act, CIDQueue *q, ClosArray *c_array)
{
	int s_prio = T_PARSER_PRIO_VALUE(act->shift.prio);
	int r_prio = T_PARSER_PRIO_VALUE(act->reduce.prio);
	int s_dir  = T_PARSER_PRIO_DIR(act->shift.prio);
	int r_dir  = T_PARSER_PRIO_DIR(act->reduce.prio);
	T_Bool reduce = T_TRUE;
	T_Result r;

	if(r_prio < s_prio){
		reduce = T_FALSE;
	}else if(r_prio == s_prio){
		if((r_dir == T_PARSER_PRIO_RIGHT) && (s_dir == T_PARSER_PRIO_RIGHT))
			reduce = T_FALSE;
	}

	T_DEBUG_I("solve shift/reduce conflict");

	if(!reduce){
		Prod prod = act->shift.prod;
		Closure clos, *pclos;
		T_ID cid;

		if((clos.dfa_sid = t_auto_add_state(&decl->dfa, -1)) < 0){
			r = clos.dfa_sid;
			return r;
		}

		act->shift.dfa_sid = clos.dfa_sid;

		T_DEBUG_I("create closure %d", clos.dfa_sid);

		prod_array_init(&clos);

		if((cid = clos_array_add(c_array, &clos)) < 0){
			r = cid;
			return r;
		}

		prod_array_deinit(&clos);

		T_DEBUG_I("push closure %d", cid);
		if((r = cid_queue_push_back(q, &cid)) < 0)
			return r;

		prod.reduce_sid = act->reduce.dfa_sid;

		pclos = clos_array_element(c_array, cid);
		if((r = prod_array_add_unique(pclos, &prod, NULL)) < 0)
			return r;

		T_DEBUG_I("add prod rule:%d expr:%d dot:%d reduce_sid:%d to closure %d",
					prod.rule_id, prod.expr_id, prod.dot, prod.reduce_sid,
					act->shift.dfa_sid);

		act->reduce.dfa_sid = -1;
	}else{
		act->shift.dfa_sid = -1;
	}

	return T_OK;
}

static T_Result
clos_build(T_ParserDecl *decl, T_ID cid, CIDQueue *q, ClosArray *c_array)
{
	ActHash act_hash;
	ActHashEntry *act_hent;
	Action *act;
	Closure *clos;
	T_ParserRule *rule;
	T_ParserExpr *expr;
	T_ParserToken *tok;
	T_HashIter iter;
	T_ID pid;
	T_Result r;

	act_hash_init(&act_hash);

	clos = clos_array_element(c_array, cid);
	T_DEBUG_I("build closure %d", clos->dfa_sid);

	for(pid = 0; pid < prod_array_length(clos); pid++){
		Prod prod = *prod_array_element(clos, pid);
		T_Bool reduce;
		Closure *next_clos;
		T_AutoState *state;

		rule = rule_array_element(decl, prod.rule_id);
		expr = expr_array_element(rule, prod.expr_id);

		reduce = (prod.dot + 1 == expr->tokens.nmem);

		assert(prod.dot < expr->tokens.nmem);

		tok = token_array_element(expr, prod.dot);

		T_DEBUG_I("check prod rule:%d expr:%d dot:%d reduce_sid:%d symbol:%d",
					prod.rule_id, prod.expr_id, prod.dot, prod.reduce_sid, tok->token);

		if((r = act_hash_add_entry(&act_hash, &tok->token, &act_hent)) < 0)
			goto end;

		act = &act_hent->data;
		if(r > 0){
			act->cid = -1;
			act->shift.dfa_sid  = -1;
			act->reduce.dfa_sid = -1;
		}

		if(T_PARSER_NONTERM_ID(tok->token) == prod.rule_id){
			next_clos = clos_array_element(c_array, prod.reduce_sid);
		}else{
			if(act->cid >= 0){
				/*Get the closure.*/
				next_clos = clos_array_element(c_array, act->cid);
			}else{
				/*Add a new closure.*/
				Closure next_c;

				if((next_c.dfa_sid = t_auto_add_state(&decl->dfa, -1)) < 0){
					r = next_c.dfa_sid;
					goto end;
				}

				T_DEBUG_I("create closure %d", next_c.dfa_sid);

				prod_array_init(&next_c);

				if((act->cid = clos_array_add(c_array, &next_c)) < 0){
					r = act->cid;
					goto end;
				}

				prod_array_deinit(&next_c);

				T_DEBUG_I("push closure %d", act->cid);
				if((r = cid_queue_push_back(q, &act->cid)) < 0)
					goto end;

				next_clos = clos_array_element(c_array, act->cid);
				clos   = clos_array_element(c_array, cid);
			}
		}

		/*Test reduce/reduce conflict.*/
		state = &decl->dfa.states.buff[next_clos->dfa_sid];
		if((state->data != -1) && (tok->reduce != -1)){
			T_DEBUG_W("reduce/reduce conflict");
		}else if(tok->reduce != -1){
			state->data = tok->reduce;
		}

		/*Add the shift production to the next closure.*/
		if(!reduce){
			Prod next_prod;

			next_prod.rule_id    = prod.rule_id;
			next_prod.expr_id    = prod.expr_id;
			next_prod.dot        = prod.dot + 1;
			next_prod.reduce_sid = prod.reduce_sid;

			if((r = prod_array_add_unique(next_clos, &next_prod, NULL)) < 0)
				goto end;

			T_DEBUG_I("add prod rule:%d expr:%d dot:%d reduce_sid:%d to closure %d",
						next_prod.rule_id, next_prod.expr_id, next_prod.dot, next_prod.reduce_sid,
						next_clos->dfa_sid);
		}


		if(T_PARSER_IS_NONTERM(tok->token)){
			/*Solve the non terminated token.*/
			T_Bool recursive = T_FALSE;

			/*Recursive check.*/
			if(prod.dot == 0){
				if(T_PARSER_NONTERM_ID(tok->token) == prod.rule_id){
					recursive = T_TRUE;
				}
			}

			if(!recursive){
				/*Add non terminated toke rules to closure.*/
				if((r = clos_add_rule(decl, clos, tok->token, next_clos->dfa_sid)) < 0)
					goto end;
			}
		}else{
			/*Solve terminated token.*/
			if(reduce && (act->reduce.dfa_sid != -1)){
				T_DEBUG_I("reduce/reduce conflict");
			}else if(!reduce && (act->shift.dfa_sid != -1) && (act->shift.dfa_sid != next_clos->dfa_sid)){
				T_DEBUG_I("shift/shift conflict");
			}else if(reduce){
				/*Reduce.*/
				act->reduce.prio    = expr->prio;
				act->reduce.prod    = prod;
				act->reduce.dfa_sid = next_clos->dfa_sid;

				if(act->shift.dfa_sid != -1){
					if((r = solve_shift_reduce(decl, act, q, c_array)) < 0)
						goto end;
				}
			}else{
				/*Shift.*/
				act->shift.prio    = expr->prio;
				act->shift.prod    = prod;
				act->shift.dfa_sid = next_clos->dfa_sid;

				if(act->reduce.dfa_sid != -1){
					if((r = solve_shift_reduce(decl, act, q, c_array)) < 0)
						goto end;
				}
			}
		}
	}

	/*Add links.*/
	act_hash_iter_first(&act_hash, &iter);
	while(!act_hash_iter_last(&iter)){

		int symbol = *act_hash_iter_key(&iter);
		Action *act = act_hash_iter_data(&iter);

		if(act->shift.dfa_sid != -1){
			T_DEBUG_I("add shift edge from:%d to:%d symbol:%d", clos->dfa_sid, act->shift.dfa_sid, symbol);
			if((r = t_auto_add_edge(&decl->dfa, clos->dfa_sid, act->shift.dfa_sid, symbol)) < 0)
				goto end;
		}else if(act->reduce.dfa_sid != -1){
			T_DEBUG_I("add shift edge from:%d to:%d symbol:%d", clos->dfa_sid, act->reduce.dfa_sid, symbol);
			if((r = t_auto_add_edge(&decl->dfa, clos->dfa_sid, act->reduce.dfa_sid, symbol)) < 0)
				goto end;
			T_DEBUG_I("add reduce edge from:%d to:%d symbol:%d", act->reduce.dfa_sid, act->reduce.prod.reduce_sid, T_AUTO_REDUCE);
			if((r = t_auto_add_edge(&decl->dfa, act->reduce.dfa_sid, act->reduce.prod.reduce_sid, T_AUTO_REDUCE)) < 0)
				goto end;
		}

		act_hash_iter_next(&iter);
	}

	r = T_OK;
end:
	act_hash_deinit(&act_hash);
	return r;
}

T_Result
t_parser_decl_build(T_ParserDecl *decl)
{
	ClosArray c_array;
	CIDQueue c_queue;
	T_ID cid;
	Closure clos;
	T_ID sid;
	T_Result r;

	if(rule_array_length(decl) <= 0)
		return T_ERR_NOTINIT;

	if(decl->dfa.states.nmem != 1)
		return T_ERR_REINIT;

	T_DEBUG_I("build DFA, rules:%d", rule_array_length(decl));

	clos_array_init(&c_array);
	cid_queue_init(&c_queue);
	prod_array_init(&clos);

	/*Add closure 0.*/
	clos.dfa_sid = 0;
	if((r = clos_add_rule(decl, &clos, T_PARSER_NONTERM_MIN, 1)) < 0)
		goto end;

	if((cid = clos_array_add(&c_array, &clos)) < 0){
		r = T_ERR_NOMEM;
		goto end;
	}
	prod_array_init(&clos);

	/*Add end state.*/
	if((sid = t_auto_add_state(&decl->dfa, -1)) < 0)
		goto end;

	T_ASSERT(sid == 1);

	/*Add closure 0.*/
	clos.dfa_sid = sid;
	if((cid = clos_array_add(&c_array, &clos)) < 0){
		r = T_ERR_NOMEM;
		goto end;
	}
	prod_array_init(&clos);

	if((r = cid_queue_push_back(&c_queue, &cid)) < 0)
		goto end;

	/*Calulate closure 0.*/
	if((r = clos_build(decl, 0, &c_queue, &c_array)) < 0)
		goto end;

	/*Check all the closures.*/
	while(!cid_queue_empty(&c_queue)){
		cid_queue_pop_back(&c_queue, &cid);

		if((r = clos_build(decl, cid, &c_queue, &c_array)) < 0)
			goto end;
	}

	t_auto_dump(&decl->dfa);
	r = T_OK;
end:
	prod_array_deinit(&clos);
	clos_array_deinit(&c_array);
	cid_queue_deinit(&c_queue);
	return r;
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
	parser->reduce    = NULL;

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
t_parser_set_reduce(T_Parser *parser, T_ParserReduceFunc func, void *user_data)
{
	T_ASSERT(parser);

	parser->reduce = func;
	parser->user_data = user_data;
}

#define T_ALLOC_TYPE       T_Parser
#define T_ALLOC_INIT       t_parser_init
#define T_ALLOC_ARGS_DECL  T_ParserDecl *decl, T_Lex *lex
#define T_ALLOC_ARGS       decl, lex
#define T_ALLOC_DEINIT     t_parser_deinit
#define T_ALLOC_FUNC(name) t_parser_##name
#include <t_alloc.h>

T_Result
t_parser_parse(T_Parser *parser)
{
}

T_Result
t_parser_get_loc(T_Parser *parser, int n, T_LexLoc *loc)
{
	T_ParserValue *pv;

	T_ASSERT(parser && loc);

	if(n >= value_queue_length(parser))
		return T_ERR_ARG;

	pv = value_queue_back(parser, n);
	*loc = pv->loc;

	return T_OK;
}

T_Result
t_parser_get_value(T_Parser *parser, int n, void **value)
{
	T_ParserValue *pv;

	T_ASSERT(parser && value);

	if(n >= value_queue_length(parser))
		return T_ERR_ARG;

	pv = value_queue_back(parser, n);
	*value= pv->value;

	return T_OK;
}

