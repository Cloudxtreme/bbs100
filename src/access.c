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
	access.c	WJ99
*/

#include "config.h"
#include "defines.h"
#include "access.h"
#include "state.h"
#include "util.h"
#include "log.h"
#include "cstring.h"
#include "Param.h"
#include "OnlineUser.h"
#include "SU_Passwd.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>


int multi_x_access(User *usr) {
User *u;
char *err_msg = NULL;

	if (!strcmp(usr->edit_buf, usr->name))
		return 1;

	if (!is_guest(usr->edit_buf) && !user_exists(usr->edit_buf))
		err_msg = " <white>--> <red>No such user";
	else
	if ((u = is_online(usr->edit_buf)) == NULL)
		err_msg = " <white>--> <red>Is not online";
	else
	if (is_guest(usr->edit_buf))
		err_msg = " <white>--> <red>Has message reception disabled";
	else
	if (in_StringList(usr->enemies, u->name) != NULL && !(usr->runtime_flags & RTF_SYSOP))
		err_msg = " <white>--> <red>Is on your enemy list";
	else
	if (u->runtime_flags & RTF_LOCKED)
		err_msg = " <white>--> <red>Is away from the terminal";
	else
	if (in_StringList(u->enemies, usr->name) != NULL && !(usr->runtime_flags & RTF_SYSOP))
		err_msg = " <white>--> <red>Has blocked you";
	else
	if ((u->flags & USR_X_DISABLED) && !(usr->runtime_flags & RTF_SYSOP)
		&& ((u->flags & USR_BLOCK_FRIENDS) || in_StringList(u->friends, usr->name) == NULL)
		&& in_StringList(u->override, usr->name) == NULL)
		err_msg = " <white>--> <red>Has message reception disabled";

	if (err_msg != NULL) {
		Put(usr, err_msg);
		usr->edit_pos += color_strlen(err_msg);
		PUSH(usr, STATE_RECIPIENTS_ERR);
		return 0;
	}
	return 1;
}

int multi_mail_access(User *usr) {
User *u = NULL;
int allocated = 0;
char err_msg[MAX_LINE] = "";

	if (!strcmp(usr->edit_buf, "Sysop")) {
		if (su_passwd == NULL) {
			bufprintf(err_msg, sizeof(err_msg), " <white>--> <red>This BBS has no Sysops");
			goto No_multi_mail;
		}
		return 1;
	}
	if (is_guest(usr->edit_buf)) {
		bufprintf(err_msg, sizeof(err_msg), " <white>--> <red>%ss can't receive <yellow>Mail><red> here", PARAM_NAME_GUEST);
		goto No_multi_mail;
	}
	if (!user_exists(usr->edit_buf)) {
		cstrcpy(err_msg, " <white>--> <red>No such user", MAX_LINE);
		goto No_multi_mail;
	}
	if (in_StringList(usr->enemies, usr->edit_buf) != NULL && !(usr->runtime_flags & RTF_SYSOP)) {
		cstrcpy(err_msg, " <white>--> <red>Is on your enemy list", MAX_LINE);
		goto No_multi_mail;
	}
	if ((u = is_online(usr->edit_buf)) == NULL) {
		if ((u = new_User()) == NULL) {
			cstrcpy(err_msg, " <white>--> <red>Out of memory", MAX_LINE);
			goto No_multi_mail;
		}
		allocated = 1;
		cstrcpy(u->name, usr->edit_buf, MAX_NAME);
		if (load_User(u, usr->edit_buf, LOAD_USER_ENEMYLIST)) {
			cstrcpy(err_msg, " <white>--> <red>Error: Failed to load user", MAX_LINE);
			goto No_multi_mail;
		}
	}
	if (in_StringList(u->enemies, usr->name) != NULL && !(usr->runtime_flags & RTF_SYSOP))
		cstrcpy(err_msg, " <white>--> <red>Has blocked you", MAX_LINE);

No_multi_mail:
	if (allocated && u != NULL)
		destroy_User(u);

	if (*err_msg) {
		Put(usr, err_msg);
		usr->edit_pos += color_strlen(err_msg);
		PUSH(usr, STATE_RECIPIENTS_ERR);
		return 0;
	}
	return 1;
}

