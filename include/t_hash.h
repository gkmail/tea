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

#if !defined(T_HASH_TYPE) || !defined(T_HASH_ENTRY_TYPE) || !defined(T_HASH_ELEM_TYPE) || !defined(T_HASH_KEY_TYPE) || !defined(T_HASH_NAME)
	#error T_HASH_TYPE, T_HASH_ENTRY_TYPE, T_HASH_ELEM_TYPE, T_HASH_KEY_TYPE or T_HASH_NAME have not been defined
#endif

#ifndef T_HASH_FUNC
	#define T_HASH_FUNC(name) name
#endif

#ifndef T_HASH_MALLOC
	#define T_HASH_MALLOC(size) malloc(size)
#endif

#ifndef T_HASH_FREE
	#define T_HASH_FREE(ptr) free(ptr)
#endif

#ifndef T_HASH_ELEM_DEINIT
	#define T_HASH_ELEM_DEINIT(ptr)
#endif

#ifndef T_HASH_KEY_DEINIT
	#define T_HASH_KEY_DEINIT(ptr)
#endif

#ifndef T_HASH_KEY
	#define T_HASH_KEY(key) ((unsigned int)*(key))
#endif

#ifndef T_HASH_EQUAL
	#ifdef T_HASH_CMP
		#define T_HASH_EQUAL(k1, k2) (T_HASH_CMP(k1, k2)==0)
	#else
		#define T_HASH_EQUAL(k1, k2) (*(k1) == *(k2))
	#endif
#endif

static T_INLINE T_Result
T_HASH_FUNC(hash_init)(T_HASH_TYPE *hash)
{
	hash->T_HASH_NAME.entries = NULL;
	hash->T_HASH_NAME.nmem   = 0;
	hash->T_HASH_NAME.bucket = 0;

	return T_OK;
}

static void
T_HASH_FUNC(hash_deinit)(T_HASH_TYPE *hash)
{
	if(hash->T_HASH_NAME.entries){
		int i;

		for(i = 0; i < hash->T_HASH_NAME.bucket; i++){
			T_HASH_ENTRY_TYPE *ent, *enext;
			for(ent = hash->T_HASH_NAME.entries[i]; ent; ent = enext){
				enext = ent->next;
				T_HASH_KEY_DEINIT(&ent->key);
				T_HASH_ELEM_DEINIT(&ent->data);
				T_HASH_FREE(ent);
			}
		}

		T_HASH_FREE(hash->T_HASH_NAME.entries);
	}
}

static void
T_HASH_FUNC(hash_reinit)(T_HASH_TYPE *hash)
{
	T_HASH_FUNC(hash_deinit)(hash);
	T_HASH_FUNC(hash_init)(hash);
}

static void
T_HASH_FUNC(hash_clear)(T_HASH_TYPE *hash)
{
	T_HASH_FUNC(hash_deinit)(hash);
	T_HASH_FUNC(hash_init)(hash);
}

static T_Result
T_HASH_FUNC(hash_add_entry)(T_HASH_TYPE *hash, T_HASH_KEY_TYPE *key, T_HASH_ENTRY_TYPE **rent)
{
	T_HASH_ENTRY_TYPE *ent, *enext;
	unsigned int kv, pos;

	T_ASSERT(hash && rent);

	kv = T_HASH_KEY(key);

	if(hash->T_HASH_NAME.nmem){
		pos = kv % hash->T_HASH_NAME.bucket;
		for(ent = hash->T_HASH_NAME.entries[pos];
					ent;
					ent = ent->next){
			if(T_HASH_EQUAL(key, &ent->key)){
				*rent = ent;
				return 0;
			}
		}
	}

	if(hash->T_HASH_NAME.nmem >= hash->T_HASH_NAME.bucket * 3){
		T_HASH_ENTRY_TYPE **buf;
		unsigned int npos;
		int size = T_MAX(hash->T_HASH_NAME.nmem, 16);
		int i;

		buf = (T_HASH_ENTRY_TYPE**)T_HASH_MALLOC(size * sizeof(T_HASH_ENTRY_TYPE*));
		if(!buf)
			return T_ERR_NOMEM;

		memset(buf, 0, size * sizeof(T_HASH_ENTRY_TYPE*));

		for(i = 0; i < hash->T_HASH_NAME.bucket; i++){
			for(ent = hash->T_HASH_NAME.entries[i];
						ent;
						ent = enext){
				enext = ent->next;
				npos  = T_HASH_KEY(&ent->key) % size;
				ent->next = buf[npos];
				buf[npos] = ent;
			}
		}

		if(hash->T_HASH_NAME.entries)
			T_HASH_FREE(hash->T_HASH_NAME.entries);

		hash->T_HASH_NAME.entries = buf;
		hash->T_HASH_NAME.bucket  = size;
	}

	ent = (T_HASH_ENTRY_TYPE*)T_HASH_MALLOC(sizeof(T_HASH_ENTRY_TYPE));
	if(!ent)
		return T_ERR_NOMEM;

	pos = kv % hash->T_HASH_NAME.bucket;
	ent->next = hash->T_HASH_NAME.entries[pos];
	hash->T_HASH_NAME.entries[pos] = ent;
	ent->key = *key;

	hash->T_HASH_NAME.nmem++;
	
	*rent = ent;
	return 1;
}

