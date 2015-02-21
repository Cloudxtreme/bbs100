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
	Chatter18	WJ97
	User.c
*/

#include "config.h"
#include "defines.h"
#include "debug.h"
#include "User.h"
#include "util.h"
#include "log.h"
#include "edit.h"
#include "state.h"
#include "state_room.h"
#include "inet.h"
#include "cstring.h"
#include "Room.h"
#include "Stats.h"
#include "Timer.h"
#include "timeout.h"
#include "CachedFile.h"
#include "passwd.h"
#include "access.h"
#include "sys_time.h"
#include "mydirentry.h"
#include "Param.h"
#include "Memory.h"
#include "FileFormat.h"
#include "Timezone.h"
#include "OnlineUser.h"
#include "bufprintf.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

User *AllUsers = NULL;
User *this_user = NULL;


User *new_User(void) {
User *usr;

	if ((usr = (User *)Malloc(sizeof(User), TYPE_USER)) == NULL)
		return NULL;

	if ((usr->display = new_Display()) == NULL) {
		destroy_User(usr);
		return NULL;
	}
	if ((usr->recipients = new_StringQueue()) == NULL) {
		destroy_User(usr);
		return NULL;
	}
	if ((usr->tablist = new_StringQueue()) == NULL) {
		destroy_User(usr);
		return NULL;
	}
	if ((usr->chat_history = new_StringQueue()) == NULL) {
		destroy_User(usr);
		return NULL;
	}
	if ((usr->text = new_StringIO()) == NULL) {
		destroy_User(usr);
		return NULL;
	}
	if ((usr->scroll = new_PQueue()) == NULL) {
		destroy_User(usr);
		return NULL;
	}
	usr->idle_timer = new_Timer(LOGIN_TIMEOUT, login_timeout, TIMER_ONESHOT);
	add_Timer(&usr->timerq, usr->idle_timer);

/* set sane defaults */
	usr->flags = USR_HIDE_ADDRESS|USR_HIDE_INFO;
	usr->default_room = LOBBY_ROOM;
	default_colors(usr);
	default_symbol_colors(usr);

	usr->curr_msg = -1;
	return usr;
}

void destroy_User(User *usr) {
int i;

	if (usr == NULL)
		return;

	Free(usr->real_name);
	Free(usr->street);
	Free(usr->zipcode);
	Free(usr->city);
	Free(usr->state);
	Free(usr->country);
	Free(usr->phone);
	Free(usr->email);
	Free(usr->www);
	Free(usr->doing);
	Free(usr->reminder);
	Free(usr->default_anon);

	unload_Timezone(usr->timezone);
	usr->tz = NULL;					/* usr->tz is just a reference and is not destroyed here */
	Free(usr->timezone);

	Free(usr->vanity);
	Free(usr->xmsg_header);
	Free(usr->away);
	Free(usr->symbols);

	for(i = 0; i < NUM_QUICK; i++)
		Free(usr->quick[i]);

	for(i = 0; i < NUM_TMP; i++)
		Free(usr->tmpbuf[i]);

	Free(usr->question_asked);

	listdestroy_StringList(usr->friends);
	listdestroy_StringList(usr->enemies);
	listdestroy_StringList(usr->override);

	destroy_StringQueue(usr->recipients);
	destroy_StringQueue(usr->tablist);
	destroy_StringQueue(usr->chat_history);

	listdestroy_Joined(usr->rooms);

	destroy_Message(usr->message);
	destroy_Message(usr->new_message);

	if (usr->mail != NULL && save_Room(usr->mail))
		log_err("destroy_User(): failed to save mail room for user %s", usr->name);

	destroy_Room(usr->mail);

	listdestroy_BufferedMsg(usr->history);
	listdestroy_BufferedMsg(usr->held_msgs);
	destroy_BufferedMsg(usr->send_msg);

	usr->idle_timer = NULL;				/* usr->idle_timer is just a reference */
	listdestroy_Timer(usr->timerq);
	destroy_Telnet(usr->telnet);

	destroy_StringIO(usr->text);
	destroy_StringIO(usr->info);

	destroy_PQueue(usr->scroll);
	destroy_Display(usr->display);

	usr->name[0] = 0;
	Free(usr);
}

void Write(User *usr, char *str) {
	if (usr != NULL)
		put_Conn(usr->conn, str);
}

void Writechar(User *usr, char c) {
	if (usr != NULL)
		putc_Conn(usr->conn, c);
}

void Flush(User *usr) {
	if (usr != NULL)
		flush_Conn(usr->conn);
}

void Put(User *usr, char *str) {
	Out(usr, str);
}