int multi_ping_access(User *usr) {
char *err_msg = NULL;

	if (!is_guest(usr->edit_buf) && !user_exists(usr->edit_buf)) {
		err_msg = " <white>--> <red>No such user";
		goto No_multi_ping;
	}
	if (is_online(usr->edit_buf) == NULL) {
		err_msg = " <white>--> <red>Is not online";
		goto No_multi_ping;
	}

No_multi_ping:
	if (err_msg != NULL) {
		Put(usr, err_msg);
		usr->edit_pos += color_strlen(err_msg);
		PUSH(usr, STATE_RECIPIENTS_ERR);
		return 0;
	}
	return 1;
}

/*
	access to a room
*/
int room_access(Room *r, char *name) {
	if (r == NULL)
		return ACCESS_OK;

	if (in_StringList(r->kicked, name) != NULL)
		return ACCESS_KICKED;

	if (in_StringList(r->room_aides, name) != NULL)
		return ACCESS_OK;

	if (r->flags & ROOM_INVITE_ONLY) {
		if (in_StringList(r->invited, name) != NULL)
			return ACCESS_INVITED;

		if (r->flags & ROOM_HIDDEN)
			return ACCESS_HIDDEN;

		return ACCESS_INVITE_ONLY;
	}
	if (!PARAM_HAVE_GUESSNAME && (r->flags & ROOM_HIDDEN))
		return ACCESS_HIDDEN;

	return ACCESS_OK;
}

/*
	is a room visible to the user?
*/
int room_visible(User *usr, Room *r) {
	if (usr == NULL || r == NULL)
		return 0;

	if (!(r->flags & ROOM_HIDDEN))
		return 1;

	return joined_visible(usr, r, in_Joined(usr->rooms, r->number));
}

/*
	return if a joined room is visible to the user
	(this func is convenient if you already have the joined structure anyway)
*/
int joined_visible(User *usr, Room *r, Joined *j) {
	if (usr == NULL || r == NULL)
		return 0;

	if (!(r->flags & ROOM_HIDDEN))
		return 1;

	if (in_StringList(r->room_aides, usr->name) != NULL
		|| ((r->flags & ROOM_INVITE_ONLY) && in_StringList(r->invited, usr->name) != NULL)) {
		if (j != NULL)
			j->generation = r->generation;
		return 1;
	}
	if (in_StringList(r->kicked, usr->name) != NULL || j == NULL || j->generation != r->generation) {
		if (j != NULL) {
			(void)remove_Joined(&usr->rooms, j);
			destroy_Joined(j);
		}
		return 0;
	}
	return 1;
}

/*
	room visibility with more primitive arguments
	(used when loading users)
*/
int room_visible_username(Room *r, char *username, unsigned long generation) {
	if (r == NULL || username == NULL || !*username)
		return 0;

	if (!(r->flags & ROOM_HIDDEN))
		return 1;

	if (in_StringList(r->room_aides, username) != NULL
		|| ((r->flags & ROOM_INVITE_ONLY) && in_StringList(r->invited, username) != NULL))
		return 1;

	if (in_StringList(r->kicked, username) != NULL || generation != r->generation)
		return 0;

	return 1;
}

