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
#include "strtoul.h"
#include "Param.h"
#include "Memory.h"
#include "FileFormat.h"
#include "Timezone.h"

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

	usr->socket = -1;
	usr->telnet_state = TS_DATA;

	usr->idle_timer = new_Timer(LOGIN_TIMEOUT, login_timeout, TIMER_ONESHOT);
	add_Timer(&usr->timerq, usr->idle_timer);

	usr->term_height = TERM_HEIGHT;		/* hard-coded defaults; may be set by TELOPT_NAWS */
	usr->term_width = TERM_WIDTH;

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

	for(i = 0; i < NUM_QUICK; i++)
		Free(usr->quick[i]);

	for(i = 0; i < NUM_TMP; i++)
		Free(usr->tmpbuf[i]);

	Free(usr->question_asked);

	listdestroy_StringList(usr->friends);
	listdestroy_StringList(usr->enemies);
	listdestroy_StringList(usr->info);
	listdestroy_StringList(usr->recipients);
	listdestroy_StringList(usr->tablist);
	listdestroy_StringList(usr->talked_to);
	listdestroy_StringList(usr->more_text);
	listdestroy_StringList(usr->chat_history);

	listdestroy_Joined(usr->rooms);

	destroy_Message(usr->message);
	destroy_Message(usr->new_message);
	save_Room(usr->mail);
	destroy_Room(usr->mail);

	listdestroy_BufferedMsg(usr->history);
	listdestroy_BufferedMsg(usr->busy_msgs);
	listdestroy_BufferedMsg(usr->held_msgs);
	destroy_BufferedMsg(usr->send_msg);

	listdestroy_Timer(usr->timerq);
	listdestroy_CallStack(usr->callstack);

	if (usr->socket > 0) {
		shutdown(usr->socket, 2);
		close(usr->socket);
	}
	Free(usr);
}

void Write(User *usr, char *str) {
int c;

	if (usr == NULL)
		return;

	c = strlen(str);

	if ((usr->output_idx+c) >= (MAX_OUTPUTBUF-1))
		Flush(usr);
	strncpy(usr->outputbuf + usr->output_idx, str, c);
	usr->output_idx += c;
	usr->outputbuf[usr->output_idx] = 0;
}

void Writechar(User *usr, char c) {
	if (usr == NULL)
		return;

	if (usr->output_idx >= (MAX_OUTPUTBUF-1))
		Flush(usr);
	usr->outputbuf[usr->output_idx++] = c;
	usr->outputbuf[usr->output_idx] = 0;
}

void Flush(User *usr) {
	if (usr == NULL)
		return;

	if (usr->socket > 0 && usr->output_idx > 0) {
		write(usr->socket, usr->outputbuf, usr->output_idx);
		usr->outputbuf[0] = 0;
		usr->output_idx = 0;
	}
}

void Print(User *usr, char *fmt, ...) {
va_list args;
char buf[PRINT_BUF];

	if (usr == NULL || fmt == NULL || !*fmt || usr->socket < 0)
		return;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);	
	va_end(args);

	Put(usr, buf);
}

void Tell(User *usr, char *fmt, ...) {
va_list args;
char buf[PRINT_BUF];

	if (usr == NULL || fmt == NULL || !*fmt || usr->socket < 0)
		return;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);	
	va_end(args);

	if (usr->runtime_flags & RTF_BUSY) {
		BufferedMsg *m;

		if ((m = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory buffering message");
			return;
		}
		if ((m->msg = new_StringList(buf)) == NULL) {
			destroy_BufferedMsg(m);
			Perror(usr, "Out of memory buffering message");
			return;
		}
		add_BufferedMsg(&usr->busy_msgs, m);
	} else
		Put(usr, buf);
}


void notify_friends(User *usr, char *msg) {
User *u;

	if (usr == NULL)
		return;

	for(u = AllUsers; u != NULL; u = u->next) {
		if (u != usr && u->name[0] && u->socket > 0
			&& in_StringList(u->friends, usr->name) != NULL)
			Tell(u, "\n<beep><cyan>%s<magenta> %s\n", usr->name, msg);
	}
}


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
	listdestroy_StringList(usr->enemies);
	listdestroy_StringList(usr->info);
	usr->friends = usr->enemies = usr->info = NULL;

	listdestroy_Joined(usr->rooms);
	usr->rooms = NULL;

	destroy_Room(usr->mail);
	usr->mail = NULL;

	unload_Timezone(usr->timezone);
	usr->tz = NULL;
	Free(usr->timezone);
	usr->timezone = NULL;

