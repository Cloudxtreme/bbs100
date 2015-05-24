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
	Slub.h WJ115
*/

#ifndef SLUB_H_WJ115
#define SLUB_H_WJ115	1

#include "Queue.h"

#define add_Slub(x,y)		(Slub *)add_Queue((x), (y))
#define remove_Slub(x,y)	(Slub *)remove_Queue((x), (y))

#define SLUB_PAGESIZE		4096

#define MAX_SLUB_OBJSIZE	256
#define SLUB_SIZESTEP		16
#define NUM_MEMCACHES		(MAX_SLUB_OBJSIZE / SLUB_SIZESTEP)

#define INIT_PAGETABLE_SIZE	512
#define GROW_PAGETABLE		512

typedef struct Slub_tag Slub;

struct Slub_tag {
	List(Slub);
	unsigned int first, numfree;
	void *page;
};

typedef QueueType SlubQueue;

typedef struct {
	unsigned int size;
	SlubQueue full, partial;
} MemCache;

typedef struct {
	void *page;
	MemCache *memcache;
	Slub *slub;
} SlubPageTable;

typedef struct {
	int nr_cache[NUM_MEMCACHES];
	int nr_cache_all;
	int nr_pages;
	int nr_foreign;
	long cache_bytes;
} MemCacheInfo;

extern MemCacheInfo memcache_info;

void init_MemCache(void);
void dump_Memcache(void);
void *memcache_alloc(unsigned int);
void memcache_free(void *);

#endif	/* SLUB_H_WJ115 */

/* EOB */
