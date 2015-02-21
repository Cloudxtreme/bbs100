/*
    bbs100 3.2 WJ115
    Copyright (C) 2015  Walter de Jong <walter@heiho.net>

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
	Slub.c	WJ115

	Slub allocator for bbs100

	FIXME the bbs should use logging rather than printf()
*/

#include "Slub.h"
#include "calloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static MemCache memcaches[NUM_MEMCACHES];


void init_MemCache(void) {
int i;

	memset(memcaches, 0, NUM_MEMCACHES * sizeof(MemCache));

	/* init sizes in steps of 16 bytes */
	for(i = 0; i < NUM_MEMCACHES; i++) {
		memcaches[i].size = (i + 1) * SLUB_SIZESTEP;
	}
}

Slub *new_Slub(unsigned int objsize) {
Slub *s;
char *page;
unsigned int i;

	if (objsize > SLUB_PAGESIZE) {
		fprintf(stderr, "fatal error: new_Slub(): objsize too large\n");
		abort();
	}

	if ((page = (char *)malloc(SLUB_PAGESIZE)) == NULL) {
		fprintf(stderr, "new_Slub(): out of memory (?)\n");
		return NULL;
	}

	if ((s = (Slub *)malloc(sizeof(Slub))) == NULL) {
		fprintf(stderr, "new_Slub(): out of memory (?)\n");
		free(page);
		return NULL;
	}
	
	memset(page, 0, SLUB_PAGESIZE);
	memset(s, 0, sizeof(Slub));

	s->numfree = SLUB_PAGESIZE / objsize;
	/* init free list */
	s->first = 0;
	char *p = page;
	for(i = 1; i < s->numfree; i++) {
		*(unsigned int *)p = i;
		p += objsize;
	}
	*(unsigned int *)p = 0xffff;	/* end marker */

	s->page = page;
	return s;
}

void destroy_Slub(Slub *s) {
	if (s != NULL) {
		s->prev = s->next = NULL;
		if (s->page != NULL) {
			free(s->page);
			s->page = NULL;
		}
		s->first = s->numfree = 0;
		free(s);
	}
}

void *Slub_alloc(Slub *s, unsigned int objsize) {
char *obj;

#ifdef DEBUG
	if (objsize > SLUB_PAGESIZE) {
		fprintf(stderr, "Slub_alloc(): invalid object size requested\n");
		abort();
	}
	if (s == NULL) {
		fprintf(stderr, "Slub_alloc(): s == NULL\n");
		abort();
	}
	if (s->page == NULL) {
		fprintf(stderr, "Slub_alloc(): s->page == NULL\n");
		abort();
	}
	if (!s->numfree) {
		fprintf(stderr, "Slub_alloc(): s->numfree == 0\n");
		abort();
	}
	if (s->numfree > SLUB_PAGESIZE / SLUB_SIZESTEP) {
		fprintf(stderr, "Slub_alloc(): s->numfree has invalid value\n");
		abort();
	}
	if (s->first > SLUB_PAGESIZE / MAX_SLUB_OBJSIZE) {
		fprintf(stderr, "Slub_alloc(): s->first has invalid value");
		abort();
	}
#endif	/* DEBUG */
	obj = (char *)s->page + s->first * objsize;
	s->first = *(unsigned int *)obj;
	s->numfree--;
#ifdef DEBUG
	if (!s->numfree) {
		if (s->first != 0xffff) {
			fprintf(stderr, "Slub_alloc(): invalid end marker\n");
			abort();
		}
	}
#endif	/* DEBUG */
	memset(obj, 0, objsize);
	return obj;
}

