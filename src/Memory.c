/*
    bbs100 3.0 WJ105
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
	Memory.c	WJ100
*/

#include "config.h"
#include "Memory.h"
#include "Param.h"
#include "memset.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MemFreeList *free_list = NULL;

unsigned long memory_total = 0UL;
unsigned long mem_stats[NUM_TYPES+1];

int alloc_balance = 0, alloc_boot_balance = 0;


int init_Memory(void) {
	memory_total = 0UL;
	memset(mem_stats, 0, sizeof(unsigned long) * (NUM_TYPES+1));

	init_memcache();
	return 0;
}

int init_memcache(void) {
	if (free_list != NULL)
		return -1;

	if ((free_list = (MemFreeList *)malloc(NUM_TYPES * sizeof(MemFreeList))) == NULL)
		return -1;

	memset(free_list, 0, NUM_TYPES * sizeof(MemFreeList));
	return 0;
}

void deinit_memcache(void) {
int n, memtype;

	if (free_list == NULL)
		return;

	for(memtype = 0; memtype < NUM_TYPES; memtype++) {
		for(n = 0; n < NUM_FREELIST; n++) {
			if (free_list[memtype].free[n] != NULL) {
				Free(free_list[memtype].free[n]);
				free_list[memtype].free[n] = NULL;
			}
		}
	}
	free(free_list);
	free_list = NULL;
}


/*
	get something from the freelist
*/
static void *get_freelist(int memtype) {
void *mem;
int n;

	if (free_list == NULL)
		return NULL;

	for(n = 0; n < NUM_FREELIST; n++) {
		if (free_list[memtype].free[n] != NULL) {
			mem = free_list[memtype].free[n];
			free_list[memtype].free[n] = NULL;
			return mem;
		}
	}
	return NULL;
}

/*
	put something on the freelist
*/
static int put_freelist(void *mem, int memtype) {
int n;

	if (free_list == NULL)
		return -1;

	for(n = 0; n < NUM_FREELIST; n++) {
		if (free_list[memtype].free[n] == NULL) {
			free_list[memtype].free[n] = mem;
			memset(mem, 0, Types_table[memtype].size + 2*sizeof(int));	/* zero it out */
			return 0;
		}
	}
	return -1;
}

/*
	Malloc() puts the size and type before the pointer it returns
	as a sanity check, it also inserts the character 'A' before
	the memory type, so Free() can see if the pointer is valid at all
	(if we haven't already crashed by then, that is)

	Malloc() first checks the free_list for quick allocation
*/
void *Malloc(unsigned long size, int memtype) {
void *mem;

	if (size <= 0)
		return NULL;

	if (memtype < 0 || memtype >= (NUM_TYPES+1))
		return NULL;

/*	log_debug("Malloc(): type %s: 0x%08lx", Types_table[memtype].type, (unsigned long)mem);	*/

/*
	asked for 1 item; search it in the free_list
*/
	if (size == Types_table[memtype].size) {
		if ((mem = get_freelist(memtype)) != NULL) {
			((int *)mem)[0] = 1UL;
			((int *)mem)[1] = ('A' << 8) | memtype;
			alloc_balance++;
			return (void *)((char *)mem + 2*sizeof(int));
		}
	}
	if ((mem = (char *)malloc(size + 2*sizeof(int))) == NULL) {
		log_err("Malloc(): out of memory allocating %d bytes for type %s", size + 2*sizeof(int), Types_table[memtype].type);
		return NULL;
	}
	memset(mem, 0, size + 2*sizeof(int));		/* malloc() sets it to 0, yeah right! :P */
	memory_total += (size + 2*sizeof(int));

	if (memtype < NUM_TYPES) {
		size /= Types_table[memtype].size;
		mem_stats[memtype] += size;
	} else
		mem_stats[NUM_TYPES]++;		/* unknown type */

	((int *)mem)[0] = (int)size;
	((int *)mem)[1] = ('A' << 8) | memtype;

	alloc_balance++;
	return (void *)((char *)mem + 2*sizeof(int));
}

void Free(void *ptr) {
int *mem, size;

	if (ptr == NULL)
		return;

/*	log_debug("Free(): freeing ptr 0x%08lx", (unsigned long)ptr);	*/

	mem = (int *)((char *)ptr - 2*sizeof(int));

	if ((mem[1] >> 8) != 'A') {							/* crude sanity check */
		log_err("Free(): sanity check failed");
		return;
	}
	mem[1] &= 0xff;
	if (mem[1] >= 0 && mem[1] < NUM_TYPES) {
		if (mem[0] == 1UL && !put_freelist(mem, mem[1])) {
			alloc_balance--;
			return;
		}
		size = mem[0] * Types_table[mem[1]].size;
		mem_stats[mem[1]] -= mem[0];
	} else {
		size = mem[0];
		mem_stats[NUM_TYPES]--;		/* unknown type */
	}
	mem[1] = 0UL;

	memory_total -= (size + 2*sizeof(int));
	alloc_balance--;
	free(mem);
}

/* EOB */