/* open the file for loading */
	sprintf(filename, "%s/%c/%s/UserData", PARAM_USERDIR, *username, username);
	path_strip(filename);

	if ((f = Fopen(filename)) == NULL)
		return -1;

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
		if (usr->timezone == NULL)
			usr->timezone = cstrdup(PARAM_DEFAULT_TIMEZONE);
		if (usr->tz == NULL)
			usr->tz = load_Timezone(usr->timezone);

		Fclose(f);
		usr->flags &= USR_ALL;

		if (!usr->name[0]) {
			strncpy(usr->name, username, MAX_NAME-1);
			usr->name[MAX_NAME-1] = 0;
		}
		return 0;
	}
	Fclose(f);
	return -1;
}


int load_User_version1(File *f, User *usr, char *username, int flags) {
char buf[MAX_PATHLEN], *p;

	if (flags & LOAD_USER_ROOMS)
		usr->mail = load_Mail(username);

	while(Fgets(f, buf, MAX_PATHLEN) != NULL) {
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
		}
		if (!strcmp(buf, "from_ip")) {
			Free(usr->tmpbuf[TMP_FROM_HOST]);
			usr->tmpbuf[TMP_FROM_HOST] = NULL;
			FF1_LOAD_DUP("from_ip", usr->tmpbuf[TMP_FROM_HOST]);
		}
		if (!strcmp(buf, "from_ipnum")) {
			Free(usr->tmpbuf[TMP_FROM_IP]);
			usr->tmpbuf[TMP_FROM_IP] = NULL;
			FF1_LOAD_DUP("from_ipnum", usr->tmpbuf[TMP_FROM_IP]);
		}
		if (flags & LOAD_USER_DATA) {
			FF1_LOAD_ULONG("birth", usr->birth);
			FF1_LOAD_ULONG("last_logout", usr->last_logout);
			FF1_LOAD_ULONG("logins", usr->logins);
			FF1_LOAD_ULONG("total_time", usr->total_time);
			FF1_LOAD_ULONG("xsent", usr->xsent);
			FF1_LOAD_ULONG("xrecv", usr->xrecv);
			FF1_LOAD_ULONG("esent", usr->esent);
			FF1_LOAD_ULONG("erecv", usr->erecv);
			FF1_LOAD_ULONG("fsent", usr->fsent);
			FF1_LOAD_ULONG("frecv", usr->frecv);
			FF1_LOAD_ULONG("posted", usr->posted);
			FF1_LOAD_ULONG("read", usr->read);

			FF1_LOAD_HEX("flags", usr->flags);

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
		}
		if (flags & LOAD_USER_FRIENDLIST)
			FF1_LOAD_USERLIST("friends", usr->friends);

		if (flags & LOAD_USER_ENEMYLIST)
			FF1_LOAD_USERLIST("enemies", usr->enemies);

		if (flags & LOAD_USER_INFO)
			FF1_LOAD_STRINGLIST("info", usr->info);

/* joined rooms */
		if ((flags & LOAD_USER_ROOMS) && !strcmp(buf, "rooms")) {
			Joined *j;
			Room *r;
			MsgIndex *m;
			char zapped;
			unsigned int number, roominfo_read;
			unsigned long generation, last_read;
/*
	When loading joined rooms, we have to check whether the room still exists,
	whether the room has changed or not, and whether we're still welcome there
*/
			if (sscanf(p, "%c %u %lu %lu %u", &zapped, &number, &generation,
				&last_read, &roominfo_read) != 5)
				FF1_ERROR;

/* already joined? */
			if ((j = in_Joined(usr->rooms, number)) != NULL)
				continue;

			if ((r = find_Roombynumber_username(usr, username, number)) == NULL)	/* does the room still exist? */
				continue;
/*
	fix last_read field if too large (room was cleaned out)
*/
			m = unwind_MsgIndex(r->msgs);
			if (m == NULL)
				last_read = 0UL;
			else
				if (last_read > m->number)
					last_read = m->number;

			if (generation != r->generation) {			/* room has changed */
				generation = r->generation;
				last_read = 0UL;						/* begin reading */

				if ((r->flags & ROOM_HIDDEN)			/* if hidden or not welcome... */
					|| in_StringList(r->kicked, usr->name) != NULL
					|| ((r->flags & ROOM_INVITE_ONLY)
					&& in_StringList(r->invited, usr->name) == NULL)) {
					unload_Room(r);
					continue;
				}
				if (zapped == 'Z')
					zapped = 'J';						/* auto-unzap it */
			}
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

			add_Joined(&usr->rooms, j);
			unload_Room(r);
			continue;
		}

/*
	add site-specific stuff here
*/


/*
	Too bad, we can't trigger an error here because we get here all the time
	due to the way we do partial loading with the LOAD_USER_xxx flags

		FF1_LOAD_UNKNOWN;
*/
	}
	return 0;
}



