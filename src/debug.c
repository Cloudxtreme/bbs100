/*
    bbs100 1.2.1 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	debug.c	WJ99

	The debug stack used to be a list of strings (function names),
	but with the coming of the SymbolTable, I have turned it into
	a (much faster) array of function addresses.
*/

#include "config.h"
#include "debug.h"
#include "SymbolTable.h"

#include <stdio.h>

#ifdef DEBUG

unsigned long debug_stack[DEBUG_STACK_SIZE];
int debug_stackp = 0;

void dump_debug_stack(void) {
int i;

	fprintf(stderr, "TD \n"
		"TD *** debug stack dump ***\n");

	if (symbol_table != NULL) {
		SymbolTable *st;

		for(i = 0; i < debug_stackp; i++) {
			if ((st = in_SymbolTable(debug_stack[i], 'T')) == NULL)
				fprintf(stderr, "TD   %08lx (unknown symbol) ??\n", debug_stack[i]);
			else
				fprintf(stderr, "TD   %08lx %s()\n", debug_stack[i], st->name);
		}
	} else {
		fprintf(stderr, "TD (missing symbol table)\n");

		for(i = 0; i < debug_stackp; i++)
			fprintf(stderr, "TD   %08lx\n", debug_stack[i]);
	}
	fprintf(stderr, "TD *** dump completed ***\n"
		"TD \n");
}

#endif	/* DEBUG */

/* EOB */
