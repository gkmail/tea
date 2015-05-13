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
	T_ARRAY_DECL(int, a);
}SymbolArray;
#define T_ARRAY_TYPE       SymbolArray
#define T_ARRAY_ELEM_TYPE  int
#define T_ARRAY_NAME       a
#define T_ARRAY_FUNC(name) symbol_##name
#include <t_array.h>

#define T_SYM_MAKE(min, max)  ((((unsigned int)(max))<<16)|((unsigned int)(min)))
#define T_SYM_MAX(sym)        (((unsigned int)(sym))>>16)
#define T_SYM_MIN(sym)        (((unsigned int)(sym))&0xFFFF)

typedef SymbolArray CSet;

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

static T_Result re_parse(ReInput *inp, T_Re *re, T_ReMach *mach);
static T_Result re_parse_set(ReInput *inp, CSet *set);

static T_INLINE T_Result
cset_init(CSet *cset)
{
	return symbol_array_init(cset);
}

static T_INLINE void
cset_deinit(CSet *cset)
{
	symbol_array_deinit(cset);
}

static T_Bool
cset_has_char(CSet *cset, int chr)
{
	T_ArrayIter iter;

	symbol_array_iter_first(cset, &iter);
	while(!symbol_array_iter_last(&iter)){
		int data = *symbol_array_iter_data(&iter);
		int min = T_SYM_MIN(data);
		int max = T_SYM_MAX(data);

		if((chr >= min) && (chr <= max))
			return T_TRUE;

		if(chr < min)
			return T_FALSE;

		symbol_array_iter_next(&iter);
	}

	return T_FALSE;
}

static T_Result
cset_add_char_range(CSet *cset, int min, int max)
{
	int id;
	int data;
	int old_data;
	int old_min;
	int old_max;
	int len = symbol_array_length(cset);
	int pos, cnt, left;
	T_Result r;

	T_ASSERT(min <= max);

	if(len == 0){
		data = T_SYM_MAKE(min, max);
		if((r = symbol_array_add(cset, &data)) < 0)
			return r;

		return T_OK;
	}

	pos = -1;
	for(id = 0; id < len; id++){
		old_data = *symbol_array_element(cset, id);
		old_min = T_SYM_MIN(old_data);
		old_max = T_SYM_MAX(old_data);

		if(min < old_min){
			data = T_SYM_MAKE(min, max);
			if((r = symbol_array_insert(cset, id, &data)) < 0)
				return r;
			pos = id + 1;
			break;
		}else if(min <= (old_max + 1)){
			if(max <= old_max)
				return T_OK;

			data = T_SYM_MAKE(old_min, max);
			*symbol_array_element(cset, id) = data;
			pos = id + 1;
			break;
		}
	}

	if(pos == -1){
		data = T_SYM_MAKE(min, max);
		if((r = symbol_array_add(cset, &data)) < 0)
			return r;

		return T_OK;
	}

	len = symbol_array_length(cset);
	for(id = pos; id < len; id++){
		old_data = *symbol_array_element(cset, id);
		old_min = T_SYM_MIN(old_data);
		old_max = T_SYM_MAX(old_data);

		if(old_min > max + 1)
			break;

		if(old_max > max){
			old_data = *symbol_array_element(cset, pos - 1);
			old_min = T_SYM_MIN(old_data);
			data = T_SYM_MAKE(old_min, old_max);
			*symbol_array_element(cset, pos - 1) = data;
		}
	}

	cnt  = id - pos;
	left = len - id;

	if(cnt > 0){
		if(left > 0)
			memmove(cset->a.buff + pos, cset->a.buff + pos + cnt, left * sizeof(int));

		cset->a.nmem -= cnt;
	}

	return T_OK;
}

static T_Result
cset_add_char(CSet *cset, int chr)
{
	return cset_add_char_range(cset, chr, chr);
}

