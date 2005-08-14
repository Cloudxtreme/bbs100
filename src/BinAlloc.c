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
	Drawbacks:  it's slow

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
	|  chunksize  |  bytes_free  |  freelist * |  unused      |
	+-------------+--------------+-------------+--------------+
	|  marker     |  size        |  data ...                  |
	+-------------+--------------+-------------+--------------+
	|  data ...                  |  marker     |  size        |
	+----------------------------+-------------+--------------+
	|  data ...                                               |
	+---------------------------------------------------------+
*/

#include "config.h"
#include "debug.h"
#include "BinAlloc.h"
#include "Memory.h"
#include "Param.h"
#include "memset.h"
#include "Types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BinChunk *root_chunk = NULL;
int Chunk_Size = 16*1024;
unsigned long bin_use = 0UL, bin_foreign = 0UL;

static BinChunk *alloc_chunk(void);
static void insert_freelist(BinChunk *, int, int);
static void remove_freelist(BinChunk *, int);


int init_BinAlloc(void) {
	if ((root_chunk = alloc_chunk()) == NULL)
		return -1;

	Malloc = BinMalloc;
	Free = BinFree;
	return 0;
}

static BinChunk *alloc_chunk(void) {
BinChunk *c;
int max_free, hdr_size, start_addr;

	ROUND4(Chunk_Size);

	hdr_size = OFFSET_FREE_LIST+2;
	ROUND4(hdr_size);

/*
	size of free space in this chunk
	the -1 is because start of memory always begins with a marker
*/
	max_free = ((Chunk_Size - hdr_size) >> 2) - 1;
	if (max_free <= 256)			/* need at least some space... */
		return NULL;

	if ((c = (BinChunk *)malloc(Chunk_Size)) == NULL)
		return NULL;

	memory_total += Chunk_Size;
	memset(c, 0, hdr_size);

	ST_WORD(c, OFFSET_CHUNK_SIZE, Chunk_Size >> 2);		/* size of this chunk */
	ST_WORD(c, OFFSET_BYTES_FREE, max_free);

	start_addr = hdr_size >> 2;
	ST_WORD(c, OFFSET_FREE_LIST, start_addr);	/* root freelist pointer for the pool */

	ST_ADDR(c, start_addr, 0);			/* root->next = NULL */
	ST_SIZE(c, start_addr, max_free);	/* fl->size = free space in this chunk */
	return c;
}

