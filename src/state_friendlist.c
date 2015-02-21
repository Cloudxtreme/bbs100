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
#include "access.h"

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
			Print(usr, "<magenta>\n"
				"<hotkey>Add friend%s", (usr->friends == NULL) ? "\n" : "                   <hotkey>Remove friend\n");

			Print(usr, "Switch to <hotkey>enemy list%s\n",
				((usr->flags & USR_X_DISABLED) && !is_guest(usr->name)) ? "         Switch to <hotkey>override list" : "");
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
			if (count_List(usr->friends) >= PARAM_MAX_FRIEND) {
				Print(usr, "<red>You already have %d friends defined\n", PARAM_MAX_FRIEND);
				break;
			}
/* remove myself from the recipient list */
			if ((sl = in_StringQueue(usr->recipients, usr->name)) != NULL) {
				(void)remove_StringQueue(usr->recipients, sl);
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

			if ((sl = in_StringQueue(usr->recipients, usr->name)) != NULL) {
				(void)remove_StringQueue(usr->recipients, sl);
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

		case 'o':
		case 'O':
			if ((usr->flags & USR_X_DISABLED) && !is_guest(usr->name)) {
				Put(usr, "Override\n");
				JMP(usr, STATE_OVERRIDE_MENU);
				Return;
			}
			break;
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
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->head)->str, MAX_LINE);
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
			(void)remove_StringList(&usr->enemies, new_friend);
			(void)prepend_StringList(&usr->friends, new_friend);
			(void)sort_StringList(&usr->friends, alphasort_StringList);
			Print(usr, "<yellow>%s<green> moved from your enemy to friend list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_friend = new_StringList(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		(void)prepend_StringList(&usr->friends, new_friend);
		(void)sort_StringList(&usr->friends, alphasort_StringList);
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
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->head)->str, MAX_LINE);
		}
		if (!strcmp(usr->name, usr->edit_buf))
			Put(usr, "<green>Stopped being friends with yourself? How sad and lonely...\n");
		else {
			if ((rm_friend = in_StringList(usr->friends, usr->edit_buf)) != NULL) {
				(void)remove_StringList(&usr->friends, rm_friend);
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
			Print(usr, "<magenta>\n"
				"<hotkey>Add enemy%s", (usr->enemies == NULL) ? "\n" : "                    <hotkey>Remove enemy\n");

			Print(usr, "Switch to <hotkey>friend list%s\n",
				(usr->flags & USR_X_DISABLED) ? "        Switch to <hotkey>override list" : "");
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
			if (count_List(usr->enemies) >= PARAM_MAX_ENEMY) {
				Print(usr, "<red>You already have %d enemies defined\n", PARAM_MAX_ENEMY);
				break;
			}

/* remove myself from the recipient list */

			if ((sl = in_StringQueue(usr->recipients, usr->name)) != NULL) {
				(void)remove_StringQueue(usr->recipients, sl);
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

			if ((sl = in_StringQueue(usr->recipients, usr->name)) != NULL) {
				(void)remove_StringQueue(usr->recipients, sl);
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

		case 'o':
		case 'O':
			if ((usr->flags & USR_X_DISABLED) && !is_guest(usr->name)) {
				Put(usr, "Override\n");
				JMP(usr, STATE_OVERRIDE_MENU);
				Return;
			}
			break;
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
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->head)->str, MAX_LINE);
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
			(void)remove_StringList(&usr->friends, new_enemy);
			(void)prepend_StringList(&usr->enemies, new_enemy);
			(void)sort_StringList(&usr->enemies, alphasort_StringList);
			Print(usr, "<yellow>%s<green> moved from your friend to enemy list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_enemy = new_StringList(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		(void)prepend_StringList(&usr->enemies, new_enemy);
		(void)sort_StringList(&usr->enemies, alphasort_StringList);
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
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->head)->str, MAX_LINE);
		}
		if (!strcmp(usr->name, usr->edit_buf))
			Put(usr, "<green>Time to come to peace with yourself?\n");
		else {
			if ((rm_enemy = in_StringList(usr->enemies, usr->edit_buf)) != NULL) {
				(void)remove_StringList(&usr->enemies, rm_enemy);
				destroy_StringList(rm_enemy);
			} else
				Put(usr, "<red>There is no such person on your enemy list\n");
		}
		RET(usr);
	}
	Return;
}


