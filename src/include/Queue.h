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
	Queue.h	WJ105
*/

#ifndef QUEUE_H_WJ105
#define QUEUE_H_WJ105	1

#include "List.h"

typedef struct {
	ListType *head, *tail;
	int count;
} QueueType;

QueueType *new_Queue(void);
void destroy_Queue(QueueType *, void *);
void deinit_Queue(QueueType *, void *);

ListType *add_Queue(QueueType *, void *);
ListType *prepend_Queue(QueueType *, void *);
ListType *remove_Queue(QueueType *, void *);
ListType *pop_Queue(QueueType *);
ListType *dequeue_Queue(QueueType *);
ListType *concat_Queue(QueueType *, void *);
void sort_Queue(QueueType *, int (*)(void *, void *));
int count_Queue(QueueType *);

#endif	/* QUEUE_H_WJ105 */

/* EOB */
