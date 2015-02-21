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
	Room.c	WJ99
*/

#include "config.h"
#include "debug.h"
#include "Room.h"
#include "cstring.h"
#include "CachedFile.h"
#include "util.h"
#include "cstring.h"
#include "mydirentry.h"
#include "Param.h"
#include "Timer.h"
#include "Memory.h"
#include "OnlineUser.h"
#include "FileFormat.h"
#include "log.h"
#include "bufprintf.h"
#include "access.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

Room *AllRooms = NULL, *HomeRooms = NULL, *Lobby_room = NULL;


Room *new_Room(void) {
Room *r;

	if ((r = (Room *)Malloc(sizeof(Room), TYPE_ROOM)) == NULL)
		return NULL;

	if ((r->inside = new_PQueue()) == NULL) {
		destroy_Room(r);
		return NULL;
	}
	r->max_msgs = PARAM_MAX_MESSAGES;
	return r;
}

void destroy_Room(Room *r) {
	if (r == NULL)
		return;

	Free(r->name);
	Free(r->category);

	listdestroy_StringList(r->room_aides);
	listdestroy_StringList(r->kicked);
	listdestroy_StringList(r->invited);
	destroy_StringQueue(r->chat_history);
	destroy_StringIO(r->info);
	destroy_PQueue(r->inside);

	Free(r);
}

/* load the RoomData file */
Room *load_Room(unsigned int number, int flags) {
char filename[MAX_PATHLEN];

	bufprintf(filename, sizeof(filename), "%s/%u/RoomData", PARAM_ROOMDIR, number);
	path_strip(filename);
	return load_RoomData(filename, number, flags);
}

Room *load_Mail(char *username, int flags) {
Room *r;
char filename[MAX_PATHLEN], roomname[MAX_LINE];

	if (username == NULL || !*username || !user_exists(username))
		return NULL;

	Enter(load_Mail);

	bufprintf(filename, sizeof(filename), "%s/%c/%s/MailData", PARAM_USERDIR, *username, username);
	path_strip(filename);

	if ((r = load_RoomData(filename, MAIL_ROOM, flags)) == NULL) {
		if ((r = new_Room()) == NULL) {
			Return NULL;
		}
		r->generation = (unsigned long)rtc;
	}
	r->number = MAIL_ROOM;
	possession(username, "Mail", roomname, MAX_LINE);
	Free(r->name);
	r->name = cstrdup(roomname);

	if (in_StringList(r->invited, username) == NULL)
		(void)prepend_StringList(&r->invited, new_StringList(username));

	r->flags = ROOM_SUBJECTS | ROOM_NOZAP | ROOM_INVITE_ONLY;

	bufprintf(filename, sizeof(filename), "%s/%c/%s/", PARAM_USERDIR, *username, username);
	path_strip(filename);
	room_readroomdir(r, filename, MAX_PATHLEN);
	Return r;
}

Room *load_Home(char *username, int flags) {
Room *r;
char filename[MAX_PATHLEN], roomname[MAX_LINE];

	if (username == NULL || !*username || !user_exists(username))
		return NULL;

	Enter(load_Home);

	bufprintf(filename, sizeof(filename), "%s/%c/%s/HomeData", PARAM_USERDIR, *username, username);
	path_strip(filename);

	if ((r = load_RoomData(filename, 2, flags)) == NULL) {
		if ((r = new_Room()) == NULL) {
			Return NULL;
		}
		r->generation = (unsigned long)rtc;
	}
	r->number = HOME_ROOM;
	possession(username, "Home", roomname, MAX_LINE);
	Free(r->name);
	r->name = cstrdup(roomname);

	if (in_StringList(r->room_aides, username) == NULL)
		(void)prepend_StringList(&r->room_aides, new_StringList(username));

	if (in_StringList(r->invited, username) == NULL)
		(void)prepend_StringList(&r->invited, new_StringList(username));

	r->flags = ROOM_CHATROOM | ROOM_NOZAP | ROOM_INVITE_ONLY;
	Return r;
}

