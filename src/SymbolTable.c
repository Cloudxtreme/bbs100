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
	SymbolTable.c	WJ99
*/

#include "config.h"
#include "debug.h"
#include "SymbolTable.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG

SymbolTable *symbol_table = NULL;

SymbolTable *new_SymbolTable(void) {
SymbolTable *st;

	if ((st = (SymbolTable *)Malloc(sizeof(SymbolTable), TYPE_SYMBOLTABLE)) == NULL)
		return NULL;

	return st;
}

void destroy_SymbolTable(SymbolTable *st) {
	if (st == NULL)
		return;

	Free(st->name);
	Free(st);
}

/*
	the symbol table is loaded from 'nm' output

	All platforms have their own 'nm' output format, but luckily some
	support POSIX format (although often poorly supported).

	POSIX.2 format is <symbol> <type> <address> [<size>]

	The symbol apperently may be empty on some systems
	The address may be empty, when type is 'U'
	The size is optional
*/
int load_SymbolTable(char *filename) {
AtomicFile *f;
char buf[MAX_LONGLINE], namebuf[MAX_LONGLINE], type;
unsigned long addr;
SymbolTable *st;

	listdestroy_SymbolTable(symbol_table);
	symbol_table = NULL;

	if ((f = openfile(filename, "r")) == NULL)
		return 1;

	while(fgets(buf, MAX_LONGLINE, f->f) != NULL) {
		cstrip_line(buf);
		if (!*buf)
			continue;

		addr = 0UL;
		type = '#';
		cstrcpy(namebuf, "<unknown>", MAX_LONGLINE);

		if (*buf == ' ')
			sscanf(buf, " %c %lx", &type, &addr);
		else
			sscanf(buf, "%s %c %lx", namebuf, &type, &addr);

		if ((st = new_SymbolTable()) == NULL) {
			closefile(f);
			listdestroy_SymbolTable(symbol_table);
			symbol_table = NULL;
			return -1;
		}
		st->name = cstrdup(namebuf);
		st->type = type;
		st->addr = addr;

		symbol_table = add_SymbolTable(&symbol_table, st);
	}
	closefile(f);
	symbol_table = rewind_SymbolTable(symbol_table);
	return 0;
}

SymbolTable *in_SymbolTable(unsigned long addr, char t) {
SymbolTable *st;

	for(st = symbol_table; st != NULL; st = st->next)
		if (st->addr == addr && st->type == t)
			return st;
	return NULL;
}

SymbolTable *in_SymbolTable_name(char *name) {
SymbolTable *st;

	if (name == NULL)
		return NULL;

	for(st = symbol_table; st != NULL; st = st->next)
		if (!strcmp(st->name, name))
			return st;
	return NULL;
}

#endif	/* DEBUG */

/* EOB */
