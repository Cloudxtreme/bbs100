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
	Memory.h	WJ100
*/

#ifndef MEMORY_H_WJ100
#define MEMORY_H_WJ100

#include "Types.h"

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

#endif	/* MEMORY_H_WJ100 */

/* EOB */
