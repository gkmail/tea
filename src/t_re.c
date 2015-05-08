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

typedef enum{
	RE_PARSE_CHAR,
	RE_PARSE_STRING,
	RE_PARSE_SET
}ReParseStatus;

typedef struct{
	const char *str;
	int         len;
	int         pos;
	T_ReFlags   flags;
}ReInput;

typedef struct{
	T_SET_DECL(int, set);
}SymbolSet;

#define T_SET_TYPE       SymbolSet
#define T_SET_ELEM_TYPE  int
#define T_SET_NAME       set
#define T_SET_FUNC(name) symbol_##name
#include <t_set.h>

typedef struct{
	char *name;
	char *re;
}RePreDefSet;

static const RePreDefSet
predef_sets[] = {
	{"alnum", "[a-zA-Z0-9]"},
	{"alpha", "[a-zA-Z]"},
	{"digit", "[0-9]"},
	{"lower", "[a-z]"},
	{"upper", "[A-Z]"},
	{"blank", "[ \\t]"},
	{"cntrl", "[\\x00-\\x1f\\x7f-\\xff]"},
	{"graph", "[\\x21-\\x7e]"},
	{"print", "[\\x20-\\x7e]"},
	{"punct", "[\\x21-\\x2f\\x3a-\\x40\\x5b-\\x60\\x7b-\\x7e]"},
	{"space", "[ \\f\\n\\r\\t\\v]"},
	{"xdigit","0-9a-fA-F"},
	{NULL, NULL}
};

static T_Result re_parse(ReInput *inp, T_Auto *aut, T_AutoMach *mach);
static T_Result re_parse_set(ReInput *inp, SymbolSet *set);