void Print(User *usr, char *fmt, ...) {
va_list args;
char buf[PRINT_BUF];

	if (usr == NULL || fmt == NULL || !*fmt)
		return;

	va_start(args, fmt);
	bufvprintf(buf, PRINT_BUF, fmt, args);	
	va_end(args);

	Out(usr, buf);
}

void Tell(User *usr, char *fmt, ...) {
va_list args;
char buf[PRINT_BUF];

	if (usr == NULL || fmt == NULL || !*fmt)
		return;

	va_start(args, fmt);
	bufvprintf(buf, PRINT_BUF, fmt, args);	
	va_end(args);

	if (usr->runtime_flags & (RTF_BUSY|RTF_HOLD)) {
		BufferedMsg *m;

		if ((m = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory buffering message");
			return;
		}
		m->flags = BUFMSG_ONESHOT;
		put_StringIO(m->msg, buf);
		write_StringIO(m->msg, "\n", 1);
		add_BufferedMsg(&usr->held_msgs, m);
	} else {
		if (usr->curr_room != NULL && (usr->curr_room->flags & ROOM_CHATROOM))
			chatroom_tell_user(usr, buf);
		else
			Out(usr, buf);
	}
}

void notify_friends(User *usr, char *msg, int force) {
User *u;

	if (usr == NULL)
		return;

	for(u = AllUsers; u != NULL; u = u->next) {
		if (u != usr && u->name[0] && in_StringList(u->friends, usr->name) != NULL
			&& ((u->flags & USR_FRIEND_NOTIFY) || force))
			Tell(u, "\n<beep><cyan>%s<magenta> %s\n", usr->name, msg);
	}
}


/*
	load a UserData file
	is really a wrapper around load_User_versionx() functions

	Note: username cannot be equal to usr->name because usr->name is reset
	to zero
*/
int load_User(User *usr, char *username, int flags) {
File *f;
char filename[MAX_PATHLEN];
int version;
int (*load_func)(File *, User *, char *, int) = NULL;

	if (usr == NULL || username == NULL || !*username)
		return -1;
/*
	these should already be NULL but I'm just making sure
*/
	usr->name[0] = 0;

	listdestroy_StringList(usr->friends);
	usr->friends = NULL;
	listdestroy_StringList(usr->enemies);
	usr->enemies = NULL;

	destroy_StringIO(usr->info);
	usr->info = NULL;

	listdestroy_Joined(usr->rooms);
	usr->rooms = NULL;

	destroy_Room(usr->mail);
	usr->mail = NULL;

	unload_Timezone(usr->timezone);
	usr->tz = NULL;
	Free(usr->timezone);
	usr->timezone = NULL;

/* open the file for loading */
	bufprintf(filename, sizeof(filename), "%s/%c/%s/UserData", PARAM_USERDIR, *username, username);
	path_strip(filename);

	if ((f = Fopen(filename)) == NULL) {
		log_err("load_User(): failed to open file %s", filename);
		return -1;
	}

/* determine file version */
	version = fileformat_version(f);
	switch(version) {
		case -1:
			log_err("load_User(): error trying to determine file format version of %s", filename);
			load_func = NULL;
			break;

		case 0:
			Frewind(f);
			load_func = load_User_version0;
			break;

		case 1:
			load_func = load_User_version1;
			break;

		default:
			log_err("load_User(): don't know how to load file format version %d of %s", version, filename);
	}
	if (load_func != NULL && !load_func(f, usr, username, flags)) {
		Room *rm;

		Fclose(f);
		usr->flags &= USR_ALL;
		usr->flags2 &= USR2_ALL;

		if (usr->timezone == NULL)
			usr->timezone = cstrdup(PARAM_DEFAULT_TIMEZONE);
		if (usr->tz == NULL)
			usr->tz = load_Timezone(usr->timezone);

		if ((rm = find_Roombynumber(usr, usr->default_room)) == NULL || room_access(rm, username) < 0)
			usr->default_room = LOBBY_ROOM;
		unload_Room(rm);

		(void)sort_StringList(&usr->friends, alphasort_StringList);
		(void)sort_StringList(&usr->enemies, alphasort_StringList);

		if (!usr->name[0])
			cstrncpy(usr->name, username, MAX_NAME);
		return 0;
	}
	Fclose(f);
	return -1;
}


int load_User_version1(File *f, User *usr, char *username, int flags) {
char buf[PRINT_BUF], *p;
int term_width, term_height, ff1_continue;

	term_width = TERM_WIDTH;
	term_height = TERM_HEIGHT;

	if (flags & LOAD_USER_ROOMS) {
		int load_room_flags = LOAD_ROOM_ALL;

		if (!PARAM_HAVE_RESIDENT_INFO)
			load_room_flags &= ~LOAD_ROOM_INFO;

		usr->mail = load_Mail(username, load_room_flags);
	}
	while(Fgets(f, buf, PRINT_BUF) != NULL) {
		FF1_PARSE;
/*
	the fun starts here
	I thought long about how to optimize this code, but the easiest and most
	comprehensible is really when you just write it all out using macros, and not
	use any tables with data format loader handler functions and all that crap
*/
		FF1_LOAD_LEN("name", usr->name, MAX_NAME);

		if (flags & LOAD_USER_PASSWORD)
			FF1_LOAD_LEN("passwd", usr->passwd, MAX_CRYPTED_PASSWD);
		else
			FF1_SKIP("passwd");

		if (flags & LOAD_USER_ADDRESS) {
			FF1_LOAD_DUP("real_name", usr->real_name);
			FF1_LOAD_DUP("street", usr->street);
			FF1_LOAD_DUP("zipcode", usr->zipcode);
			FF1_LOAD_DUP("city", usr->city);
			FF1_LOAD_DUP("state", usr->state);
			FF1_LOAD_DUP("country", usr->country);
			FF1_LOAD_DUP("phone", usr->phone);
			FF1_LOAD_DUP("email", usr->email);
			FF1_LOAD_DUP("www", usr->www);
			FF1_LOAD_DUP("doing", usr->doing);
			FF1_LOAD_DUP("reminder", usr->reminder);
			FF1_LOAD_DUP("default_anon", usr->default_anon);
			FF1_LOAD_DUP("timezone", usr->timezone);
			FF1_LOAD_DUP("vanity", usr->vanity);
			FF1_LOAD_DUP("xmsg_header", usr->xmsg_header);
			FF1_LOAD_DUP("symbols", usr->symbols);
		} else {
			FF1_SKIP("real_name");
			FF1_SKIP("street");
			FF1_SKIP("zipcode");
			FF1_SKIP("city");
			FF1_SKIP("state");
			FF1_SKIP("country");
			FF1_SKIP("phone");
			FF1_SKIP("email");
			FF1_SKIP("www");
			FF1_SKIP("doing");
			FF1_SKIP("reminder");
			FF1_SKIP("default_anon");
			FF1_SKIP("timezone");
			FF1_SKIP("vanity");
			FF1_SKIP("xmsg_header");
			FF1_SKIP("symbols");
		}
		if (flags & LOAD_USER_DATA) {
			FF1_LOAD_DUP("hostname", usr->tmpbuf[TMP_FROM_HOST]);
			FF1_LOAD_DUP("ipnum", usr->tmpbuf[TMP_FROM_IP]);

			FF1_LOAD_ULONG("birth", usr->birth);
			FF1_LOAD_ULONG("last_logout", usr->last_logout);
			FF1_LOAD_ULONG("last_online_time", usr->last_online_time);
			FF1_LOAD_ULONG("logins", usr->logins);
			FF1_LOAD_ULONG("total_time", usr->total_time);
			FF1_LOAD_ULONG("xsent", usr->xsent);
			FF1_LOAD_ULONG("xrecv", usr->xrecv);
			FF1_LOAD_ULONG("esent", usr->esent);
			FF1_LOAD_ULONG("erecv", usr->erecv);
			FF1_LOAD_ULONG("fsent", usr->fsent);
			FF1_LOAD_ULONG("frecv", usr->frecv);
			FF1_LOAD_ULONG("qsent", usr->qsent);
			FF1_LOAD_ULONG("qansw", usr->qansw);
			FF1_LOAD_ULONG("posted", usr->posted);
			FF1_LOAD_ULONG("read", usr->read);
			FF1_LOAD_INT("term_width", term_width);
			FF1_LOAD_INT("term_height", term_height);
			FF1_LOAD_UINT("default_room", usr->default_room);
			FF1_LOAD_HEX("flags", usr->flags);
			FF1_LOAD_HEX("flags2", usr->flags2);

/* custom colors */
			if (!strcmp(buf, "colors")) {
				int i;

				sscanf(p, "%d %d %d %d %d %d %d %d %d",
					&usr->colors[BACKGROUND],
					&usr->colors[RED],
					&usr->colors[GREEN],
					&usr->colors[YELLOW],
					&usr->colors[BLUE],
					&usr->colors[MAGENTA],
					&usr->colors[CYAN],
					&usr->colors[WHITE],
					&usr->colors[HOTKEY]);

/* check for valid values */
				for(i = 0; i < 8; i++)
					if (usr->colors[i] < 0 || usr->colors[i] > 7)
						usr->colors[i] = i;

				if (usr->colors[8] < 0 || usr->colors[8] > 7)
					usr->colors[8] = 3;

				continue;
			}
			if (!strcmp(buf, "symbol_colors")) {
				int i;

				sscanf(p, "%d %d %d %d %d %d %d %d",
					&usr->symbol_colors[BLACK],
					&usr->symbol_colors[RED],
					&usr->symbol_colors[GREEN],
					&usr->symbol_colors[YELLOW],
					&usr->symbol_colors[BLUE],
					&usr->symbol_colors[MAGENTA],
					&usr->symbol_colors[CYAN],
					&usr->symbol_colors[WHITE]
				);

/* check for valid values */
				for(i = 0; i < 8; i++)
					if (usr->symbol_colors[i] < 0 || usr->symbol_colors[i] > 7)
						usr->symbol_colors[i] = i;

				continue;
			}
		} else {
			FF1_SKIP("hostname");
			FF1_SKIP("ipnum");
			FF1_SKIP("birth");
			FF1_SKIP("last_logout");
			FF1_SKIP("last_online_time");
			FF1_SKIP("logins");
			FF1_SKIP("total_time");
			FF1_SKIP("xsent");
			FF1_SKIP("xrecv");
			FF1_SKIP("esent");
			FF1_SKIP("erecv");
			FF1_SKIP("fsent");
			FF1_SKIP("frecv");
			FF1_SKIP("qsent");
			FF1_SKIP("qansw");
			FF1_SKIP("posted");
			FF1_SKIP("read");
			FF1_SKIP("term_width");
			FF1_SKIP("term_height");
			FF1_SKIP("default_room");
			FF1_SKIP("flags");
			FF1_SKIP("flags2");
			FF1_SKIP("colors");
			FF1_SKIP("symbol_colors");
		}
/* quicklist */
		if ((flags & LOAD_USER_QUICKLIST) && !strcmp(buf, "quick")) {
			char *q;
			int n;

			if ((q = cstrchr(p, ' ')) == NULL)
				FF1_ERROR;

			*q = 0;
			q++;
			if (!*q)
				continue;

			n = atoi(p);
			if (n < 0 || n > 9)
				FF1_ERROR;

			if (strlen(q) >= MAX_NAME)
				FF1_ERROR;

			if (!user_exists(q))
				continue;

			usr->quick[n] = cstrdup(q);
			continue;
		} else
			FF1_SKIP("quick");

		if (flags & LOAD_USER_FRIENDLIST)
			FF1_LOAD_USERLIST("friends", usr->friends);
		else
			FF1_SKIP("friends");

		if (flags & LOAD_USER_ENEMYLIST)
			FF1_LOAD_USERLIST("enemies", usr->enemies);
		else
			FF1_SKIP("enemies");

		if (flags & LOAD_USER_INFO)
			FF1_LOAD_STRINGIO("info", usr->info);
		else
			FF1_SKIP("info");

/* joined rooms */
		if ((flags & LOAD_USER_ROOMS) && !strcmp(buf, "rooms")) {
			Joined *j;
			Room *r;
			char zapped;
			unsigned int number, roominfo_read;
			unsigned long generation;
			long last_read;
/*
	When loading joined rooms, we have to check whether the room still exists,
	whether the room has changed or not, and whether we're still welcome there
*/
			if (sscanf(p, "%c %u %lu %ld %u", &zapped, &number, &generation,
				&last_read, &roominfo_read) != 5)
				FF1_ERROR;

/* already joined? (double entry) */
			if ((j = in_Joined(usr->rooms, number)) != NULL)
				continue;

			if ((r = find_Roombynumber_username(usr, username, number)) == NULL)	/* does the room still exist? */
				continue;
/*
	fix last_read field if too large (room was cleaned out)
*/
			if (last_read < 0L)
				last_read = 0L;
			if (last_read > r->head_msg)
				last_read = r->head_msg;

			if (!room_visible_username(r, username, generation)) {		/* room went hidden */
				unload_Room(r);
				continue;
			}
			if (generation != r->generation) {			/* room has changed */
				generation = r->generation;
				if (r->number != MAIL_ROOM)
					last_read = 0L;						/* begin reading */
			}
/*
	when not welcome any more, zap the room
	the user may be invited again, and won't have to re-read the entire room
*/
			if (room_access(r, username) < 0)			/* not welcome anymore */
				zapped = 'Z';

			if ((j = new_Joined()) == NULL) {
				unload_Room(r);
				FF1_ERROR;
			}
			if (zapped == 'Z' && !(r->flags & ROOM_NOZAP))
				j->zapped = 1;

			j->number = number;
			j->generation = generation;
			j->last_read = last_read;
			j->roominfo_read = roominfo_read;

			(void)prepend_Joined(&usr->rooms, j);
			unload_Room(r);
			continue;
		} else
			FF1_SKIP("rooms");

/*
	add site-specific stuff here
*/

/*
	log unknown keywords
*/
		FF1_LOAD_UNKNOWN;
	}
	if (usr->flags & USR_FORCE_TERM) {
		if (term_width < 1)
			term_width = TERM_WIDTH;
		if (term_height < 1)
			term_height = TERM_HEIGHT;

		if (term_width > MAX_TERM)
			term_width = MAX_TERM;
		if (term_height > MAX_TERM)
			term_height = MAX_TERM;

		usr->display->term_width = term_width;
		usr->display->term_height = term_height;
	}
	return 0;
}


#define LOAD_USERSTRING(x)	do {							\
		Free(x);											\
		(x) = NULL;											\
		if (Fgets(f, buf, MAX_LINE) == NULL)				\
			goto err_load_User;								\
		cstrip_line(buf);									\
		if (*buf) {											\
			if (((x) = cstrdup(buf)) == NULL)				\
				goto err_load_User;							\
		}													\
	} while(0)