void Slub_free(Slub *s, void *addr, unsigned int objsize) {
unsigned int nextfree;
char *p;

	if (addr == NULL) {
		return;
	}

#ifdef DEBUG
	if (s == NULL) {
		fprintf(stderr, "Slub_free(): s == NULL\n");
		abort();
	}
	if (s->page == NULL) {
		fprintf(stderr, "Slub_free(): s->page == NULL\n");
		abort();
	}
	if (s->numfree > SLUB_PAGESIZE / SLUB_SIZESTEP) {
		fprintf(stderr, "Slub_free(): s->numfree has invalid value\n");
		abort();
	}
	if (objsize > SLUB_PAGESIZE) {
		fprintf(stderr, "Slub_free(): invalid object size requested\n");
		abort();
	}
	if (addr < s->page || addr > s->page + SLUB_PAGESIZE - objsize) {
		fprintf(stderr, "Slub_free(): invalid address\n");
		abort();
	}
#endif	/* DEBUG */

	memset(addr, 0, objsize);
	nextfree = s->first;
	s->first = ((char *)addr - (char *)s->page) / objsize;
	p = (char *)s->page + s->first * objsize;
	*(unsigned int *)p = nextfree;
	s->numfree++;

#ifdef DEBUG
	if (s->numfree > SLUB_PAGESIZE / SLUB_SIZESTEP) {
		fprintf(stderr, "Slub_free(): s->numfree has invalid value\n");
		abort();
	}
	if (s->first > SLUB_PAGESIZE / MAX_SLUB_OBJSIZE) {
		fprintf(stderr, "Slub_free(): s->first has invalid value");
		abort();
	}
#endif	/* DEBUG */
}

void *MemCache_alloc(MemCache *m) {
void *addr;
Slub *s;

	if (m->partial == NULL) {
		/*
			no empty slots available
			get a new slab
		*/
		if ((m->partial = new_Slub(m->size)) == NULL) {
			return NULL;
		}
	}

	addr = Slub_alloc(m->partial, m->size);
	if (addr == NULL) {
		fprintf(stderr, "MemCache_alloc(): failed to allocate %u bytes", m->size);
		abort();
	}

	if (!m->partial->numfree) {
		/*
			slab is now full
			so move it to the full list
		*/
		s = remove_Slub(&m->partial, m->partial);
		s = prepend_Slub(&m->full, s);
	}
	return addr;
}

/*
	try free object from this MemCache
	Returns 0 if object was not found, not freed
	Returns 1 if indeed object was in this MemCache, now freed
*/
int MemCache_free(MemCache *m, void *addr) {
Slub *s;

	for(s = m->full; s != NULL; s = s->next) {
		if (addr >= s->page && addr < s->page + SLUB_PAGESIZE) {
			Slub_free(s, addr, m->size);
			if (s->numfree >= SLUB_PAGESIZE / m->size) {
				/*
					slab is now empty
					let's free it
				*/
				s = remove_Slub(&m->full, s);
				destroy_Slub(s);
			}
			return 1;
		}
	}

	/* it was not on the full list, look in partial */
	for(s = m->partial; s != NULL; s = s->next) {
		if (addr >= s->page && addr < s->page + SLUB_PAGESIZE) {
			Slub_free(s, addr, m->size);
			if (s->numfree >= SLUB_PAGESIZE / m->size) {
				/*
					slab is now empty
					let's free it
				*/
				s = remove_Slub(&m->partial, s);
				destroy_Slub(s);
			}
			return 1;
		}
	}
	return 0;
}

void *memcache_alloc(unsigned int size) {
int idx;

	if (!size) {
		fprintf(stderr, "memcache_alloc(): invalid size requested\n");
		abort();
	}

	if (size >= MAX_SLUB_OBJSIZE) {
		return calloc(1, size);
	}

	idx = size / SLUB_SIZESTEP;
	if (idx >= NUM_MEMCACHES) {
		fprintf(stderr, "memcache_alloc(): how did this happen? size == %u\n", size);
		abort();
	}
	return MemCache_alloc(&memcaches[idx]);
}

void memcache_free(void *addr) {
int i;

	if (addr == NULL) {
		return;
	}

	/*
		find the page where addr lives
		This is not fast code ...
		sadly we can not calculate the page number from the address
		because we do not have a page table, as we do not reserve
		a static block of memory at startup

		Could be sped up iff we passed the type/objsize as argument

		FIXME what if we made a sorted list of addrs that acts like a page table?
	*/
	for(i = 0; i < NUM_MEMCACHES; i++) {
		/* try free the object */
		if (MemCache_free(&memcaches[i], addr)) {
			return;
		}
	}

	/* not found; must have been malloc()ed or calloc()ed */
	free(addr);
}

/* EOB */
