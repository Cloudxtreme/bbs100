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
	BufferedMsg.h	WJ99
*/

#ifndef BUFFEREDMSG_H_WJ99
#define BUFFEREDMSG_H_WJ99 1

#include <config.h>

#include "defines.h"
#include "PList.h"
#include "StringList.h"
#include "StringIO.h"
#include "sys_time.h"

#define rewind_BufferedMsg(x)		rewind_PList(x)
#define unwind_BufferedMsg(x)		unwind_PList(x)

#define BUFMSG_ONESHOT		0		/* used for friend notifies and such */
#define BUFMSG_XMSG			1
#define BUFMSG_EMOTE		2
#define BUFMSG_FEELING		3
#define BUFMSG_QUESTION		4
#define BUFMSG_ANSWER		5
#define BUFMSG_TYPE			0xff	/* room for 256 message types (how many do you want?) */
#define BUFMSG_SYSOP		0x100

typedef struct {
	int flags, refcount;
	time_t mtime;
	char from[MAX_NAME], *xmsg_header;
	StringList *to;
	StringIO *msg;
} BufferedMsg;

BufferedMsg *new_BufferedMsg(void);
void destroy_BufferedMsg(BufferedMsg *);

BufferedMsg *ref_BufferedMsg(BufferedMsg *);

BufferedMsg *add_BufferedMsg(PList **, BufferedMsg *);
PList *concat_BufferedMsg(PList **, PList *);
BufferedMsg *remove_BufferedMsg(PList **, BufferedMsg *);
void listdestroy_BufferedMsg(PList *);

#endif	/* BUFFEREDMSG_H_WJ99 */

/* EOB */
