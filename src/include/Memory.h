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
	Memory.h	WJ100
*/

#ifndef MEMORY_H_WJ100
#define MEMORY_H_WJ100

#include "Types.h"
#include "debug.h"

/*
	all routines use Malloc() and Free()
*/

#ifdef USE_SLUB

#include "Slub.h"

#define Malloc(x,y)		memcache_alloc(x)
#define Free(x)			do {					\
							memcache_free(x);	\
							(x) = NULL;			\
						} while(0)

#else

#include <stdlib.h>

#define Malloc(x,y)		memalloc(x)
#define Free(x)			do {				\
							free(x);		\
							(x) = NULL;		\
						} while(0)

void *memalloc(size_t);

#endif	/* USE_SLUB */

int init_Memory(void);

#ifdef DEBUG
void test_Memory(void);
#endif	/* DEBUG */

#endif	/* MEMORY_H_WJ100 */

/* EOB */