static int
char_to_hex(int ch)
{
	if((ch >= '0') && (ch <= '9'))
		return ch - '0';
	else if((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	else
		return ch - 'A' + 10;
}

static void
re_input_init(ReInput *inp, const char *str, int len)
{
	inp->str = str;
	if(len == -1)
		inp->len = strlen(str);
	else
		inp->len = len;
	inp->pos = 0;
}

static T_INLINE int
re_getch(ReInput *inp)
{
	if(inp->pos >= inp->len)
		return -1;

	return inp->str[inp->pos++];
}

static T_INLINE void
re_unget(ReInput *inp, int ch)
{
	if(ch < 0)
		return;

	T_ASSERT(inp->pos > 0);
	inp->pos--;
}

static int
re_parse_char(ReInput *inp, ReParseStatus status)
{
	int ch;
	int r = T_ERR_SYNTAX;

	ch = re_getch(inp);
	switch(ch){
		case '\\':
			ch = re_getch(inp);
			switch(ch){
				case 'a':
					r = '\a';
					break;
				case 'b':
					r = '\b';
					break;
				case 'f':
					r = '\f';
					break;
				case 'n':
					r = '\n';
					break;
				case 'r':
					r = '\r';
					break;
				case 't':
					r = '\t';
					break;
				case 'v':
					r = '\v';
					break;
				case 'x':
					ch = re_getch(inp);
					if(!isxdigit(ch))
						return T_ERR_SYNTAX;
					r = char_to_hex(ch) * 16;
					ch = re_getch(inp);
					if(!isxdigit(ch))
						return T_ERR_SYNTAX;
					r += char_to_hex(ch);	
					break;
				case '0':
					r = 0;
					break;
				case '1'...'9':
					r = ch - '0';
					ch = re_getch(inp);
					if(!isdigit(ch)){
						re_unget(inp, ch);
						T_DEBUG_I("CHAR \"%c\"", r);
						return r;
					}
					r *= 10;
					r += ch - '0';
					ch = re_getch(inp);
					if(!isdigit(ch)){
						re_unget(inp, ch);
						T_DEBUG_I("CHAR \"%c\"", r);
						return r;
					}
					if(r * 10 + ch - '0' > 255){
						re_unget(inp, ch);
						T_DEBUG_I("CHAR \"%c\"", r);
						return r;
					}
					r *= 10;
					r += ch - '0';
					break;
				default:
					if(isprint(ch)){
						r = ch;
					}
					break;
			}
			break;
		default:
			if(isprint(ch)){
				if((status == RE_PARSE_STRING) && (ch != '\"')){
					r = ch;
				}else if((status == RE_PARSE_SET) && (ch != '-') && (ch != ']')){
					r = ch;
				}else{
					switch(ch){
						case '(':
						case ')':
						case '|':
						case '+':
						case '*':
						case '?':
						case '[':
						case ']':
						case '{':
						case '}':
						case '^':
						case '$':
						case '\"':
							re_unget(inp, ch);
							break;
						default:
							r = ch;
							break;
					}
				}
			}
			break;
	}

	if(r >= 0)
		T_DEBUG_I("CHAR \"%c\"", r);

	return r;
}

static T_Result
re_parse_predef_set(ReInput *inp, SymbolSet *set)
{
	const char *begin;
	int len;
	int ch;
	int r;
	const RePreDefSet *pd;

	ch = re_getch(inp);
	if(ch != ':')
		return T_ERR_SYNTAX;

	begin = inp->str + inp->pos;

	do{
		ch = re_getch(inp);
		if(!isalpha(ch))
			break;
	}while(1);

	len = inp->str + inp->pos - begin - 1;

	if(ch != ':')
		return T_ERR_SYNTAX;

	ch = re_getch(inp);
	if(ch != ']')
		return T_ERR_SYNTAX;

	r = T_ERR_SYNTAX;
	pd = predef_sets;
	while(pd->name){
		int plen = strlen(pd->name);

		if((len == plen) && !strncmp(begin, pd->name, len)){
			ReInput pd_inp;

			re_input_init(&pd_inp, pd->re, -1);

			T_DEBUG_I("[:%s:]", pd->name);
			r = re_parse_set(&pd_inp, set);
			break;
		}

		pd++;
	}

	return r;
}

static T_Result
re_parse_set(ReInput *inp, SymbolSet *set)
{
	T_Result r;
	int ch;
	T_Bool neg = T_FALSE;

	ch = re_getch(inp);
	if(ch != '[')
		return T_ERR_SYNTAX;

	T_DEBUG_I("SET [");

	ch = re_getch(inp);
	if(ch == '^'){
		T_DEBUG_I("NEG ^");
		neg = T_TRUE;
		ch = re_getch(inp);
	}

	do{
		if(ch == ']')
			break;
		else if(ch == -1)
			return T_ERR_SYNTAX;

		if(ch == '['){
			if((r = re_parse_predef_set(inp, set)) < 0)
				return r;

			ch = re_getch(inp);
		}else{
			int c1, c2;

			re_unget(inp, ch);
			if((c1 = re_parse_char(inp, RE_PARSE_SET)) < 0)
				return c1;

			ch = re_getch(inp);
			if(ch == '-'){
				int c;

				T_DEBUG_I("RANGE -");

				if((c2 = re_parse_char(inp, RE_PARSE_SET)) < 0)
					return c2;

				if(c1 > c2){
					T_DEBUG_E("illegal charset");
					return T_ERR_SYNTAX;
				}

				for(c = c1; c <= c2; c++){
					if((r = symbol_set_add_not_check(set, &c)) < 0)
						return r;
				}

				symbol_set_sort(set);

				ch = re_getch(inp);
			}else{
				if((r = symbol_set_add(set, &c1, NULL)) < 0)
					return r;
			}
		}
	}while(1);

	if(ch != ']')
		return T_ERR_SYNTAX;

	T_DEBUG_I("SET ]");

	/*Negate*/
	if(neg){
		SymbolSet nset;
		int c;

		symbol_set_init(&nset);
		for(c = 0; c < 256; c++){
			if(symbol_set_search(set, &c) == -1){
				if((r = symbol_set_add_not_check(&nset, &c)) < 0){
					symbol_set_deinit(&nset);
					return r;
				}
			}
		}

		symbol_set_deinit(set);
		*set = nset;
	}

	return T_OK;
}

static T_Result
re_parse_full_set(ReInput *inp, T_Auto *aut, T_AutoMach *mach)
{
	SymbolSet s1, s2;
	T_Result r;
	int ch;
	int op;

	symbol_set_init(&s1);
	symbol_set_init(&s2);

	if((r = re_parse_set(inp, &s1)) < 0)
		goto end;

	ch = re_getch(inp);
	if(ch == '{'){
		op = re_getch(inp);
		if((op != '+') && (op != '-')){
			re_unget(inp, op);
			re_unget(inp, ch);
			goto end;
		}

		T_DEBUG_I("OP %c", op);

		ch = re_getch(inp);
		if(ch != '}'){
			r = T_ERR_SYNTAX;
			goto end;
		}
	}else{
		re_unget(inp, ch);
		goto end;
	}

	if((r = re_parse_set(inp, &s2)) < 0)
		goto end;

	if(op == '+'){
		r = symbol_set_union(&s1, &s2);
	}else{
		r = symbol_set_diff(&s1, &s2);
	}

end:

	if(r >= 0){
		r = t_auto_mach_add_symbols(aut, mach, symbol_set_element(&s1, 0), symbol_set_length(&s1), inp->flags);
	}

	symbol_set_deinit(&s1);
	symbol_set_deinit(&s2);

	return r;
}

static T_Result
re_parse_str(ReInput *inp, T_Auto *aut, T_AutoMach *mach)
{
	int ch;
	T_Result r;

	T_DEBUG_I("\"");

	if((ch = re_parse_char(inp, RE_PARSE_STRING)) < 0)
		return ch;

	if((r = t_auto_mach_add_symbol(aut, mach, ch, inp->flags)) < 0)
		return r;

	do{
		ch = re_getch(inp);
		if(ch == '\"')
			break;
		re_unget(inp, ch);

		if((ch = re_parse_char(inp, RE_PARSE_STRING)) < 0)
			return ch;

		if((r = t_auto_mach_add_symbol(aut, mach, ch, inp->flags)) < 0)
			return r;
	}while(1);

	T_DEBUG_I("\"");

	return T_OK;
}

static int
re_parse_num(ReInput *inp)
{
	int ch;
	int v;

	ch = re_getch(inp);
	if(!isdigit(ch))
		return T_ERR_SYNTAX;
	v = ch - '0';

	do{
		ch = re_getch(inp);
		if(!isdigit(ch)){
			re_unget(inp, ch);
			break;
		}

		v *= 10;
		v += ch - '0';
	}while(1);

	T_DEBUG_I("NUM: %d", v);

	return v;
}

static T_Result
re_parse_repeat(ReInput *inp, T_Auto *aut, T_AutoMach *mach)
{
	int min, max;
	int ch;
	int r;

	if((min = re_parse_num(inp)) < 0)
		return min;

	ch = re_getch(inp);
	if(ch == ','){
		ch = re_getch(inp);
		if(ch == '}'){
			max = -1;
		}else{
			re_unget(inp, ch);

			if((max = re_parse_num(inp)) < 0)
				return max;

			ch = re_getch(inp);
			if(ch != '}')
				return T_ERR_SYNTAX;
		}
	}else if(ch == '}'){
		max = min;
	}else{
		return T_ERR_SYNTAX;
	}

	T_DEBUG_I("REPEAT %d,%d", min, max);

	if((r = t_auto_mach_repeat(aut, mach, min, max)) < 0)
		return r;

	return T_OK;
}

static T_Result
re_parse_single(ReInput *inp, T_Auto *aut, T_AutoMach *mach)
{
	T_Result r;
	int ch;

	t_auto_mach_init(mach);

	ch = re_getch(inp);
	switch(ch){
		case '[':
			re_unget(inp, ch);
			if((r = re_parse_full_set(inp, aut, mach)) < 0)
				return r;
			break;
		case '\"':
			if((r = re_parse_str(inp, aut, mach)) < 0)
				return r;
			break;
		case '.':
			T_DEBUG_I(".");
			if((r = t_auto_mach_add_symbol_range(aut, mach, 0, 255, inp->flags)) < 0)
				return r;
			break;
		case '(':
			T_DEBUG_I("(");
			if((r = re_parse(inp, aut, mach)) < 0)
				return r;
			if((ch = re_getch(inp)) != ')')
				return T_ERR_SYNTAX;
			T_DEBUG_I(")");
			break;
		case -1:
			return T_ERR_SYNTAX;
		default:
			re_unget(inp, ch);
			if((ch = re_parse_char(inp, RE_PARSE_CHAR)) < 0)
				return ch;
			if((r = t_auto_mach_add_symbol(aut, mach, ch, inp->flags)) < 0)
				return r;
			break;
	}

	ch = re_getch(inp);
	switch(ch){
		case '?':
			T_DEBUG_I("REPEAT ?");
			if((r = t_auto_mach_repeat(aut, mach, 0, 1)) < 0)
				return r;
			break;
		case '+':
			T_DEBUG_I("REPEAT +");
			if((r = t_auto_mach_repeat(aut, mach, 1, -1)) < 0)
				return r;
			break;
		case '*':
			T_DEBUG_I("REPEAT *");
			if((r = t_auto_mach_repeat(aut, mach, 0, -1)) < 0)
				return r;
			break;
		case '{':
			if((r = re_parse_repeat(inp, aut, mach)) < 0)
				return r;
			break;
		default:
			re_unget(inp, ch);
	}

	return T_OK;
}

static T_Result
re_parse_series(ReInput *inp, T_Auto *aut, T_AutoMach *mach)
{
	T_Result r;
	int ch;

	if((r = re_parse_single(inp, aut, mach)) < 0)
		return r;

	while(1){
		T_AutoMach mnext;

		ch = re_getch(inp);
		re_unget(inp, ch);

		if((ch == '|') || (ch == ')') || (ch == -1))
			break;

		if((r = re_parse_single(inp, aut, &mnext)) < 0)
			return r;

		if((r = t_auto_mach_link(aut, mach, &mnext)) < 0)
			return r;
	}

	return T_OK;
}

static T_Result
re_parse(ReInput *inp, T_Auto *aut, T_AutoMach *mach)
{
	T_Result r;
	int ch;

	if((r = re_parse_series(inp, aut, mach)) < 0)
		return r;

	while(1){
		ch = re_getch(inp);
		if(ch == '|'){
			T_AutoMach mnext;

			T_DEBUG_I("|");

			if((r = re_parse_series(inp, aut, &mnext)) < 0)
				return r;

			if((r = t_auto_mach_or(aut, mach, &mnext)) < 0)
				return r;
		}else{
			re_unget(inp, ch);
			break;
		}
	}

	return T_OK;
}

static T_Result
re_parse_full(ReInput *inp, T_Auto *aut, T_AutoData data)
{
	T_AutoMach mach;
	T_Bool bol, eol;
	int ch;
	T_ID s, e;
	T_Result r;

	if((ch = re_getch(inp)) < 0)
		return T_ERR_SYNTAX;

	if(ch == '^'){
		T_DEBUG_I("^");
		bol = T_TRUE;
	}else{
		bol = T_FALSE;
		re_unget(inp, ch);
	}

	if((r = re_parse(inp, aut, &mach)) < 0)
		return r;

	ch = re_getch(inp);
	if(ch == '$'){
		T_DEBUG_I("$");
		eol = T_TRUE;
	}else{
		eol = T_FALSE;
		re_unget(inp, ch);
	}

	if(inp->pos != inp->len)
		return T_ERR_SYNTAX;

	if(bol || !(inp->flags & T_AUTO_FL_NO_BOL)){
		if((e = t_auto_add_edge(aut, 0, mach.s_begin, T_AUTO_BOL)) < 0)
			return e;
	}

	if(!bol){
		if((e = t_auto_add_edge(aut, 0, mach.s_begin, T_AUTO_EPSILON)) < 0)
			return e;
	}

	if(eol){
		if((s = t_auto_add_state(aut, data)) < 0)
			return s;
		if((e = t_auto_add_edge(aut, mach.s_end, s, T_AUTO_BOL)) < 0)
			return e;
	}else{
		T_AutoState *se = t_auto_get_state(aut, mach.s_end);
		se->data = data;
	}

	return T_OK;
}

T_Result
t_re_to_auto(T_Auto *aut, const char *str, int len, T_AutoData data, T_ReFlags flags, int *errcol)
{
	ReInput inp;
	T_Result r;

	T_ASSERT(aut && str);

	re_input_init(&inp, str, len);
	inp.flags = flags;

	if((r = re_parse_full(&inp, aut, data)) < 0){
		if(errcol){
			*errcol = inp.pos;
		}
		return r;
	}

	return T_OK;
}

T_Re*
t_re_create(const char *str, int len, T_ReFlags flags, int *errcol)
{
	T_Auto aut;
	T_Re *re = NULL;

	T_ASSERT(str);

	t_auto_init(&aut);

	if(t_re_to_auto(&aut, str, len, 0, flags, errcol) < 0)
		goto error;

	if(!(re = t_auto_create()))
		goto error;

	if(t_auto_to_dfa(&aut, re) < 0)
		goto error;

	t_auto_deinit(&aut);

	return re;
error:
	if(re)
		t_auto_destroy(re);
	t_auto_deinit(&aut);
	return NULL;
}
		
void
t_re_destroy(T_Re *re)
{
	t_auto_destroy(re);
}

static T_AutoData
re_match(T_Re *re, const char *str, int len, const char **estr, T_Bool bol)
{
	ReInput inp;
	int ch;
	T_ID sid, eid;
	T_AutoState *s;
	T_AutoEdge *e;

	T_DEBUG_I("match \"%s\" len %d", str, len);

	re_input_init(&inp, str, len);

	sid = 0;

	while(1){
next_state:
		s = &re->states.buff[sid];

		T_DEBUG_I("state: %d", sid);

		if(bol){
			ch  = T_AUTO_BOL;
			bol = T_FALSE;
		}else{
			ch  = re_getch(&inp);
		}

		T_DEBUG_I("symbol: %d", ch);

		eid = s->edges;
		while(eid != -1){
			e = &re->edges.buff[eid];

			if(e->symbol == ch){
				sid = e->dest;
				goto next_state;
			}else if((e->symbol == T_AUTO_EOL) && (ch == -1)){
				sid = e->dest;
				goto next_state;
			}

			eid = e->next;
		}

		re_unget(&inp, ch);

		if(s->data != -1){
			*estr = str + inp.pos;
			return s->data;
		}

		break;
	}

	return -1;
}

T_Result
t_re_match(T_Re *re, const char *str, int len, int *start, int *end)
{
	const char *ptr = str;
	const char *estr;
	int left = len;
	T_AutoData data;
	T_Bool bol = T_TRUE;

	T_ASSERT(re && str);

	if(left == -1)
		left = strlen(str);

	while(left){
		data = re_match(re, ptr, left, &estr, bol);
		if(data != -1){
			if(start)
				*start = ptr - str;
			if(end)
				*end = estr - str;
			return 1;
		}

		ptr++;
		left--;
		bol = T_FALSE;
	}

	return 0;
}

