#include <tea.h>

enum{
	DIGIT = 256
};

enum{
	EXPR = T_PARSER_NONTERM_BEGIN
};

enum{
	PRIO_ADD = T_PARSER_PRIO(0, T_PARSER_PRIO_LEFT),
	PRIO_SUB = T_PARSER_PRIO(0, T_PARSER_PRIO_LEFT),
	PRIO_MUL = T_PARSER_PRIO(1, T_PARSER_PRIO_LEFT),
	PRIO_DIV = T_PARSER_PRIO(1, T_PARSER_PRIO_LEFT)
};

enum{
	REDUCE_ADD = T_PARSER_REDUCE_BEGIN,
	REDUCE_SUB,
	REDUCE_MUL,
	REDUCE_DIV,
	REDUCE_DIGIT
};

static T_LexToken
value_func(void *udata, T_Parser *parser, T_LexToken tok, void **val)
{
	const char *str;
	float f;

	switch(tok){
		case DIGIT:
			str = t_lex_get_text(parser->lex);
			f = strtof(str, NULL);
			*(float*)val = f;
			break;
	}

	return tok;
}

static T_Result
reduce_func(void *udata, T_Parser *parser, T_ParserReduce reduce, void **val)
{
	float f1, f2, fr;
	float *pf;
	void *v;

	switch(reduce){
		case REDUCE_DIGIT:
			t_parser_get_value(parser, 0, &v);
			*val = v;
			pf = (float*)&v;
			printf("number: %f\n", *pf);
			break;
		case REDUCE_ADD:
			t_parser_get_value(parser, 0, &v);
			pf = (float*)&v;
			f1 = *pf;
			t_parser_get_value(parser, 2, &v);
			pf = (float*)&v;
			f2 = *pf;
			fr = f1 + f2;
			*(float*)val = fr;
			printf("%f + %f = %f\n", f1, f2, fr);
			break;
		case REDUCE_SUB:
			t_parser_get_value(parser, 0, &v);
			pf = (float*)&v;
			f1 = *pf;
			t_parser_get_value(parser, 2, &v);
			pf = (float*)&v;
			f2 = *pf;
			fr = f1 - f2;
			*(float*)val = fr;
			printf("%f - %f = %f\n", f1, f2, fr);
			break;
		case REDUCE_MUL:
			t_parser_get_value(parser, 0, &v);
			pf = (float*)&v;
			f1 = *pf;
			t_parser_get_value(parser, 2, &v);
			pf = (float*)&v;
			f2 = *pf;
			fr = f1 * f2;
			*(float*)val = fr;
			printf("%f * %f = %f\n", f1, f2, fr);
			break;
		case REDUCE_DIV:
			t_parser_get_value(parser, 0, &v);
			pf = (float*)&v;
			f1 = *pf;
			t_parser_get_value(parser, 2, &v);
			pf = (float*)&v;
			f2 = *pf;
			fr = f1 / f2;
			*(float*)val = fr;
			printf("%f / %f = %f\n", f1, f2, fr);
			break;
	}

	return T_OK;
}

static void
error_func(void *udata, T_Parser *parser)
{
	T_LexLoc *loc = &parser->fetched_v.loc;

	printf("error: %d.%d-%d.%d\n", loc->first_lineno, loc->first_column, loc->last_lineno, loc->last_column);
}

int main(int argc, char **argv)
{
	T_LexDecl *l_decl;
	T_Lex *lex;
	T_ParserDecl *p_decl;
	T_Parser *parser;

	l_decl = t_lex_decl_create();
	t_lex_decl_add_rule(l_decl, 0, "[[:digit:]]+", -1, DIGIT, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:digit:]]+\\.[[:digit:]]+", -1, DIGIT, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:space:]]+", -1, 0, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"+\"", -1, '+', 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"-\"", -1, '-', 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"*\"", -1, '*', 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"/\"", -1, '/', 0, NULL);
	t_lex_decl_build(l_decl);

	p_decl = t_parser_decl_create();
	t_parser_decl_add_rule(p_decl, EXPR, EXPR, '+', EXPR, REDUCE_ADD, PRIO_ADD, -1);
	t_parser_decl_add_rule(p_decl, EXPR, EXPR, '-', EXPR, REDUCE_SUB, PRIO_SUB, -1);
	t_parser_decl_add_rule(p_decl, EXPR, EXPR, '*', EXPR, REDUCE_MUL, PRIO_MUL, -1);
	t_parser_decl_add_rule(p_decl, EXPR, EXPR, '/', EXPR, REDUCE_DIV, PRIO_DIV, -1);
	t_parser_decl_add_rule(p_decl, EXPR, DIGIT, REDUCE_DIGIT, -1);
	t_parser_decl_add_rule(p_decl, EXPR, T_PARSER_NONTERM_ERR, EXPR, REDUCE_DIGIT, -1);
	t_parser_decl_build(p_decl);

	lex = t_lex_create(l_decl);
	parser = t_parser_create(p_decl, lex);
	t_parser_set_func(parser, value_func, reduce_func, error_func, NULL);

	t_lex_push_text(lex, "100 + 3.14 * 2 - 100 / 50", -1, NULL, NULL);
	t_parser_parse(parser);

	t_parser_destroy(parser);
	t_parser_decl_destroy(p_decl);
	t_lex_destroy(lex);
	t_lex_decl_destroy(l_decl);

	return 0;
}

