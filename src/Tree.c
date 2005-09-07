/*
    bbs100 3.0 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

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
	Tree.c	WJ105
*/

#include "config.h"
#include "Tree.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

void destroy_Tree(void *v, void (*destroy_func)(void *)) {
TreeType *t;

	t = (TreeType *)v;
	if (t == NULL)
		return;

	if (t->left != NULL) {
		destroy_Tree(t->left, destroy_func);
		t->left = NULL;
	}
	if (t->right != NULL) {
		destroy_Tree(t->right, destroy_func);
		t->right = NULL;
	}
	destroy_func(t);
}

TreeType *add_Tree(void *v1, void *v2, int (*compar_func)(void *, void *)) {
TreeType **root, *t, *tp;
int r;

	if (v1 == NULL || v2 == NULL || compar_func == NULL)
		return NULL;

	root = (TreeType **)v1;
	t = (TreeType *)v2;
	t->left = t->right = NULL;

	if (*root == NULL) {
		*root = t;
		return t;
	}
	for(tp = *root;;) {
		r = compar_func(tp, t);
		if (r < 0) {
			if (tp->left == NULL) {
				tp->left = t;
				return t;
			}
			tp = tp->left;
			continue;
		}
		if (r < 0) {
			if (tp->right == NULL) {
				tp->right = t;
				return t;
			}
			tp = tp->right;
			continue;
		}
		break;			/* collision! element already is in the tree */
	}
	return NULL;
}

TreeType *remove_Tree(void *v1, void *v2, int (*compar_func)(void *, void *)) {
TreeType **root, *tp, **parent_addr, *successor, **successor_parent;
int r;

	if (v1 == NULL || v2 == NULL || compar_func == NULL)
		return NULL;

	root = (TreeType **)v1;

	parent_addr = root;
	for(tp = *root; tp != NULL;) {
		r = compar_func(tp, v2);
		if (r < 0) {
			parent_addr = &tp->left;
			tp = tp->left;
			continue;
		}
		if (r > 0) {
			parent_addr = &tp->right;
			tp = tp->right;
			continue;
		}
/*
	no right child (easy case)
	move the left child into the place of the removed node

	also works if tp is a leaf or if *root is the only node in the tree
*/
		if (tp->right == NULL) {
			*parent_addr = tp->left;
			tp->left = NULL;
			return tp;
		}
/*
	right child has no left child
	attach the left subtree of tp as the left subtree of its right child
	then move the right child into the removed node's place
*/
		if (tp->right->left == NULL) {
			tp->right->left = tp->left;
			tp->left = NULL;
			*parent_addr = tp->right;
			tp->right = NULL;
			return tp;
		}
/*
	right child has a left child
	remove the in-order successor and move it into place of the removed node

	the in-order successor is the next closest value; this always is the smallest (most-left)
	value on the right (higher values) subtree
*/
		successor_parent = &tp->right;
		successor = tp->right->left;
		while(successor->left != NULL) {
			successor_parent = &successor->left;
			successor = successor->left;
		}
		*successor_parent = successor->right;		/* unlink successor */

		successor->left = tp->left;					/* put successor in place */
		successor->right = tp->right;
		*parent_addr = successor;

		tp->left = tp->right = NULL;
		return tp;
	}
	return NULL;		/* not found */
}

TreeType *in_Tree(void *v1, void *v2, int (*compar_func)(void *, void *)) {
TreeType *t;
int r;

	if (v1 == NULL || v2 == NULL || compar_func == NULL)
		return NULL;

	for(t = (TreeType *)v1; t != NULL;) {
		r = compar_func(t, v2);
		if (r < 0) {
			t = t->left;
			continue;
		}
		if (r > 0) {
			t = t->right;
			continue;
		}
		return t;
	}
	return NULL;
}

TreeType *min_Tree(void *v) {
TreeType *t;

	t = (TreeType *)v;
	if (t == NULL)
		return NULL;

	while(t->left != NULL)
		t = t->left;

	return t;
}

TreeType *max_Tree(void *v) {
TreeType *t;

	t = (TreeType *)v;
	if (t == NULL)
		return NULL;

	while(t->right != NULL)
		t = t->right;

	return t;
}

void dump_Tree(void *v, char *(*print_func)(void *)) {
TreeType *t;

	if (print_func == NULL)
		return;

	t = (TreeType *)v;
	if (t == NULL)
		return;

	if (t->left != NULL)
		dump_Tree(t->left, print_func);

	log_debug("tree: %s", print_func(t));

	if (t->right != NULL)
		dump_Tree(t->right, print_func);
}

/* EOB */