Room *load_RoomData(char *filename, unsigned int number, int flags) {
Room *r;
File *f;
int (*load_func)(File *, Room *, int) = NULL;
int version;

	if (filename == NULL || !*filename || (r = new_Room()) == NULL)
		return NULL;

	if ((f = Fopen(filename)) == NULL) {
		destroy_Room(r);
		return NULL;
	}
	r->number = number;

	version = fileformat_version(f);
	switch(version) {
		case -1:
			log_err("load_RoomData(): error trying to determine file format version of %s", filename);
			load_func = NULL;
			break;

		case 0:
			Frewind(f);
			load_func = load_RoomData_version0;
			break;

		case 1:
			load_func = load_RoomData_version1;
			break;

		default:
			log_err("load_RoomData(): don't know how to load version %d of %s", version, filename);
	}
	if (load_func != NULL && !load_func(f, r, flags)) {
		Fclose(f);
		r->flags &= ROOM_ALL;
/*
	force the room name for Mail> and Home>
	so that find_abbrevRoom() won't act strangely when these names are different in the files
	for some strange reason
*/
		if (r->number == MAIL_ROOM) {
			Free(r->name);
			r->name = cstrdup("Mail");
		}
		if (r->number == HOME_ROOM) {
			Free(r->name);
			r->name = cstrdup("Home");
		}
		if (r->number == MAIL_ROOM)
			r->max_msgs = PARAM_MAX_MAIL_MSGS;
		else
			if (r->max_msgs < 1)
				r->max_msgs = PARAM_MAX_MESSAGES;

		if (PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM) && r->chat_history == NULL)
			r->chat_history = new_StringQueue();

		if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM) && r->number != HOME_ROOM) {
			r->flags &= ~ROOM_CHATROOM;
			r->flags |= ROOM_DIRTY;
		}
		(void)sort_StringList(&r->room_aides, alphasort_StringList);
		(void)sort_StringList(&r->invited, alphasort_StringList);
		(void)sort_StringList(&r->kicked, alphasort_StringList);
		return r;
	}
	destroy_Room(r);
	Fclose(f);
	return NULL;
}


int load_RoomData_version1(File *f, Room *r, int flags) {
char buf[PRINT_BUF], *p;
int ff1_continue;

	while(Fgets(f, buf, PRINT_BUF) != NULL) {
		FF1_PARSE;

		FF1_LOAD_DUP("name", r->name);

		if (flags & LOAD_ROOM_DATA) {
			FF1_LOAD_DUP("category", r->category);
			FF1_LOAD_ULONG("generation", r->generation);
			FF1_LOAD_HEX("flags", r->flags);
			FF1_LOAD_UINT("roominfo_changed", r->roominfo_changed);
			FF1_LOAD_UINT("max_msgs", r->max_msgs);
		} else {
			FF1_SKIP("category");
			FF1_SKIP("generation");
			FF1_SKIP("flags");
			FF1_SKIP("roominfo_changed");
			FF1_SKIP("max_msgs");
		}
		if (flags & LOAD_ROOM_AIDES)
			FF1_LOAD_USERLIST("room_aides", r->room_aides);
		else
			FF1_SKIP("room_aides");

		if (flags & LOAD_ROOM_INVITED)
			FF1_LOAD_USERLIST("invited", r->invited);
		else
			FF1_SKIP("invited");

		if (flags & LOAD_ROOM_KICKED)
			FF1_LOAD_USERLIST("kicked", r->kicked);
		else
			FF1_SKIP("kicked");

		if (flags & LOAD_ROOM_CHAT_HISTORY) {
			if (!strcmp(buf, "chat_history") && r->chat_history == NULL)
				r->chat_history = new_StringQueue();

			FF1_LOAD_STRINGQUEUE("chat_history", r->chat_history);
		} else
			FF1_SKIP("chat_history");

		if (PARAM_HAVE_RESIDENT_INFO || (flags & LOAD_ROOM_INFO))
			FF1_LOAD_STRINGIO("info", r->info);
		else
			FF1_SKIP("info");

		FF1_LOAD_UNKNOWN;
	}
	return 0;
}

