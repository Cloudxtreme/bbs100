/*
    bbs100 3.2 WJ107
    Copyright (C) 2007  Walter de Jong <walter@heiho.net>

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
	Slub.h WJ115
*/

#ifndef SLUB_H_WJ115
#define SLUB_H_WJ115	1

#include "List.h"

#define prepend_Slub(x,y)	(Slub *)prepend_List((x), (y))
#define remove_Slub(x,y)	(Slub *)remove_List((x), (y))

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

typedef struct {
	unsigned int size;
	Slub *full, *partial;
} MemCache;

typedef struct {
	void *page;
	MemCache *memcache;
} SlubPageTable;

void init_MemCache(void);

void *memcache_alloc(unsigned int);
void memcache_free(void *);

#endif	/* SLUB_H_WJ115 */

/* EOB */
