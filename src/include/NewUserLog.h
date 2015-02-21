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
	NewUserLog.h	WJ105
*/

#ifndef NEWUSERLOG_H_WJ105
#define NEWUSERLOG_H_WJ105	1

#include "List.h"
#include "Queue.h"
#include "CachedFile.h"
#include "defines.h"

#define add_NewUserLog(x,y)			(NewUserLog *)add_List((x), (y))
#define prepend_NewUserLog(x,y)		(NewUserLog *)prepend_List((x), (y))
#define remove_NewUserLog(x,y)		(NewUserLog *)remove_List((x), (y))
#define concat_NewUserLog(x,y)		(NewUserLog *)concat_List((x), (y))
#define pop_NewUserLog(x)			(NewUserLog *)pop_List(x)
#define listdestroy_NewUserLog(x)	listdestroy_List((x), destroy_NewUserLog)
#define rewind_NewUserLog(x)		(NewUserLog *)rewind_List(x)
#define unwind_NewUserLog(x)		(NewUserLog *)unwind_List(x)
#define sort_NewUserLog(x, y)		(NewUserLog *)sort_List((x), (y))

#define add_NewUserQueue(x,y)		(NewUserLog *)add_Queue((x), (y))
#define remove_NewUserQueue(x,y)	(NewUserLog *)remove_Queue((x), (y))
#define concat_NewUserQueue(x,y)	(NewUserLog *)concat_Queue((x), (y))
#define pop_NewUserQueue(x)			(NewUserLog *)pop_Queue(x)
#define dequeue_NewUserQueue(x)		(NewUserLog *)dequeue_Queue(x)
#define new_NewUserQueue			new_Queue
#define destroy_NewUserQueue(x)		destroy_Queue((x), destroy_NewUserLog)
#define deinit_NewUserQueue(x)		deinit_Queue((x), destroy_NewUserLog)
#define sort_NewUserQueue(x, y)		sort_Queue((x), (y))

typedef QueueType NewUserQueue;

typedef struct NewUserLog_tag NewUserLog;

struct NewUserLog_tag {
	List(NewUserLog);

	unsigned long timestamp;
	char name[MAX_NAME];
};

NewUserLog *new_NewUserLog(void);
void destroy_NewUserLog(NewUserLog *);

NewUserQueue *load_NewUserQueue(char *);
int load_NewUserQueue_version1(File *, NewUserQueue *);
int save_NewUserQueue(NewUserQueue *, char *);

void add_newuserlog(NewUserQueue *, NewUserLog *);

void log_newuser(char *);

#endif	/* NEWUSERLOG_H_WJ105 */

/* EOB */
