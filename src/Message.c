/*
    bbs100 2.1 WJ104
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
	Message.c	WJ99
*/

#include "config.h"
#include "Message.h"
#include "cstring.h"
#include "Timer.h"
#include "CachedFile.h"
#include "sys_time.h"
#include "cstring.h"
#include "strtoul.h"
#include "Memory.h"
#include "FileFormat.h"
#include "log.h"

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
File *f;
int (*load_func)(File *, Message *) = NULL;
int version;

	if (filename == NULL || (f = Fopen(filename)) == NULL)
		return NULL;

/* first fill in the message number */
	if ((m = new_Message()) == NULL) {
		Fclose(f);
		return NULL;
	}
	if (idx == NULL) {
		char *p;

		if ((p = cstrrchr(filename, '/')) == NULL)		/* get msg number from filename */
			p = filename;

		m->number = strtoul(p, NULL, 10);
	} else
		m->number = idx->number;

/* get file format version */
	version = fileformat_version(f);
	switch(version) {
		case -1:
			log_err("load_Message(): error trying to determine file format version of %s", filename);
			load_func = NULL;
			break;

		case 0:
			Frewind(f);
			load_func = load_Message_version0;
			break;

		case 1:
			load_func = load_Message_version1;
			break;

		default:
			log_err("load_Message(): don't know how to load version %d of %s", version, filename);
	}
	if (load_func != NULL && !load_func(f, m)) {
		Fclose(f);
		return m;
	}
	destroy_Message(m);
	Fclose(f);
	return NULL;
}

int load_Message_version1(File *f, Message *m) {
char buf[MAX_LINE*3], *p;

	while(Fgets(f, buf, MAX_LINE*3) != NULL) {
		FF1_PARSE;

		FF1_LOAD_LEN("from", m->from, MAX_NAME);
		FF1_LOAD_LEN("anon", m->anon, MAX_NAME);
		FF1_LOAD_LEN("subject", m->subject, MAX_LINE);
		FF1_LOAD_LEN("deleted_by", m->deleted_by, MAX_NAME);

		FF1_LOAD_ULONG("reply_number", m->reply_number);
		FF1_LOAD_ULONG("mtime", m->mtime);
		FF1_LOAD_ULONG("deleted", m->deleted);
		FF1_LOAD_HEX("flags", m->flags);

		FF1_LOAD_STRINGLIST("to", m->to);
		FF1_LOAD_STRINGLIST("msg", m->msg);

		FF1_LOAD_UNKNOWN;
	}
	m->flags &= MSG_ALL;
	return 0;
}

int load_Message_version0(File *f, Message *m) {
char buf[MAX_LINE*3];

/* mtime */
	if (Fgets(f, buf, MAX_LINE*3) == NULL)
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
	return 0;

err_load_message:
	return -1;
}


int save_Message(Message *m, char *filename) {
int ret;
File *f;

	if (m == NULL || filename == NULL || !*filename
		|| (f = Fcreate(filename)) == NULL)
		return -1;

	m->flags &= MSG_ALL;
	ret = save_Message_version1(f, m);
	Fclose(f);
	return ret;
}

int save_Message_version1(File *f, Message *m) {
StringList *sl;

	FF1_SAVE_VERSION;
	FF1_SAVE_STR("from", m->from);
	FF1_SAVE_STR("anon", m->anon);
	FF1_SAVE_STR("subject", m->subject);
	FF1_SAVE_STR("deleted_by", m->deleted_by);

	Fprintf(f, "reply_number=%lu", m->reply_number);
	Fprintf(f, "mtime=%lu", m->mtime);
	Fprintf(f, "deleted=%lu", m->deleted);
	Fprintf(f, "flags=0x%x", m->flags);

	FF1_SAVE_STRINGLIST("to", m->to);
	FF1_SAVE_STRINGLIST("msg", m->msg);

	return 0;
}

int save_Message_version0(File *f, Message *m) {
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
