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
	KVPair.h	WJ105
*/

#ifndef KVPAIR_H_WJ105
#define KVPAIR_H_WJ105	1

#include "List.h"

#define add_KVPair(x,y)			(KVPair *)add_List((x), (y))
#define remove_KVPair(x,y)		(KVPair *)remove_List((x), (y))
#define listdestroy_KVPair(x)	listdestroy_List((x), destroy_KVPair)
#define rewind_KVPair(x)		(KVPair *)rewind_List(x)
#define unwind_KVPair(x)		(KVPair *)unwind_List(x)
#define sort_KVPair(x, y)		(KVPair *)sort_List((x), (y))

#define KV_UNKNOWN	0
#define KV_BOOL		1
#define KV_INT		2
#define KV_LONG		3
#define KV_STRING	4
#define KV_POINTER	5

typedef union {
	int i;
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
void KVPair_setlong(KVPair *, char *, long);
void KVPair_setstring(KVPair *, char *, char *);
void KVPair_setpointer(KVPair *, char *, void *, void (*)(void *));

int KVPair_getbool(KVPair *);
int KVPair_getint(KVPair *);
long KVPair_getlong(KVPair *);
char *KVPair_getstring(KVPair *);
void *KVPair_getpointer(KVPair *);

int print_KVPair(KVPair *, char *);

#endif	/* KVPAIR_H_WJ105 */

/* EOB */