static T_Result
T_HASH_FUNC(hash_lookup_entry)(T_HASH_TYPE *hash, T_HASH_KEY_TYPE *key, T_HASH_ENTRY_TYPE **rent)
{
	T_HASH_ENTRY_TYPE *ent;
	unsigned int pos;

	T_ASSERT(hash);

	if(!hash->T_HASH_NAME.nmem)
		return 0;

	pos = T_HASH_KEY(key) % hash->T_HASH_NAME.bucket;
	for(ent = hash->T_HASH_NAME.entries[pos];
				ent;
				ent = ent->next){
		if(T_HASH_EQUAL(key, &ent->key)){
			if(rent)
				*rent = ent;
			return 1;
		}
	}

	return 0;
}

static T_Result
T_HASH_FUNC(hash_add)(T_HASH_TYPE *hash, T_HASH_KEY_TYPE *key, T_HASH_ELEM_TYPE *elem)
{
	T_HASH_ENTRY_TYPE *ent;
	T_Result r;

	r = T_HASH_FUNC(hash_add_entry)(hash, key, &ent);
	if(r > 0){
		ent->data = *elem;
	}

	return r;
}

static T_Result
T_HASH_FUNC(hash_lookup)(T_HASH_TYPE *hash, T_HASH_KEY_TYPE *key, T_HASH_ELEM_TYPE *elem)
{
	T_HASH_ENTRY_TYPE *ent;
	T_Result r;

	r = T_HASH_FUNC(hash_lookup_entry)(hash, key, &ent);
	if(r > 0){
		*elem = ent->data;
	}

	return r;
}

static T_Result
T_HASH_FUNC(hash_remove)(T_HASH_TYPE *hash, T_HASH_KEY_TYPE *key)
{
	T_HASH_ENTRY_TYPE *ent, *eprev;
	unsigned int pos;

	T_ASSERT(hash);

	if(!hash->T_HASH_NAME.nmem)
		return 0;

	pos = T_HASH_KEY(key) % hash->T_HASH_NAME.bucket;
	for(ent = hash->T_HASH_NAME.entries[pos], eprev = NULL;
				ent;
				eprev = ent, ent = ent->next){
		if(T_HASH_EQUAL(key, &ent->key)){
			if(eprev)
				eprev->next = ent->next;
			else
				hash->T_HASH_NAME.entries[pos] = ent->next;

			T_HASH_KEY_DEINIT(&ent->key);
			T_HASH_ELEM_DEINIT(&ent->data);
			T_HASH_FREE(ent);

			hash->T_HASH_NAME.nmem--;
			return 1;
		}
	}

	return 0;
}

static T_INLINE void
T_HASH_FUNC(hash_iter_first)(T_HASH_TYPE *hash, T_HashIter *iter)
{
	T_HASH_ENTRY_TYPE *ent;
	int i;

	iter->hash = hash;
	iter->pos  = 0;

	for(i = 0; i < hash->T_HASH_NAME.bucket; i++){
		for(ent = hash->T_HASH_NAME.entries[i];
					ent;
					ent = ent->next){
			iter->pos   = i;
			iter->entry = ent;
			return;
		}
	}

	iter->entry = NULL;
}

static T_INLINE T_HASH_KEY_TYPE*
T_HASH_FUNC(hash_iter_key)(T_HashIter *iter)
{
	T_HASH_ENTRY_TYPE *ent = iter->entry;
	return &ent->key;
}

static T_INLINE T_HASH_ELEM_TYPE*
T_HASH_FUNC(hash_iter_data)(T_HashIter *iter)
{
	T_HASH_ENTRY_TYPE *ent = iter->entry;
	return &ent->data;
}

static T_INLINE void
T_HASH_FUNC(hash_iter_next)(T_HashIter *iter)
{
	T_HASH_TYPE *hash = iter->hash;
	T_HASH_ENTRY_TYPE *ent = iter->entry;
	int i = iter->pos;

	ent = ent->next;
	if(ent){
		iter->entry = ent;
		return;
	}

	for(++i; i < hash->T_HASH_NAME.bucket; i++){
		ent = hash->T_HASH_NAME.entries[i];
		if(ent){
			iter->pos   = i;
			iter->entry = ent;
			return;
		}
	}

	iter->entry = NULL;
}

static T_INLINE T_Bool
T_HASH_FUNC(hash_iter_last)(T_HashIter *iter)
{
	return iter->entry == NULL;
}

#undef T_HASH_TYPE
#undef T_HASH_ENTRY_TYPE
#undef T_HASH_ELEM_TYPE
#undef T_HASH_KEY_TYPE
#undef T_HASH_NAME
#undef T_HASH_FUNC
#undef T_HASH_MALLOC
#undef T_HASH_FREE
#undef T_HASH_ELEM_DEINIT
#undef T_HASH_KEY_DEINIT
#undef T_HASH_KEY
#undef T_HASH_EQUAL
#ifdef T_HASH_CMP
	#undef T_HASH_CMP
#endif
