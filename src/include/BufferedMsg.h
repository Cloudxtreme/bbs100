/*
    bbs100 1.2.2 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	BufferedMsg.h	WJ99
*/

#ifndef BUFFEREDMSG_H_WJ99
#define BUFFEREDMSG_H_WJ99 1

#include <config.h>

#include "defines.h"
#include "StringList.h"
#include "sys_time.h"

#define add_BufferedMsg(x,y)		add_List((x), (y))
#define concat_BufferedMsg(x,y)		concat_List((x), (y))
#define remove_BufferedMsg(x,y)		remove_List((x), (y))
#define listdestroy_BufferedMsg(x)	listdestroy_List((x), destroy_BufferedMsg)
#define rewind_BufferedMsg(x)		(BufferedMsg *)rewind_List(x)
#define unwind_BufferedMsg(x)		(BufferedMsg *)unwind_List(x)

#define BUFMSG_SEEN			1
#define BUFMSG_XMSG			2
#define BUFMSG_EMOTE		4
#define BUFMSG_FEELING		8
#define BUFMSG_QUESTION		0x10
#define BUFMSG_SYSOP		0x20
#define BUFMSG_ROOMAIDE		0x40

typedef struct BufferedMsg_tag BufferedMsg;

struct BufferedMsg_tag {
	List(BufferedMsg);

	unsigned int flags;
	time_t mtime;
	char from[MAX_NAME];
	StringList *to, *msg;
};

BufferedMsg *new_BufferedMsg(void);
void destroy_BufferedMsg(BufferedMsg *);

BufferedMsg *copy_BufferedMsg(BufferedMsg *);

#endif	/* BUFFEREDMSG_H_WJ99 */

/* EOB */