/*
	allocate memory
*/
void *BinMalloc(unsigned long size, int type) {
char *p;
int max_free, hdr_size, n;
unsigned long *ulptr;

	if (type < 0 || type >= NUM_TYPES)
		type = TYPE_CHAR;

	mem_balance[type]++;
	alloc_balance++;

	ROUND4(size);

	hdr_size = OFFSET_FREE_LIST+2;
	ROUND4(hdr_size);
	max_free = Chunk_Size - hdr_size - 4;
	if (size < max_free) {
/*
	small alloc: get from chunks
*/
		BinChunk *c;
		int chunk_size, bytes_free, prev_fl, fl_addr, fl_next, fl_size, fl_adj;

		size >>= 2;
		max_free >>= 2;

		for(c = root_chunk; c != NULL; c = c->next) {
			LD_WORD(c, OFFSET_CHUNK_SIZE, chunk_size);
			LD_WORD(c, OFFSET_BYTES_FREE, bytes_free);
			if (bytes_free < 0 || bytes_free > max_free)
				abort();

			if (bytes_free >= size) {
/*
	this is a 'good guess'; if the sum of the free space is enough, maybe
	we can get a good malloc here
*/
				prev_fl = 0;
				LD_WORD(c, OFFSET_FREE_LIST, fl_addr);
				while(fl_addr) {
					LD_ADDR(c, fl_addr, fl_next);
					LD_SIZE(c, fl_addr, fl_size);
					if (size > max_free)
						abort();
/*
	also satisfy for pieces that are just a bit bigger; splitting off a very
	small size wouldn't pay off
*/
					if (fl_size == size || fl_size == size+1 || fl_size == size+2) {
						if (prev_fl)						/* unlink this piece */
							ST_ADDR(c, prev_fl, fl_next);
						else
							ST_WORD(c, OFFSET_FREE_LIST, fl_next);

						ST_ALLOC(c, fl_addr, type);
						n = (fl_size << 2) / Types_table[type].size;
						mem_stats[type] += n;

						bytes_free -= fl_size;
						if (bytes_free < 0)
							abort();
						ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);

						bin_use += (size << 2);

						CLEAR_MEM(c, fl_addr, size);
						return REAL_ADDRESS(c, fl_addr);
					}
/*
	split a piece in two
*/
					if (fl_size > size) {
						if (prev_fl)					/* unlink this piece */
							ST_ADDR(c, prev_fl, fl_next);
						else
							ST_WORD(c, OFFSET_FREE_LIST, fl_next);

						ST_ALLOC(c, fl_addr, type);		/* allocate */
						ST_SIZE(c, fl_addr, size);
						n = (size << 2) / Types_table[type].size;
						mem_stats[type] += n;

/* split the piece; the marker takes up 1 cell as well */
						size++;
						fl_addr += size;
						fl_size -= size;

						insert_freelist(c, fl_addr, fl_size);

						bytes_free -= size;			/* we allocated size, and the marker takes 1 cell as well */
						if (bytes_free < 0)
							abort();
						ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);

						bin_use += ((size-1) << 2);

						CLEAR_MEM(c, fl_addr - size, size-1);
						return REAL_ADDRESS(c, fl_addr - size);
					}
/*
	the piece is too small, but maybe it can be merged with the next one
	mind to check for 'end of memory', or else it just overflows
*/
					if ((fl_addr + fl_size + 1) < chunk_size) {
						LD_WORD(c, (fl_addr + fl_size+1) << 2, fl_adj);
						if (fl_adj && (fl_adj & 0xff00) != 0xff00) {	/* prob for high addresses */
							int merge_size;

							LD_SIZE(c, fl_addr + fl_size+1, merge_size);

							remove_freelist(c, fl_addr + fl_size+1);

							fl_size++;
							fl_size += merge_size;
							ST_SIZE(c, fl_addr, fl_size);

							bytes_free++;					/* marker space released */
							if (bytes_free > max_free)
								abort();
							ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);
/*
	the freelist is sorted by size
	therefore we must now reinsert this merged piece into the freelist
*/
							remove_freelist(c, fl_addr);
							insert_freelist(c, fl_addr, fl_size);
/*
	because the list is sorted, we must restart the search from the beginning
*/
							prev_fl = 0;
							LD_WORD(c, OFFSET_FREE_LIST, fl_addr);
							continue;
						}
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
				if (bytes_free < size) {		/* well, that's odd ... this should never happen */
					abort();
					return NULL;
				}
			}
		}
		abort();		/* should never get here */
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
	ST_WORD(p, 2, 0xff00 | (type & 0xff));

	memory_total += size;
	bin_foreign += size;
	n = size / Types_table[type].size;
	mem_stats[type] += n;

	p = p + 4;
	size -= (sizeof(unsigned long) + 4);
	memset(p, 0, size);
	return (void *)p;
}