#define LOAD_ROOM_SKIPLINES(x)						\
	do {											\
		for(i = 0; i < (x); i++) {					\
			if (Fgets(f, buf, MAX_LINE) == NULL)	\
				goto err_load_room;					\
		}											\
	} while(0)

#define LOAD_ROOM_SKIPLIST							\
	do {											\
		while(Fgets(f, buf, MAX_LINE) != NULL) {	\
			cstrip_line(buf);						\
			if (!*buf)								\
				break;								\
		}											\
	} while(0)


int load_RoomData_version0(File *f, Room *r, int flags) {
char buf[MAX_LONGLINE];
StringList *sl;
int i;

/* name */
	if (Fgets(f, buf, MAX_LINE) == NULL)
		goto err_load_room;

	cstrip_line(buf);
	Free(r->name);
	r->name = cstrdup(buf);

	if (flags & LOAD_ROOM_DATA) {
/* generation/creation date */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_room;
		cstrip_line(buf);
		r->generation = cstrtoul(buf, 10);

/* flags */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_room;
		cstrip_line(buf);
		r->flags = (unsigned int)cstrtoul(buf, 16);
		r->flags &= ROOM_ALL;		/* reset non-existant flags */

/* roominfo_changed */
		if (Fgets(f, buf, MAX_LINE) == NULL)
			goto err_load_room;
		cstrip_line(buf);
		r->roominfo_changed = (unsigned int)cstrtoul(buf, 10);
	} else
		LOAD_ROOM_SKIPLINES(3);

/* info */
	destroy_StringIO(r->info);
	r->info = NULL;

	if (PARAM_HAVE_RESIDENT_INFO || (flags & LOAD_ROOM_INFO)) {
		while(Fgets(f, buf, MAX_LINE) != NULL) {
			if (!*buf)
				break;

			if (r->info == NULL && (r->info = new_StringIO()) == NULL)
				continue;

			put_StringIO(r->info, buf);
			write_StringIO(r->info, "\n", 1);
		}
	} else
		LOAD_ROOM_SKIPLIST;

/* room aides */
	listdestroy_StringList(r->room_aides);
	r->room_aides = NULL;

	if (flags & LOAD_ROOM_AIDES) {
		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				r->room_aides = add_StringList(&r->room_aides, sl);
		}
		r->room_aides = rewind_StringList(r->room_aides);
	} else
		LOAD_ROOM_SKIPLIST;

/* invited */
	listdestroy_StringList(r->invited);
	r->invited = NULL;

	if (flags & LOAD_ROOM_INVITED) {
		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				r->invited = add_StringList(&r->invited, sl);
		}
		r->invited = rewind_StringList(r->invited);
	} else
		LOAD_ROOM_SKIPLIST;

/* kicked */
	listdestroy_StringList(r->kicked);
	r->kicked = NULL;

	if (flags & LOAD_ROOM_KICKED) {
		while(Fgets(f, buf, MAX_LINE) != NULL) {
			cstrip_line(buf);
			if (!*buf)
				break;

			if (user_exists(buf) && (sl = new_StringList(buf)) != NULL)
				r->kicked = add_StringList(&r->kicked, sl);
		}
		r->kicked = rewind_StringList(r->kicked);
	} else
		LOAD_ROOM_SKIPLIST;

/* chat history */
	if ((flags & LOAD_ROOM_CHAT_HISTORY) && (r->flags & ROOM_CHATROOM)) {
		if (r->chat_history == NULL && (r->chat_history = new_StringQueue()) == NULL)
			LOAD_ROOM_SKIPLIST;
		else {
			r->chat_history->tail = (ListType *)Fgetlist(f);
			r->chat_history->head = (ListType *)unwind_StringList((StringList *)r->chat_history->tail);
			r->chat_history->count = count_List(r->chat_history->tail);
		}
	} else
		LOAD_ROOM_SKIPLIST;

	return 0;

err_load_room:
	return -1;
}