#define LOAD_USER_SKIPLIST									\
	while(Fgets(f, buf, MAX_LINE) != NULL) {				\
		cstrip_line(buf);									\
		if (!*buf)											\
			break;											\
	}

#define LOAD_USER_SKIPLINES(x)								\
	for(i = 0; i < (x); i++) {								\
		if (Fgets(f, buf, MAX_LINE) == NULL)				\
			goto err_load_User;								\
	}

/*
	load_User_version0() loads old-style UserData files, in which everything
	is saved in a specific order
	This routine is old, but still in use as long as there are userfiles
	on your system that have not been converted yet.
*/
int load_User_version0(File *f, User *usr, char *username, int flags) {
char buf[MAX_PATHLEN];
StringList *sl;
int i;

	destroy_StringIO(usr->info);
	usr->info = NULL;

/* passwd */
	if (flags & LOAD_USER_PASSWORD) {
		if (Fgets(f, buf, MAX_PASSPHRASE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		cstrncpy(usr->passwd, buf, MAX_CRYPTED_PASSWD);
	} else
		LOAD_USER_SKIPLINES(1);

	if (flags & LOAD_USER_ADDRESS) {
		LOAD_USERSTRING(usr->real_name);
		LOAD_USERSTRING(usr->street);
		LOAD_USERSTRING(usr->zipcode);
		LOAD_USERSTRING(usr->city);
		LOAD_USERSTRING(usr->state);
		LOAD_USERSTRING(usr->country);
		LOAD_USERSTRING(usr->phone);
		LOAD_USERSTRING(usr->email);
		LOAD_USERSTRING(usr->www);
		LOAD_USERSTRING(usr->doing);
		LOAD_USERSTRING(usr->reminder);
		LOAD_USERSTRING(usr->default_anon);
	} else
		LOAD_USER_SKIPLINES(12);

/* from_ip */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_User;

	cstrip_line(buf);

/* put the former from_ip in usr->tmpbuf */
	Free(usr->tmpbuf[TMP_FROM_HOST]);
	usr->tmpbuf[TMP_FROM_HOST] = cstrdup(buf);

/* ipnum */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_User;
	cstrip_line(buf);

/* put former ip number in tmpbuf */
	Free(usr->tmpbuf[TMP_FROM_IP]);
	usr->tmpbuf[TMP_FROM_IP] = cstrdup(buf);

	if (flags & LOAD_USER_DATA) {
/* birth */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->birth = (time_t)cstrtoul(buf, 10);

/* last_logout */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->last_logout = (time_t)cstrtoul(buf, 10);

/* flags */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->flags |= (unsigned int)cstrtoul(buf, 16);
		usr->flags &= USR_ALL;			/* reset non-existant flags */

/* logins */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->logins = cstrtoul(buf, 10);

/* total_time */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->total_time = cstrtoul(buf, 10);

/* xsent */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->xsent = cstrtoul(buf, 10);

/* xrecv */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->xrecv = cstrtoul(buf, 10);

/* esent */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->esent = cstrtoul(buf, 10);

/* erecv */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->erecv = cstrtoul(buf, 10);

/* posted */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->posted = cstrtoul(buf, 10);

/* read */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->read = cstrtoul(buf, 10);

/* colors */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		sscanf(buf, "%d %d %d %d %d %d %d %d %d",
			&usr->colors[BACKGROUND],
			&usr->colors[RED],
			&usr->colors[GREEN],
			&usr->colors[YELLOW],
			&usr->colors[BLUE],
			&usr->colors[MAGENTA],
			&usr->colors[CYAN],
			&usr->colors[WHITE],
			&usr->colors[HOTKEY]);

/* check for valid values */
		for(i = 0; i < 8; i++)
			if (usr->colors[i] < 0 || usr->colors[i] > 7)
				usr->colors[i] = i;
		if (usr->colors[8] < 0 || usr->colors[8] > 7)
			usr->colors[8] = 3;
	} else
		LOAD_USER_SKIPLINES(12);

/* load joined rooms */
	if (flags & LOAD_USER_ROOMS) {
		Joined *j;
		Room *r;
		char zapped;
		unsigned int number, roominfo_read;
		unsigned long generation;
		long last_read;
		int load_room_flags;

		listdestroy_Joined(usr->rooms);
		usr->rooms = NULL;

/* setup the user's mail room */
		destroy_Room(usr->mail);

		load_room_flags = LOAD_ROOM_ALL;
		if (!PARAM_HAVE_RESIDENT_INFO)
			load_room_flags &= ~LOAD_ROOM_INFO;

		usr->mail = load_Mail(username, load_room_flags);

/*
	When loading joined rooms, we have to check whether the room still exists,
	whether the room has changed or not, and whether we're still welcome there
*/
		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (sscanf(buf, "%c %u %lu %ld %u", &zapped, &number, &generation,
				&last_read, &roominfo_read) != 5)
				goto err_load_User;

/* already joined?? (fix old bug in _really_ old user files) */
			if ((j = in_Joined(usr->rooms, number)) != NULL)
				continue;

			if ((r = find_Roombynumber_username(usr, username, number)) == NULL)	/* does the room still exist? */
				continue;
/*
	fix last_read field if too large (room was cleaned out)
*/
			if (last_read < 0L)
				last_read = 0L;
			if (last_read > r->head_msg)
				last_read = r->head_msg;

			if (!room_visible_username(r, username, generation)) {		/* room went hidden */
				unload_Room(r);
				continue;
			}
			if (generation != r->generation) {			/* room has changed */
				generation = r->generation;
				if (r->number != MAIL_ROOM)
					last_read = 0L;						/* begin reading */
			}
			if (room_access(r, username) < 0)			/* not welcome anymore */
				zapped = 'Z';

			if ((j = new_Joined()) == NULL) {
				unload_Room(r);
				goto err_load_User;
			}
			if (zapped == 'Z' && !(r->flags & ROOM_NOZAP))
				j->zapped = 1;

			j->number = number;
			j->generation = generation;
			j->last_read = last_read;
			j->roominfo_read = roominfo_read;

			(void)prepend_Joined(&usr->rooms, j);
			unload_Room(r);
		}
	} else
		LOAD_USER_SKIPLIST;

/* quicklist */
	if (flags & LOAD_USER_QUICKLIST) {
		for(i = 0; i < 10; i++) {
			Free(usr->quick[i]);
			usr->quick[i] = NULL;

			if (Fgets(f, buf, MAX_LINE) == NULL)
				goto err_load_User;

			cstrip_line(buf);
			if (*buf && user_exists(buf))
				usr->quick[i] = cstrdup(buf);
		}
	} else
		LOAD_USER_SKIPLINES(10);
		
/* friendlist */
	if (flags & LOAD_USER_FRIENDLIST) {
		listdestroy_StringList(usr->friends);
		usr->friends = NULL;

		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				(void)prepend_StringList(&usr->friends, sl);
		}
	} else
		LOAD_USER_SKIPLIST;

/* enemy list */
	if (flags & LOAD_USER_ENEMYLIST) {
		listdestroy_StringList(usr->enemies);
		usr->enemies = NULL;

		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				(void)prepend_StringList(&usr->enemies, sl);
		}
	} else
		LOAD_USER_SKIPLIST;

