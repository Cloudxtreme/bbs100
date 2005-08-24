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
	Memory.h	WJ100
*/

#ifndef MEMORY_H_WJ100
#define MEMORY_H_WJ100

#include "Types.h"
#include "Timer.h"
#include "sys_time.h"

/*
	all routines use Malloc() and Free()
*/
#define Malloc(x,y)		MemAlloc((x),(y))
#define Free(x)			MemFree(x)

/*
	alloc/free functions called by MemAlloc() and MemFree()
*/
#define ALLOCATOR(x,y)	BinMalloc((x),(y))
#define DEALLOCATOR(x)	BinFree(x)


#define NUM_FREELIST	8

typedef struct {
	void *slots[NUM_FREELIST];
	int in_use;
} MemFreeList;

typedef struct {
	int num, balance;
	unsigned long size;
} MemStats;

int init_Memory(void);
int init_MemCache(void);
void deinit_MemCache(void);
void *MemAlloc(unsigned long, int);
void MemFree(void *);

int get_MemStats(MemStats *, int);
int get_MemFreeListInfo(int *);

#endif	/* MEMORY_H_WJ100 */

/* EOB */
