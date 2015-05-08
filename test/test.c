#include <tea.h>

enum{
	DIGIT = 256,
	SPACE
};

enum{
	EXPR = T_PARSER_NONTERM_MIN
};

enum{
	PRIO_ADD = T_PARSER_PRIO(0, T_PARSER_PRIO_LEFT),
	PRIO_SUB = T_PARSER_PRIO(0, T_PARSER_PRIO_LEFT),
	PRIO_MUL = T_PARSER_PRIO(1, T_PARSER_PRIO_LEFT),
	PRIO_DIV = T_PARSER_PRIO(1, T_PARSER_PRIO_LEFT)
};

enum{
	REDUCE_ADD = T_PARSER_REDUCE_MIN,
	REDUCE_SUB,
	REDUCE_MUL,
	REDUCE_DIV,
	REDUCE_DIGIT
};

int main(int argc, char **argv)
{
	T_LexDecl *l_decl;
	T_Lex *lex;
	T_ParserDecl *p_decl;
	T_Parser *parser;
	T_LexToken tok;
	T_LexLoc loc;

	l_decl = t_lex_decl_create();
	t_lex_decl_add_rule(l_decl, 0, "[[:digit:]]+", -1, DIGIT, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:digit:]]+\\.[[:digit:]]+", -1, DIGIT, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:space:]]+", -1, SPACE, 0, NULL);
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
	t_parser_decl_build(p_decl);

	lex = t_lex_create(l_decl);
	parser = t_parser_create(p_decl, lex);

	t_lex_push_text(lex, "100 + 3.14 * 2", -1, NULL, NULL);

	do{
		tok = t_lex_lex(lex, &loc);
		printf("token: %d, \"%s\" %d.%d-%d.%d\n",
					tok,
					t_lex_get_text(lex),
					loc.first_lineno, loc.first_column, loc.last_lineno, loc.last_column);
	}while(tok >= 0);

	t_parser_destroy(parser);
	t_parser_decl_destroy(p_decl);
	t_lex_destroy(lex);
	t_lex_decl_destroy(l_decl);
	return 0;
}