static T_Result
cset_union(CSet *c1, CSet *c2)
{
	T_ArrayIter iter;
	T_Result r;

	symbol_array_iter_first(c2, &iter);
	while(!symbol_array_iter_last(&iter)){
		int data = *symbol_array_iter_data(&iter);
		int min = T_SYM_MIN(data);
		int max = T_SYM_MAX(data);

		if((r = cset_add_char_range(c1, min, max)) < 0)
			return r;
		
		symbol_array_iter_next(&iter);
	}

	return T_OK;
}

static T_Result
cset_diff(CSet *c1, CSet *c2)
{
	CSet cr;
	T_SetIter iter, iter2;
	T_Result r;

	cset_init(&cr);

	symbol_array_iter_first(c1, &iter);
	while(!symbol_array_iter_last(&iter)){
		int data1 = *symbol_array_iter_data(&iter);
		int min1 = T_SYM_MIN(data1);
		int max1 = T_SYM_MAX(data1);

		symbol_array_iter_first(c2, &iter2);
		while(!symbol_array_iter_last(&iter2)){
			int data2 = *symbol_array_iter_data(&iter2);
			int min2 = T_SYM_MIN(data2);
			int max2 = T_SYM_MAX(data2);

			if(max1 < min2)
				break;

			if(min1 <= min2){
				if(min1 < min2){
					if((r = cset_add_char_range(&cr, min1, min2 - 1)) < 0)
						goto end;
				}
				min1 = max2 + 1;
			}

			if(min1 > max1)
				break;

			symbol_array_iter_next(&iter2);
		}

		if(min1 <= max1){
			if((r = cset_add_char_range(&cr, min1, max1)) < 0)
				goto end;
		}

		symbol_array_iter_next(&iter);
	}

	cset_deinit(c1);
	*c1 = cr;
	cset_init(&cr);
	r = T_OK;
end:
	cset_deinit(&cr);

	return r;
}

static T_Result
cset_neg(CSet *c)
{
	CSet cr;
	T_Result r;

	cset_init(&cr);

	if((r = cset_add_char_range(&cr, T_RE_CHAR_MIN, T_RE_CHAR_MAX)) < 0)
		goto end;

	if((r = cset_diff(&cr, c)) < 0)
		goto end;

	cset_deinit(c);
	*c = cr;
	cset_init(&cr);
	r = T_OK;
end:
	cset_deinit(&cr);
	return r;
}

static void
mach_join(T_ReMach *m1, T_ReMach *m2)
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
mach_add_char(T_Re *re, T_ReMach *mach, int ch, T_ReFlags flags)
{
	T_ID sb, se, e;
	int data = T_SYM_MAKE(ch, ch);

	if(mach->s_end == -1){
		if((sb = t_re_mach_add_state(re, mach, -1)) < 0)
			return sb;
	}else{
		sb = mach->s_end;
	}

	if((se = t_re_mach_add_state(re, mach, -1)) < 0)
		return se;

	if((e = t_re_mach_add_edge(re, mach, sb, se, data)) < 0)
		return e;

	if((flags & T_RE_FL_IGNORE_CASE) && isalpha(ch)){
		ch = islower(ch) ? toupper(ch) : tolower(ch);
		data = T_SYM_MAKE(ch, ch);

		if((e = t_re_mach_add_edge(re, mach, sb, se, data)) < 0)
			return e;
	}

	mach->s_end = se;

	return T_OK;
}

static T_Result
mach_add_any(T_Re *re, T_ReMach *mach)
{
	T_ID sb, se, e;
	int data = T_SYM_MAKE(T_RE_CHAR_MIN, T_RE_CHAR_MAX);

	if(mach->s_end == -1){
		if((sb = t_re_mach_add_state(re, mach, -1)) < 0)
			return sb;
	}else{
		sb = mach->s_end;
	}

	if((se = t_re_mach_add_state(re, mach, -1)) < 0)
		return se;

	if((e = t_re_mach_add_edge(re, mach, sb, se, data)) < 0)
		return e;

	mach->s_end = se;

	return T_OK;
}

