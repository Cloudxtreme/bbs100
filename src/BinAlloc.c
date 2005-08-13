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
	BinAlloc.c	WJ105

	simple allocator that works with a linear freelist

	While the bbs doesn't leak memory, the process tends to use
	much more memory then it's reporting itself that it's using.
	It's a problem that was very hard to put a finger on. The answer is
	that the bbs fragments memory like crazy due to the many small
	malloc()s that it does.
	Unix malloc() will break blocks into smaller pieces and not reclaim
	this memory until it's really needed to do so. As a result, the memory
	becomes 'filled up' with only tiny free blocks that are not coalesced
	into one large free block again => fragmentation.
	When the bbs then allocates a larger piece, the process will grow, even
	though the memory was really already free and available. malloc() didn't
	see this because it's broken up the memory into hundreds of tiny pieces.
	Theoretically, this goes until malloc() feels it's OK to do some
	house-cleaning, and from then on the process size will remain
	more or less stable.

	To fight this problem, I implemented this simple special purpose
	allocator, that works with chunks of malloc()ed memory and uses
	that as pool to hand out smaller sizes. It keeps track of free
	space by putting a linked list in between the allocated blocks.
	The advantage of using a linked list is that it's easy to find
	a free block. A binary search tree, or an AVL tree perform better
	when searching, but have more overhead otherwise.
	The freelist is sorted by size to find a free block efficiently.
	To be able to do free block coalescing, markers are used before
	and after allocated/free blocks, so that adjacent blocks can
	be combined.
	To make allocating known data structures faster, it uses multiple
	freelists; one for every type. The freelist of TYPE_CHAR is used
	as pool (the "wilderness") to split off smaller blocks.

	Advantages: much lower memory consumption
	Drawbacks:  can be (relatively?) slow when trying best-fit and
		having to skip over lots and lots of small blocks that are
		scattered over many pages

	Allocating a block:
		check freelist for a best-fit block
			- split the block if necessary
			- if not found, allocate new chunk and point to it
		set allocated bit + allocated size in marker preceding the block,
		but don't link it in the freelist -- link it 'out' instead
		if it was a split, set free size in next marker and link it in
		return pointer

	Freeing a block:
		determine what chunk this address is in by examining chunk addresses,
		and get the freelist root pointer for this chunk
		get size from preceding marker
		check if next marker is free
		if so, remove from freelist and join blocks
		reinsert in freelist
		if root freelist->next == NULL, entire chunk can be free()d

	This allocator works with tiny structures, so we want to keep the
	overhead as low as possible. The following rules apply:
	- chunks can be small, like PAGE_SIZE, or larger, but no more than 256K
	- a marker is 4 bytes
	- sizes/addresses in the marker are divided by 4 ... this malloc
	  rounds up sizes to the next quad
	- if on the freelist, a marker is a 2-byte pointer to the next
	  (sorted) free block in the chunk (addr/4) and a 2-byte size
	  of the following free space
	  NULL is invalid pointer because address 0 is in use by prev/next
	  pointers to next chunk anyway
	- if not on the freelist, a marker has the first byte set to the
	  magic 0xff, the second is a memory type, and has a 2-byte size of the
	  allocated block (size/4)   Mind that the check for 0xff is crude as
	  it also represents a valid address if it were on the freelist
	  and if you use a large chunk size
	- end of chunk is marked with a pointer to NULL, and size 0
	- address space is 2-bytes: 2^16 * 4 = 256K
	- waste is 4 bytes per malloc
	  Additionally there are 2 pointers for prev and next chunk, and 4 bytes for
	  the root freelist pointer in every chunk
	- allocs larger than (chunk_size - overhead) will be handled by malloc()
	  and free() and will be marked with the magic marker 0001

	Memory layout looks like this:

	+----------------------------+----------------------------+
	|  Chunk *prev               |  Chunk *next               |
	+-------------+--------------+-------------+--------------+
	|  chunksize  |  bytes_free  |  freelist * |  freelist *  |
	+-------------+--------------+-------------+--------------+
	|  freelist * |  freelist *  |  freelist * |  freelist *  |
	+-------------+--------------+-------------+--------------+
	|  marker p   |  size        |  data ...                  |
	+-------------+--------------+-------------+--------------+
	|  data ...                  |  marker p   |  size        |
	+----------------------------+-------------+--------------+
	|  data ...                  |  NULL       |  0           |
	+----------------------------+-------------+--------------+
