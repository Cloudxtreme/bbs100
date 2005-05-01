/*
    bbs100 2.2 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
	KVPair.c	WJ105

	This is a key-value pair
*/

#include "KVPair.h"
#include "Memory.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>


KVPair *new_KVPair(void) {
	return (KVPair *)Malloc(sizeof(KVPair), TYPE_KVPAIR);
}

void destroy_KVPair(KVPair *kv) {
	if (kv == NULL)
		return;

	Free(kv->key);

	if (kv->destroy != NULL)
		kv->destroy(kv->value.v);

	Free(kv);
}

void KVPair_setint(KVPair *kv, char *key, int value) {
	if (kv == NULL)
		return;

	if (key == NULL || !*key)
		kv->key = NULL;
	else
		kv->key = cstrdup(key);

	kv->value.i = value;
	kv->destroy = NULL;
}

void KVPair_setlong(KVPair *kv, char *key, long value) {
	if (kv == NULL)
		return;

	if (key == NULL || !*key)
		kv->key = NULL;
	else
		kv->key = cstrdup(key);

	kv->value.l = value;
	kv->destroy = NULL;
}

void KVPair_setstring(KVPair *kv, char *key, char *value) {
	if (kv == NULL)
		return;

	if (key == NULL || !*key)
		kv->key = NULL;
	else
		kv->key = cstrdup(key);

	if (value == NULL || !*value)
		kv->value.s = NULL;
	else
		kv->value.s = cstrdup(value);

	kv->destroy = Free;
}

void KVPair_setpointer(KVPair *kv, char *key, void *value, void (*destroy)(void *)) {
	if (kv == NULL)
		return;

	if (key == NULL || !*key)
		kv->key = NULL;
	else
		kv->key = cstrdup(key);

	kv->value.v = value;
	kv->destroy = destroy;
}

/* EOB */
