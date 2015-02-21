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
	Message.h	WJ99
*/

#ifndef MESSAGE_H_WJ99
#define MESSAGE_H_WJ99 1

#include <config.h>

#include "defines.h"
#include "Queue.h"
#include "StringList.h"
#include "sys_time.h"
#include "CachedFile.h"

#define new_MailToQueue				new_Queue
#define add_MailToQueue(x,y)		(MailTo *)add_Queue((x), (y))
#define prepend_MailToQueue(x,y)	(MailTo *)prepend_Queue((x), (y))
#define concat_MailToQueue(x,y)		(MailTo *)concat_Queue((x), (y))
#define remove_MailToQueue(x,y)		(MailTo *)remove_Queue((x), (y))
#define pop_MailToQueue(x)			(MailTo *)pop_Queue(x)
#define dequeue_MailToQueue(x)		(MailTo *)dequeue_Queue(x)
#define destroy_MailToQueue(x)		destroy_Queue((x), destroy_MailTo)
#define deinit_MailToQueue(x)		deinit_Queue((x), destroy_MailTo)
#define sort_MailToQueue(x,y)		(MailTo *)sort_Queue((x), (y))

#define MSG_FROM_SYSOP				1
#define MSG_FROM_ROOMAIDE			2
#define MSG_REPLY					4
#define MSG_FORWARDED				8
#define MSG_DELETED_BY_SYSOP		0x10
#define MSG_DELETED_BY_ROOMAIDE		0x20
#define MSG_DELETED_BY_ANON			0x40
#define REPLY_TO_SYSOP				0x80
#define REPLY_TO_ROOMAIDE			0x100
#define REPLY_TO_ANON				0x200
#define MSG_ALL						0x3ff	/* MSG_FROM_SYSOP | ... | MSG_.._RA */

#define SAVE_MAILTO		1	/* flag for save_Message() */

typedef QueueType MailToQueue;

typedef struct MailTo_tag MailTo;

struct MailTo_tag {
	List(MailTo);

	char *name;
	unsigned long number;
};

typedef struct {
	unsigned long number, reply_number;
	time_t mtime, deleted;
	unsigned int flags;

	char from[MAX_NAME];
	char *subject, *anon, *deleted_by, *reply_name, *room_name;

	MailToQueue *to;
	StringIO *msg;
} Message;

Message *new_Message(void);
void destroy_Message(Message *);

MailTo *new_MailTo(void);
void destroy_MailTo(MailTo *);

Message *load_Message(char *, unsigned long);
int save_Message(Message *, char *, int);
int load_Message_version0(File *, Message *);
int load_Message_version1(File *, Message *);
int save_Message_version1(File *, Message *, int);
Message *copy_Message(Message *);

MailTo *new_MailTo_from_str(char *);
MailTo *in_MailToQueue(MailToQueue *, char *);

#endif	/* MESSAGE_H_WJ99 */

/* EOB */