int load_roominfo(Room *r, char *username) {
Room *tmp;

	if (r == NULL)
		return -1;

	if (r->number == MAIL_ROOM)				/* don't bother */
		return 0;

	if (r->info != NULL)					/* already resident */
		return 0;

	tmp = new_Room();

	if (r->number == MAIL_ROOM)
		tmp = load_Mail(username, LOAD_ROOM_INFO);
	else
		if (r->number == HOME_ROOM)
			tmp = load_Home(username, LOAD_ROOM_INFO);
		else
			tmp = load_Room(r->number, LOAD_ROOM_INFO);

	if (tmp == NULL)
		return -1;

	destroy_StringIO(r->info);
	r->info = tmp->info;
	tmp->info = NULL;

	destroy_Room(tmp);
	return 0;
}


/* save the RoomData file */

int save_Room(Room *r) {
char filename[MAX_PATHLEN];
File *f;

	if (r == NULL)
		return -1;

	if (!(r->flags & ROOM_DIRTY)) {
/*
	if the room is not dirty, don't save it
	this is a performance improvement for demand loaded rooms; Home> and Mail>
*/
		if (!PARAM_HAVE_RESIDENT_INFO) {
			destroy_StringIO(r->info);
			r->info = NULL;
		}
		return 0;
	}
	r->flags &= ROOM_ALL;			/* this automatically clears ROOM_DIRTY */

	if (r->number == MAIL_ROOM || r->number == HOME_ROOM) {
		char name[MAX_LINE], *p;

		cstrcpy(name, r->name, MAX_LINE);
		if ((p = cstrchr(name, '\'')) == NULL)
			return -1;

		*p = 0;

		if (r->number == MAIL_ROOM)
			bufprintf(filename, sizeof(filename), "%s/%c/%s/MailData", PARAM_USERDIR, *name, name);
		else
			if (r->number == HOME_ROOM)
				bufprintf(filename, sizeof(filename), "%s/%c/%s/HomeData", PARAM_USERDIR, *name, name);

		load_roominfo(r, name);
	} else {
		bufprintf(filename, sizeof(filename), "%s/%u/RoomData", PARAM_ROOMDIR, r->number);
		load_roominfo(r, NULL);
	}
	path_strip(filename);

	if ((f = Fcreate(filename)) == NULL)
		return -1;

	if (save_Room_version1(f, r)) {
		Fcancel(f);
		return -1;
	}
	if (Fclose(f))
		return -1;

	if (!PARAM_HAVE_RESIDENT_INFO) {
		destroy_StringIO(r->info);
		r->info = NULL;
	}
	return 0;
}

int save_Room_version1(File *f, Room *r) {
StringList *sl;
char buf[PRINT_BUF];

	FF1_SAVE_VERSION;
	FF1_SAVE_STR("name", r->name);
	FF1_SAVE_STR("category", r->category);

	Fprintf(f, "generation=%lu", r->generation);
	Fprintf(f, "flags=0x%x", r->flags);
	Fprintf(f, "roominfo_changed=%u", r->roominfo_changed);
	Fprintf(f, "max_msgs=%u", r->max_msgs);

	FF1_SAVE_STRINGLIST("room_aides", r->room_aides);
	FF1_SAVE_STRINGLIST("invited", r->invited);
	FF1_SAVE_STRINGLIST("kicked", r->kicked);
	FF1_SAVE_STRINGQUEUE("chat_history", r->chat_history);
	FF1_SAVE_STRINGIO("info", r->info);

	return 0;
}

/*
	returns first new message in the room (usr->curr_msg should be set to this)
*/
long newMsgs(Room *r, long num) {
	if (r == NULL || r->head_msg <= 0L)
		return -1L;

	if (num < r->tail_msg)
		return r->tail_msg;

	num++;
	if (num > r->head_msg)
		return -1L;

	return num;
}

