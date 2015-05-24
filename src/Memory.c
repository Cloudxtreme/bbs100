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
	Memory.c	WJ100
*/

#include "config.h"
#include "Memory.h"
#include "Slub.h"

#ifndef USE_SLUB
#include "calloc.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int init_Memory(void) {
#ifdef USE_SLUB
	init_MemCache();
#endif /* USE_SLUB */
	return 0;
}

#ifndef USE_SLUB
void *memalloc(size_t n) {
	return calloc(1, n);
}
#endif	/* USE_SLUB */

#ifdef DEBUG
void test_Memory(void) {
char *p;
int i;

	dump_Memcache();
	p = (char *)Malloc(12, TYPE_CHAR);
	Free(p);
	dump_Memcache();

	for(i = 0; i < 1024; i++) {
		p = Malloc(12, TYPE_CHAR);
		Free(p);
		p = Malloc(36, TYPE_CHAR);
		Free(p);
		p = Malloc(78, TYPE_CHAR);
		Free(p);
		p = Malloc(148, TYPE_CHAR);
		Free(p);
		p = Malloc(262, TYPE_CHAR);
		Free(p);
		p = Malloc(522, TYPE_CHAR);
		Free(p);
	}
	dump_Memcache();
}
#endif	/* DEBUG */

/* EOB */
