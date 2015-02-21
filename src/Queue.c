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
	Queue.c	WJ105

	Queues are convenient when having to count_List() the list often
	Adding and prepending to a Queue is efficient
*/

#include "config.h"
#include "Queue.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

QueueType *new_Queue(void) {
	return (QueueType *)Malloc(sizeof(QueueType), TYPE_QUEUE);
}

void destroy_Queue(QueueType *q, void *destroy_func) {
	if (q == NULL || destroy_func == NULL)
		return;

	deinit_Queue(q, destroy_func);
	Free(q);
}

void deinit_Queue(QueueType *q, void *destroy_func) {
	if (q == NULL || destroy_func == NULL)
		return;

	listdestroy_List(q->tail, (void (*)(ListType *))destroy_func);
	q->head = q->tail = NULL;
	q->count = 0;
}

ListType *add_Queue(QueueType *q, void *l) {
ListType *ret;

	if (q == NULL || l == NULL)
		return NULL;

	if ((ret = add_List(&q->head, (ListType *)l)) == NULL)
		return NULL;

	q->head = ret;
	if (q->tail == NULL)
		q->tail = q->head;

	if (ret != NULL)
		q->count++;
	return q->head;
}

ListType *prepend_Queue(QueueType *q, void *l) {
ListType *ret;

	if (q == NULL || l == NULL)
		return NULL;

	if ((ret = prepend_List(&q->tail, (ListType *)l)) == NULL)
		return NULL;

	q->tail = ret;
	if (q->head == NULL)
		q->head = q->tail;

	if (ret != NULL)
		q->count++;
	return q->tail;
}

ListType *remove_Queue(QueueType *q, void *l) {
ListType *ret;

	if (q == NULL || l == NULL)
		return NULL;

	if (q->head == (ListType *)l)
		q->head = q->head->prev;

	if ((ret = remove_List(&q->tail, (ListType *)l)) != NULL) {
		q->count--;
		if (q->count < 0) {
			q->count = 0;
			q->head = q->tail = NULL;
		}
	}
	return ret;
}

ListType *pop_Queue(QueueType *q) {
ListType *ret;

	if (q == NULL)
		return NULL;

	if ((ret = pop_List(&q->tail)) != NULL)
		q->count--;

	if (q->tail == NULL)
		q->head = NULL;

	return ret;
}

/* pop off the end */
ListType *dequeue_Queue(QueueType *q) {
ListType *ret;

	if (q == NULL || q->head == NULL)
		return NULL;

	ret = q->head;
	q->count--;

	q->head = q->head->prev;
	if (q->head == NULL)
		q->tail = NULL;

	return ret;
}

ListType *concat_Queue(QueueType *q, void *l) {
	if (q == NULL || l == NULL)
		return NULL;

	if (concat_List(&q->head, (ListType *)l) == NULL)
		return NULL;

	if (q->tail == NULL)
		q->tail = rewind_List(q->head);
	q->head = unwind_List(q->head);

	while(l != NULL) {
		l = ((ListType *)l)->next;
		q->count++;
	}
	return q->tail;
}

void sort_Queue(QueueType *q, int (*sort_func)(void *, void *)) {
	if (q == NULL || sort_func == NULL)
		return;

	sort_List(&q->tail, sort_func);
	q->head = unwind_List(q->tail);
}

int count_Queue(QueueType *q) {
	if (q == NULL)
		return -1;

	return q->count;
}

/* EOB */
