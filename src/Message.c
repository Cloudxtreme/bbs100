/*
    bbs100 1.2.0 WJ102
    Copyright (C) 2002  Walter de Jong <walter@heiho.net>

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
	Message.c	WJ99
*/

#include <config.h>

#include "Message.h"
#include "cstring.h"
#include "Timer.h"
#include "CachedFile.h"
#include "sys_time.h"
#include "cstring.h"
#include "strtoul.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

Message *new_Message(void) {
Message *m;

	if ((m = (Message *)Malloc(sizeof(Message), TYPE_MESSAGE)) == NULL)
		return NULL;

	m->mtime = rtc;
	return m;
}

void destroy_Message(Message *m) {
	if (m == NULL)
		return;

	listdestroy_StringList(m->to);
	listdestroy_StringList(m->msg);
	Free(m);
}


Message *load_Message(char *filename, MsgIndex *idx) {
Message *m;
char buf[MAX_LINE];
File *f;

	if (filename == NULL || (f = Fopen(filename)) == NULL)
		return NULL;

	if ((m = new_Message()) == NULL) {
		Fclose(f);
		return NULL;
	}
	if (idx == NULL) {
		char *p;

		if ((p = cstrrchr(filename, '/')) == NULL)
			p = filename;
		m->number = strtoul(p, NULL, 10);
	} else
		m->number = idx->number;

/* mtime */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	m->mtime = (time_t)strtoul(buf, NULL, 10);

/* deleted */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	m->deleted = (time_t)strtoul(buf, NULL, 10);

/* flags */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	m->flags = (time_t)strtoul(buf, NULL, 16);
	m->flags &= MSG_ALL;

/* from */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	strncpy(m->from, buf, MAX_NAME);
	m->from[MAX_NAME-1] = 0;

/* anon (may be empty) */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	strncpy(m->anon, buf, MAX_NAME);
	m->anon[MAX_NAME-1] = 0;

/* deleted_by */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	strncpy(m->deleted_by, buf, MAX_NAME);
	m->deleted_by[MAX_NAME-1] = 0;

/* subject (may be empty) */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	strncpy(m->subject, buf, MAX_LINE);
	m->subject[MAX_LINE-1] = 0;

/* to */
	listdestroy_StringList(m->to);
	m->to = Fgetlist(f);

/* the message */
	listdestroy_StringList(m->msg);
	m->msg = Fgetlist(f);

/* reply_number */
	if (Fgets(f, buf, MAX_LINE) != NULL) {
		cstrip_line(buf);
		if (*buf)
			m->reply_number = strtoul(buf, NULL, 10);
	}
	Fclose(f);
	return m;

err_load_message:
	Fclose(f);
	destroy_Message(m);
	return NULL;
}

int save_Message(Message *m, char *filename) {
File *f;

	if (m == NULL || filename == NULL || !*filename
		|| (f = Fcreate(filename)) == NULL)
		return -1;

/* mtime + deleted + flags */
	Fprintf(f, "%lu", m->mtime);
	Fprintf(f, "%lu", m->deleted);
	Fprintf(f, "%X", m->flags);

/* from + anon + deleted_by + subject */
	Fputs(f, m->from);
	Fputs(f, m->anon);
	Fputs(f, m->deleted_by);
	Fputs(f, m->subject);

/* to */
	Fputlist(f, m->to);

/* msg data */
	m->msg = rewind_StringList(m->msg);
	Fputlist(f, m->msg);

/* reply_number */
	Fprintf(f, "%lu", m->reply_number);

	Fclose(f);
	return 0;
}

Message *copy_Message(Message *msg) {
Message *m;

	if (msg == NULL)
		return NULL;

	if ((m = new_Message()) == NULL)
		return NULL;

	memcpy(m, msg, sizeof(Message));

	m->to = copy_StringList(msg->to);
	m->msg = copy_StringList(msg->msg);

	return m;
}

/* EOB */
