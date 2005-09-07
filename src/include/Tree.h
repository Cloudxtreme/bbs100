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
	Tree.h	WJ105
*/

#ifndef TREE_H_WJ105
#define TREE_H_WJ105	1

#define Tree(x)			x *left, *right

typedef struct Tree_tag TreeType;

struct Tree_tag {
	Tree(TreeType);
};

void destroy_Tree(void *, void (*)(void *));
TreeType *add_Tree(void *, void *, int (*)(void *, void *));
TreeType *remove_Tree(void *, void *, int (*)(void *, void *));
TreeType *in_Tree(void *, void *, int (*)(void *, void *));
TreeType *min_Tree(void *);
TreeType *max_Tree(void *);

#endif	/* TREE_H_WJ105 */

/* EOB */
