/*
	Hash.c	WJ105

	bbs100-style generic auto-sizing hash code

	NOTES:
	- To be able to use the Hash, give it a hashaddr function
	- It does not know about how to destroy the data in the hash
	  calling destroy_Hash() might easily leak memory ...
*/

#include "config.h"
#include "Hash.h"
#include "Memory.h"
#include "cstring.h"
#include "log.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>


HashList *new_HashList(char *key, void *value) {
HashList *hl;

	if (key == NULL || !*key || (hl = (HashList *)Malloc(sizeof(HashList), TYPE_HASHLIST)) == NULL)
		return NULL;

	strncpy(hl->key, key, MAX_HASH_KEY-1);
	hl->key[MAX_HASH_KEY-1] = 0;
	hl->value = value;
	return hl;
}

void destroy_HashList(HashList *hl) {
	Free(hl);
}


Hash *new_Hash(void) {
Hash *h;

	if ((h = (Hash *)Malloc(sizeof(Hash), TYPE_HASH)) == NULL)
		return NULL;

	if ((h->hash = (HashList **)Malloc(HASH_MIN_SIZE * sizeof(HashList *), TYPE_POINTER)) == NULL) {
		Free(h);
		return NULL;
	}
	h->size = HASH_MIN_SIZE;
	return h;
}

void destroy_Hash(Hash *h) {
	if (h != NULL) {
		if (h->hash != NULL) {
			int i;

			for(i = 0; i < h->size; i++)
				listdestroy_HashList(h->hash[i]);

			Free(h->hash);
			h->hash = NULL;
			h->size = h->num = 0;
		}
	}
	Free(h);
}

int resize_Hash(Hash *h, int newsize) {
HashList **new_hash, *hl, *hl_next;
int i, addr;

	if (h == NULL || h->hashaddr == NULL)
		return -1;

	if (newsize < HASH_MIN_SIZE)
		newsize = HASH_MIN_SIZE;

	if (newsize == h->size)
		return 0;

	if ((new_hash = (HashList **)Malloc(newsize * sizeof(HashList **), TYPE_POINTER)) == NULL)
		return -2;

	for(i = 0; i < h->size; i++) {
		for(hl = h->hash[i]; hl != NULL; hl = hl_next) {
			hl_next = hl->next;
			addr = h->hashaddr(hl->key);
			if (addr < 0)
				addr = -addr;
			addr %= newsize;

			hl->prev = hl->next = NULL;
			add_HashList(&new_hash[addr], hl);
		}
	}
	Free(h->hash);
	h->hash = new_hash;
	h->size = newsize;
	return 0;
}

int add_Hash(Hash *h, char *key, void *obj) {
int addr;
HashList *hl;

	if (h == NULL || h->hashaddr == NULL || h->size < HASH_MIN_SIZE
		|| (hl = new_HashList(key, obj)) == NULL)
		return -1;

	addr = h->hashaddr(key);
	if (addr < 0)
		addr = -addr;
	addr %= h->size;

	add_HashList(&h->hash[addr], hl);
	h->num++;
/*
	a hash performs well if it has a percentage of free slots
	but I'm also satisfied with just a fixed number of slots
*/
	if (h->size - h->num <= HASH_GROW_SIZE / 2)
		resize_Hash(h, h->size + HASH_GROW_SIZE);

	return 0;
}

void *remove_Hash(Hash *h, char *key) {
int addr;
HashList *hl;

	if (h == NULL || h->hashaddr == NULL || h->size < HASH_MIN_SIZE || key == NULL || !*key)
		return NULL;

	addr = h->hashaddr(key);
	if (addr < 0)
		addr = -addr;
	addr %= h->size;

	for(hl = h->hash[addr]; hl != NULL; hl = hl->next) {
		if (!strcmp(hl->key, key)) {
			void *obj;

			obj = hl->value;
			remove_HashList(&h->hash[addr], hl);
			destroy_HashList(hl);

			h->num--;
			if (h->num < 0)
				h->num = 0;

/* auto-shrink the hash */
			if (h->size - h->num >= HASH_GROW_SIZE + HASH_GROW_SIZE / 2)
				resize_Hash(h, h->size - HASH_GROW_SIZE);

			return obj;
		}
	}
	return NULL;
}

void *in_Hash(Hash *h, char *key) {
int addr;
HashList *hl;

	if (h == NULL || h->hashaddr == NULL || h->size < HASH_MIN_SIZE || key == NULL || !*key)
		return NULL;

	addr = h->hashaddr(key);
	if (addr < 0)
		addr = -addr;
	addr %= h->size;

	for(hl = h->hash[addr]; hl != NULL; hl = hl->next) {
		if (!strcmp(hl->key, key))
			return hl->value;
	}
	return NULL;
}


/*
	example hashaddr function
*/
int hashaddr_ascii(char *key) {
char *p;
int addr, c;

	p = key;
	addr = *p;
	p++;
	while(*p) {
		c = *p - ' ';

		addr <<= 1;
		addr ^= c;

		p++;
	}
	return addr;
}

/* EOB */