*/

#include "config.h"
#include "BinAlloc.h"
#include "Memory.h"
#include "Param.h"
#include "memset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BinChunk *root_chunk = NULL;

int binalloc_balance = 0;
unsigned long bin_allocated = 0UL, foreign_alloc = 0UL;

int Chunk_Size = 4096;

static BinChunk *alloc_chunk(void);
static void insert_freelist(BinChunk *, int, int, int);
static void remove_freelist(BinChunk *, int, int);


int init_BinAlloc(void) {
	if ((root_chunk = alloc_chunk()) == NULL)
		return -1;

	Malloc = BinMalloc;
	Free = BinFree;
	return 0;
}

static BinChunk *alloc_chunk(void) {
BinChunk *c;
int n, hdr_size, start_addr;

	ROUND4(Chunk_Size);

	hdr_size = OFFSET_FREE_LIST(NUM_TYPES+1);
	ROUND4(hdr_size);

	if (Chunk_Size <= hdr_size)
		return NULL;

	if ((c = (BinChunk *)malloc(Chunk_Size)) == NULL)
		return NULL;

	memset(c, 0, hdr_size);

	ST_WORD(c, OFFSET_CHUNK_SIZE, Chunk_Size >> 2);			/* size of this chunk */

/*
	size of free space in this chunk
	the -1 is because end of memory is marked with 'NULL|0'
*/
	n = (((Chunk_Size - hdr_size) >> 2) - 1) & ~3;
	start_addr = hdr_size >> 2;
	ST_WORD(c, OFFSET_BYTES_FREE, n);
	ST_WORD(c, OFFSET_FREE_LIST(TYPE_CHAR), start_addr);	/* root freelist pointer for the pool */

	ST_ADDR(c, start_addr, 0);	/* root->next = NULL */
	ST_SIZE(c, start_addr, n);	/* fl->size = free space in this chunk */
	return c;
}