/* info */
	if (flags & LOAD_USER_INFO) {
		while(Fgets(f, buf, MAX_PATHLEN) != NULL) {
			if (!*buf)
				break;

			if (usr->info == NULL && (usr->info = new_StringIO()) == NULL)
				continue;

			put_StringIO(usr->info, buf);
			write_StringIO(usr->info, "\n", 1);
		}
	} else
		LOAD_USER_SKIPLIST;

/* time displacement */
	if (flags & LOAD_USER_DATA) {
/*		usr->time_disp = 0;						deprecated by timezones */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto end_load_User;

		cstrip_line(buf);
/*		usr->time_disp = atoi(buf);				deprecated by timezones */

/* fsent */
		usr->fsent = 0UL;
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto end_load_User;

		cstrip_line(buf);
		usr->fsent = cstrtoul(buf, 10);

/* frecv */
		usr->frecv = 0UL;
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto end_load_User;

		cstrip_line(buf);
		usr->frecv = cstrtoul(buf, 10);
	}

end_load_User:
	return site_load_User_version0(usr, username, flags);

err_load_User:
	return -1;
}

int site_load_User_version0(User *usr, char *username, int flags) {
/*
File *f;
char buf[MAX_PATHLEN];

	if (usr == NULL)
		return -1;

	bufprintf(buf, sizeof(buf), "%s/%c/%s/UserData.site", PARAM_USERDIR, *username, username);
	path_strip(buf);

	if ((f = Fopen(buf)) == NULL)
		return 0;


	DO SITE-SPECIFIC STUFF HERE


	Fclose(f);
	Return 0;
*/
	return 0;
}