/*
	free allocated memory
*/
void BinFree(void *ptr) {
char *p;
int addr, type, size, n;

	if (ptr == NULL)
		return;

	alloc_balance--;

	p = (char *)ptr - 4;
	LD_WORD(p, 0, addr);

	if (addr == 1) {				/* was allocated by malloc() */
		unsigned long *ulptr, ulsize;

		debug_breakpoint();
		LD_WORD(p, 2, size);
		if ((size & 0xff00) != 0xff00) {		/* check for corruption */
			abort();
		}
		type = size & 0xff;
		if (type < 0 || type > NUM_TYPES)
			type = TYPE_CHAR;
		mem_balance[type]--;

		ulptr = (unsigned long *)p;
		ulptr--;
		ulsize = *ulptr;

		memory_total -= ulsize;
		bin_foreign -= ulsize;

		n = ulsize / Types_table[type].size;
		mem_stats[type] -= n;

		free(ulptr);
		return;
	} else {
		BinChunk *c;
		int chunk_size, bytes_free, hdr_size;
		char *end;

		if ((unsigned char)p[0] != 0xff) {		/* check for corruption */
			abort();
		}
		type = p[1] & 0xff;
		if (type < 0 || type > NUM_TYPES)
			type = TYPE_CHAR;
		mem_balance[type]--;

		LD_SIZE(p, 0, size);

		n = (size << 2) / Types_table[type].size;
		mem_stats[type] -= n;

		for(c = root_chunk; c != NULL; c = c->next) {
			LD_WORD(c, OFFSET_CHUNK_SIZE, chunk_size);
			chunk_size <<= 2;
			end = (char *)c + chunk_size;
			if ((unsigned long)p > (unsigned long)c && (unsigned long)p < (unsigned long)end) {
				addr = (unsigned long)p - (unsigned long)c;
				if (addr % 4)				/* corrupted */
					abort();

				addr >>= 2;
				insert_freelist(c, addr, size);

				bin_use -= (size << 2);

				LD_WORD(c, OFFSET_BYTES_FREE, bytes_free);
				bytes_free += size;
				ST_WORD(c, OFFSET_BYTES_FREE, bytes_free);
/*
	see if this entire chunk is empty
*/
				if (c->prev != NULL) {
					int max_free;

					hdr_size = OFFSET_FREE_LIST+2;
					ROUND4(hdr_size);
					max_free = ((chunk_size - hdr_size) >> 2) - 1;
					if (bytes_free > max_free)
						abort();

					if (bytes_free == max_free) {
						c->prev->next = c->next;
						c->prev = c->next = NULL;
						free(c);
						memory_total -= chunk_size;
					}
				}
				return;
			}
		}
	}
	abort();	/* not found, problem */
}

/*
	freelists are sorted by size
*/
static void insert_freelist(BinChunk *c, int addr, int size) {
int prev_fl, fl_addr, fl_next, fl_size;

	if (c == NULL || !addr || size <= 0)
		return;

	ASSERT_ADDR(addr);
	ASSERT_SIZE(size);

	prev_fl = 0;
	LD_WORD(c, OFFSET_FREE_LIST, fl_addr);
	while(fl_addr) {
		LD_ADDR(c, fl_addr, fl_next);
		LD_SIZE(c, fl_addr, fl_size);
		if (fl_size >= size)
			break;

		prev_fl = fl_addr;
		fl_addr = fl_next;
	}
	if (prev_fl)
		ST_ADDR(c, prev_fl, addr);
	else
		ST_WORD(c, OFFSET_FREE_LIST, addr);

	ST_ADDR(c, addr, fl_addr);
	ST_SIZE(c, addr, size);
}

/*
	remove a piece from the freelist
	this routine is inefficient because there is no prev pointer in a marker,
	so it walks the freelist from the start
*/
static void remove_freelist(BinChunk *c, int addr) {
int prev_fl, fl_addr, fl_next, addr_next;

	if (c == NULL)
		return;

	LD_ADDR(c, addr, addr_next);
	ST_ADDR(c, addr, 0);

	prev_fl = 0;
	LD_WORD(c, OFFSET_FREE_LIST, fl_addr);
	while(fl_addr) {
		if (fl_addr == addr) {
			if (prev_fl)
				ST_ADDR(c, prev_fl, addr_next);
			else
				ST_WORD(c, OFFSET_FREE_LIST, addr_next);
			return;
		}
		prev_fl = fl_addr;
		LD_ADDR(c, fl_addr, fl_next);
		fl_addr = fl_next;
	}
	abort();		/* shouldn't get here */
}

/* EOB */