#define LOAD_USERSTRING(x)									\
	Free(x);												\
	(x) = NULL;												\
	if (Fgets(f, buf, MAX_LINE) == NULL)					\
		goto err_load_User;									\
	cstrip_line(buf);										\
	if (*buf) {												\
		if (((x) = cstrdup(buf)) == NULL)					\
			goto err_load_User;								\
	}

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

/* passwd */
	if (flags & LOAD_USER_PASSWORD) {
		if (Fgets(f, buf, MAX_PASSPHRASE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		strncpy(usr->passwd, buf, MAX_CRYPTED_PASSWD-1);
		usr->passwd[MAX_CRYPTED_PASSWD-1] = 0;
	} else {
		LOAD_USER_SKIPLINES(1);
	}

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
	} else {
		LOAD_USER_SKIPLINES(12);
	}

/* from_ip */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_User;

	cstrip_line(buf);

/* put the former from_ip in usr->tmpbuf */
	if (usr->tmpbuf[TMP_FROM_HOST] == NULL)
		usr->tmpbuf[TMP_FROM_HOST] = cstrdup(buf);
	else
		strcpy(usr->tmpbuf[TMP_FROM_HOST], buf);

/* ipnum */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_User;
	cstrip_line(buf);

/* put former ip number in tmpbuf */
	if (usr->tmpbuf[TMP_FROM_IP] == NULL)
		usr->tmpbuf[TMP_FROM_IP] = cstrdup(buf);
	else
		strcpy(usr->tmpbuf[TMP_FROM_IP], buf);


	if (flags & LOAD_USER_DATA) {
/* birth */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->birth = (time_t)strtoul(buf, NULL, 10);

/* last_logout */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->last_logout = (time_t)strtoul(buf, NULL, 10);

/* flags */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->flags |= (unsigned int)strtoul(buf, NULL, 16);
		usr->flags &= USR_ALL;			/* reset non-existant flags */

/* logins */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->logins = strtoul(buf, NULL, 10);

/* total_time */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->total_time = strtoul(buf, NULL, 10);

/* xsent */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->xsent = strtoul(buf, NULL, 10);

/* xrecv */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->xrecv = strtoul(buf, NULL, 10);

/* esent */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->esent = strtoul(buf, NULL, 10);

/* erecv */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->erecv = strtoul(buf, NULL, 10);

/* posted */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->posted = strtoul(buf, NULL, 10);

/* read */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_User;

		cstrip_line(buf);
		usr->read = strtoul(buf, NULL, 10);

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
	} else {
		LOAD_USER_SKIPLINES(12);
	}

/* load joined rooms */
	if (flags & LOAD_USER_ROOMS) {
		Joined *j;
		Room *r;
		MsgIndex *m;
		char zapped;
		unsigned int number, roominfo_read;
		unsigned long generation, last_read;

		listdestroy_Joined(usr->rooms);
		usr->rooms = NULL;

/* setup the user's mail room */
		destroy_Room(usr->mail);
		usr->mail = load_Mail(username);

/*
	When loading joined rooms, we have to check whether the room still exists,
	whether the room has changed or not, and whether we're still welcome there
*/
		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (sscanf(buf, "%c %u %lu %lu %u", &zapped, &number, &generation,
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
			m = unwind_MsgIndex(r->msgs);
			if (m == NULL) {
				last_read = 0UL;
			} else
				if (last_read > m->number)
					last_read = m->number;

			if (generation != r->generation) {			/* room has changed */
				generation = r->generation;
				last_read = 0UL;						/* begin reading */

				if ((r->flags & ROOM_HIDDEN)			/* if hidden or not welcome... */
					|| in_StringList(r->kicked, usr->name) != NULL
					|| ((r->flags & ROOM_INVITE_ONLY)
					&& in_StringList(r->invited, usr->name) == NULL)) {
					unload_Room(r);
					continue;
				}
				if (zapped == 'Z')
					zapped = 'J';						/* auto-unzap it */
			}
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

			add_Joined(&usr->rooms, j);
			unload_Room(r);
		}
	} else {
		LOAD_USER_SKIPLIST;
	}

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
	} else {
		LOAD_USER_SKIPLINES(10);
	}
		
/* friendlist */
	if (flags & LOAD_USER_FRIENDLIST) {
		listdestroy_StringList(usr->friends);
		usr->friends = NULL;

		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				usr->friends = add_StringList(&usr->friends, sl);
		}
		usr->friends = rewind_StringList(usr->friends);
	} else {
		LOAD_USER_SKIPLIST;
	}

