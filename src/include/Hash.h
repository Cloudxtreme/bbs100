/*
    bbs100 2.1 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	Hash.h	WJ103
*/

#ifndef HASH_H_WJ103
#define HASH_H_WJ103	1

#include "List.h"

#define add_HashList(x,y)		(HashList *)add_List((x), (y))
#define concat_HashList(x,y)	(HashList *)concat_List((x), (y))
#define remove_HashList(x,y)	remove_List((x), (y))
#define listdestroy_HashList(x)	listdestroy_List((x), destroy_HashList)
#define rewind_HashList(x)		(HashList *)rewind_List(x)
#define unwind_HashList(x)		(HashList *)unwind_List(x)

#define HASH_MIN_SIZE	32
#define HASH_GROW_SIZE	32

#define MAX_HASH_KEY	32

typedef struct HashList_tag HashList;

struct HashList_tag {
	List(HashList);

	char key[MAX_HASH_KEY];
	void *value;
};

typedef struct {
	int size, num;

	int (*hashaddr)(char *);
	HashList **hash;
} Hash;

Hash *new_Hash(void);
void destroy_Hash(Hash *);

int resize_Hash(Hash *, int);

int add_Hash(Hash *, char *, void *);
void *remove_Hash(Hash *, char *);
void *in_Hash(Hash *, char *);

int hashaddr_ascii(char *);

#endif	/* HASH_H_WJ103 */

/* EOB */
