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
	Category.c	WJ105

	Categories are like floors
*/


#include "config.h"
#include "Category.h"
#include "Param.h"
#include "AtomicFile.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>


StringList *category = NULL;


int init_Category(void) {
	printf("loading room categories %s ...", PARAM_CATEGORIES_FILE);
	fflush(stdout);
	printf(" %s\n", load_Category() == 0 ? "ok" : "failed");
	return (category == NULL) ? -1 : 0;
}

int load_Category(void) {
AtomicFile *f;
char buf[1024];

	if ((f = openfile(PARAM_CATEGORIES_FILE, "r")) == NULL)
		return -1;

	while(fgets(buf, 1024, f->f) != NULL) {
		chop(buf);
		if (!*buf)
			continue;

		add_Category(buf);
	}
	closefile(f);
	return 0;
}

int save_Category(void) {
	return save_StringList(category, PARAM_CATEGORIES_FILE);
}

void add_Category(char *c) {
	if (!in_Category(c)) {
		(void)add_StringList(&category, new_StringList(c));
		(void)sort_StringList(&category, alphasort_StringList);
	}
}

void remove_Category(char *c) {
StringList *sl;

	if ((sl = in_StringList(category, c)) != NULL) {
		(void)remove_StringList(&category, sl);
		destroy_StringList(sl);
	}
}

int in_Category(char *c) {
	return (in_StringList(category, c) != NULL);
}

/* EOB */