static T_Result
mach_add_cset(T_Re *re, T_ReMach *mach, CSet *cset, T_ReFlags flags)
{
	T_ID sb, se, e;
	T_SetIter iter;

	if(mach->s_end == -1){
		if((sb = t_re_mach_add_state(re, mach, -1)) < 0)
			return sb;
	}else{
		sb = mach->s_end;
	}

	if((se = t_re_mach_add_state(re, mach, -1)) < 0)
		return se;

	symbol_array_iter_first(cset, &iter);
	while(!symbol_array_iter_last(&iter)){
		int data = *symbol_array_iter_data(&iter);
		int min = T_SYM_MIN(data);
		int max = T_SYM_MAX(data);

		if((e = t_re_mach_add_edge(re, mach, sb, se, data)) < 0)
			return e;

		if(flags & T_RE_FL_IGNORE_CASE){
			int amin, amax;

			amin = T_MAX(min, 'a');
			amax = T_MIN(max, 'z');

			if(amin <= amax){
				data = T_SYM_MAKE(amin, amax);
				if((e = t_re_mach_add_edge(re, mach, sb, se, data)) < 0)
					return e;
			}

			amin = T_MAX(min, 'A');
			amax = T_MIN(max, 'Z');

			if(amin <= amax){
				data = T_SYM_MAKE(amin, amax);
				if((e = t_re_mach_add_edge(re, mach, sb, se, data)) < 0)
					return e;
			}
		}

		symbol_array_iter_next(&iter);
	}

	mach->s_end = se;

	return T_OK;
}

