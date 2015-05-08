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

#ifndef _T_PARSER_H_
#define _T_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define T_PARSER_NONTERM_MIN 10000
#define T_PARSER_PRIO_MIN    20000
#define T_PARSER_REDUCE_MIN  30000

#define T_PARSER_IS_TERM(n)    (((n)>=T_LEX_EOF) && ((n)<T_PARSER_NONTERM_MIN))
#define T_PARSER_IS_NONTERM(n) (((n)>=T_PARSER_NONTERM_MIN) && ((n)<T_PARSER_PRIO_MIN))
#define T_PARSER_IS_PRIO(n)    (((n)>=T_PARSER_PRIO_MIN) && ((n)<T_PARSER_REDUCE_MIN))
#define T_PARSER_IS_REDUCE(n)  ((n)>=T_PARSER_REDUCE_MIN)

#define T_PARSER_NONTERM(id)   ((id)+T_PARSER_NONTERM_MIN)
#define T_PARSER_NONTERM_ID(n) ((n)-T_PARSER_NONTERM_MIN)

#define T_PARSER_PRIO_LEFT  0
#define T_PARSER_PRIO_RIGHT 1
#define T_PARSER_PRIO(v, d)    (((v)*2+T_PARSER_PRIO_MIN)|(d))
#define T_PARSER_PRIO_DIR(p)   ((p)&1)
#define T_PARSER_PRIO_VALUE(p) ((p)&~1)

typedef int T_ParserTerm;
typedef int T_ParserNonterm;
typedef int T_ParserPrio;
typedef int T_ParserReduce;

typedef struct{
	int            token;
	T_ParserReduce reduce;
}T_ParserToken;

typedef struct{
	T_ARRAY_DECL(T_ParserToken, tokens);
	T_ParserPrio         prio;
}T_ParserExpr;

typedef struct{
	T_ARRAY_DECL(T_ParserExpr, exprs);
}T_ParserRule;

typedef struct{
	T_ARRAY_DECL(T_ParserRule, rules);
	T_Auto               dfa;
}T_ParserDecl;

typedef struct T_Parser_s T_Parser;
typedef T_Result (*T_ParserReduceFunc)(void *user_data, T_Parser *parser, T_ParserReduce reduce, void **value);

typedef struct{
	T_LexLoc            loc;
	void               *value;
}T_ParserValue;

struct T_Parser_s{
	T_Lex              *lex;
	T_ParserDecl       *decl;
	void               *user_data;
	T_ParserReduceFunc  reduce;
	T_QUEUE_DECL(T_ParserValue, stack);
};

extern T_Result      t_parser_decl_init(T_ParserDecl *decl);
extern void          t_parser_decl_deinit(T_ParserDecl *decl);
extern T_ParserDecl* t_parser_decl_create(void);
extern void          t_parser_decl_destroy(T_ParserDecl *decl);
extern T_Result      t_parser_decl_add_rule(T_ParserDecl *decl, T_ParserNonterm nonterm, ...);
extern T_Result      t_parser_decl_add_rulev(T_ParserDecl *decl, T_ParserNonterm nonterm, int *rule, int len);
extern T_Result      t_parser_decl_build(T_ParserDecl *decl);

extern T_Result      t_parser_init(T_Parser *parser, T_ParserDecl *decl, T_Lex *lex);
extern void          t_parser_deinit(T_Parser *parser);
extern T_Parser*     t_parser_create(T_ParserDecl *decl, T_Lex *lex);
extern void          t_parser_destroy(T_Parser *parser);
extern void          t_parser_set_reduce(T_Parser *parser, T_ParserReduceFunc func, void *user_data);
extern T_Result      t_parser_parse(T_Parser *parser);
extern T_Result      t_parser_get_loc(T_Parser *parser, int n, T_LexLoc *loc);
extern T_Result      t_parser_get_value(T_Parser *parser, int n, void **value);

#ifdef __cplusplus
}
#endif

#endif