/*
	expire oldest message
*/
void expire_msg(Room *r, User *usr) {
char filename[MAX_PATHLEN];

	if (r == NULL)
		return;

	if (r->number == MAIL_ROOM) {
		if (usr == NULL)
			filename[0] = 0;
		else
			bufprintf(filename, sizeof(filename), "%s/%c/%s/%ld", PARAM_USERDIR, usr->name[0], usr->name, r->tail_msg);
	} else
		bufprintf(filename, sizeof(filename), "%s/%u/%ld", PARAM_ROOMDIR, r->number, r->tail_msg);

	if (r->head_msg - r->tail_msg > r->max_msgs) {
		if (filename[0] && unlink_file(filename) == -1)
			log_err("expire_msg(): failed to delete file %s", filename);

		r->tail_msg++;
		r->flags |= ROOM_DIRTY;
	}
}

/*
	post a new message and expire the oldest one
*/
void newMsg(Room *r, User *usr) {
	if (r == NULL)
		return;

	expire_msg(r, usr);
	r->head_msg++;
	r->flags |= ROOM_DIRTY;

	if (r->tail_msg <= 0L)				/* room was empty? */
		r->tail_msg = r->head_msg;
}

/*
	size the maximum amount of messages in a Room
*/
void resize_Room(Room *r, long newsize, User *usr) {
	if (r == NULL || newsize <= 0L || newsize == r->max_msgs)
		return;

	r->max_msgs = newsize;
	while(r->head_msg - r->tail_msg > r->max_msgs)
		expire_msg(r, usr);

	r->flags |= ROOM_DIRTY;
}

/*
	print the room name into a buffer
	user has choice to have room numbers in the name
*/
char *room_name(User *usr, Room *r, char *buf, int buflen) {
	if (buf == NULL)
		return NULL;

	*buf = 0;
	if (usr == NULL || r == NULL)
		return buf;

	if (usr->flags & USR_ROOMNUMBERS)
		bufprintf(buf, buflen, "<white>%u <yellow>%s>", r->number, r->name);
	else
		bufprintf(buf, buflen, "<yellow>%s>", r->name);
	return buf;
}

void room_readdir(Room *r) {
char dirname[MAX_PATHLEN];

	if (r == NULL || (r->flags & ROOM_CHATROOM))
		return;

	bufprintf(dirname, sizeof(dirname), "%s/%u/", PARAM_ROOMDIR, r->number);
	path_strip(dirname);
	room_readroomdir(r, dirname, MAX_PATHLEN);
}

void room_readmaildir(Room *r, char *username) {
char buf[MAX_PATHLEN];

	if (r == NULL || username == NULL || !*username)
		return;

	bufprintf(buf, sizeof(buf), "%s/%c/%s/", PARAM_USERDIR, *username, username);
	path_strip(buf);
	room_readroomdir(r, buf, MAX_PATHLEN);
}

void room_readroomdir(Room *r, char *dirname, int buflen) {
DIR *dirp;
struct dirent *direntp;
long num;

	if (r == NULL || dirname == NULL || buflen <= 0)
		return;

	r->tail_msg = r->head_msg = 0L;

	if ((dirp = opendir(dirname)) == NULL)
		return;

	while((direntp = readdir(dirp)) != NULL) {
		if (is_numeric(direntp->d_name)) {			/* messages have numeric filenames */
			num = (long)cstrtoul(direntp->d_name, 10);

			if (!r->tail_msg)
				r->tail_msg = num;

			if (num < r->tail_msg)
				r->tail_msg = num;

			if (num > r->head_msg)
				r->head_msg = num;
		}
	}
	closedir(dirp);
}

long room_top(Room *r) {
	return (r == NULL) ? 0L : r->head_msg;
}

