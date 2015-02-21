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
	PList.h    WJ100
*/

#ifndef PLIST_H_WJ100
#define PLIST_H_WJ100 1

#include "Queue.h"
#include "List.h"

#define add_PList(x,y)			(PList *)add_List((x), (y))
#define concat_PList(x,y)		(PList *)concat_List((x), (y))
#define remove_PList(x,y)		(PList *)remove_List((x), (y))
#define pop_PList(x)			(PList *)pop_List(x)
#define listdestroy_PList(x)	listdestroy_List((x), destroy_PList)
#define rewind_PList(x)			(PList *)rewind_List(x)
#define unwind_PList(x)			(PList *)unwind_List(x)
#define sort_PList(x,y)			(PList *)sort_List((x), (y))

#define new_PQueue				new_Queue
#define add_PQueue(x,y)			(PList *)add_Queue((x), (y))
#define concat_PQueue(x,y)		(PList *)concat_Queue((x), (y))
#define remove_PQueue(x,y)		(PList *)remove_Queue((x), (y))
#define pop_PQueue(x)			(PList *)pop_Queue(x)
#define destroy_PQueue(x)		destroy_Queue((x), destroy_PList)
#define deinit_PQueue(x)		deinit_Queue((x), destroy_PList)
#define sort_PQueue(x,y)		sort_Queue((x), (y))

typedef QueueType PQueue;

typedef struct PList_tag PList;

struct PList_tag {
	List(PList);
	void *p;
};

PList *new_PList(void *);
void destroy_PList(PList *);
PList *in_PList(PList *, void *);

#endif	/* PLIST_H_WJ100 */

/* EOB */