static T_Result
mach_dup(T_Re *re, T_ReMach *morg, T_ReMach *mdup)
{
	int soff, eoff;
	T_ID sorg_id, sdup_id;
	T_ID eorg_id, edup_id;

	t_re_mach_init(mdup);

	if(morg->s_first == -1)
		return T_OK;

	soff = re->states.nmem - morg->s_first;
	eoff = re->edges.nmem - morg->e_first;

	sorg_id = morg->s_first;
	while(1){
		T_AutoState *sorg;
		T_AutoState sdup;

		sorg = &re->states.buff[sorg_id];
		sdup.data  = sorg->data;
		sdup.edges = (sorg->edges == -1) ? -1 : sorg->edges + eoff;

		if((sdup_id = t_auto_add_state_data(re, &sdup)) < 0)
			return sdup_id;

		if(sorg_id == morg->s_last)
			break;
		sorg_id++;
	}

	eorg_id = morg->e_first;
	while(1){
		T_AutoEdge *eorg;
		T_AutoEdge edup;

		eorg = &re->edges.buff[eorg_id];
		edup.dest   = eorg->dest + soff;
		edup.symbol = eorg->symbol;
		edup.next   = (eorg->next == -1) ? -1 : eorg->next + eoff;

		if((edup_id = t_auto_add_edge_data(re, &edup)) < 0)
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

void
t_re_mach_init(T_ReMach *mach)
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
t_re_mach_add_state(T_Re *re, T_ReMach *mach, T_AutoData data)
{
	T_ID s;

	T_ASSERT(re && mach);

	s = t_auto_add_state(re, data);
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
t_re_mach_add_edge(T_Re *re, T_ReMach *mach, T_ID from, T_ID to, T_AutoSymbol symbol)
{
	T_ID e;

	T_ASSERT(re && mach);

	e = t_auto_add_edge(re, from, to, symbol);
	if(e < 0)
		return e;

	if(mach->e_first == -1)
		mach->e_first = e;

	mach->e_last = e;

	return e;
}

T_Result
t_re_mach_or(T_Re *re, T_ReMach *mach, T_ReMach *mor)
{
	T_ID sb, se, e;

	T_ASSERT(re && mach && mor);

	if((sb = t_re_mach_add_state(re, mach, -1)) < 0)
		return sb;

	if((se = t_re_mach_add_state(re, mach, -1)) < 0)
		return se;

	if(mach->s_begin != -1){
		if((e = t_re_mach_add_edge(re, mach, sb, mach->s_begin, T_RE_EPSILON)) < 0)
			return e;
	}

	if(mor->s_begin != -1){
		if((e = t_re_mach_add_edge(re, mach, sb, mor->s_begin, T_RE_EPSILON)) < 0)
			return e;
	}

	if(mach->s_end != -1){
		if((e = t_re_mach_add_edge(re, mach, mach->s_end, se, T_RE_EPSILON)) < 0)
			return e;
	}

	if(mor->s_end != -1){
		if((e = t_re_mach_add_edge(re, mach, mor->s_end, se, T_RE_EPSILON)) < 0)
			return e;
	}

	if((mach->s_begin == -1) || (mor->s_begin == -1)){
		if((e = t_re_mach_add_edge(re, mach, sb, se, T_RE_EPSILON)) < 0)
			return e;
	}

	mach_join(mach, mor);

	mach->s_begin = sb;
	mach->s_end   = se;

	return T_OK;
}

T_Result
t_re_mach_link(T_Re *re, T_ReMach *mach, T_ReMach *ml)
{
	T_ID e;

	T_ASSERT(re && mach && ml);

	if(ml->s_begin == -1)
		return T_OK;

	if(mach->s_begin == -1){
		*mach = *ml;
		return T_OK;
	}

	if((e = t_re_mach_add_edge(re, mach, mach->s_end, ml->s_begin, T_RE_EPSILON)) < 0)
		return e;

	mach_join(mach, ml);

	mach->s_end = ml->s_end;

	return T_OK;
}

T_Result
t_re_mach_repeat(T_Re *re, T_ReMach *mach, int min, int max)
{
	T_ReMach mret;
	T_ID s, e;
	int cnt;
	T_Result r;

	T_ASSERT(re && mach && (min >= 0));

	if((min == 0) && (max == 0)){
		if((s = t_re_mach_add_state(re, mach, -1)) < 0)
			return s;

		mach->s_begin = s;
		mach->s_end   = s;
		return T_OK;
	}

	cnt = min;
	if(max >= 0)
		cnt = max;
	else
		cnt++;

	{
		T_ReMach mdup[cnt];
		T_ReMach mempty;
		int i;

		mdup[0] = *mach;
		for(i = 1; i < cnt; i++){
			if((r = mach_dup(re, mach, &mdup[i])) < 0)
				return r;
		}

		t_re_mach_init(&mret);

		for(i = 0; i < min; i++){
			if((r = t_re_mach_link(re, &mret, &mdup[i])) < 0)
				return r;
		}

		t_re_mach_init(&mempty);

		if(max < 0)
			cnt--;

		for(; i < cnt; i++){
			if((r = t_re_mach_or(re, &mdup[i], &mempty)) < 0)
				return r;
			if((r = t_re_mach_link(re, &mret, &mdup[i])) < 0)
				return r;
		}

		if(max < 0){
			s = mret.s_end;
			if(s == -1){
				if((s = t_re_mach_add_state(re, &mret, -1)) < 0)
					return s;
			}

			if((e = t_re_mach_add_edge(re, &mret, s, mdup[cnt].s_begin, T_RE_EPSILON)) < 0)
				return e;
			if((e = t_re_mach_add_edge(re, &mret, mdup[cnt].s_end, s, T_RE_EPSILON)) < 0)
				return e;
		}
	}

	*mach = mret;
	return T_OK;
}

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
re_parse_predef_set(ReInput *inp, CSet *set)
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
re_parse_set(ReInput *inp, CSet *set)
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
				T_DEBUG_I("RANGE -");

				if((c2 = re_parse_char(inp, RE_PARSE_SET)) < 0)
					return c2;

				if(c1 > c2){
					T_DEBUG_E("illegal charset");
					return T_ERR_SYNTAX;
				}

				if((r = cset_add_char_range(set, c1, c2)) < 0)
					return r;

				ch = re_getch(inp);
			}else{
				if((r = cset_add_char(set, c1)) < 0)
					return r;
			}
		}
	}while(1);

	if(ch != ']')
		return T_ERR_SYNTAX;

	T_DEBUG_I("SET ]");

	/*Negate*/
	if(neg){
		if((r = cset_neg(set)) < 0)
			return r;
	}

	return T_OK;
}

