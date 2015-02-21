/*
    bbs100 3.3 WJ107
    Copyright (C) 1997-2015  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/*
	Hash.c	WJ105

	bbs100-style generic auto-sizing hash code
*/

#include "config.h"
#include "Hash.h"
#include "Memory.h"
#include "cstring.h"
#include "log.h"
#include "debug.h"
#include "crc32.h"

#include <stdio.h>
#include <stdlib.h>


Hash *new_Hash(void) {
Hash *h;

	if ((h = (Hash *)Malloc(sizeof(Hash), TYPE_HASH)) == NULL)
		return NULL;

	if ((h->hash = (KVPair **)Malloc(HASH_MIN_SIZE * sizeof(KVPair *), TYPE_POINTER)) == NULL) {
		Free(h);
		return NULL;
	}
	h->size = HASH_MIN_SIZE;
	h->hashaddr = hashaddr_crc32;
	return h;
}

void destroy_Hash(Hash *h) {
	if (h != NULL) {
		if (h->hash != NULL) {
			int i;

			for(i = 0; i < h->size; i++)
				listdestroy_KVPair(h->hash[i]);

			Free(h->hash);
			h->hash = NULL;
			h->size = h->num = 0;
		}
	}
	Free(h);
}

int resize_Hash(Hash *h, int newsize) {
KVPair **new_hash, *kv, *kv_next;
int i, addr;

	if (h == NULL || h->hashaddr == NULL)
		return -1;

	if (newsize < HASH_MIN_SIZE)
		newsize = HASH_MIN_SIZE;

	if (newsize == h->size)
		return 0;

	if ((new_hash = (KVPair **)Malloc(newsize * sizeof(KVPair **), TYPE_POINTER)) == NULL)
		return -2;

	for(i = 0; i < h->size; i++) {
		for(kv = h->hash[i]; kv != NULL; kv = kv_next) {
			kv_next = kv->next;
			addr = h->hashaddr(kv->key);
			if (addr < 0)
				addr = -addr;
			addr %= newsize;

			kv->prev = kv->next = NULL;
			(void)add_KVPair(&new_hash[addr], kv);
		}
	}
	Free(h->hash);
	h->hash = new_hash;
	h->size = newsize;
	return 0;
}

int add_Hash(Hash *h, char *key, void *obj, void (*destroy)(void *)) {
int addr;
KVPair *kv;

	if (h == NULL || h->hashaddr == NULL || h->size < HASH_MIN_SIZE
		|| (kv = new_KVPair()) == NULL)
		return -1;

	KVPair_setpointer(kv, key, obj, destroy);

	addr = h->hashaddr(key);
	if (addr < 0)
		addr = -addr;
	addr %= h->size;

	(void)add_KVPair(&h->hash[addr], kv);
	h->num++;
/*
	a hash performs well if it has a percentage of free slots
	but I'm also satisfied with just a fixed number of slots
*/
	if (h->size - h->num <= HASH_GROW_SIZE / 2)
		resize_Hash(h, h->size + HASH_GROW_SIZE);

	return 0;
}

void remove_Hash(Hash *h, char *key) {
int addr;
KVPair *kv;

	if (h == NULL || h->hashaddr == NULL || h->size < HASH_MIN_SIZE || key == NULL || !*key)
		return;

	addr = h->hashaddr(key);
	if (addr < 0)
		addr = -addr;
	addr %= h->size;

	for(kv = h->hash[addr]; kv != NULL; kv = kv->next) {
		if (!strcmp(kv->key, key)) {
			kv->value.v = NULL;
			(void)remove_KVPair(&h->hash[addr], kv);
			destroy_KVPair(kv);

			h->num--;
			if (h->num < 0)
				h->num = 0;

/* auto-shrink the hash */
			if (h->size - h->num >= HASH_GROW_SIZE + HASH_GROW_SIZE / 2)
				resize_Hash(h, h->size - HASH_GROW_SIZE);

			return;
		}
	}
}

void *in_Hash(Hash *h, char *key) {
int addr;
KVPair *kv;

	if (h == NULL || h->hashaddr == NULL || h->size < HASH_MIN_SIZE || key == NULL || !*key)
		return NULL;

	addr = h->hashaddr(key);
	if (addr < 0)
		addr = -addr;
	addr %= h->size;

	for(kv = h->hash[addr]; kv != NULL; kv = kv->next) {
		if (!strcmp(kv->key, key))
			return kv->value.v;
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

int hashaddr_crc32(char *key) {
	return (int)update_crc32(0UL, key, strlen(key));
}

/* EOB */
