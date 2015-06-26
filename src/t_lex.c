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

#define T_LEX_UNGET_SIZE 16
#define T_LEX_BUF_SIZE   2048

#define T_LEX_BOL -2

#define T_ARRAY_TYPE        T_LexDecl
#define T_ARRAY_ELEM_TYPE   T_Auto
#define T_ARRAY_ELEM_INIT   t_auto_init
#define T_ARRAY_ELEM_DEINIT t_auto_deinit
#define T_ARRAY_NAME        conds
#define T_ARRAY_FUNC(name)  cond_##name
#include <t_array.h>

T_Result
t_lex_decl_init(T_LexDecl *decl)
{
	T_ASSERT(decl);
	cond_array_init(decl);
	return T_OK;
}

void
t_lex_decl_deinit(T_LexDecl *decl)
{
	T_ASSERT(decl);
	cond_array_deinit(decl);
}

#define T_ALLOC_TYPE   T_LexDecl
#define T_ALLOC_INIT   t_lex_decl_init
#define T_ALLOC_DEINIT t_lex_decl_deinit
#define T_ALLOC_FUNC(name) t_lex_decl_##name
#include <t_alloc.h>

T_Result
t_lex_decl_add_rule(T_LexDecl *decl, T_LexCond cond, const char *re, int len, T_LexToken tok, T_ReFlags flags, int *errcol)
{
	T_Auto *aut;

	T_ASSERT(decl && re && (cond >= 0));

	aut = cond_array_element_resize(decl, cond);
	if(!aut)
		return T_ERR_NOMEM;

	return t_re_to_auto(aut, re, len, tok, flags, errcol);
}

T_Result
t_lex_decl_build(T_LexDecl *decl)
{
	T_ArrayIter iter;
	T_Result r;

	T_ASSERT(decl);

	cond_array_iter_first(decl, &iter);
	while(!cond_array_iter_last(&iter)){
		T_Auto *nfa, dfa;

		nfa = cond_array_iter_data(&iter);
		t_auto_init(&dfa);

		if((r = t_re_to_dfa(nfa, &dfa)) < 0){
			t_auto_deinit(&dfa);
			return r;
		}

		t_auto_deinit(nfa);
		*nfa = dfa;

		cond_array_iter_next(&iter);
	}

	return T_OK;
}

static T_LexInput*
lex_input_create(int flags)
{
	T_LexInput *inp;

	inp = (T_LexInput*)malloc(sizeof(T_LexInput));
	if(!inp)
		return NULL;

	inp->flags  = flags | T_LEX_FL_NEW_LINE;
	inp->lineno = 0;
	inp->column = 0;
	inp->last_column = 0;
	inp->b_pos  = 0;

	if(flags & T_LEX_FL_INTERNAL_BUF){
		inp->b_buf = (char*)malloc(T_LEX_BUF_SIZE);
		if(!inp->b_buf){
			free(inp);
			return NULL;
		}

		inp->b_size  = T_LEX_BUF_SIZE;
		inp->b_count = 0;
	}

	return inp;
}

static void
lex_input_destroy(T_LexInput *inp)
{
	if(inp->close)
		inp->close(inp->user_data);

	if(inp->flags & T_LEX_FL_INTERNAL_BUF){
		if(inp->b_buf)
			free(inp->b_buf);
	}

	free(inp);
}

#define T_ARRAY_TYPE       T_Lex
#define T_ARRAY_ELEM_TYPE  char
#define T_ARRAY_NAME       text
#define T_ARRAY_FUNC(name) text_##name
#include <t_array.h>

#define T_SLIST_TYPE        T_Lex
#define T_SLIST_ELEM_TYPE   T_LexInput
#define T_SLIST_ELEM_DEINIT lex_input_destroy
#define T_SLIST_NAME        inputs
#define T_SLIST_FUNC(name)  input_##name
#include <t_slist.h>

T_Result
t_lex_init(T_Lex *lex, T_LexDecl *decl)
{
	T_Result r;

	T_ASSERT(lex && decl);

	if(cond_array_length(decl) <= 0)
		return T_ERR_NOTINIT;

	lex->curr_cond = 0;
	lex->decl = decl;
	lex->more = T_FALSE;

	if((r = text_array_init(lex)) < 0)
		return r;

	if((r = input_slist_init(lex)) < 0){
		text_array_deinit(lex);
		return r;
	}

	return T_OK;
}

