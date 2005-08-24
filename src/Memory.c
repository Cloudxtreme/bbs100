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

	Memory allocation in bbs100 has a staged design;
	- routines call Malloc() and Free()
	- Malloc() and Free() are defined to MemAlloc() and MemFree()
	- MemAlloc() and MemFree() can be configured to use a memory object cache; a 'free list' to
	  speed up allocation of single objects (such as StringLists, StringIO objects, PLists, etc)
	- MemAlloc() and MemFree() use a generic ALLOCATOR and DEALLOCTOR function to get that memory
	- ALLOCATOR() and DEALLOCATOR() are defined to BinMalloc() and BinFree()
	- in turn, BinMalloc() and BinFree() can be configured to directly call malloc() and free()
*/

#include "config.h"
#include "Memory.h"
#include "BinAlloc.h"
#include "Param.h"
#include "memset.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MemFreeList *free_list = NULL;

int init_Memory(void) {
	init_BinAlloc();
/*	init_MemCache();		TD TODO uncomment this		*/
	return 0;
}

int init_MemCache(void) {
	if (free_list != NULL)
		return -1;
/*
	it's not really TYPE_POINTER, but I'm cheating here because I don't really want
	to make	a TYPE_MEMFREELIST (as it would be useless to have it)
*/
	if ((free_list = (MemFreeList *)Malloc(NUM_TYPES * sizeof(MemFreeList), TYPE_POINTER)) == NULL)
		return -1;

	return 0;
}

void deinit_MemCache(void) {
int n, memtype;

	if (free_list == NULL)
		return;

	for(memtype = 0; memtype < NUM_TYPES; memtype++) {
		for(n = 0; n < NUM_FREELIST; n++) {
			if (free_list[memtype].slots[n] != NULL)
				Free(free_list[memtype].slots[n]);
		}
	}
	Free(free_list);
	free_list = NULL;
}


/*
	get something from the freelist
*/
static void *get_freelist(int memtype) {
void *mem;
int n;

	if (free_list == NULL || memtype < 0 || memtype >= NUM_TYPES
		|| free_list[memtype].in_use <= 0)
		return NULL;

	for(n = 0; n < NUM_FREELIST; n++) {
		if (free_list[memtype].slots[n] != NULL) {
			mem = free_list[memtype].slots[n];
			free_list[memtype].slots[n] = NULL;
			free_list[memtype].in_use--;
			if (free_list[memtype].in_use < 0) {
				log_err("get_freelist(): corrupted free_list for type %s", Types_table[memtype].type);
				memset(&free_list[memtype], 0, sizeof(MemFreeList));
				return NULL;
			}
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

	if (free_list == NULL || mem == NULL || memtype < 0 || memtype >= NUM_TYPES
		|| free_list[memtype].in_use >= NUM_FREELIST)
		return -1;

	for(n = 0; n < NUM_FREELIST; n++) {
		if (free_list[memtype].slots[n] == NULL) {
			free_list[memtype].slots[n] = mem;
			free_list[memtype].in_use++;
			if (free_list[memtype].in_use > NUM_FREELIST) {
				log_err("put_freelist(): corrupted free_list for type %s", Types_table[memtype].type);
				memset(&free_list[memtype], 0, sizeof(MemFreeList));
				return -1;
			}
			memset(mem, 0, Types_table[memtype].size);	/* zero it out */
			return 0;
		}
	}
	return -1;
}

/*
	allocate memory
	try getting it from the free_list, if active
*/
void *MemAlloc(unsigned long size, int memtype) {
char *mem;
int n;

	if (!size || memtype < 0 || memtype >= NUM_TYPES)
		return NULL;
/*
	asked for 1 item; search it in the free_list
*/
	if (free_list != NULL && size == Types_table[memtype].size
		&& (mem = get_freelist(memtype)) != NULL)
		return mem;

	if ((mem = (char *)ALLOCATOR(size+1, memtype)) == NULL)
		return NULL;

	n = size / Types_table[memtype].size;
	if (size % Types_table[memtype].size)
		n++;

	if (n == 1)
		*mem = (char)(memtype & 0xff);		/* candidate for free_list, later */
	else
		*mem = 0xff;
	return (void *)(mem + 1);
}

/*
	free memory that was allocated with MemAlloc()
	put it on the free_list, if active
*/
void MemFree(void *ptr) {
char *mem;
int type;

	if (ptr == NULL)
		return;

	mem = (char *)ptr;
	mem--;
	type = *mem & 0xff;

	if (free_list != NULL && type < NUM_TYPES && !put_freelist(ptr, type))
		return;

	DEALLOCATOR(mem);
}

/*
	return number of slots in use
	arr should be at least NUM_TYPES in size
*/
int get_MemFreeListInfo(int *arr) {
	if (free_list == NULL)
		memset(arr, 0, NUM_TYPES * sizeof(int));
	else {
		int i;

		for(i = 0; i < NUM_TYPES; i++)
			arr[i] = free_list[i].in_use;
	}
	return 0;
}

/* EOB */