/*
	mail_access: used by the Reply function
*/
int mail_access(User *usr, char *name) {
User *u = NULL;
int allocated = 0;

	if (is_guest(name)) {
		Print(usr, "<red>%ss can't receive <yellow>Mail><red> here\n", PARAM_NAME_GUEST);
		goto No_mail_access;
	}
	if (!user_exists(name)) {
		Print(usr, "<red>No such user<yellow> '%s'\n", name);
		goto No_mail_access;
	}
	if (in_StringList(usr->enemies, name) != NULL && !(usr->runtime_flags & RTF_SYSOP)) {
		Print(usr, "<yellow>%s<red> is on your enemy list\n", name);
		goto No_mail_access;
	}
	if ((u = is_online(name)) == NULL) {
		if ((u = new_User()) == NULL) {
			Perror(usr, "Out of memory");
			goto No_mail_access;
		}
		allocated = 1;
		cstrcpy(u->name, name, MAX_NAME);
		if (load_User(u, name, LOAD_USER_ENEMYLIST)) {
			Perror(usr, "Failed to load user");
			goto No_mail_access;
		}
	}
	if (in_StringList(u->enemies, usr->name) != NULL && !(usr->runtime_flags & RTF_SYSOP)) {
		Print(usr, "<yellow>%s<red> has blocked you\n", name);
		goto No_mail_access;
	}
	if (allocated)
		destroy_User(u);
	return 0;

No_mail_access:
	if (allocated)
		destroy_User(u);
	return 1;
}


/*
	check usr->recipients for valid xmsg recipients
	invalid entries are removed from the list
*/
void check_recipients(User *usr) {
StringList *sl, *sl_next;
User *u;

	if (usr == NULL)
		return;

	for(sl = (StringList *)usr->recipients->tail; sl != NULL; sl = sl_next) {
		sl_next = sl->next;

		if (!strcmp(sl->str, usr->name))
			continue;

		if ((u = is_online(sl->str)) == NULL) {
			Print(usr, "<red>In the meantime, <yellow>%s<red> has logged off\n", sl->str);
			goto Remove_Checked_Recipient;
		}
		if (is_guest(sl->str)) {
			Print(usr, "<red>Please leave the <yellow>%s<red> user alone\n", PARAM_NAME_GUEST);
			goto Remove_Checked_Recipient;
		}
		if (u->runtime_flags & RTF_LOCKED) {
			if (u->away != NULL && u->away[0])
				Print(usr, "<red>Sorry, but <yellow>%s<red> is locked; %s\n", sl->str, u->away);
			else
				Print(usr, "<red>Sorry, but <yellow>%s<red> has locked the terminal\n", sl->str);
			goto Remove_Checked_Recipient;
		}
		if (!(usr->runtime_flags & RTF_SYSOP)) {
			if (in_StringList(usr->enemies, u->name) != NULL) {
				Print(usr, "<yellow>%s<red> is on your enemy list\n", u->name);
				goto Remove_Checked_Recipient;
			}
			if (in_StringList(u->enemies, usr->name) != NULL) {
				Print(usr, "<red>Sorry, but <yellow>%s<red> does not wish to receive messages from you anymore\n", sl->str);
				goto Remove_Checked_Recipient;
			}
			if ((u->flags & USR_X_DISABLED) && !(usr->runtime_flags & RTF_SYSOP)
				&& ((u->flags & USR_BLOCK_FRIENDS) || in_StringList(u->friends, usr->name) == NULL)
				&& in_StringList(u->override, usr->name) == NULL) {
				Print(usr, "<red>Sorry, but <yellow>%s<red> does not wish to receive any messages right now\n", sl->str);

Remove_Checked_Recipient:
				(void)remove_StringQueue(usr->recipients, sl);
				destroy_StringList(sl);
				continue;
			}
		}
	}
}

/*
	Name is 'Guest' or 'Guest 2' or 'Guest 123' (or 'Guest 0'/'Guest 1')
*/
int is_guest(char *name) {
int j;

	if (name == NULL || !*name)
		return 0;

	j = strlen(PARAM_NAME_GUEST);
	if (!strncmp(name, PARAM_NAME_GUEST, j)) {
		char *p;

		if (!name[j])
			return 1;

		if (name[j] != ' ')
			return 0;

		p = name+j+1;
		while(*p >= '0' && *p <= '9')
			p++;

		if (!*p)
			return 1;
	}
	return 0;
}

int is_sysop(char *name) {
	if (name == NULL || !*name || get_su_passwd(name) == NULL)
		return 0;

	return 1;
}

/* EOB */