/*
	allocate memory
*/
void *BinMalloc(unsigned long size, int type) {
char *p;
int max_size;
unsigned long *ulptr;

	max_size = (Chunk_Size - (sizeof(BinChunk) + 4 + (NUM_TYPES << 1) + 4)) & ~3;
	if (size < max_size) {
/*
	small alloc: get from chunks
*/
		BinChunk *c;
		int bytes_free, prev_fl, fl_addr, fl_next, fl_size, alloc_type;

/*
	only allocate from type-specific freelist if exactly 1 object was requested
*/
		alloc_type = type;
		if (alloc_type > 0 && alloc_type < NUM_TYPES) {
			if (Types_table[alloc_type].size != size)
				alloc_type = TYPE_CHAR;
		} else
			alloc_type = TYPE_CHAR;

		ROUND4(size);
		size >>= 2;

		if (alloc_type != TYPE_CHAR) {
			for(c = root_chunk; c != NULL; c = c->next) {
/*
	find chunk with enough free space for this alloc
	this is still a gamble, because the sum of free space does not garantee
	we can satisfy the alloc here
*/
				LD_WORD(c, OFFSET_BYTES_FREE, bytes_free);
				if (bytes_free >= size) {
					prev_fl = 0;
					LD_WORD(c, OFFSET_FREE_LIST(alloc_type), fl_addr);
					while(fl_addr) {
						LD_ADDR(c, fl_addr, fl_next);
						if (fl_next < 0 || fl_next > (Chunk_Size >> 2)) {
							abort();			/* invalid address */
						}
						LD_SIZE(c, fl_addr, fl_size);
/*
	also satisfy for pieces that are just a bit bigger; splitting off a very
	small size wouldn't pay off
*/
						if (fl_size == size || fl_size == size+1 || fl_size == size+2) {
							if (prev_fl)						/* unlink this piece */
								ST_ADDR(c, prev_fl, fl_next);
							else
								ST_WORD(c, OFFSET_FREE_LIST(alloc_type), fl_next);

							ST_ALLOC(c, fl_addr, alloc_type);

							bytes_free -= fl_size;
							ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);

							CLEAR_MEM(c, fl_addr, size);
							return REAL_ADDRESS(c, fl_addr);
						}
						prev_fl = fl_addr;
						fl_addr = fl_next;
					}
				}
			}
		}
/*
	didn't find anything on the freelists
	try again, but take it from the pool
*/
		for(c = root_chunk; c != NULL; c = c->next) {
			LD_WORD(c, OFFSET_BYTES_FREE, bytes_free);
			if (bytes_free >= size) {
/* same as above, but now from the TYPE_CHAR pool */
				prev_fl = 0;
				LD_WORD(c, OFFSET_FREE_LIST(TYPE_CHAR), fl_addr);
				while(fl_addr) {
					LD_ADDR(c, fl_addr, fl_next);
					if (fl_next < 0 || fl_next > (Chunk_Size >> 2)) {
						abort();			/* invalid address */
					}
					LD_SIZE(c, fl_addr, fl_size);
/*
	also satisfy for pieces that are just a bit bigger; splitting off a very
	small size wouldn't pay off
*/
					if (fl_size == size || fl_size == size+1 || fl_size == size+2) {
						if (prev_fl)						/* unlink this piece */
							ST_ADDR(c, prev_fl, fl_next);
						else
							ST_WORD(c, OFFSET_FREE_LIST(TYPE_CHAR), fl_next);

						ST_ALLOC(c, fl_addr, alloc_type);

						bytes_free -= fl_size;
						ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);

						CLEAR_MEM(c, fl_addr, size);
						return REAL_ADDRESS(c, fl_addr);
					}
/*
	split off a piece from the pool
*/
					if (fl_size > size) {
						if (prev_fl)						/* unlink this piece */
							ST_ADDR(c, prev_fl, fl_next);
						else
							ST_WORD(c, OFFSET_FREE_LIST(TYPE_CHAR), fl_next);

						ST_ALLOC(c, fl_addr, alloc_type);		/* allocate */
						ST_SIZE(c, fl_addr, size);

/* split the piece; the marker takes up 1 cell as well */
						size++;
						fl_addr += size;
						fl_size -= size;
						insert_freelist(c, fl_addr, fl_size, TYPE_CHAR);

						bytes_free--;				/* the marker takes some space */
						ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);

						CLEAR_MEM(c, fl_addr - size, size);
						return REAL_ADDRESS(c, fl_addr - size);
					}
					prev_fl = fl_addr;
					fl_addr = fl_next;
				}
			}
/* out of chunks, allocate a new chunk */
			if (c->next == NULL) {
				if ((c->next = alloc_chunk()) == NULL)
					return NULL;

				c->next->prev = c;
				LD_WORD(c->next, OFFSET_BYTES_FREE, bytes_free);
				if (bytes_free < size)		/* well, that's odd ... this should never happen */
					return NULL;
			}
		}
		/* we shouldn't get here */
	}
/*
	requested size is too large to fit in a chunk, call standard malloc()
*/
	size += 4 + sizeof(unsigned long);
	if ((ulptr = (unsigned long *)malloc(size)) == NULL)
		return NULL;

	*ulptr = size;
	ulptr++;
	p = (char *)ulptr;
	ST_ADDR(p, 0, 1);				/* 0001 means 'malloc() used'  */
	ST_SIZE(p, 0, 0xff00 | (type & 0xff));

	binalloc_balance++;
	bin_allocated += size;
	foreign_alloc += size;

	memset(p+4, 0, size);
	return (void *)(p + 4);
}