/*
	find a room by name
*/
Room *find_Room(User *usr, char *name) {
Room *r;

	if (name == NULL || !*name || usr == NULL)
		return NULL;

	if (*name >= '0' && *name <= '9')
		return find_Roombynumber(usr, (unsigned int)atoi(name));
	else {
		char *p, *quote;

		if (PARAM_HAVE_MAILROOM && !strcmp(name, "Mail"))
			return usr->mail;

		if (PARAM_HAVE_HOMEROOM && !strcmp(name, "Home"))
			return find_Home(usr->name);

		if ((p = cstrchr(name, '\'')) != NULL) {
			quote = p;
			*quote = 0;
			p++;
			if (*p == 's')
				p++;
			if (*p == ' ') {
				p++;
				if (PARAM_HAVE_MAILROOM && !strcmp(p, "Mail")) {
					User *u;

					if ((u = is_online(name)) == NULL) {
						int load_room_flags = LOAD_ROOM_ALL;

						if (!PARAM_HAVE_RESIDENT_INFO)
							load_room_flags &= ~LOAD_ROOM_INFO;

						r = load_Mail(name, load_room_flags);
						*quote = '\'';
						(void)prepend_Room(&HomeRooms, r);
						return r;
					}
					*quote = '\'';
					return u->mail;
				}
				if (PARAM_HAVE_HOMEROOM && !strcmp(p, "Home")) {
					r = find_Home(name);
					*quote = '\'';
					return r;
				}
			}
			*quote = '\'';
		}

/* find 'normal' room */

		for(r = AllRooms; r != NULL; r = r->next) {
			if (!strcmp(r->name, name)) {
				if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
					return find_Roombynumber(usr, r->number);

/* guess-name rooms can be found by typing the correct name */
				if (!PARAM_HAVE_GUESSNAME && !room_visible(usr, r))
					return NULL;

				if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
					return NULL;
				return r;
			}
		}
	}
	return NULL;
}

/*
	find_Room() with abbreviated name
*/
Room *find_abbrevRoom(User *usr, char *name) {
Room *r;
char room_name[MAX_LINE+2], match[MAX_LINE+2];

	if (name == NULL || !*name || usr == NULL)
		return NULL;

	if ((r = find_Room(usr, name)) == NULL) {
		int l;

		l = strlen(name);
		for(r = AllRooms; r != NULL; r = r->next) {
			if (!strncmp(r->name, name, l)) {
				if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
					return find_Roombynumber(usr, r->number);

				if (!room_visible(usr, r))
					continue;

				if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
					continue;
				return r;
			}
		}
/*
	didn't find any room, try a substring that matches whole words
*/
		bufprintf(match, sizeof(match), " %s ", name);
		for(r = AllRooms; r != NULL; r = r->next) {
			bufprintf(room_name, sizeof(room_name), " %s ", r->name);
			if (cstrstr(room_name, match) != NULL) {
				if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
					return find_Roombynumber(usr, r->number);

				if (!room_visible(usr, r))
					continue;

				if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
					continue;
				return r;
			}
		}
/*
	didn't find any room, try a substring that matches half words
*/
		for(r = AllRooms; r != NULL; r = r->next) {
			if (cstrstr(r->name, name) != NULL) {
				if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
					return find_Roombynumber(usr, r->number);

				if (!room_visible(usr, r))
					continue;

				if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
					continue;
				return r;
			}
		}
		return NULL;
	}
	return r;
}

Room *find_Roombynumber(User *usr, unsigned int u) {
Room *r;

	switch(u) {
		case 0:
			return Lobby_room;

		case 1:
			if (!PARAM_HAVE_MAILROOM)
				break;
			return usr->mail;

		case 2:
			if (!PARAM_HAVE_HOMEROOM)
				break;

			if ((r = find_Home(usr->name)) != NULL)
				return r;
			break;

		default:
			for(r = AllRooms; r != NULL; r = r->next)
				if (r->number == u) {
					if (!room_visible(usr, r))		/* can't find hidden rooms by number */
						break;

					if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
						break;

					return r;
				}
	}
	return NULL;
}

/*
	... only used by load_User() to see if the room still exists and still
	is accessible
*/
Room *find_Roombynumber_username(User *usr, char *username, unsigned int u) {
Room *r;

	switch(u) {
		case 0:
			return Lobby_room;

		case 1:
			if (!PARAM_HAVE_MAILROOM)
				break;
			return usr->mail;

		case 2:
			if (!PARAM_HAVE_HOMEROOM)
				break;

			if ((r = find_Home(username)) != NULL) {
				if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
					return NULL;
				return r;
			}
			break;

		default:
			for(r = AllRooms; r != NULL; r = r->next)
				if (r->number == u) {
/*
					if (!room_visible(usr, r))
						break;
*/
					if (!PARAM_HAVE_CHATROOMS && (r->flags & ROOM_CHATROOM))
						break;

					return r;
				}
	}
	return NULL;
}

