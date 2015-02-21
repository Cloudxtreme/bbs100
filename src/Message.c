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
	Message.c	WJ99
*/

#include "config.h"
#include "Message.h"
#include "cstring.h"
#include "Timer.h"
#include "CachedFile.h"
#include "sys_time.h"
#include "cstring.h"
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

	if ((m->msg = new_StringIO()) == NULL) {
		destroy_Message(m);
		return NULL;
	}
	m->mtime = rtc;
	return m;
}

void destroy_Message(Message *m) {
	if (m == NULL)
		return;

	Free(m->subject);
	Free(m->anon);
	Free(m->deleted_by);
	Free(m->reply_name);
	Free(m->room_name);
	destroy_MailToQueue(m->to);
	destroy_StringIO(m->msg);
	Free(m);
}

MailTo *new_MailTo(void) {
	return (MailTo *)Malloc(sizeof(MailTo), TYPE_MAILTO);
}

void destroy_MailTo(MailTo *m) {
	if (m == NULL)
		return;

	Free(m->name);
	Free(m);
}


Message *load_Message(char *filename, unsigned long number) {
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
	if (!number) {
		char *p;

		if ((p = cstrrchr(filename, '/')) == NULL)		/* get msg number from filename */
			p = filename;

		m->number = cstrtoul(p, 10);
	} else
		m->number = number;

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
char buf[PRINT_BUF], *p;
int ff1_continue;

	while(Fgets(f, buf, PRINT_BUF) != NULL) {
		FF1_PARSE;

		FF1_LOAD_LEN("from", m->from, MAX_NAME);
		FF1_LOAD_DUP("anon", m->anon);
		FF1_LOAD_DUP("subject", m->subject);
		FF1_LOAD_DUP("deleted_by", m->deleted_by);
		FF1_LOAD_DUP("reply_name", m->reply_name);
		FF1_LOAD_DUP("room_name", m->room_name);

		FF1_LOAD_ULONG("reply_number", m->reply_number);
		FF1_LOAD_ULONG("mtime", m->mtime);
		FF1_LOAD_ULONG("deleted", m->deleted);
		FF1_LOAD_HEX("flags", m->flags);

		FF1_LOAD_MAILTO("to", m->to);
		FF1_LOAD_STRINGIO("msg", m->msg);

		FF1_LOAD_UNKNOWN;
	}
	m->flags &= MSG_ALL;
	return 0;
}

int load_Message_version0(File *f, Message *m) {
char buf[MAX_LONGLINE];
StringList *sl, *mailto;

/* mtime */
	if (Fgets(f, buf, MAX_LONGLINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	m->mtime = (time_t)cstrtoul(buf, 10);

/* deleted */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	m->deleted = (time_t)cstrtoul(buf, 10);

/* flags */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	m->flags = (time_t)cstrtoul(buf, 16);
	m->flags &= MSG_ALL;

/* from */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	if (!*buf)
		goto err_load_message;

	cstrncpy(m->from, buf, MAX_NAME);

/* anon (may be empty) */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	m->anon = cstrdup(buf);

/* deleted_by */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	m->deleted_by = cstrdup(buf);

/* subject (may be empty) */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_message;

	cstrip_line(buf);
	m->subject = cstrdup(buf);

/* to */
	destroy_MailToQueue(m->to);
	m->to = NULL;

	if ((mailto = Fgetlist(f)) != NULL) {
		if ((m->to = new_MailToQueue()) == NULL)
			goto err_load_message;

		for(sl = pop_StringList(&mailto); sl != NULL; sl = pop_StringList(&mailto)) {
			(void)add_MailToQueue(m->to, new_MailTo_from_str(sl->str));
			destroy_StringList(sl);
		}
		mailto = sl = NULL;
	}
/* the message */
	free_StringIO(m->msg);
	while(Fgets(f, buf, MAX_LINE) != NULL) {
		if (!*buf)
			break;

		put_StringIO(m->msg, buf);
		write_StringIO(m->msg, "\n", 1);
	}
/* reply_number */
	if (Fgets(f, buf, MAX_LINE) != NULL) {
		cstrip_line(buf);
		if (*buf)
			m->reply_number = cstrtoul(buf, 10);
	}
	return 0;

err_load_message:
	return -1;
}

/*
	flag can be SAVE_MAILTO, to save the mailto list
	this is only done for the sender of the mail, so the sender can delete
	all copies of the mail
*/
int save_Message(Message *m, char *filename, int flags) {
File *f;

	if (m == NULL || filename == NULL || !*filename || (f = Fcreate(filename)) == NULL)
		return -1;

	m->flags &= MSG_ALL;
	if (save_Message_version1(f, m, flags)) {
		Fcancel(f);
		return -1;
	}
	return Fclose(f);
}

int save_Message_version1(File *f, Message *m, int flags) {
MailTo *to;
char buf[PRINT_BUF];

	FF1_SAVE_VERSION;
	FF1_SAVE_STR("from", m->from);
	FF1_SAVE_STR("anon", m->anon);
	FF1_SAVE_STR("subject", m->subject);
	FF1_SAVE_STR("deleted_by", m->deleted_by);
	FF1_SAVE_STR("reply_name", m->reply_name);
	FF1_SAVE_STR("room_name", m->room_name);

	Fprintf(f, "reply_number=%lu", m->reply_number);
	Fprintf(f, "mtime=%lu", m->mtime);
	Fprintf(f, "deleted=%lu", m->deleted);
	Fprintf(f, "flags=0x%x", m->flags);

	FF1_SAVE_MAILTO("to", m->to);
	FF1_SAVE_STRINGIO("msg", m->msg);
	return 0;
}

/*
	used for forwarding, so msg->to is not copied over
*/
Message *copy_Message(Message *msg) {
Message *m;

	if (msg == NULL)
		return NULL;

	if ((m = new_Message()) == NULL)
		return NULL;

	m->number = msg->number;
	m->reply_number = msg->reply_number;
	m->mtime = msg->mtime;
	m->deleted = msg->deleted;
	m->flags = msg->flags;
	strcpy(m->from, msg->from);

	m->subject = cstrdup(msg->subject);
	m->anon = cstrdup(msg->anon);
	m->deleted_by = cstrdup(msg->deleted_by);
	m->reply_name = cstrdup(msg->reply_name);
	m->room_name = cstrdup(msg->room_name);

	m->to = NULL;			/* this is not copied */

	copy_StringIO(m->msg, msg->msg);
	return m;
}

MailTo *new_MailTo_from_str(char *str) {
MailTo *to;
char *p;

	if (str == NULL)
		return NULL;

	if ((to = new_MailTo()) == NULL)
		return NULL;

	if ((p = cstrchr(str, '|')) != NULL) {
		*p = 0;
		to->name = cstrdup(str);
		*p = '|';

		to->number = cstrtoul(p+1, 10);
	} else
		to->name = cstrdup(str);

	if (to->name == NULL) {
		destroy_MailTo(to);
		return NULL;
	}
	return to;
}

MailTo *in_MailToQueue(MailToQueue *m, char *name) {
MailTo *l;

	if (name == NULL || !*name)
		return NULL;

	for(l = (MailTo *)m->tail; l != NULL; l = l->next) {
		if (!strcmp(l->name, name))
			return l;
	}
	return NULL;
}

/* EOB */