/*
	free allocated memory
*/
void BinFree(void *ptr) {
char *p;
int addr, type, size;

	if (ptr == NULL)
		return;

	p = (char *)ptr - 4;
	LD_WORD(p, 0, addr);

	if (addr == 1) {				/* was allocated by malloc() */
		unsigned long *ulptr, ulsize;

		if ((unsigned char)p[3] != 0xff) {		/* check for corruption */
			return;
		}
		type = p[4] & 0xff;

		ulptr = (unsigned long *)p;
		ulptr--;
		ulsize = *ulptr;

		binalloc_balance--;
		bin_allocated -= ulsize;
		foreign_alloc -= ulsize;

		free(ulptr);
		return;
	} else {
		BinChunk *c;
		int chunk_size, bytes_free;
		char *end;

		if ((unsigned char)p[0] != 0xff) {		/* check for corruption */
			return;
		}
		type = p[1] & 0xff;

		LD_SIZE(p, 0, size);

		for(c = root_chunk; c != NULL; c = c->next) {
			LD_WORD(c, OFFSET_CHUNK_SIZE, chunk_size);
			chunk_size <<= 2;
			end = (char *)c + chunk_size;
			if ((unsigned long)p > (unsigned long)c && (unsigned long)p < (unsigned long)end) {
				addr = (unsigned long)p - (unsigned long)c;
				if (addr % 4) {				/* corrupted */
					return;
				}
				addr >>= 2;
				insert_freelist(c, addr, size, type);

				LD_WORD(c, OFFSET_BYTES_FREE, bytes_free);
				bytes_free += size;
				ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);
				return;
			}
		}
	}
	/* not found, problem, just let it leak */
}

/*
	freelists are sorted by size
*/
static void insert_freelist(BinChunk *c, int addr, int size, int alloc_type) {
int prev_fl, fl_addr, fl_next = 0, fl_size;

	if (c == NULL || !addr || size <= 0 || alloc_type < 0 || alloc_type >= NUM_TYPES)
		return;

/*
	try to do merging
	merging is slow and it has a habit of eliminating type-specific freelists,
	and thereby slowing down allocations
	it does, however, return memory to the free pool and fight fragmentation

	Note that high addresses are never merged because there is no way of
	telling whether they are allocated or free. This only happens when using
	the maximum chunk size (256K)
*/
	LD_ADDR(c, addr + size+1, fl_next);
	if (fl_next && (fl_next & 0xff00) != 0xff00) {	/* prob for high addresses */
		int merge_size, bytes_free;

		LD_SIZE(c, addr + size+1, merge_size);
		remove_freelist(c, addr + size+1, fl_next & 0xff);

		size++;
		size += merge_size;

		LD_WORD(c, OFFSET_BYTES_FREE, bytes_free);
		bytes_free++;					/* marker space released */
		ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);

		alloc_type = TYPE_CHAR;			/* insert it back into the pool */
	}
	prev_fl = 0;
	LD_WORD(c, OFFSET_FREE_LIST(alloc_type), fl_addr);
	while(fl_addr) {
		LD_ADDR(c, fl_addr, fl_next);
		if (fl_next < 0 || fl_next > (Chunk_Size >> 2)) {
			abort();			/* invalid address */
		}
		LD_SIZE(c, fl_addr, fl_size);
		if (fl_size >= size)
			break;

		prev_fl = fl_addr;
		fl_addr = fl_next;
	}
	if (prev_fl)
		ST_ADDR(c, prev_fl, addr);
	else
		ST_WORD(c, OFFSET_FREE_LIST(alloc_type), addr);

	ST_ADDR(c, addr, fl_addr);
	ST_SIZE(c, addr, size);
}

/*
	remove a piece from its freelist
	this routine is a bit inefficient because there is no prev pointer
	in a marker, so it walks the freelist from the start
	so do not use this routine in malloc, only for free block merging
*/
static void remove_freelist(BinChunk *c, int addr, int alloc_type) {
int prev_fl, fl_addr, fl_next;

	if (c == NULL || addr <= 0 || alloc_type < 0 || alloc_type >= NUM_TYPES)
		return;

	prev_fl = 0;
	LD_WORD(c, OFFSET_FREE_LIST(alloc_type), fl_addr);
	while(fl_addr) {
		LD_ADDR(c, fl_addr, fl_next);
		if (fl_next < 0 || fl_next > (Chunk_Size >> 2)) {
			abort();			/* invalid address */
		}
		if (fl_addr == addr) {
			ST_ADDR(c, fl_addr, 0);
			ST_ADDR(c, prev_fl, fl_next);
			return;
		}
		prev_fl = fl_addr;
		fl_addr = fl_next;
	}
	/* shouldn't get here */
}

/* EOB */