void
t_lex_deinit(T_Lex *lex)
{
	T_ASSERT(lex);

	text_array_deinit(lex);
	input_slist_deinit(lex);
}

#define T_ALLOC_TYPE       T_Lex
#define T_ALLOC_INIT       t_lex_init
#define T_ALLOC_DEINIT     t_lex_deinit
#define T_ALLOC_ARGS_DECL  T_LexDecl *decl
#define T_ALLOC_ARGS       decl
#define T_ALLOC_FUNC(name) t_lex_##name
#include <t_alloc.h>

T_Result
t_lex_push_text(T_Lex *lex, const char *text, int len, T_LexCloseFunc close, void *user_data, void *loc_data)
{
	T_LexInput *inp;

	T_ASSERT(lex && text);

	if(!(inp = lex_input_create(0)))
		return T_ERR_NOMEM;

	inp->b_buf  = (char*)text;
	inp->b_size = len;
	if(len < 0)
		inp->b_size = strlen(text);
	inp->b_count = inp->b_size;

	inp->input  = NULL;
	inp->close  = close;
	inp->user_data = user_data;
	inp->loc_data  = loc_data;

	input_slist_push(lex, inp);

	return T_OK;
}

T_Result
t_lex_push_input(T_Lex *lex, T_LexInputFunc input, T_LexCloseFunc close, void *user_data, void *loc_data)
{
	T_LexInput *inp;

	T_ASSERT(lex && input);

	if(!(inp = lex_input_create(T_LEX_FL_INTERNAL_BUF)))
		return T_ERR_NOMEM;

	inp->input  = input;
	inp->close  = close;
	inp->user_data = user_data;
	inp->loc_data  = loc_data;

	input_slist_push(lex, inp);

	return T_OK;
}

void
t_lex_pop_input(T_Lex *lex)
{
	T_ASSERT(lex);

	input_slist_pop(lex);
}

static int
lex_getch(T_Lex *lex)
{
	T_LexInput *inp;
	int ch;
	char c;
	T_Result r;

	inp = input_slist_top(lex);
	if(!inp)
		return T_LEX_EOF;

	if(inp->flags & T_LEX_FL_NEW_LINE){
		inp->lineno++;
		inp->last_column = inp->column;
		inp->column = 0;
		inp->flags &= ~T_LEX_FL_NEW_LINE;
		return T_LEX_BOL;
	}

	if(inp->b_count <= 0){
		if(inp->flags & T_LEX_FL_INTERNAL_BUF){
			r = inp->input(inp->user_data, inp->b_buf + T_LEX_UNGET_SIZE, inp->b_size - T_LEX_UNGET_SIZE);
			if(r < 0)
				return r;
			if(r == 0)
				return T_LEX_EOF;

			inp->b_count = r;
			inp->b_pos   = T_LEX_UNGET_SIZE;
		}else{
			return T_LEX_EOF;
		}
	}

	ch = inp->b_buf[inp->b_pos++];
	inp->b_count--;
	inp->column++;

	if(ch == '\n')
		inp->flags |= T_LEX_FL_NEW_LINE;

	c = ch;
	if((r = text_array_add(lex, &c)) < 0)
		return r;

	T_DEBUG_I("getch: \"%c\"", ch);

	return ch;
}

static void
lex_unget(T_Lex *lex, int ch)
{
	T_LexInput *inp;

	inp = input_slist_top(lex);
	if(!inp)
		return;

	if(ch == '\n'){
		inp->flags &= ~T_LEX_FL_NEW_LINE;
	}else if(ch == T_LEX_BOL){
		inp->lineno--;
		inp->column = inp->last_column;
		inp->flags |= T_LEX_FL_NEW_LINE;
	}

	if(ch >= 0){
		if(lex->text.nmem > 0)
			lex->text.nmem--;
		if(inp->b_pos > 0){
			inp->b_pos--;
			inp->b_count++;

			if(inp->flags & T_LEX_FL_INTERNAL_BUF)
				inp->b_buf[inp->b_pos] = ch;
		}
		if(inp->column > 0)
			inp->column--;
	}
}