int load_profile_info(User *usr) {
User *tmp_user;

	if (usr == NULL || !usr->name[0])
		return -1;

	if (usr->info != NULL)
		return 0;

	tmp_user = new_User();

	if (load_User(tmp_user, usr->name, LOAD_USER_INFO) < 0) {
		cstrcpy(tmp_user->name, "<tmp>", MAX_NAME);
		destroy_User(tmp_user);
		return -1;
	}
	destroy_StringIO(usr->info);
	usr->info = tmp_user->info;
	tmp_user->info = NULL;

	cstrcpy(tmp_user->name, "<tmp>", MAX_NAME);
	destroy_User(tmp_user);
	return 0;
}

int save_User(User *usr) {
char filename[MAX_PATHLEN];
File *f;

	if (usr == NULL)
		return -1;

	if (!usr->name[0])
		return 1;

	if (is_guest(usr->name))		/* don't save Guest user */
		return 0;

	if (usr->logins > 0)			/* new users don't have this yet */
		load_profile_info(usr);

	usr->last_online_time = (unsigned long)rtc - (unsigned long)usr->login_time;

	bufprintf(filename, sizeof(filename), "%s/%c/%s/UserData", PARAM_USERDIR, usr->name[0], usr->name);
	path_strip(filename);

	if ((f = Fcreate(filename)) == NULL) {
		Perror(usr, "failed to save userfile");
		return -1;
	}
	usr->flags &= USR_ALL;
	usr->flags2 &= USR2_ALL;
	if (save_User_version1(f, usr)) {
		Perror(usr, "failed to save userfile");
		Fcancel(f);
		return -1;
	}
	if (Fclose(f)) {
		Perror(usr, "failed to save userfile");
		return -1;
	}
	if (!PARAM_HAVE_RESIDENT_INFO) {
		destroy_StringIO(usr->info);		/* don't keep it resident */
		usr->info = NULL;
	}
	return 0;
}

