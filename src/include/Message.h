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
	Message.h	WJ99
*/

#ifndef MESSAGE_H_WJ99
#define MESSAGE_H_WJ99 1

#include <config.h>

#include "defines.h"
#include "StringList.h"
#include "sys_time.h"
#include "CachedFile.h"

#define add_MailTo(x,y)			(MailTo *)add_List((x), (y))
#define prepend_MailTo(x,y)		(MailTo *)prepend_List((x), (y))
#define concat_MailTo(x,y)		(MailTo *)concat_List((x), (y))
#define remove_MailTo(x,y)		(MailTo *)remove_List((x), (y))
#define pop_MailTo(x)			(MailTo *)pop_List(x)
#define listdestroy_MailTo(x)	listdestroy_List((x), destroy_MailTo)
#define rewind_MailTo(x)		(MailTo *)rewind_List(x)
#define unwind_MailTo(x)		(MailTo *)unwind_List(x)
#define sort_MailTo(x,y)		(MailTo *)sort_List((x), (y))

#define MSG_FROM_SYSOP				1
#define MSG_FROM_ROOMAIDE			2
#define MSG_REPLY					4
#define MSG_FORWARDED				8
#define MSG_DELETED_BY_SYSOP		0x10
#define MSG_DELETED_BY_ROOMAIDE		0x20
#define MSG_DELETED_BY_ANON			0x40
#define MSG_ALL						0x7f	/* MSG_FROM_SYSOP | ... | MSG_.._RA */

#define SAVE_MAILTO		1	/* flag for save_Message() */

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

	char from[MAX_NAME], *subject, *anon, *deleted_by;

	MailTo *to;
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
MailTo *in_MailTo(MailTo *, char *);

#endif	/* MESSAGE_H_WJ99 */

/* EOB */
