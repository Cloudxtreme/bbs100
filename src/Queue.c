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
	Queue.c	WJ105

	Queues are convenient when having to list_Count() the list often
	Adding and prepending to a Queue is efficient
*/

#include "config.h"
#include "Queue.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

Queue *new_Queue(void) {
	return (Queue *)Malloc(sizeof(Queue), TYPE_QUEUE);
}

void destroy_Queue(Queue *q, void *destroy_func) {
	if (q == NULL || destroy_func == NULL)
		return;

	listdestroy_List(q->tail, (void (*)(ListType *))destroy_func);
	Free(q);
}

ListType *add_Queue(Queue *q, void *l) {
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

ListType *prepend_Queue(Queue *q, void *l) {
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

ListType *remove_Queue(Queue *q, void *l) {
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

ListType *pop_Queue(Queue *q) {
ListType *ret;

	if (q == NULL)
		return NULL;

	ret = pop_List(&q->tail);
	if (q->tail == NULL)
		q->head = NULL;
	return ret;
}

void concat_Queue(Queue *q, void *l) {
	if (q == NULL || l == NULL)
		return;

	concat_List(&q->head, (ListType *)l);
	if (q->tail == NULL)
		q->tail = rewind_List(q->head);
}

void sort_Queue(Queue *q, int (*sort_func)(void *, void *)) {
	if (q == NULL || sort_func == NULL)
		return;

	q->tail = sort_List(q->tail, sort_func);
	q->head = unwind_List(q->tail);
}

int Queue_count(Queue *q) {
	if (q == NULL)
		return -1;

	return q->count;
}

/* EOB */
