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
	List.h	WJ99
*/

#ifndef LIST_H_WJ99
#define LIST_H_WJ99	1

#define List(x)		x *prev, *next

typedef struct List_tag ListType;

struct List_tag {
	List(ListType);
};

ListType *add_List(void *, void *);
ListType *prepend_List(void *, void *);
void listdestroy_List(void *, void *);
ListType *concat_List(void *, void *);
ListType *remove_List(void *, void *);
ListType *pop_List(void *);
int count_List(void *);
ListType *rewind_List(void *);
ListType *unwind_List(void *);
ListType *sort_List(void *, int (*)(void *, void *));

#endif	/* LIST_H_WJ99 */

/* EOB */
