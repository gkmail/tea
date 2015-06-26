#include <tea.h>

enum{
	SPACE,
	IF,
	ELSE,
	DO,
	WHILE,
	INT,
	FLOAT,
	ID,
	A,
	AAAA
};

const char*
name[] = {
	"space",
	"if",
	"else",
	"do",
	"while",
	"int",
	"float",
	"id",
	"a",
	"aaaa"
};

int main(int argc, char **argv)
{
	T_LexDecl *l_decl;
	T_Lex *lex;
	T_LexToken tok;
	T_LexLoc loc;

	l_decl = t_lex_decl_create();
	t_lex_decl_add_rule(l_decl, 0, "[[:space:]]+", -1, SPACE, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:digit:]]+", -1, INT, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:digit:]]+\\.[[:digit:]]+", -1, FLOAT, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"if\"", -1, IF, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"else\"", -1, ELSE, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"do\"", -1, DO, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"while\"", -1, WHILE, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"aaaa\"", -1, AAAA, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "\"a\"", -1, A, 0, NULL);
	t_lex_decl_add_rule(l_decl, 0, "[[:alpha:]_][[:alnum:]_]*", -1, ID, 0, NULL);
	t_lex_decl_build(l_decl);

	lex = t_lex_create(l_decl);

	t_lex_push_text(lex, "if 0 else do while 3.1415926 myvar _var _var1 __vv aaa", -1, NULL, NULL, NULL);

	do{
		tok = t_lex_lex(lex, &loc);
		if(tok == -1)
			break;
		printf("token: %d, %s, \"%s\" %d.%d-%d.%d\n",
					tok,
					name[tok],
					t_lex_get_text(lex),
					loc.first_lineno, loc.first_column, loc.last_lineno, loc.last_column);
	}while(tok >= 0);

	t_lex_destroy(lex);
	t_lex_decl_destroy(l_decl);

	return 0;
}