static T_Result
re_parse_full_set(ReInput *inp, T_Auto *aut, T_ReMach *mach)
{
	CSet s1, s2;
	T_Result r;
	int ch;
	int op;

	cset_init(&s1);
	cset_init(&s2);

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
		r = cset_union(&s1, &s2);
	}else{
		r = cset_diff(&s1, &s2);
	}

end:

	if(r >= 0){
		r = mach_add_cset(aut, mach, &s1, inp->flags);
	}

	cset_deinit(&s1);
	cset_deinit(&s2);

	return r;
}

static T_Result
re_parse_str(ReInput *inp, T_Auto *aut, T_ReMach *mach)
{
	int ch;
	T_Result r;

	T_DEBUG_I("\"");

	if((ch = re_parse_char(inp, RE_PARSE_STRING)) < 0)
		return ch;

	if((r = mach_add_char(aut, mach, ch, inp->flags)) < 0)
		return r;

	do{
		ch = re_getch(inp);
		if(ch == '\"')
			break;
		re_unget(inp, ch);

		if((ch = re_parse_char(inp, RE_PARSE_STRING)) < 0)
			return ch;

		if((r = mach_add_char(aut, mach, ch, inp->flags)) < 0)
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
re_parse_repeat(ReInput *inp, T_Auto *aut, T_ReMach *mach)
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

	if((r = t_re_mach_repeat(aut, mach, min, max)) < 0)
		return r;

	return T_OK;
}

static T_Result
re_parse_single(ReInput *inp, T_Auto *aut, T_ReMach *mach)
{
	T_Result r;
	int ch;

	t_re_mach_init(mach);

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
			if((r = mach_add_any(aut, mach)) < 0)
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
			if((r = mach_add_char(aut, mach, ch, inp->flags)) < 0)
				return r;
			break;
	}

	ch = re_getch(inp);
	switch(ch){
		case '?':
			T_DEBUG_I("REPEAT ?");
			if((r = t_re_mach_repeat(aut, mach, 0, 1)) < 0)
				return r;
			break;
		case '+':
			T_DEBUG_I("REPEAT +");
			if((r = t_re_mach_repeat(aut, mach, 1, -1)) < 0)
				return r;
			break;
		case '*':
			T_DEBUG_I("REPEAT *");
			if((r = t_re_mach_repeat(aut, mach, 0, -1)) < 0)
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
re_parse_series(ReInput *inp, T_Auto *aut, T_ReMach *mach)
{
	T_Result r;
	int ch;

	if((r = re_parse_single(inp, aut, mach)) < 0)
		return r;

	while(1){
		T_ReMach mnext;

		ch = re_getch(inp);
		re_unget(inp, ch);

		if((ch == '|') || (ch == ')') || (ch == -1))
			break;

		if((r = re_parse_single(inp, aut, &mnext)) < 0)
			return r;

		if((r = t_re_mach_link(aut, mach, &mnext)) < 0)
			return r;
	}

	return T_OK;
}

static T_Result
re_parse(ReInput *inp, T_Auto *aut, T_ReMach *mach)
{
	T_Result r;
	int ch;

	if((r = re_parse_series(inp, aut, mach)) < 0)
		return r;

	while(1){
		ch = re_getch(inp);
		if(ch == '|'){
			T_ReMach mnext;

			T_DEBUG_I("|");

			if((r = re_parse_series(inp, aut, &mnext)) < 0)
				return r;

			if((r = t_re_mach_or(aut, mach, &mnext)) < 0)
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
	T_ReMach mach;
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

	if(bol || !(inp->flags & T_RE_FL_NO_BOL)){
		if((e = t_auto_add_edge(aut, 0, mach.s_begin, T_RE_BOL)) < 0)
			return e;
	}

	if(!bol){
		if((e = t_auto_add_edge(aut, 0, mach.s_begin, T_RE_EPSILON)) < 0)
			return e;
	}

	if(eol){
		if((s = t_auto_add_state(aut, data)) < 0)
			return s;
		if((e = t_auto_add_edge(aut, mach.s_end, s, T_RE_EOL)) < 0)
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

	if(t_re_to_dfa(&aut, re) < 0)
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
			ch  = -2;
			bol = T_FALSE;
		}else{
			ch  = re_getch(&inp);
		}

		T_DEBUG_I("symbol: %d", ch);

		eid = s->edges;
		while(eid != -1){
			e = &re->edges.buff[eid];

			if((e->symbol == T_RE_EOL) && (ch == -1)){
				sid = e->dest;
				goto next_state;
			}else if((e->symbol == T_RE_BOL) && (ch == -2)){
				sid = e->dest;
				goto next_state;
			}else{
				int min = T_SYM_MIN(e->symbol);
				int max = T_SYM_MAX(e->symbol);

				if((ch >= min) && (ch <= max)){
					sid = e->dest;
					goto next_state;
				}
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


typedef struct{
	T_SET_DECL(T_ID, a);
}EpsArray;
#define T_ARRAY_TYPE       EpsArray
#define T_ARRAY_ELEM_TYPE  T_ID
#define T_ARRAY_NAME       a
#define T_ARRAY_FUNC(name) eps_##name
#define T_ARRAY_CMP(p1, p2) (*(p1) - *(p2))
#include <t_array.h>

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

static T_Result
nfa_clear_epsilon(T_Re *nfa)
{
	EpsArray eps_array;
	SIDSet full_set;
	SIDQueue q;
	T_AutoState *s, *eps;
	T_AutoEdge *e;
	T_ID sid, dest_sid, eps_sid, data_sid, eid;
	T_Result r;

	eps_array_init(&eps_array);
	sid_set_init(&full_set);
	sid_queue_init(&q);

	sid = 0;
	if((r = sid_queue_push_back(&q, &sid)) < 0)
		goto end;
	if((r = sid_set_add(&full_set, &sid, NULL)) < 0)
		goto end;

	do{
		T_ArrayIter iter;
		int eps_pos;
		
		sid_queue_pop_front(&q, &sid);
		s = &nfa->states.buff[sid];
		data_sid = sid;

		T_DEBUG_I("check state %d", sid);

		/*Find epsilon states.*/
		eps_pos = 0;
		eps = s;
		while(1){
			eid = eps->edges;
			while(eid != -1){
				e = &nfa->edges.buff[eid];
				dest_sid = e->dest;

				if(e->symbol == T_RE_EPSILON){
					T_DEBUG_I("add eps %d", dest_sid);
					r = eps_array_find(&eps_array, &dest_sid);
					if(r < 0){
						if((r = eps_array_add(&eps_array, &dest_sid)) < 0)
							goto end;
					}
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

			if(eps_pos >= eps_array_length(&eps_array))
				break;

			eps_sid = *eps_array_element(&eps_array, eps_pos++);
			eps = &nfa->states.buff[eps_sid];
		}

		/*Relink edges.*/
		eps_array_iter_first(&eps_array, &iter);
		while(!eps_array_iter_last(&iter)){

			eps_sid = *eps_array_iter_data(&iter);
			eps = &nfa->states.buff[eps_sid];

			T_DEBUG_I("eps %d", eps_sid);

			eid = eps->edges;
			while(eid != -1){
				e = &nfa->edges.buff[eid];
				if(e->symbol != T_RE_EPSILON){
					dest_sid = e->dest;

					if((r = sid_set_add(&full_set, &dest_sid, NULL)) < 0)
						goto end;
					if(r > 0){
						T_DEBUG_I("add real %d", dest_sid);
						if((r = sid_queue_push_back(&q, &dest_sid)) < 0)
							goto end;
					}

					T_DEBUG_I("relink %d %d %08x", sid, dest_sid, e->symbol);
					if((r = t_auto_add_edge(nfa, sid, dest_sid, e->symbol)) < 0)
						goto end;
				}

				eid = nfa->edges.buff[eid].next;
			}

			if((eps->data != -1) && ((s->data == -1) || (data_sid > eps_sid))){
				s->data = eps->data;
				data_sid = eps_sid;
			}

			eps_array_iter_next(&iter);
		}

		eps_array_reinit(&eps_array);
	}while(!sid_queue_empty(&q));

	r = T_OK;
end:
	eps_array_deinit(&eps_array);
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
#define T_HASH_KEY_DEINIT  sgrp_set_deinit
#define T_HASH_FUNC(name)  sid_##name
#include <t_hash.h>

/*Symbol state group set.*/
typedef struct{
	int    symbol;
	SGroup sgrp;
}SymSGrp;

typedef struct{
	T_SET_DECL(SymSGrp, s);
}SymSGrpSet;

static T_INLINE void
sym_sgrp_deinit(SymSGrp *elem)
{
	sgrp_set_deinit(&elem->sgrp);
}

#define T_SET_TYPE      SymSGrpSet
#define T_SET_ELEM_TYPE SymSGrp
#define T_SET_NAME      s
#define T_SET_ELEM_DEINIT sym_sgrp_deinit
#define T_SET_CMP(p1, p2) (((p1)->symbol & 0xFFFF) - ((p2)->symbol & 0xFFFF))
#define T_SET_FUNC(name)  sym_sgrp_##name
#include <t_set.h>

static T_Result
sym_sgrp_set_add_symbol(SymSGrpSet *set, int symbol, int dest)
{
	SymSGrp ss, *pss;
	T_ID id, rid;
	T_Result r = T_OK;

	if((symbol != T_RE_EOL) && (symbol != T_RE_BOL)){
		int min = T_SYM_MIN(symbol);
		int max = T_SYM_MAX(symbol);
		int len = sym_sgrp_set_length(set);

		for(id = 0; id < len; id++){
			int old_min, old_max;

			pss = sym_sgrp_set_element(set, id);
			old_min = T_SYM_MIN(pss->symbol);
			old_max = T_SYM_MAX(pss->symbol);

			if(max < old_min)
				break;

			if(((min >= old_min) && (min <= old_max)) || ((max >= old_min) && (max <= old_max))){
				int omin, omax;
				SGroup *sgrp;

				omin = T_MAX(min, old_min);
				omax = T_MIN(max, old_max);

				if(min < omin){
					ss.symbol = T_SYM_MAKE(min, omin - 1);
					sgrp_set_init(&ss.sgrp);
					if((r = sgrp_set_add(&ss.sgrp, &dest, NULL)) < 0)
						return r;

					if((r = sym_sgrp_set_add_not_check(set, &ss)) < 0){
						sgrp_set_deinit(&ss.sgrp);
						return r;
					}
				}

				if(old_min < omin){
					ss.symbol = T_SYM_MAKE(old_min, omin - 1);
					if((r = sgrp_set_dup(&ss.sgrp, &sym_sgrp_set_element(set, id)->sgrp)) < 0)
						return r;

					if((r = sym_sgrp_set_add_not_check(set, &ss)) < 0){
						sgrp_set_deinit(&ss.sgrp);
						return r;
					}
				}

				sym_sgrp_set_element(set, id)->symbol = T_SYM_MAKE(omin, omax);
				sgrp = &sym_sgrp_set_element(set, id)->sgrp;
				if((r = sgrp_set_add(sgrp, &dest, NULL)) < 0)
					return r;

				if(old_max > omax){
					ss.symbol = T_SYM_MAKE(omax + 1, old_max);
					if((r = sgrp_set_dup(&ss.sgrp, &sym_sgrp_set_element(set, id)->sgrp)) < 0)
						return r;

					if((r = sym_sgrp_set_add_not_check(set, &ss)) < 0){
						sgrp_set_deinit(&ss.sgrp);
						return r;
					}
				}

				min = old_max + 1;
			}
		}

		if(min <= max){
			ss.symbol = T_SYM_MAKE(min, max);
			sgrp_set_init(&ss.sgrp);
			if((r = sgrp_set_add(&ss.sgrp, &dest, NULL)) < 0)
				return r;

			if((r = sym_sgrp_set_add_not_check(set, &ss)) < 0){
				sgrp_set_deinit(&ss.sgrp);
				return r;
			}
		}

		sym_sgrp_set_sort(set);
	}else{
		ss.symbol = symbol;
		sgrp_set_init(&ss.sgrp);

		if((r = sgrp_set_add(&ss.sgrp, &dest, NULL)) < 0)
			return r;

		if((r = sym_sgrp_set_add(set, &ss, &rid)) < 0){
			sgrp_set_deinit(&ss.sgrp);
			return r;
		}else if(r == 0){
			sgrp_set_deinit(&ss.sgrp);
			pss = sym_sgrp_set_element(set, rid);

			if((r = sgrp_set_add(&pss->sgrp, &dest, NULL)) < 0)
				return r;
		}
	}

	return T_OK;
}


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
nfa_to_dfa(T_Re *nfa, T_Re *dfa)
{
	SIDHash sid_hash;
	SIDHashEntry *sid_hent;
	SymSGrpSet ss_set;
	T_ArrayIter a_iter;
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
	sym_sgrp_set_init(&ss_set);
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
			s = &nfa->states.buff[sid];

			T_DEBUG_I("check state %d", sid);

			/*Generate symbol set.*/
			eid = s->edges;
			while(eid != -1){
				e = &nfa->edges.buff[eid];
				sym = e->symbol;

				if(sym != T_RE_EPSILON){
					dest = e->dest;

					if((r = sym_sgrp_set_add_symbol(&ss_set, sym, dest)) < 0)
						goto end;
				}

				eid = e->next;
			}

			sgrp_set_iter_next(&s_iter);
		}

		/*Create DFA states.*/
		sym_sgrp_set_iter_first(&ss_set, &a_iter);
		while(!sym_sgrp_set_iter_last(&a_iter)){

			sym  = sym_sgrp_set_iter_data(&a_iter)->symbol;
			pgrp = &sym_sgrp_set_iter_data(&a_iter)->sgrp;

			T_DEBUG_I("symbol %08x", sym);

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
					T_AutoState *t_s = &nfa->states.buff[t_id];

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

			T_DEBUG_I("dfa edge %d->%08x->%d", orig_dfa_sid, sym, dfa_sid);

			if((r = t_auto_add_edge(dfa, orig_dfa_sid, dfa_sid, sym)) < 0)
				goto end;

			sym_sgrp_set_iter_next(&a_iter);
		}

		sym_sgrp_set_reinit(&ss_set);
	}while(!sgrp_queue_empty(&q));

	r = T_OK;
end:
	sgrp_queue_deinit(&q);
	sid_hash_deinit(&sid_hash);
	sym_sgrp_set_deinit(&ss_set);

	return r;
}

T_Result
t_re_to_dfa(T_Re *nfa, T_Re *dfa)
{
	T_Result r;

	T_ASSERT(nfa && dfa);

	T_DEBUG_I("NFA:");
#ifdef T_ENABLE_DEBUG
	t_auto_dump(nfa);
#endif

	if((r = nfa_clear_epsilon(nfa)) < 0)
		return r;

	T_DEBUG_I("NFA after clear epsilon:");
#ifdef T_ENABLE_DEBUG
	t_auto_dump(nfa);
#endif

	if((r = nfa_to_dfa(nfa, dfa)) < 0)
		return r;

	T_DEBUG_I("DFA:");
#ifdef T_ENABLE_DEBUG
	t_auto_dump(nfa);
#endif

	return T_OK;
}