int save_User_version1(File *f, User *usr) {
int i;
Joined *j;
StringList *sl;
char buf[PRINT_BUF];

	FF1_SAVE_VERSION;
	FF1_SAVE_STR("name", usr->name);
	FF1_SAVE_STR("passwd", usr->passwd);
	FF1_SAVE_STR("real_name", usr->real_name);
	FF1_SAVE_STR("street", usr->street);
	FF1_SAVE_STR("zipcode", usr->zipcode);
	FF1_SAVE_STR("city", usr->city);
	FF1_SAVE_STR("state", usr->state);
	FF1_SAVE_STR("country", usr->country);
	FF1_SAVE_STR("phone", usr->phone);
	FF1_SAVE_STR("email", usr->email);
	FF1_SAVE_STR("www", usr->www);
	FF1_SAVE_STR("doing", usr->doing);
	FF1_SAVE_STR("reminder", usr->reminder);
	FF1_SAVE_STR("default_anon", usr->default_anon);
	FF1_SAVE_STR("timezone", usr->timezone);
	FF1_SAVE_STR("vanity", usr->vanity);
	FF1_SAVE_STR("xmsg_header", usr->xmsg_header);
	FF1_SAVE_STR("symbols", usr->symbols);

	FF1_SAVE_STR("hostname", usr->conn->hostname);
	FF1_SAVE_STR("ipnum", usr->conn->ipnum);

	Fprintf(f, "birth=%lu", (unsigned long)usr->birth);
	Fprintf(f, "last_logout=%lu", (unsigned long)usr->last_logout);
	Fprintf(f, "last_online_time=%lu", usr->last_online_time);
	Fprintf(f, "default_room=%u", usr->default_room);
	Fprintf(f, "flags=0x%x", usr->flags);
	Fprintf(f, "flags2=0x%x", usr->flags2);
	Fprintf(f, "logins=%lu", usr->logins);
	Fprintf(f, "total_time=%lu", usr->total_time);

	Fprintf(f, "xsent=%lu", usr->xsent);
	Fprintf(f, "xrecv=%lu", usr->xrecv);
	Fprintf(f, "esent=%lu", usr->esent);
	Fprintf(f, "erecv=%lu", usr->erecv);
	Fprintf(f, "fsent=%lu", usr->fsent);
	Fprintf(f, "frecv=%lu", usr->frecv);
	Fprintf(f, "qsent=%lu", usr->qsent);
	Fprintf(f, "qansw=%lu", usr->qansw);
	Fprintf(f, "posted=%lu", usr->posted);
	Fprintf(f, "read=%lu", usr->read);

	Fprintf(f, "term_width=%d", usr->display->term_width);
	Fprintf(f, "term_height=%d", usr->display->term_height);

	Fprintf(f, "colors=%d %d %d %d %d %d %d %d %d",
		usr->colors[BACKGROUND],
		usr->colors[RED],
		usr->colors[GREEN],
		usr->colors[YELLOW],
		usr->colors[BLUE],
		usr->colors[MAGENTA],
		usr->colors[CYAN],
		usr->colors[WHITE],
		usr->colors[HOTKEY]
	);
	Fprintf(f, "symbol_colors=%d %d %d %d %d %d %d %d",
		usr->symbol_colors[BLACK],
		usr->symbol_colors[RED],
		usr->symbol_colors[GREEN],
		usr->symbol_colors[YELLOW],
		usr->symbol_colors[BLUE],
		usr->symbol_colors[MAGENTA],
		usr->symbol_colors[CYAN],
		usr->symbol_colors[WHITE]
	);
	for(i = 0; i < 10; i++)
		if (usr->quick[i] != NULL)
			Fprintf(f, "quick=%d %s", i, usr->quick[i]);

	for(j = usr->rooms; j != NULL; j = j->next)
		Fprintf(f, "rooms=%c %u %lu %ld %u", (j->zapped == 0) ? 'J' : 'Z', j->number,
			j->generation, j->last_read, j->roominfo_read);

	FF1_SAVE_STRINGLIST("friends", usr->friends);
	FF1_SAVE_STRINGLIST("enemies", usr->enemies);
	FF1_SAVE_STRINGIO("info", usr->info);

/*
	add site-specific stuff here

	it's okay to add new stuff to the same UserData file, as long as you don't use
	any keywords that I am going to use in future versions ...
*/
	return 0;
}

