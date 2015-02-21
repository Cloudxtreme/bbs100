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
	StringList.c	WJ98
*/

#include "config.h"
#include "StringList.h"
#include "cstring.h"
#include "Memory.h"
#include "AtomicFile.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

StringList *new_StringList(char *s) {
StringList *sl;

	if (s == NULL || (sl = (StringList *)Malloc(sizeof(StringList), TYPE_STRINGLIST)) == NULL)
		return NULL;

	if ((sl->str = cstrdup(s)) == NULL) {
		Free(sl);
		return NULL;
	}
	return sl;
}

void destroy_StringList(StringList *sl) {
	if (sl == NULL)
		return;

	Free(sl->str);
	Free(sl);
}


StringList *in_StringList(StringList *sl, char *p) {
	if (p == NULL || !*p)
		return NULL;

	while(sl != NULL) {
		if (sl->str != NULL && !strcmp(sl->str, p))
			return sl;
		sl = sl->next;
	}
	return sl;
}

StringList *in_StringQueue(QueueType *q, char *s) {
	if (q == NULL || s == NULL)
		return NULL;

	return in_StringList((StringList *)q->tail, s);
}

/*
	converts stringlist to string; returned string must be Free()d
*/
char *str_StringList(StringList *root) {
char *str;
int len = 1;
StringList *sl;

	for(sl = root; sl != NULL; sl = sl->next)
		if (sl->str != NULL)
			len += strlen(sl->str) + 1;

	if ((str = (char *)Malloc(len, TYPE_CHAR)) == NULL)
		return NULL;

	*str = 0;
	for(sl = root; sl != NULL; sl = sl->next) {
		if (sl->str != NULL) {
			cstrcat(str, sl->str, len);
			cstrcat(str, "\n", len);
		}
	}
	return str;
}

StringList *load_StringList(char *file) {
StringList *sl = NULL;
AtomicFile *f;
char buf[1024];

	if ((f = openfile(file, "r")) == NULL)
		return NULL;

	while(fgets(buf, 1024, f->f) != NULL) {
		chop(buf);
		(void)add_StringList(&sl, new_StringList(buf));
	}
	closefile(f);
	return sl;
}

int save_StringList(StringList *sl, char *file) {
AtomicFile *f;

	if ((f = openfile(file, "w")) == NULL)
		return -1;

	while(sl != NULL) {
		fprintf(f->f, "%s\n", sl->str);
		sl = sl->next;
	}
	return closefile(f);
}

StringList *copy_StringList(StringList *sl) {
StringList *root = NULL, *cp = NULL;

	if (sl == NULL)
		return NULL;

	root = cp = new_StringList(sl->str);
	sl = sl->next;

	while(sl != NULL) {
		cp = add_StringList(&cp, new_StringList(sl->str));
		sl = sl->next;
	}
	return root;
}


StringList *vadd_String(StringList **slp, char *fmt, va_list ap) {
StringList *sl;
char buf[PRINT_BUF];

	if (slp == NULL)
		return NULL;

	bufvprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if ((sl = new_StringList(buf)) == NULL)
		return *slp;

	return add_StringList(slp, sl);
}

StringList *add_String(StringList **slp, char *fmt, ...) {
va_list ap;

	va_start(ap, fmt);
	return vadd_String(slp, fmt, ap);
}

/*
	sortfunc for sort_StringList()
	also handles strings that end with a numeric value
*/
int alphasort_StringList(void *v1, void *v2) {
StringList *s1, *s2;
char *p, *q;
int i, j;

	if (v1 == NULL || v2 == NULL)
		return 0;

	s1 = *(StringList **)v1;
	s2 = *(StringList **)v2;

	if (s1 == NULL || s2 == NULL)
		return 0;

	p = s1->str;
	if (p == NULL)
		return 0;

	q = s2->str;
	if (q == NULL)
		return 0;

	i = strlen(p) - 1;
	j = strlen(q) - 1;
	if (i >= 0 && p[i] >= '0' && p[i] <= '9'
		&& j >= 0 && q[j] >= '0' && q[j] <= '9') {
		while(i >= 0 && p[i] >= '0' && p[i] <= '9')
			i--;
		i++;

		while(j >= 0 && q[j] >= '0' && q[j] <= '9')
			j--;
		j++;

		if (i == j && (!i || !strncmp(p, q, i))) {
			i = atoi(p+i);
			j = atoi(q+j);

			if (i < j)
				return -1;

			if (i > j)
				return 1;

			return 0;
		}
	}
	return strcmp(s1->str, s2->str);
}

/* EOB */