Room *find_Home(char *username) {
Room *r;
char buf[MAX_LINE];
int load_room_flags;

	if (!PARAM_HAVE_HOMEROOM || username == NULL || !*username)
		return NULL;

	possession(username, "Home", buf, MAX_LINE);

	for(r = HomeRooms; r != NULL; r = r->next)
		if (!strcmp(r->name, buf))
			return r;

	load_room_flags = LOAD_ROOM_ALL;
	if (!PARAM_HAVE_RESIDENT_INFO)
		load_room_flags &= ~LOAD_ROOM_INFO;

	if ((r = load_Home(username, load_room_flags)) == NULL)
		return NULL;

	(void)prepend_Room(&HomeRooms, r);
	return r;
}

/* purely check if it exists at all */

int room_exists(char *name) {
char *p;

	if ((p = cstrchr(name, '\'')) != NULL) {
		if ((PARAM_HAVE_HOMEROOM && (!strcmp(p, "'s Home") || !strcmp(p, "' Home")))
			|| (PARAM_HAVE_MAILROOM && (!strcmp(p, "'s Mail") || !strcmp(p, "' Mail")))) {
			*p = 0;
			if (user_exists(name)) {
				*p = '\'';
				return 1;
			}
			*p = '\'';
			return 0;
		}
	}
	if (*name >= '0' && *name <= '9')
		return roomnumber_exists((unsigned int)atoi(name));
	else {
		Room *r;

		for(r = AllRooms; r != NULL; r = r->next)
			if (!strcmp(r->name, name)) {
				if (r->number == MAIL_ROOM && !PARAM_HAVE_MAILROOM)
					return 0;

				if (r->number == HOME_ROOM && !PARAM_HAVE_HOMEROOM)
					return 0;

				if ((r->flags & ROOM_CHATROOM) && !PARAM_HAVE_CHATROOMS)
					return 0;

				return 1;
			}
	}
	return 0;
}

int roomnumber_exists(unsigned int u) {
Room *r;

	if (u == LOBBY_ROOM)
		return 1;

	if (u == MAIL_ROOM) {
		if (PARAM_HAVE_MAILROOM)
			return 1;
		return 0;
	}
	if (u == HOME_ROOM) {
		if (PARAM_HAVE_HOMEROOM)
			return 1;
		return 0;
	}
	for(r = AllRooms; r != NULL; r = r->next)
		if (r->number == u) {
			if ((r->flags & ROOM_CHATROOM) && !PARAM_HAVE_CHATROOMS)
				return 0;

			return 1;
		}
	return 0;
}

/*
	unload an 'on demand loaded' room

	Sometimes, the find_Room() functions load a room on demand
	This is cool, but there are cases in which the loaded room
	needs to be destroyed again -- this is done by unload_Room()
*/
void unload_Room(Room *r) {
	if (r == NULL)
		return;

	if (r->number == HOME_ROOM && count_Queue(r->inside) <= 0) {	/* demand loaded Home room */
		(void)remove_Room(&HomeRooms, r);
		if (save_Room(r))
			log_err("unload_Room(): failed to save room #%u %s", r->number, r->name);

		destroy_Room(r);
		return;
	}
	if (r->number == MAIL_ROOM) {			/* demand loaded Mail> room */
		Room *h;
/*
	Note: mail rooms are usually stored in the user as usr->mail
	However, if the user was not online, it was put on the HomeRooms list
	so if we can find it there, it should be unloaded
	if we can't find it there, the room should not be unloaded

	This procedure is more efficient than scanning all users for "(usr->mail == r)"
	because the HomeRooms list is usually very short or empty
*/
		for(h = HomeRooms; h != NULL; h = h->next) {
			if (h == r) {
				(void)remove_Room(&HomeRooms, r);
				if (save_Room(r))
					log_err("unload_Room(): failed to save HomeRoom #%u %s", r->number, r->name);
				destroy_Room(r);
				return;
			}
/*
	do some house-cleaning
	Note: due the 'return' statement just above, we may not complete the
	house-cleaning work...
*/
			if (count_Queue(h->inside) <= 0) {
				(void)remove_Room(&HomeRooms, r);
				if (save_Room(r))
					log_err("unload_Room(): failed to save room #%u %s", r->number, r->name);

				destroy_Room(r);
			}
		}
	}
}

