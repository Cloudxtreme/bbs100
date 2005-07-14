/*
    bbs100 2.2 WJ105
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
	BufferedMsg.c	WJ99
*/

#include "config.h"
#include "BufferedMsg.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

BufferedMsg *new_BufferedMsg(void) {
BufferedMsg *m;

	if ((m = (BufferedMsg *)Malloc(sizeof(BufferedMsg), TYPE_BUFFEREDMSG)) == NULL)
		return NULL;

	if ((m->msg = new_StringIO()) == NULL) {
		destroy_BufferedMsg(m);
		return NULL;
	}
	return m;
}

void destroy_BufferedMsg(BufferedMsg *m) {
	if (m == NULL)
		return;

	Free(m->xmsg_header);
	listdestroy_StringList(m->to);
	destroy_StringIO(m->msg);
	Free(m);
}

BufferedMsg *copy_BufferedMsg(BufferedMsg *m) {
BufferedMsg *cp;

	if ((cp = new_BufferedMsg()) == NULL)
		return NULL;

	if (m->xmsg_header != NULL && m->xmsg_header[0])
		cp->xmsg_header = cstrdup(m->xmsg_header);

	strcpy(cp->from, m->from);
	cp->mtime = m->mtime;
	cp->flags = m->flags;

	if (m->to != NULL) {
		if ((cp->to = copy_StringList(m->to)) == NULL) {
			destroy_BufferedMsg(cp);
			return NULL;
		}
	}
	copy_StringIO(cp->msg, m->msg);
	return cp;
}

/* EOB */
