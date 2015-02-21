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
	StringList.h    WJ98
*/

#ifndef STRINGLIST_H_WJ98
#define STRINGLIST_H_WJ98 1

#include "List.h"
#include "Queue.h"

#include <stdarg.h>

#define add_StringList(x,y)			(StringList *)add_List((x), (y))
#define prepend_StringList(x,y)		(StringList *)prepend_List((x), (y))
#define remove_StringList(x,y)		(StringList *)remove_List((x), (y))
#define concat_StringList(x,y)		(StringList *)concat_List((x), (y))
#define pop_StringList(x)			(StringList *)pop_List(x)
#define listdestroy_StringList(x)	listdestroy_List((x), destroy_StringList)
#define rewind_StringList(x)		(StringList *)rewind_List(x)
#define unwind_StringList(x)		(StringList *)unwind_List(x)
#define sort_StringList(x, y)		(StringList *)sort_List((x), (y))

#define add_StringQueue(x,y)		(StringList *)add_Queue((x), (y))
#define remove_StringQueue(x,y)		(StringList *)remove_Queue((x), (y))
#define concat_StringQueue(x,y)		(StringList *)concat_Queue((x), (y))
#define pop_StringQueue(x)			(StringList *)pop_Queue(x)
#define dequeue_StringQueue(x)		(StringList *)dequeue_Queue(x)
#define new_StringQueue				new_Queue
#define destroy_StringQueue(x)		destroy_Queue((x), destroy_StringList)
#define deinit_StringQueue(x)		deinit_Queue((x), destroy_StringList)
#define sort_StringQueue(x, y)		sort_Queue((x), (y))

typedef QueueType StringQueue;

typedef struct StringList_tag StringList;

struct StringList_tag {
	List(StringList);

	char *str;
};

StringList *new_StringList(char *);
void destroy_StringList(StringList *);
StringList *in_StringList(StringList *, char *);
StringList *in_StringQueue(QueueType *, char *);
char *str_StringList(StringList *);
StringList *load_StringList(char *);
int save_StringList(StringList *, char *);
StringList *copy_StringList(StringList *);
StringList *vadd_String(StringList **, char *, va_list);
StringList *add_String(StringList **, char *, ...);
int alphasort_StringList(void *, void *);

#endif	/* STRINGLIST_H_WJ98 */

/* EOB */
