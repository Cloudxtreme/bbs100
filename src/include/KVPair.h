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
	KVPair.h	WJ105
*/

#ifndef KVPAIR_H_WJ105
#define KVPAIR_H_WJ105	1

#include "List.h"

#define add_KVPair(x,y)			(KVPair *)add_List((x), (y))
#define prepend_KVPair(x,y)		(KVPair *)prepend_List((x), (y))
#define remove_KVPair(x,y)		(KVPair *)remove_List((x), (y))
#define listdestroy_KVPair(x)	listdestroy_List((x), destroy_KVPair)
#define rewind_KVPair(x)		(KVPair *)rewind_List(x)
#define unwind_KVPair(x)		(KVPair *)unwind_List(x)
#define sort_KVPair(x, y)		(KVPair *)sort_List((x), (y))

#define KV_UNKNOWN	0
#define KV_BOOL		1
#define KV_INT		2
#define KV_OCTAL	3
#define KV_LONG		4
#define KV_STRING	5
#define KV_POINTER	6

#define KV_TRUE		1
#define KV_FALSE	0

typedef union {
	int i, o, bool;
	long l;
	char *s;
	void *v;
} KVPair_value;

typedef struct KVPair_tag KVPair;

struct KVPair_tag {
	List(KVPair);

	int type;					/* mostly for debugging purposes only */
	char *key;
	KVPair_value value;
	void (*destroy)(void *);
};

KVPair *new_KVPair(void);
void destroy_KVPair(KVPair *);

void KVPair_setbool(KVPair *, char *, int);
void KVPair_setint(KVPair *, char *, int);
void KVPair_setoctal(KVPair *, char *, int);
void KVPair_setlong(KVPair *, char *, long);
void KVPair_setstring(KVPair *, char *, char *);
void KVPair_setpointer(KVPair *, char *, void *, void (*)(void *));

int KVPair_getbool(KVPair *);
int KVPair_getint(KVPair *);
int KVPair_getoctal(KVPair *);
long KVPair_getlong(KVPair *);
char *KVPair_getstring(KVPair *);
void *KVPair_getpointer(KVPair *);

int print_KVPair(KVPair *, char *, int);

#endif	/* KVPAIR_H_WJ105 */

/* EOB */
