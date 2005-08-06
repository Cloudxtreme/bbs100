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
	state_friendlist.c	WJ99
*/

#include "config.h"
#include "defines.h"
#include "state_friendlist.h"
#include "state.h"
#include "state_msg.h"
#include "edit.h"
#include "util.h"
#include "log.h"
#include "debug.h"
#include "cstring.h"
#include "Param.h"

#include <stdio.h>
#include <stdlib.h>

void state_friendlist_prompt(User *usr, char c) {
StringList *sl;

	if (usr == NULL)
		return;

	Enter(state_friendlist_prompt);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			if (usr->friends == NULL)
				Put(usr, "<cyan>\nYour friends list is empty\n");
			else {
				Put(usr, "\n");
				print_columns(usr, usr->friends, 0);
			}
			Put(usr, "<magenta>\n"
				"<hotkey>Add friend\n");

			if (usr->friends != NULL)
				Put(usr, "<hotkey>Remove friend\n");

			Put(usr, "Switch to <hotkey>enemy list\n");
			read_menu(usr);
			Return;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_ESC:
		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Exit\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'A':
		case 'a':
		case '+':
		case '=':
			Put(usr, "Add friend\n");
			if (list_Count(usr->friends) >= PARAM_MAX_FRIEND) {
				Print(usr, "<red>You already have %d friends defined\n", PARAM_MAX_FRIEND);
				break;
			}
/* remove myself from the recipient list */
			if ((sl = in_StringList(usr->recipients, usr->name)) != NULL) {
				remove_StringList(&usr->recipients, sl);
				destroy_StringList(sl);
			}
			enter_name(usr, STATE_ADD_FRIEND);
			Return;

		case 'd':
		case 'D':
		case 'R':
		case 'r':
		case '-':
		case '_':
			if (usr->friends == NULL)
				break;

			Put(usr, "Remove friend\n");

/* remove myself from the recipient list */

			if ((sl = in_StringList(usr->recipients, usr->name)) != NULL) {
				remove_StringList(&usr->recipients, sl);
				destroy_StringList(sl);
			}
			enter_name(usr, STATE_REMOVE_FRIEND);
			Return;

		case '<':
		case 'e':
		case 'E':
			Put(usr, "Enemies\n");
			JMP(usr, STATE_ENEMYLIST_PROMPT);
			Return;
	}
	Print(usr, "<yellow>\n[Config] Friends%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_add_friend(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_add_friend);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *new_friend;

		if (!usr->edit_buf[0]) {
			if (usr->recipients == NULL) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, usr->recipients->str, MAX_LINE);
		}
		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");
			RET(usr);
			Return;
		}
		if (!strcmp(usr->name, usr->edit_buf)) {
			Put(usr, "<green>Heh, your best friend is <yellow>YOU\n");
			RET(usr);
			Return;
		}
		if (in_StringList(usr->friends, usr->edit_buf) != NULL) {
			Print(usr, "<yellow>%s<red> already is on your friend list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_friend = in_StringList(usr->enemies, usr->edit_buf)) != NULL) {
			remove_StringList(&usr->enemies, new_friend);
			add_StringList(&usr->friends, new_friend);
			usr->friends = sort_StringList(usr->friends, alphasort_StringList);
			Print(usr, "<yellow>%s<green> moved from your enemy to friend list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_friend = new_StringList(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		add_StringList(&(usr->friends), new_friend);
		usr->friends = sort_StringList(usr->friends, alphasort_StringList);
		RET(usr);
	}
	Return;
}

void state_remove_friend(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_remove_friend);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *rm_friend;

		if (!usr->edit_buf[0]) {
			if (usr->recipients == NULL) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, usr->recipients->str, MAX_LINE);
		}
		if (!strcmp(usr->name, usr->edit_buf))
			Put(usr, "<green>Stopped being friends with yourself? How sad and lonely...\n");
		else {
			if ((rm_friend = in_StringList(usr->friends, usr->edit_buf)) != NULL) {
				remove_StringList(&usr->friends, rm_friend);
				destroy_StringList(rm_friend);
			} else
				Put(usr, "<red>There is no such person on your friend list\n");
		}
		RET(usr);
	}
	Return;
}


