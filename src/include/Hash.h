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
	Hash.h	WJ103
*/

#ifndef HASH_H_WJ103
#define HASH_H_WJ103	1

#include "KVPair.h"

#define HASH_MIN_SIZE	32
#define HASH_GROW_SIZE	32

#define MAX_HASH_KEY	32

typedef struct HashList_tag HashList;

typedef struct {
	int size, num;

	int (*hashaddr)(char *);
	KVPair **hash;
} Hash;

Hash *new_Hash(void);
void destroy_Hash(Hash *);

int resize_Hash(Hash *, int);

int add_Hash(Hash *, char *, void *, void (*)(void *));
void remove_Hash(Hash *, char *);
void *in_Hash(Hash *, char *);

int hashaddr_ascii(char *);
int hashaddr_crc32(char *);

#endif	/* HASH_H_WJ103 */

/* EOB */
