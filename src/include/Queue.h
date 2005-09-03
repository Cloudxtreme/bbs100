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
	Queue.h	WJ105
*/

#ifndef QUEUE_H_WJ105
#define QUEUE_H_WJ105	1

#include "List.h"

typedef struct {
	ListType *head, *tail;
	int count;
} Queue;

Queue *new_Queue(void);
void destroy_Queue(Queue *, void *);

ListType *add_Queue(Queue *, void *);
ListType *prepend_Queue(Queue *, void *);
ListType *remove_Queue(Queue *, void *);
ListType *pop_Queue(Queue *);
void concat_Queue(Queue *, void *);
void sort_Queue(Queue *, int (*)(void *, void *));
int Queue_count(Queue *);

#endif	/* QUEUE_H_WJ105 */

/* EOB */