void state_enemylist_prompt(User *usr, char c) {
StringList *sl;

	if (usr == NULL)
		return;

	Enter(state_enemylist_prompt);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			
			buffer_text(usr);

			if (usr->enemies == NULL)
				Put(usr, "<cyan>\nYour enemy list is empty\n");
			else {
				Put(usr, "\n");
				print_columns(usr, usr->enemies, 0);
			}
			Put(usr, "<magenta>\n"
				"<hotkey>Add enemy\n");

			if (usr->enemies != NULL)
				Put(usr, "<hotkey>Remove enemy\n");

			Put(usr, "Switch to <hotkey>friend list\n");
			read_menu(usr);
			Return;

		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_ESC:
		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Exit\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'A':
		case 'a':
		case '+':
		case '=':
			Put(usr, "Add enemy\n");
			if (list_Count(usr->enemies) >= PARAM_MAX_ENEMY) {
				Print(usr, "<red>You already have %d enemies defined\n", PARAM_MAX_ENEMY);
				break;
			}

/* remove myself from the recipient list */

			if ((sl = in_StringList(usr->recipients, usr->name)) != NULL) {
				remove_StringList(&usr->recipients, sl);
				destroy_StringList(sl);
			}
			enter_name(usr, STATE_ADD_ENEMY);
			Return;

		case 'R':
		case 'r':
		case 'd':
		case 'D':
		case '-':
		case '_':
			if (usr->enemies == NULL)
				break;

			Put(usr, "Remove enemy\n");

/* remove myself from the recipient list */

			if ((sl = in_StringList(usr->recipients, usr->name)) != NULL) {
				remove_StringList(&usr->recipients, sl);
				destroy_StringList(sl);
			}
			enter_name(usr, STATE_REMOVE_ENEMY);
			Return;

		case '>':
		case 'f':
		case 'F':
			Put(usr, "Friends\n");
			JMP(usr, STATE_FRIENDLIST_PROMPT);
			Return;
	}
	Print(usr, "<yellow>\n[Config] Enemies%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_add_enemy(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_add_enemy);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *new_enemy;

		if (!usr->edit_buf[0]) {
			if (usr->recipients == NULL) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, usr->recipients->str, MAX_LINE);
		}
		if (!strcmp(usr->name, usr->edit_buf)) {
			Put(usr, "<red>So, your worst enemy is <white>YOU\n");
			RET(usr);
			Return;
		}
		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");
			RET(usr);
			Return;
		}
		if (in_StringList(usr->enemies, usr->edit_buf) != NULL) {
			Print(usr, "<yellow>%s<red> already is on your enemy list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_enemy = in_StringList(usr->friends, usr->edit_buf)) != NULL) {
			remove_StringList(&usr->friends, new_enemy);
			add_StringList(&usr->enemies, new_enemy);
			usr->enemies = sort_StringList(usr->enemies, alphasort_StringList);
			Print(usr, "<yellow>%s<green> moved from your friend to enemy list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_enemy = new_StringList(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		add_StringList(&(usr->enemies), new_enemy);
		usr->enemies = sort_StringList(usr->enemies, alphasort_StringList);
		RET(usr);
	}
	Return;
}


void state_remove_enemy(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_remove_enemy);

/*
	edit_tabname() is used, but Tab will only work for enemies if
	USR_SHOW_ENEMIES is set
*/
	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *rm_enemy;

		if (!usr->edit_buf[0]) {
			if (usr->recipients == NULL) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, usr->recipients->str, MAX_LINE);
		}
		if (!strcmp(usr->name, usr->edit_buf))
			Put(usr, "<green>Time to come to peace with yourself?\n");
		else {
			if ((rm_enemy = in_StringList(usr->enemies, usr->edit_buf)) != NULL) {
				remove_StringList(&usr->enemies, rm_enemy);
				destroy_StringList(rm_enemy);
			} else
				Put(usr, "<red>There is no such person on your enemy list\n");
		}
		RET(usr);
	}
	Return;
}

/* EOB */
