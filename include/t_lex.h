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

#ifndef _T_LEX_H_
#define _T_LEX_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int         T_LexCond;
typedef T_AutoData  T_LexToken;

#define T_LEX_EOF   -1

typedef struct{
	T_ARRAY_DECL(T_Auto, conds);
}T_LexDecl;

typedef struct{
	int        first_lineno;
	int        first_column;
	int        last_lineno;
	int        last_column;
	void      *user_data;
}T_LexLoc;

typedef struct T_LexInput_s T_LexInput;
typedef int  (*T_LexInputFunc)(void *user_data, char *buf, int size);
typedef void (*T_LexCloseFunc)(void *user_data);

#define T_LEX_FL_INTERNAL_BUF 1
#define T_LEX_FL_NEW_LINE     2

struct T_LexInput_s{
	T_SLIST_DECL(T_LexInput, inputs);
	int          lineno;
	int          column;
	int          last_column;
	int          flags;
	char        *b_buf;
	int          b_size;
	int          b_pos;
	int          b_count;
	T_LexInputFunc input;
	T_LexCloseFunc close;
	void          *user_data;
};

typedef struct{
	T_LexDecl   *decl;
	T_ID         curr_cond;
	T_Bool       more;
	T_SLIST_DECL(T_LexInput, inputs);
	T_ARRAY_DECL(char, text);
}T_Lex;

extern T_Result   t_lex_decl_init(T_LexDecl *decl);
extern void       t_lex_decl_deinit(T_LexDecl *decl);
extern T_LexDecl* t_lex_decl_create(void);
extern void       t_lex_decl_destroy(T_LexDecl *decl);
extern T_Result   t_lex_decl_add_rule(T_LexDecl *decl, T_LexCond cond, const char *re, int len, T_LexToken tok, T_ReFlags flags, int *errcol);
extern T_Result   t_lex_decl_build(T_LexDecl *decl);

extern T_Result   t_lex_init(T_Lex *lex, T_LexDecl *decl);
extern void       t_lex_deinit(T_Lex *lex);
extern T_Lex*     t_lex_create(T_LexDecl *decl);
extern void       t_lex_destroy(T_Lex *lex);
extern T_Result   t_lex_push_text(T_Lex *lex, const char *text, int len, T_LexCloseFunc close, void *user_data);
extern T_Result   t_lex_push_input(T_Lex *lex, T_LexInputFunc input, T_LexCloseFunc close, void *user_data);
extern void       t_lex_pop_input(T_Lex *lex);
extern T_LexToken t_lex_lex(T_Lex *lex, T_LexLoc *loc);
extern T_Result   t_lex_set_cond(T_Lex *lex, T_LexCond cond);
extern const char* t_lex_get_text(T_Lex *lex);
extern void       t_lex_set_more(T_Lex *lex);

#ifdef __cplusplus
}
#endif

#endif