T_LexToken
t_lex_lex(T_Lex *lex, T_LexLoc *loc)
{
	T_LexInput *inp;
	T_LexToken tok;
	T_Auto *dfa;
	T_ID sid, eid, eol_sid;
	T_AutoState *s;
	T_AutoEdge *e;
	int sym;
	int sym_cnt;
	int match_sym_cnt;
	int match_data;
	T_Bool have_bol;
	T_Bool first;

	T_ASSERT(lex);

retry:
	first = T_TRUE;
	sym_cnt = 0;
	match_sym_cnt = 0;
	match_data = -1;
	have_bol = T_FALSE;

	if(!lex->more)
		text_array_reinit(lex);

	dfa = cond_array_element(lex->decl, lex->curr_cond);
	sid = 0;

	T_DEBUG_I("lex");

	while(1){
next_state:
		sym = lex_getch(lex);

		if(first && (sym >= 0)){
			if(loc){
				inp = input_slist_top(lex);
				if(inp){
					loc->first_lineno = inp->lineno;
					loc->first_column = inp->column;
					loc->user_data    = inp->loc_data;
				}
			}

			first = T_FALSE;
		}

		T_DEBUG_I("state %d symbol %d", sid, sym);

		s = &dfa->states.buff[sid];

		if(s->data != -1){
			match_sym_cnt = sym_cnt;
			match_data = s->data;
			have_bol = T_FALSE;
		}

		if(sym >= 0)
			sym_cnt++;
		else if(sym == T_LEX_BOL)
			have_bol = T_TRUE;

		eid = s->edges;
		eol_sid = -1;

		while(eid != -1){
			e = &dfa->edges.buff[eid];

			if(e->symbol == T_RE_EOL){
				eol_sid = e->dest;
			}else if((e->symbol == T_RE_BOL) && (sym == T_LEX_BOL)){
				sid = e->dest;
				goto next_state;
			}else{
				int min = ((unsigned int)e->symbol) & 0xFFFF;
				int max = ((unsigned int)e->symbol) >> 16;

				if((sym >= min) && (sym <= max)){
					sid = e->dest;
					goto next_state;
				}
			}

			eid = e->next;
		}

		if(((sym == '\n') || (sym == T_LEX_EOF)) && (eol_sid != -1)){
			lex_unget(lex, sym);
			sid = eol_sid;
			goto next_state;
		}

		if(match_data != -1){
			int cnt = sym_cnt - match_sym_cnt;

			while(cnt--){
				sym = lex->text.buff[lex->text.nmem - 1];
				lex_unget(lex, sym);
			}

			if(have_bol){
				lex_unget(lex, T_LEX_BOL);
			}

			tok = match_data;
			break;
		}

		if((sym == T_LEX_EOF) && !sym_cnt){
			tok = T_LEX_EOF;
		}else{
			tok = T_ERR_SYNTAX;
		}
		break;
	}

	if(loc){
		inp = input_slist_top(lex);
		if(inp){
			if(first){
				loc->first_lineno = inp->lineno;
				loc->first_column = inp->column;
				loc->user_data    = inp->loc_data;
			}
			loc->last_lineno = inp->lineno;
			loc->last_column = inp->column;
		}
	}

	lex->more = T_FALSE;

	if(tok == 0)
		goto retry;

	return tok;
}

T_Result
t_lex_set_cond(T_Lex *lex, T_LexCond cond)
{
	T_ASSERT(lex);

	if((cond < 0) || (cond >= cond_array_length(lex->decl)))
		return T_ERR_ARG;

	lex->curr_cond = cond;
	return T_OK;
}

const char*
t_lex_get_text(T_Lex *lex)
{
	T_Result r;

	T_ASSERT(lex);

	if(lex->text.nmem >= lex->text.size){
		char chr = 0;
		
		if((r = text_array_add(lex, &chr)) < 0)
			return NULL;

		lex->text.nmem--;
	}else{
		lex->text.buff[lex->text.nmem] = 0;
	}

	return lex->text.buff;
}

void
t_lex_set_more(T_Lex *lex)
{
	T_ASSERT(lex);

	lex->more = T_TRUE;
}

int
t_lex_getch(T_Lex *lex)
{
	T_ASSERT(lex);

	return lex_getch(lex);
}

void
t_lex_unget(T_Lex *lex, int ch)
{
	T_ASSERT(lex);

	lex_unget(lex, ch);
}

