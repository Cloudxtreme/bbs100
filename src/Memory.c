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
	Memory.c	WJ100

	This file used to contain useful code, but currently it's just
	a placeholder for BinAlloc
*/

#include "config.h"
#include "Memory.h"
#include "BinAlloc.h"

#ifndef USE_BINALLOC
#include "calloc.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int init_Memory(void) {
#ifdef USE_BINALLOC
	return init_BinAlloc();
#else
	return 0;
#endif
}

void deinit_Memory(void) {
#ifdef USE_BINALLOC
	deinit_BinAlloc();
#endif
}

#ifndef USE_BINALLOC
void *memalloc(size_t size) {
	return calloc(size, 1);
}
#endif

/* EOB */
