/*
    bbs100 3.3 WJ107
    Copyright (C) 1997-2015  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/
/*
	debug.h	WJ99
*/

#ifndef DEBUG_H_WJ99
#define DEBUG_H_WJ99 1

#ifdef EBUG
#define DEBUG 1
#endif

#ifdef DEBUG

#include "log.h"

#include <stdio.h>

#ifdef __GNUC__
  #define TD		log_debug("%d %s %s()", __LINE__, __FILE__, __PRETTY_FUNCTION__);
#else
  #define TD		log_debug("%d %s", __LINE__, __FILE__);
#endif

#define TDC			log_debug("%d", __LINE__);

#define Enter(x)	do {										\
		if (debug_stackp < DEBUG_STACK_SIZE)					\
			debug_stack[debug_stackp++] = (unsigned long)(x);	\
	} while(0)

#define Leave		do {						\
		if (debug_stackp > 0)					\
			debug_stack[--debug_stackp] = 0UL;	\
	} while(0)

/*
	blast, no do { } while construct possible here, making this
	a 'dangerous' macro
*/
#define Return		Leave; return

#define DEBUG_STACK_SIZE	64

extern unsigned long debug_stack[DEBUG_STACK_SIZE];
extern int debug_stackp;

void dump_debug_stack(void);
void debug_breakpoint(void);
void debug_breakpoint2(void);

#else	/* NO DEBUG */

#define TD
#define TDC
#define debug_breakpoint
#define Enter(x)
#define Return	return

#endif	/* DEBUG */

#endif	/* DEBUG_H_WJ99 */

/* EOB */