void logout_user(User *usr) {
	Enter(logout_user);

	if (usr->name[0]) {
		if (usr->flags & USR_HELPING_HAND)
			remove_helper(usr);

		if (usr->runtime_flags & RTF_WAS_HH)	/* restore HH status before saving the User */
			usr->flags |= USR_HELPING_HAND;

		usr->last_logout = (unsigned long)rtc;
		update_stats(usr);

		if (save_User(usr))
			log_err("failed to save user %s", usr->name);

		remove_OnlineUser(usr);
	}
	leave_room(usr);
	usr->name[0] = 0;

	if (usr->conn->sock > 0) {
		Put(usr, "<default>\n");
		Flush(usr);
	}
	Return;
}

void close_connection(User *usr, char *reason, ...) {
	if (usr == NULL)
		return;

	Enter(close_connection);

	if (reason != NULL) {			/* log why we're being disconnected */
		va_list ap;
		char buf[PRINT_BUF];
		int len;

		if (usr->name[0])
			len = bufprintf(buf, sizeof(buf), "CLOSE %s (%s): ", usr->name, usr->conn->hostname);
		else
			len = bufprintf(buf, sizeof(buf), "CLOSE (%s): ", usr->conn->hostname);

		va_start(ap, reason);
		bufvprintf(buf+len, sizeof(buf) - len, reason, ap);
		va_end(ap);

		log_auth(buf);
	}
	logout_user(usr);

	if (usr->conn != NULL)
		usr->conn->state = CONN_WAIT_CLOSE;
	Return;
}

void close_logout(void *v) {
User *usr;

	if (v == NULL)
		return;

	usr = (User *)v;
	if (usr == NULL)
		return;

	if (usr->conn->sock > 0) {
		Put(usr, "<default>\n");
		Flush(usr);
	}
	close_Conn(usr->conn);
}

/* EOB */