/* enemy list */
	if (flags & LOAD_USER_ENEMYLIST) {
		listdestroy_StringList(usr->enemies);
		usr->enemies = NULL;

		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				usr->enemies = add_StringList(&usr->enemies, sl);
		}
		usr->enemies = rewind_StringList(usr->enemies);
	} else {
		LOAD_USER_SKIPLIST;
	}

/* info */
	if (flags & LOAD_USER_INFO) {
		listdestroy_StringList(usr->info);
		usr->info = Fgetlist(f);
	} else {
		LOAD_USER_SKIPLIST;
	}

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
		usr->fsent = strtoul(buf, NULL, 10);

/* frecv */
		usr->frecv = 0UL;
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto end_load_User;

		cstrip_line(buf);
		usr->frecv = strtoul(buf, NULL, 10);
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

	sprintf(buf, "%s/%c/%s/UserData.site", PARAM_USERDIR, *username, username);
	path_strip(buf);

	if ((f = Fopen(buf)) == NULL)
		return 0;


	DO SITE-SPECIFIC STUFF HERE


	Fclose(f);
	Return 0;
*/
	return 0;
}


int save_User(User *usr) {
int ret;
char filename[MAX_PATHLEN];
File *f;

	if (usr == NULL)
		return -1;

	if (!usr->name[0])
		return 1;

	if (is_guest(usr->name))		/* don't save Guest user */
		return 0;

	sprintf(filename, "%s/%c/%s/UserData", PARAM_USERDIR, usr->name[0], usr->name);
	path_strip(filename);

	if ((f = Fcreate(filename)) == NULL) {
		Perror(usr, "Failed to save userfile");
		return -1;
	}
	usr->flags &= USR_ALL;
	ret = save_User_version1(f, usr);
	Fclose(f);
	return ret;
}

int save_User_version1(File *f, User *usr) {
int i;
Joined *j;
StringList *sl;

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

	FF1_SAVE_STR("from_ip", usr->from_ip);

	Fprintf(f, "from_ipnum=%d.%d.%d.%d",
		(int)((usr->ipnum >> 24) & 255),
		(int)((usr->ipnum >> 16) & 255),
		(int)((usr->ipnum >> 8) & 255),
		(int)(usr->ipnum & 255)
	);
	Fprintf(f, "birth=%lu", (unsigned long)usr->birth);
	Fprintf(f, "last_logout=%lu", (unsigned long)usr->last_logout);
	Fprintf(f, "flags=0x%x", usr->flags);
	Fprintf(f, "logins=%lu", usr->logins);
	Fprintf(f, "total_time=%lu", usr->total_time);

	Fprintf(f, "xsent=%lu", usr->xsent);
	Fprintf(f, "xrecv=%lu", usr->xrecv);
	Fprintf(f, "esent=%lu", usr->esent);
	Fprintf(f, "erecv=%lu", usr->erecv);
	Fprintf(f, "fsent=%lu", usr->fsent);
	Fprintf(f, "frecv=%lu", usr->frecv);
	Fprintf(f, "posted=%lu", usr->posted);
	Fprintf(f, "read=%lu", usr->read);

	Fprintf(f, "colors=%d %d %d %d %d %d %d %d %d",
		usr->colors[BACKGROUND],
		usr->colors[RED],
		usr->colors[GREEN],
		usr->colors[YELLOW],
		usr->colors[BLUE],
		usr->colors[MAGENTA],
		usr->colors[CYAN],
		usr->colors[WHITE],
		usr->colors[HOTKEY]);

	for(i = 0; i < 10; i++)
		if (usr->quick[i] != NULL)
			Fprintf(f, "quick=%d %s", i, usr->quick[i]);

	for(j = usr->rooms; j != NULL; j = j->next)
		Fprintf(f, "rooms=%c %u %lu %lu %u", (j->zapped == 0) ? 'J' : 'Z', j->number,
			j->generation, j->last_read, j->roominfo_read);

	FF1_SAVE_STRINGLIST("friends", usr->friends);
	FF1_SAVE_STRINGLIST("enemies", usr->enemies);
	FF1_SAVE_STRINGLIST("info", usr->info);

/*
	add site-specific stuff here

	it's okay to add new stuff to the same UserData file, as long as you don't use
	any keywords that I am going to use in future versions ...
*/
	return 0;
}