/* override list for when having Xes disabled */

void state_override_menu(User *usr, char c) {
int n;

	if (usr == NULL)
		return;

	Enter(state_override_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			if (usr->override == NULL)
				Put(usr, "<cyan>\nYour override list is empty\n");
			else {
				Put(usr, "\n");
				print_columns(usr, usr->override, 0);
			}
			Print(usr, "<magenta>\n"
				"<hotkey>Add override%s", (usr->override == NULL) ? "\n" : "                 <hotkey>Remove override\n");

			if (usr->override != NULL)
				Put(usr, "<hotkey>Clean out\n"
					"\n");

			Put(usr, "Switch to <hotkey>friend list        Switch to <hotkey>enemy list\n");
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
			Put(usr, "Add override\n");
			n = count_List(usr->override);
			if (n >= PARAM_MAX_FRIEND) {
				Print(usr, "<red>You already have %d overrides defined\n", n);
				break;
			}
			enter_name(usr, STATE_ADD_OVERRIDE);
			Return;

		case 'd':
		case 'D':
		case 'R':
		case 'r':
		case '-':
		case '_':
			if (usr->override == NULL)
				break;

			Put(usr, "Remove override\n");
			enter_name(usr, STATE_REMOVE_OVERRIDE);
			Return;

		case 'c':
		case 'C':
			if (usr->override != NULL) {
				Put(usr, "Clean out\n");
				listdestroy_StringList(usr->override);
				usr->override = NULL;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case '>':
		case 'f':
		case 'F':
			Put(usr, "Friends\n");
			JMP(usr, STATE_FRIENDLIST_PROMPT);
			Return;

		case '<':
		case 'e':
		case 'E':
			Put(usr, "Enemies\n");
			JMP(usr, STATE_ENEMYLIST_PROMPT);
			Return;
	}
	Print(usr, "<yellow>\n[Config] Override%c <white>", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_add_override(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_add_override);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *new_override;

		if (!usr->edit_buf[0]) {
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->head)->str, MAX_LINE);
		}
		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");
			RET(usr);
			Return;
		}
		if (!strcmp(usr->name, usr->edit_buf)) {
			Put(usr, "<green>You may always send yourself messages\n");
			RET(usr);
			Return;
		}
		if (in_StringList(usr->override, usr->edit_buf) != NULL) {
			Print(usr, "<yellow>%s<red> already is on your override list\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if (in_StringList(usr->enemies, usr->edit_buf) != NULL) {
			Print(usr, "<red>But <yellow>%s<red> is on your enemy list!\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		if ((new_override = new_StringList(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		(void)prepend_StringList(&usr->override, new_override);
		(void)sort_StringList(&usr->override, alphasort_StringList);
		RET(usr);
	}
	Return;
}

void state_remove_override(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_remove_override);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *rm_override;

		if (!usr->edit_buf[0]) {
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				Return;
			}
			cstrcpy(usr->edit_buf, ((StringList *)usr->recipients->head)->str, MAX_LINE);
		}
		if (!strcmp(usr->name, usr->edit_buf))
			Put(usr, "<green>You may always send yourself messages\n");
		else {
			if ((rm_override = in_StringList(usr->override, usr->edit_buf)) != NULL) {
				(void)remove_StringList(&usr->override, rm_override);
				destroy_StringList(rm_override);
			} else
				Put(usr, "<red>There is no such person on your override list\n");
		}
		RET(usr);
	}
	Return;
}

/* EOB */
