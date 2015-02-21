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
	SymbolTable.h	WJ99
*/

#ifndef SYMBOLTABLE_H_WJ99
#define SYMBOLTABLE_H_WJ99 1

#include "List.h"

#define add_SymbolTable(x,y)		(SymbolTable *)add_List((x), (y))
#define concat_SymbolTable(x,y)		(SymbolTable *)concat_List((x), (y))
#define remove_SymbolTable(x,y)		(SymbolTable *)remove_List((x), (y))
#define listdestroy_SymbolTable(x)	listdestroy_List((x), destroy_SymbolTable)
#define rewind_SymbolTable(x)		(SymbolTable *)rewind_List(x)
#define unwind_SymbolTable(x)		(SymbolTable *)unwind_List(x)

typedef struct SymbolTable_tag SymbolTable;

struct SymbolTable_tag {
	List(SymbolTable);

	char *name;
	char type;				/* 'T' for TEXT, 'D' for DATA, etc */
	unsigned long addr;
};

#ifdef DEBUG

extern SymbolTable *symbol_table;

SymbolTable *new_SymbolTable(void);
void destroy_SymbolTable(SymbolTable *);

int load_SymbolTable(char *);
SymbolTable *in_SymbolTable(unsigned long, char);
SymbolTable *in_SymbolTable_name(char *);

#endif	/* DEBUG */

#endif	/* SYMBOLTABLE_H_WJ99 */

/* EOB */
