/*
    bbs100 3.1 WJ107
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
	BinAlloc.h WJ105
*/

#ifndef BINALLOC_H_WJ105
#define BINALLOC_H_WJ105	1

#include "List.h"

#define add_MemBin(x,y)			(MemBin *)add_List((x), (y))
#define concat_MemBin(x,y)		(MemBin *)concat_List((x), (y))
#define remove_MemBin(x,y)		(MemBin *)remove_List((x), (y))
#define pop_MemBin(x)			(MemBin *)pop_List(x)
#define listdestroy_MemBin(x)	listdestroy_List((x), destroy_MemBin)
#define rewind_MemBin(x)		(MemBin *)rewind_List(x)
#define unwind_MemBin(x)		(MemBin *)unwind_List(x)
#define sort_MemBin(x, y)		(MemBin *)sort_List((x), (y))

/*
	due to memory alignment on some architectures, the marker _must_ be sizeof(void *)
*/
#define MARKER_SIZE		sizeof(void *)
#define BIN_SIZE		256
#define MAX_BIN_FREE	(BIN_SIZE - sizeof(MemBin) - 2*MARKER_SIZE)
#define BIN_MEM_START	sizeof(MemBin)
#define BIN_MEM_END		(BIN_SIZE - MARKER_SIZE)

#define LD_MARK(x,y)	(y) = (((char *)(x))[0] & 0xff)
#define LD_TYPE(x,y)	(y) = (((char *)(x))[1] & 0xff)

#define ST_MARK(x,y)	((char *)(x))[0] = (char)((y) & 0xff)
#define ST_TYPE(x,y)	((char *)(x))[1] = (char)((y) & 0xff)

/*
	markers
*/
#define MARK_FREE		0
#define MARK_MALLOC		0xff


typedef struct MemBin_tag MemBin;

struct MemBin_tag {
	List(MemBin);
	int free;				/* bytes free in this bin */
};

typedef struct {
	int bins;				/* # of bins in use for this type */
	unsigned long free;		/* total free bytes */
} MemBinInfo;

typedef struct {
	unsigned long total, in_use, malloc;
	int balance;
} MemInfo;

typedef struct {
	int num, balance;
} MemStats;

int init_BinAlloc(void);
void deinit_BinAlloc(void);
void enable_BinAlloc(void);
void disable_BinAlloc(void);

void *BinMalloc(unsigned long, int);
void BinFree(void *);

int get_MemBinInfo(MemBinInfo *, int);
int get_MemStats(MemStats *, int);
int get_MemInfo(MemInfo *);

#endif	/* BINALLOC_H_WJ105 */

/* EOB */
