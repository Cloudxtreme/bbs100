/*
    bbs100 1.2.3 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
#include "MsgIndex.h"
#include "sys_time.h"
#include "CachedFile.h"

#define MSG_FROM_SYSOP				1
#define MSG_FROM_ROOMAIDE			2
#define MSG_REPLY					4
#define MSG_FORWARDED				8
#define MSG_DELETED_BY_SYSOP		0x10
#define MSG_DELETED_BY_ROOMAIDE		0x20
#define MSG_DELETED_BY_ANON			0x40
#define MSG_ALL						0x7f	/* MSG_FROM_SYSOP | ... | MSG_.._RA */

typedef struct {
	unsigned long number, reply_number;
	time_t mtime, deleted;
	unsigned int flags;

	char from[MAX_NAME], anon[MAX_NAME], subject[MAX_LINE], deleted_by[MAX_NAME];

	StringList *to, *msg;
} Message;

Message *new_Message(void);
void destroy_Message(Message *);

Message *load_Message(char *, MsgIndex *);
int save_Message(Message *, char *);

int load_Message_version0(File *, Message *);
int load_Message_version1(File *, Message *);

int save_Message_version0(File *, Message *);
int save_Message_version1(File *, Message *);

Message *copy_Message(Message *);

#endif	/* MESSAGE_H_WJ99 */

/* EOB */
