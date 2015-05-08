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

#if !defined(T_QUEUE_TYPE) || !defined(T_QUEUE_ELEM_TYPE) || !defined(T_QUEUE_NAME)
	#error T_QUEUE_TYPE, T_QUEUE_ELEM_TYPE or T_QUEUE_NAME have not been defined
#endif

#ifndef T_QUEUE_FUNC
	#define T_QUEUE_FUNC(name) name
#endif

#ifndef T_QUEUE_MALLOC
	#define T_QUEUE_MALLOC(size) malloc(size)
#endif

#ifndef T_QUEUE_FREE
	#define T_QUEUE_FREE(ptr) free(ptr)
#endif

#ifdef T_QUEUE_ELEM_DEINIT
static T_INLINE void
T_QUEUE_FUNC(queue_element_destroy)(T_QUEUE_TYPE *q)
{
	T_QUEUE_ELEM_TYPE *elem;
	int cnt = q->T_QUEUE_NAME.nmem;
	int pos = q->T_QUEUE_NAME.head;

	while(cnt--){
		elem = &q->T_QUEUE_NAME.buff[pos++];
		T_QUEUE_ELEM_DEINIT(elem);
		pos %= q->T_QUEUE_NAME.size;
	}
}
#else
static T_INLINE void
T_QUEUE_FUNC(queue_element_destroy)(T_QUEUE_TYPE *q)
{
}
#endif

static T_Result
T_QUEUE_FUNC(queue_init)(T_QUEUE_TYPE *q)
{
	T_ASSERT(q);

	q->T_QUEUE_NAME.size = 0;
	q->T_QUEUE_NAME.nmem = 0;
	q->T_QUEUE_NAME.head = 0;
	q->T_QUEUE_NAME.buff = NULL;

	return T_OK;
}

static void
T_QUEUE_FUNC(queue_deinit)(T_QUEUE_TYPE *q)
{
	T_ASSERT(q);

	if(q->T_QUEUE_NAME.buff){
		T_QUEUE_FUNC(queue_element_destroy)(q);
		T_QUEUE_FREE(q->T_QUEUE_NAME.buff);
	}
}

static T_INLINE int
T_QUEUE_FUNC(queue_length)(T_QUEUE_TYPE *q)
{
	T_ASSERT(q);
	return q->T_QUEUE_NAME.nmem;
}

static T_INLINE T_Bool
T_QUEUE_FUNC(queue_empty)(T_QUEUE_TYPE *q)
{
	T_ASSERT(q);
	return q->T_QUEUE_NAME.nmem == 0;
}

static T_INLINE T_Result
T_QUEUE_FUNC(queue_resize_buffer)(T_QUEUE_TYPE *q)
{
	if(q->T_QUEUE_NAME.nmem >= q->T_QUEUE_NAME.size){
		T_QUEUE_ELEM_TYPE *buf;
		int size = T_MAX(q->T_QUEUE_NAME.size * 2, 16);
		int c1, c2;

		buf = (T_QUEUE_ELEM_TYPE*)T_QUEUE_MALLOC(size * sizeof(T_QUEUE_ELEM_TYPE));
		if(!buf)
			return T_ERR_NOMEM;

		c1 = T_MIN(q->T_QUEUE_NAME.size - q->T_QUEUE_NAME.head, q->T_QUEUE_NAME.nmem);
		c2 = q->T_QUEUE_NAME.nmem - c1;

		if(c1 > 0)
			memcpy(buf, q->T_QUEUE_NAME.buff + q->T_QUEUE_NAME.head, c1 * sizeof(T_QUEUE_ELEM_TYPE));
		if(c2 > 0)
			memcpy(buf + c1, q->T_QUEUE_NAME.buff, c2 * sizeof(T_QUEUE_ELEM_TYPE));

		if(q->T_QUEUE_NAME.buff)
			T_QUEUE_FREE(q->T_QUEUE_NAME.buff);

		q->T_QUEUE_NAME.buff = buf;
		q->T_QUEUE_NAME.size = size;
		q->T_QUEUE_NAME.head = 0;
	}

	return T_OK;
}

static T_Result
T_QUEUE_FUNC(queue_push_front)(T_QUEUE_TYPE *q, T_QUEUE_ELEM_TYPE *elem)
{
	T_Result r;

	T_ASSERT(q && elem);

	if((r = T_QUEUE_FUNC(queue_resize_buffer)(q)) < 0)
		return r;

	q->T_QUEUE_NAME.head--;
	if(q->T_QUEUE_NAME.head < 0)
		q->T_QUEUE_NAME.head = q->T_QUEUE_NAME.size - 1;

	q->T_QUEUE_NAME.buff[q->T_QUEUE_NAME.head] = *elem;

	q->T_QUEUE_NAME.nmem++;
	return T_OK;
}

static T_Result
T_QUEUE_FUNC(queue_push_back)(T_QUEUE_TYPE *q, T_QUEUE_ELEM_TYPE *elem)
{
	T_Result r;
	int pos;

	T_ASSERT(q && elem);

	if((r = T_QUEUE_FUNC(queue_resize_buffer)(q)) < 0)
		return r;

	pos = (q->T_QUEUE_NAME.head + q->T_QUEUE_NAME.nmem) % q->T_QUEUE_NAME.size;
	q->T_QUEUE_NAME.buff[pos] = *elem;

	q->T_QUEUE_NAME.nmem++;
	return T_OK;
}

static void
T_QUEUE_FUNC(queue_pop_front)(T_QUEUE_TYPE *q, T_QUEUE_ELEM_TYPE *elem)
{
	T_ASSERT(q && elem && (q->T_QUEUE_NAME.nmem > 0));

	*elem = q->T_QUEUE_NAME.buff[q->T_QUEUE_NAME.head++];
	q->T_QUEUE_NAME.head %= q->T_QUEUE_NAME.size;
	q->T_QUEUE_NAME.nmem--;
}

static void
T_QUEUE_FUNC(queue_pop_back)(T_QUEUE_TYPE *q, T_QUEUE_ELEM_TYPE *elem)
{
	int pos;

	T_ASSERT(q && elem && (q->T_QUEUE_NAME.nmem > 0));

	pos = (q->T_QUEUE_NAME.head + q->T_QUEUE_NAME.nmem - 1) % q->T_QUEUE_NAME.size;
	*elem = q->T_QUEUE_NAME.buff[pos];
	q->T_QUEUE_NAME.nmem--;
}

static T_INLINE T_QUEUE_ELEM_TYPE*
T_QUEUE_FUNC(queue_front)(T_QUEUE_TYPE *q, int n)
{
	int pos;

	T_ASSERT(q && (n < q->T_QUEUE_NAME.nmem));

	pos = (q->T_QUEUE_NAME.head + n) % q->T_QUEUE_NAME.size;
	return &q->T_QUEUE_NAME.buff[pos];
}

static T_INLINE T_QUEUE_ELEM_TYPE*
T_QUEUE_FUNC(queue_back)(T_QUEUE_TYPE *q, int n)
{
	int pos;

	T_ASSERT(q && (n < q->T_QUEUE_NAME.nmem));

	pos = (q->T_QUEUE_NAME.head + q->T_QUEUE_NAME.nmem - n - 1) % q->T_QUEUE_NAME.size;
	return &q->T_QUEUE_NAME.buff[pos];
}

#undef T_QUEUE_REALLOC
#undef T_QUEUE_TYPE
#undef T_QUEUE_ELEM_TYPE
#undef T_QUEUE_NAME
#undef T_QUEUE_FUNC
#undef T_QUEUE_ELEM_DEINIT