int room_sort_by_category(void *v1, void *v2) {
int i;
Room *r1, *r2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	r1 = *(Room **)v1;
	r2 = *(Room **)v2;

	if (r1 == NULL || r2 == NULL)
		return 0;

	if (r1->category == NULL) {
		if (r2->category == NULL) {
			if (r1->number < r2->number)
				return -1;

			if (r1->number > r2->number)
				return 1;

			return 0;
		}
		return -1;
	}
	if (r2->category == NULL)
		return 1;

	i = strcmp(r1->category, r2->category);
	if (!i) {
		if (r1->number < r2->number)
			return -1;

		if (r1->number > r2->number)
			return 1;

		return 0;
	}
	return i;
}

int room_sort_by_number(void *v1, void *v2) {
Room *r1, *r2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	r1 = *(Room **)v1;
	r2 = *(Room **)v2;

	if (r1 == NULL || r2 == NULL)
		return 0;

	if (r1->number < r2->number)
		return -1;

	if (r1->number > r2->number)
		return 1;

	return 0;
}

/*
	load all rooms definitions at startup
	Note that room #1 and room #2 are 'shadow' rooms for Mail> and Home>
	the BBS cannot work without room #0, #1, and #2
*/
int init_Room(void) {
char buf[MAX_PATHLEN], *bufp;
DIR *dirp;
struct dirent *direntp;
struct stat statbuf;
Room *newroom;
unsigned int u;
int load_room_flags, len;

	printf("\n");

	listdestroy_Room(AllRooms);
	AllRooms = Lobby_room = NULL;

	load_room_flags = LOAD_ROOM_ALL;
	if (!PARAM_HAVE_RESIDENT_INFO)
		load_room_flags &= ~LOAD_ROOM_INFO;

	bufprintf(buf, sizeof(buf), "%s/", PARAM_ROOMDIR);
	path_strip(buf);
	len = strlen(buf);
	bufp = buf+len;

	if ((dirp = opendir(buf)) == NULL)
		return -1;

	while((direntp = readdir(dirp)) != NULL) {
		if (is_numeric(direntp->d_name)) {		/* only do numeric directories */
			cstrcpy(bufp, direntp->d_name, MAX_PATHLEN - len);
			if (stat(buf, &statbuf))
				continue;

			if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
				u = (unsigned int)cstrtoul(bufp, 10);

				printf("loading room %3u ... ", u);
				fflush(stdout);

				if ((newroom = load_Room(u, load_room_flags)) != NULL) {
					(void)prepend_Room(&AllRooms, newroom);
					room_readdir(newroom);
					printf("%s>\n", newroom->name);
				} else {
					printf("FAILED!\n");
					closedir(dirp);

					listdestroy_Room(AllRooms);
					AllRooms = Lobby_room = NULL;
					return -1;
				}
			}
		}
	}
	closedir(dirp);

	if (PARAM_HAVE_CATEGORY)
		(void)sort_Room(&AllRooms, room_sort_by_category);
	else
		(void)sort_Room(&AllRooms, room_sort_by_number);

/*
	find the Lobby>
	It should be first, but you never know...
*/
	if (Lobby_room == NULL) {
		Room *rm;

		for(rm = AllRooms; rm != NULL; rm = rm->next) {
			if (!rm->number) {
				Lobby_room = rm;
				break;
			}
		}
	}
	if (Lobby_room == NULL) {
		printf("Failed to find the Lobby> (room 0)\n"
			"Please create room definition file '%s/0/RoomData'\n", PARAM_ROOMDIR);

		listdestroy_Room(AllRooms);
		AllRooms = NULL;
		return -1;
	}
	printf("\n");
	return 0;
}

/* EOB */