#define SAVE_USERSTRING(x)	Fputs(f, ((x) == NULL) ? "" : (x))

int save_User_version0(File *f, User *usr) {
Joined *j;
int i;

	Enter(save_User_version0);

	Fputs(f, usr->passwd);

	SAVE_USERSTRING(usr->real_name);
	SAVE_USERSTRING(usr->street);
	SAVE_USERSTRING(usr->zipcode);
	SAVE_USERSTRING(usr->city);
	SAVE_USERSTRING(usr->state);
	SAVE_USERSTRING(usr->country);
	SAVE_USERSTRING(usr->phone);
	SAVE_USERSTRING(usr->email);
	SAVE_USERSTRING(usr->www);
	SAVE_USERSTRING(usr->doing);
	SAVE_USERSTRING(usr->reminder);
	SAVE_USERSTRING(usr->default_anon);

	Fputs(f, usr->from_ip);
	Fprintf(f, "%d.%d.%d.%d", (int)((usr->ipnum >> 24) & 255), (int)((usr->ipnum >> 16) & 255), (int)((usr->ipnum >> 8) & 255), (int)(usr->ipnum & 255));

	Fprintf(f, "%lu", (unsigned long)usr->birth);
	Fprintf(f, "%lu", (unsigned long)usr->last_logout);
	Fprintf(f, "0x%X", usr->flags);
	Fprintf(f, "%lu", usr->logins);
	Fprintf(f, "%lu", usr->total_time);

	Fprintf(f, "%lu", usr->xsent);
	Fprintf(f, "%lu", usr->xrecv);
	Fprintf(f, "%lu", usr->esent);
	Fprintf(f, "%lu", usr->erecv);
	Fprintf(f, "%lu", usr->posted);
	Fprintf(f, "%lu", usr->read);

	Fprintf(f, "%d %d %d %d %d %d %d %d %d",
		usr->colors[BACKGROUND],
		usr->colors[RED],
		usr->colors[GREEN],
		usr->colors[YELLOW],
		usr->colors[BLUE],
		usr->colors[MAGENTA],
		usr->colors[CYAN],
		usr->colors[WHITE],
		usr->colors[HOTKEY]);

	for(j = usr->rooms; j != NULL; j = j->next)
		Fprintf(f, "%c %u %lu %lu %u", (j->zapped == 0) ? 'J' : 'Z', j->number,
			j->generation, j->last_read, j->roominfo_read);
	Fprintf(f, "");

	for(i = 0; i < 10; i++)
		SAVE_USERSTRING(usr->quick[i]);

	Fputlist(f, usr->friends);
	Fputlist(f, usr->enemies);
	Fputlist(f, usr->info);

/*	Fprintf(f, "%d", usr->time_disp);		deprecated by timezones	*/
	Fprintf(f, "0");
	Fprintf(f, "%lu", usr->fsent);
	Fprintf(f, "%lu", usr->frecv);

	Return site_save_User_version0(usr);
}

int site_save_User_version0(User *usr) {
/*
File *f;
char buf[MAX_PATHLEN];

	Enter(site_save_User);

	sprintf(buf, "%s/%c/%s/UserData.site", PARAM_USERDIR, usr->name[0], usr->name);
	path_strip(buf);

	if ((f = Fcreate(buf)) == NULL) {
		Perror(usr, "Failed to save site-specific userfile");
		Return -1;
	}

	DO SITE-SPECIFIC STUFF HERE


	Fclose(f);
	Return 0;
*/
	return 0;
}


/*
	process input and pass it on to the function that is on the callstack
*/
void process(User *usr, char c) {
	if (usr == NULL || usr->callstack == NULL || usr->callstack->ip == NULL
		|| (c = telnet_negotiations(usr, (unsigned char)c)) == (char)-1)
		return;

/* user is doing something, reset idle timer */
	usr->idle_time = rtc;

/* reset timeout timer, unless locked */
	if (!(usr->runtime_flags & RTF_LOCKED) && usr->idle_timer != NULL) {
		usr->idle_timer->sleeptime = usr->idle_timer->maxtime;
		usr->idle_timer->restart = TIMEOUT_USER;
	}
/* call routine on top of the callstack */
	this_user = usr;
	usr->callstack->ip(usr, c);				/* process input */
	this_user = NULL;
}

/* EOB */
