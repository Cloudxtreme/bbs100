/*
    bbs100 3.0 WJ105
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
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>


int multi_x_access(User *usr) {
User *u;
char *err_msg = NULL;

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
	if ((u->flags & USR_X_DISABLED) && in_StringList(u->friends, usr->name) == NULL && !(usr->runtime_flags & RTF_SYSOP))
		err_msg = " <white>--> <red>Has message reception disabled";
	else
	if ((usr->flags & USR_X_DISABLED) && in_StringList(usr->friends, u->name) == NULL && !(usr->runtime_flags & RTF_SYSOP))
		err_msg = " <white>--> <red>Is not on your friend list";

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

	if (is_guest(usr->edit_buf)) {
		bufprintf(err_msg, MAX_LINE, " <white>--> <red>%ss can't receive <yellow>Mail><red> here", PARAM_NAME_GUEST);
		goto No_multi_mail;
	}
	if (!user_exists(usr->edit_buf)) {
		strcpy(err_msg, " <white>--> <red>No such user");
		goto No_multi_mail;
	}
	if (in_StringList(usr->enemies, usr->edit_buf) != NULL && !(usr->runtime_flags & RTF_SYSOP)) {
		strcpy(err_msg, " <white>--> <red>Is on your enemy list");
		goto No_multi_mail;
	}
	if ((u = is_online(usr->edit_buf)) == NULL) {
		if ((u = new_User()) == NULL) {
			strcpy(err_msg, " <white>--> <red>Out of memory");
			goto No_multi_mail;
		}
		allocated = 1;
		strcpy(u->name, usr->edit_buf);
		if (load_User(u, usr->edit_buf, LOAD_USER_ENEMYLIST)) {
			strcpy(err_msg, " <white>--> <red>Error: Failed to load user");
			goto No_multi_mail;
		}
	}
	if (in_StringList(u->enemies, usr->name) != NULL && !(usr->runtime_flags & RTF_SYSOP))
		strcpy(err_msg, " <white>--> <red>Has blocked you");

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

	if (in_StringList(r->invited, name) != NULL)
		return ACCESS_INVITED;

	if (r->flags & ROOM_INVITE_ONLY)
		return ACCESS_INVITE_ONLY;
	return ACCESS_OK;
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
		strcpy(u->name, name);
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

	for(sl = usr->recipients; sl != NULL; sl = sl_next) {
		sl_next = sl->next;

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
			if ((u->flags & USR_X_DISABLED) && (in_StringList(u->friends, usr->name) == NULL)) {
				Print(usr, "<red>Sorry, but <yellow>%s<red> does not wish to receive any messages right now\n", sl->str);
				goto Remove_Checked_Recipient;
			}
			if ((usr->flags & USR_X_DISABLED) && (in_StringList(usr->friends, u->name) == NULL)) {
				Print(usr, "<red>Sorry, but <yellow>%s<red> is not on your friend list\n", sl->str);

Remove_Checked_Recipient:
				remove_StringList(&usr->recipients, sl);
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

/* EOB */
