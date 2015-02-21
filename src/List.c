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
	List.c	WJ98
*/

#include "config.h"
#include "debug.h"
#include "List.h"
#include "Memory.h"

#include <stdlib.h>

#ifndef NULL
#define NULL	0
#endif

ListType *add_List(void *v1, void *v2) {
ListType **root, *l;

	if (v1 == NULL)
		return NULL;

	root = (ListType **)v1;
	l = (ListType *)v2;

	if (l == NULL)
		return (void *)*root;

	l->prev = l->next = NULL;
	if (*root == NULL)
		*root = l;
	else {
		ListType *lp;

/* Link in at the end of the list */

		for(lp = *root; lp->next != NULL; lp = lp->next);
		lp->next = l;
		l->prev = lp;
	}
	return l;
}

ListType *prepend_List(void *v1, void *v2) {
ListType **root, *l;

	if (v1 == NULL)
		return NULL;

	root = (ListType **)v1;
	l = (ListType *)v2;

	if (l == NULL)
		return (void *)*root;

	l->prev = l->next = NULL;
	if (*root != NULL) {
		ListType *lp;

/* Link in at the beginning of the list */

		for(lp = *root; lp->prev != NULL; lp = lp->prev);
		lp->prev = l;
		l->next = lp;
	}
	*root = l;
	return l;
}

/*
	Note: listdestroy_List() now auto-rewinds the list (!)
*/
void listdestroy_List(void *v1, void *v2) {
ListType *l, *l2;
void (*destroy_func)(ListType *);

	if (v1 == NULL || v2 == NULL)
		return;

	l = (ListType *)v1;
	destroy_func = (void (*)(ListType *))v2;

	while(l->prev != NULL)			/* auto-rewind */
		l = l->prev;

	while(l != NULL) {
		l2 = l->next;
		destroy_func(l);
		l = l2;
	}
}

ListType *concat_List(void *v1, void *v2) {
ListType **root, *l;

	if (v1 == NULL)
		return NULL;

	root = (ListType **)v1;
	l = (ListType *)v2;

	if (l == NULL)
		return (void *)*root;

	if (*root == NULL)
		*root = l;
	else {
		ListType *p;

		for(p = *root; p->next != NULL; p = p->next);
		p->next = l;
		l->prev = p;
	}
	return (void *)l;
}

ListType *remove_List(void *v1, void *v2) {
ListType **root, *l;

	if (v1 == NULL || v2 == NULL)
		return NULL;

	root = (ListType **)v1;
	l = (ListType *)v2;

	if (*root == NULL)
		return NULL;

	if (l->prev == NULL)				/* it is the root node */
		*root = l->next;
	else
		l->prev->next = l->next;

	if (l->next != NULL)
		l->next->prev = l->prev;
	l->next = l->prev = NULL;
	return l;
}

/*
	pops off the beginning of the list
*/
ListType *pop_List(void *v) {
ListType **root, *l;

	if (v == NULL)
		return NULL;

	root = (ListType **)v;
	if (*root == NULL)
		return NULL;

	l = *root;
	if (l != NULL)
		while(l->prev != NULL)
			l = l->prev;

	*root = l->next;
	if (*root != NULL)
		(*root)->prev = NULL;
	l->prev = l->next = NULL;
	return l;
}

int count_List(void *v) {
ListType *l;
int c = 0;

	for(l = (ListType *)v; l != NULL; l = l->next)
		c++;
	return c;
}

ListType *rewind_List(void *v) {
ListType *root;

	if (v == NULL)
		return NULL;

	root = (ListType *)v;

	while(root->prev != NULL)
		root = root->prev;
	return root;		
}

ListType *unwind_List(void *v) {
ListType *root;

	if (v == NULL)
		return NULL;

	root = (ListType *)v;

	while(root->next != NULL)
		root = root->next;
	return root;		
}

/*
	sort a list using merge sort

	implemented as described by Simon Tatham on the web, and with help
	of his free open source example
*/
ListType *sort_List(void *v, int (*sort_func)(void *, void *)) {
ListType **root, *p, *q, *l, *e;
int k = 1, n = 0, i, psize, qsize;

	if (v == NULL)
		return NULL;

	root = (ListType **)v;
	if (*root == NULL || sort_func == NULL)
		return *root;

	for(;;) {
		p = *root;
		*root = NULL;
		l = NULL;
		n = 0;

		while(p != NULL) {
			n++;
			q = p;
/* step along q for k places */
			for(i = 0; i < k && q != NULL; i++)
				q = q->next;

			psize = i;
			qsize = k;

/* sort elements until end of this sublist is reached */
			while(psize > 0 || (qsize > 0 && q != NULL)) {
				if (psize <= 0) {
					e = q;
					q = q->next;
					qsize--;
				} else {
					if (qsize <= 0 || q == NULL) {
						e = p;
						p = p->next;
						psize--;
					} else {
						if (sort_func(&p, &q) <= 0) {
							e = p;
							p = p->next;
							psize--;
						} else {
							e = q;
							q = q->next;
							qsize--;
						}
					}
				}
/* put sorted element e onto results list l (tail pointer to *root) */
				if (l == NULL) {
					*root = l = e;
					e->prev = e->next = NULL;
				} else {
					l->next = e;
					e->prev = l;
					e->next = NULL;
					l = e;
				}
			}
/* do it for the next part of the list */
			p = q;
		}
/* do until the there's nothing to be sorted any more */
		if (n <= 1)
			return *root;

		k *= 2;
	}
	return *root;
}

/* EOB */
