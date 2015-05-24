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
	WY	WJ97
	state.c
*/

/* #include "*.h" :P */

#include "config.h"
#include "defines.h"
#include "debug.h"
#include "User.h"
#include "util.h"
#include "log.h"
#include "edit.h"
#include "state.h"
#include "state_msg.h"
#include "state_login.h"
#include "state_sysop.h"
#include "state_config.h"
#include "state_roomconfig.h"
#include "state_friendlist.h"
#include "state_history.h"
#include "inet.h"
#include "passwd.h"
#include "patchlist.h"
#include "Room.h"
#include "Stats.h"
#include "access.h"
#include "SU_Passwd.h"
#include "timeout.h"
#include "CallStack.h"
#include "screens.h"
#include "cstring.h"
#include "Param.h"
#include "Feeling.h"
#include "HostMap.h"
#include "copyright.h"
#include "OnlineUser.h"
#include "Worldclock.h"
#include "Category.h"
#include "Memory.h"
#include "memset.h"
#include "bufprintf.h"
#include "helper.h"
#include "DirList.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

/*
	run the 'common' functions, these are all functions but no eXpress Messages and
	no message reading functions

	Be advised: This function calls other states, and may reset the RTF_BUSY flag
*/
int fun_common(User *usr, char c) {
	if (usr == NULL)
		return 1;

	Enter(fun_common);

	switch(c) {
		case KEY_CTRL('G'):
			Put(usr, "<white>GNU General Public License\n<green>");
			if (load_screen(usr->text, PARAM_GPL_SCREEN) < 0) {
				Put(usr, "<red>The GPL file is missing\n");				/* or out of memory! */
				break;
			}
			read_text(usr);
			Return 1;

		case '[':
			Put(usr, "<white>bbs100 version information\n");
			print_version_info(usr);
			break;

		case ']':
			Put(usr, "<white>Local modifications made to bbs100");
			if (load_screen(usr->text, PARAM_MODS_SCREEN) < 0) {
				Put(usr, "\n<red>The local mods file is missing\n");	/* or out of memory! */
				break;
			}
			Put(usr, "<green>");
			read_text(usr);
			Return 1;

		case '{':
		case '}':
			Put(usr, "<white>Credits\n");

			if (usr->text == NULL && (usr->text = new_StringIO()) == NULL) {
				Perror(usr, "Out of memory");
				break;
			}
			free_StringIO(usr->text);
			if (load_screen(usr->text, PARAM_CREDITS_SCREEN) < 0) {
				Put(usr, "<red>The credits file is missing\n");		/* or out of memory! */
				break;
			}
			Put(usr, "<green>");
			read_text(usr);
			Return 1;

		case 'w':
			Put(usr, "<white>Who\n");
			if (usr->flags & USR_SHORT_WHO)
				who_list(usr, WHO_LIST_SHORT);
			else
				who_list(usr, WHO_LIST_LONG);
			Return 1;

		case 'W':
			Put(usr, "<white>Who\n");
			if (usr->flags & USR_SHORT_WHO)
				who_list(usr, WHO_LIST_LONG);
			else
				who_list(usr, WHO_LIST_SHORT);
			Return 1;

		case KEY_CTRL('W'):
			Put(usr, "<white>Customize Who list\n");
			CALL(usr, STATE_CONFIG_WHO);
			Return 1;

		case KEY_CTRL('F'):
			Put(usr, "<white>Online friends\n");
			online_friends_list(usr);
			Return 1;

		case KEY_CTRL('T'):
			if (PARAM_HAVE_TALKEDTO) {
				Put(usr, "<white>Talked to list\n");
				talked_list(usr);
				Return 1;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Talked To lists<red> are not enabled on this server\n");
			break;

		case KEY_CTRL('P'):
			Put(usr, "<white>Ping\n");
			enter_recipients(usr, STATE_PING_PROMPT);
			Return 1;

		case 'p':
		case 'P':
			Put(usr, "<white>Profile\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the anonymous <yellow>%s<red> user cannot profile anyone\n", PARAM_NAME_GUEST);
				break;
			}
			if (usr->message != NULL) {
				deinit_StringQueue(usr->recipients);
				if (usr->message->anon != NULL && usr->message->anon[0]) {
					if (usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE))
						(void)add_StringQueue(usr->recipients, new_StringList(usr->message->from));
				} else
					(void)add_StringQueue(usr->recipients, new_StringList(usr->message->from));
			}
			if (c == 'p')
				enter_name(usr, STATE_LONG_PROFILE);
			else
				enter_name(usr, STATE_SHORT_PROFILE);
			Return 1;

		case 'X':
			Put(usr, "<white>Toggle message reception\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot enable message reception\n", PARAM_NAME_GUEST);
				break;
			}
			usr->flags ^= USR_X_DISABLED;
			listdestroy_StringList(usr->override);
			usr->override = NULL;

			Print(usr, "<magenta>Message reception is now turned <yellow>%s\n", (usr->flags & USR_X_DISABLED) ? "off" : "on");

			if (usr->flags & USR_X_DISABLED) {
				if (usr->flags & USR_HELPING_HAND) {
					usr->flags &= ~USR_HELPING_HAND;
					remove_helper(usr);
					Put(usr, "<magenta>You are no longer available to help others\n");
					usr->runtime_flags |= RTF_WAS_HH;
				}
			} else {
				if (usr->runtime_flags & RTF_WAS_HH) {
					usr->flags |= USR_HELPING_HAND;
					usr->runtime_flags &= ~RTF_WAS_HH;
					add_helper(usr);
					Put(usr, "<magenta>You are now available to help others\n");
				}
			}
			break;

		case 'o':
			Put(usr, "<white>Override\n");
			if (!(usr->flags & USR_X_DISABLED)) {
				Put(usr, "<red>Override is non-functional when you are able to receive messages\n");
				break;
			}
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot use the Override function\n", PARAM_NAME_GUEST);
				break;
			}
			CALL(usr, STATE_OVERRIDE_MENU);
			Return 1;

		case '$':
			if (usr->runtime_flags & RTF_SYSOP) {
				drop_sysop_privs(usr);
				Put(usr, "\n");
				break;
			}
			if (is_sysop(usr->name)) {
				Print(usr, "<white>%s mode\n", PARAM_NAME_SYSOP);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot play %s\n", PARAM_NAME_GUEST, PARAM_NAME_SYSOP);
					break;
				}
				CALL(usr, STATE_SU_PROMPT);
				Return 1;
			}

		case 't':
		case 'T':
			Put(usr, "<white>Time");
			buffer_text(usr);
			print_calendar(usr);
			read_text(usr);
			Return 1;

		case KEY_CTRL('D'):
			Put(usr, "<white>Doing\n");
			CALL(usr, STATE_CONFIG_DOING);
			Return 1;

		case '`':
			CALL(usr, STATE_BOSS);
			Return 1;

		case 'B':
			usr->flags ^= USR_BEEP;
			if (usr->flags & USR_BEEP)
				usr->flags |= USR_ROOMBEEP;
			else
				usr->flags &= ~USR_ROOMBEEP;

			Print(usr, "<white>Toggle beeping\n"
				"<magenta>Messages will %s beep on arrival\n", (usr->flags & USR_BEEP) ? "now" : "<yellow>not<magenta>");
			break;

		case 'F':
			if (PARAM_HAVE_FOLLOWUP) {
				usr->flags ^= USR_FOLLOWUP;
				Print(usr, "<white>Toggle follow-up mode\n"
					"<magenta>Follow up mode is now <yellow>%s\n", (usr->flags & USR_FOLLOWUP) ? "enabled" : "disabled");
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Follow-up mode<red> is not enabled on this server\n");
			break;

		case 'S':
			Put(usr, "<white>Statistics\n<green>");
			print_stats(usr);
			Return 1;

		case '>':
			Put(usr, "<white>Friends\n");
			CALL(usr, STATE_FRIENDLIST_PROMPT);
			Return 1;

		case '<':
			Put(usr, "<white>Enemies\n");
			CALL(usr, STATE_ENEMYLIST_PROMPT);
			Return 1;

		case 'c':
		case 'C':
			Put(usr, "<red>Press<yellow> <Ctrl-C><red> or<yellow> <Ctrl-O><red> to access the Config menu\n");
			break;

		case KEY_CTRL('C'):				/* this don't work for some people (?) */
		case KEY_CTRL('O'):				/* so I added Ctrl-O by special request */
			Put(usr, "<white>Config menu\n");
			CALL(usr, STATE_CONFIG_MENU);
			Return 1;

		case 'A':
		case KEY_CTRL('A'):
			if (usr->curr_room->number != MAIL_ROOM) {
				if (!(usr->runtime_flags & RTF_ROOMAIDE)
					&& in_StringList(usr->curr_room->room_aides, usr->name) != NULL) {
					if (is_guest(usr->name)) {
						Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot play %s\n", PARAM_NAME_GUEST, PARAM_NAME_ROOMAIDE);
						break;
					}
					Print(usr, "<magenta>Auto-enabling %s functions\n\n", PARAM_NAME_ROOMAIDE);
					usr->runtime_flags |= RTF_ROOMAIDE;
				}
				if (usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)) {
					Put(usr, "<white>Room Config menu\n");
					CALL(usr, STATE_ROOM_CONFIG_MENU);
					Return 1;
				}
				break;
			}
			break;

		case KEY_CTRL('S'):
			if (usr->runtime_flags & RTF_SYSOP) {
				Print(usr, "<white>%s menu\n", PARAM_NAME_SYSOP);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot play %s\n", PARAM_NAME_GUEST, PARAM_NAME_SYSOP);
					break;
				}
				CALL(usr, STATE_SYSOP_MENU);
				Return 1;
			}
			break;
	}
	Return 0;
}


void state_dummy(User *usr, char c) {
	if (usr == NULL)
		return;

	POP(usr);				/* dummy ret; just pop the call off the stack */
}

void print_version_info(User *usr) {
char version_buf[MAX_LONGLINE];

	Print(usr, "<yellow>This is <white>%s<yellow>, %s", PARAM_BBS_NAME,
		print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL, version_buf, MAX_LONGLINE));

	if (usr->runtime_flags & RTF_SYSOP) {
		version_buf[0] = 0;
#ifdef DEBUG
		cstrcat(version_buf, "[DEBUG] ", MAX_LONGLINE);
#endif
#ifdef USE_SLUB
		cstrcat(version_buf, "[SLUB] ", MAX_LONGLINE);
#endif
		if (*version_buf)
			Print(usr, "<green>Compile flags:<yellow> %s\n", version_buf);

		if (*patchlist)
			Print(usr, "<green>Patches: <white>%s\n", patchlist);
	}
}

void enter_recipients(User *usr, void (*state_func)(User *, char)) {
	if (usr == NULL)
		return;

	Enter(enter_recipients);

	if (count_Queue(usr->recipients) <= 0)
		Put(usr, "<green>Enter recipient: <yellow>");
	else {
		if (count_Queue(usr->recipients) == 1)
			Print(usr, "<green>Enter recipient <white>[<yellow>%s<white>]:<yellow> ", ((StringList *)usr->recipients->head)->str);
		else
			Put(usr, "<green>Enter recipient <white>[<green><many<green>><white>]:<yellow> ");
	}
	usr->runtime_flags |= RTF_BUSY;

	PUSH(usr, state_func);
	edit_recipients(usr, EDIT_INIT, NULL);
	Return;
}

void enter_name(User *usr, void (*state_func)(User *, char)) {
	if (usr == NULL)
		return;

	Enter(enter_name);

	if (count_Queue(usr->recipients) <= 0)
		Put(usr, "<green>Enter name: <yellow>");
	else {
		if (count_Queue(usr->recipients) > 1) {
			usr->recipients->tail->next->prev = NULL;
			listdestroy_StringList(usr->recipients->tail->next);
			usr->recipients->tail->next = NULL;
			usr->recipients->head = usr->recipients->tail;
			usr->recipients->count = 1;
		}
		Print(usr, "<green>Enter name <white>[<yellow>%s<white>]:<yellow> ", ((StringList *)usr->recipients->head)->str);
	}
	usr->runtime_flags |= RTF_BUSY;
	usr->edit_pos = 0;
	usr->edit_buf[0] = 0;

	PUSH(usr, state_func);
	Return;
}

void state_x_prompt(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_x_prompt);

	switch(edit_recipients(usr, c, multi_x_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			check_recipients(usr);
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				break;
			}
			JMP(usr, STATE_EDIT_X);
	}
	Return;
}

void state_recipients_err(User *usr, char c) {
	Enter(state_recipients_err);

	if (c == KEY_RETURN || c == ' ' || c == KEY_CTRL('C') || c == KEY_CTRL('D')
		|| c == KEY_BS || c == KEY_CTRL('H') || c == KEY_ESC) {

		while(usr->edit_pos > 0) {
			usr->edit_pos--;
			Put(usr, "\b \b");
		}
		Put(usr, "<yellow>");
		usr->edit_buf[0] = 0;
		POP(usr);				/* pop this call (DON'T RET; RET() re-initializes!!) */
	} else
		Put(usr, "<beep>");
	Return;
}

void state_emote_prompt(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_emote_prompt);

	switch(edit_recipients(usr, c, multi_x_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			check_recipients(usr);
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				break;
			}
			JMP(usr, STATE_EDIT_EMOTE);
	}
	Return;
}

void state_feelings_prompt(User *usr, char c) {
DirList *feelings;

	if (usr == NULL)
		return;

	Enter(state_feelings_prompt);

	switch(edit_recipients(usr, c, multi_x_access)) {
		case EDIT_BREAK:
			POP_ARG(usr, &feelings, sizeof(DirList));
			destroy_DirList(feelings);
			RET(usr);
			break;

		case EDIT_RETURN:
			check_recipients(usr);
			if (count_Queue(usr->recipients) <= 0) {
				POP_ARG(usr, &feelings, sizeof(DirList));
				destroy_DirList(feelings);
				RET(usr);
				break;
			}
			JMP(usr, STATE_CHOOSE_FEELING);
	}
	Return;
}

void state_ping_prompt(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_ping_prompt);

	switch(edit_recipients(usr, c, multi_ping_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			if (count_Queue(usr->recipients) <= 0) {
				RET(usr);
				break;
			}
			JMP(usr, LOOP_PING);
	}
	Return;
}

void loop_ping(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(loop_ping);

	if (c == INIT_STATE) {
		LOOP(usr, count_Queue(usr->recipients));
	} else {
		StringList *sl;
		User *u;
		unsigned long i, tdiff;

		sl = (StringList *)usr->recipients->tail;
		if (sl == NULL) {
			LOOP(usr, 0UL);
			RET(usr);
			Return;
		}
		for(i = 0UL; i < usr->conn->loop_counter; i++) {		/* do the next recipient */
			sl = sl->next;
			if (sl == NULL) {
				LOOP(usr, 0UL);
				RET(usr);
				Return;
			}
		}
		if (!strcmp(sl->str, usr->name)) {
			Put(usr, "<red>You are keeping yourself busy pinging yourself\n");
			(void)remove_StringQueue(usr->recipients, sl);
			destroy_StringList(sl);
			Return;
		}
		if ((u = is_online(sl->str)) == NULL) {
			Print(usr, "<yellow>%s <red>suddenly logged off!\n", sl->str);
			(void)remove_StringQueue(usr->recipients, sl);
			destroy_StringList(sl);
			Return;
		}
		if (u->runtime_flags & RTF_LOCKED) {
			if (u->away != NULL && u->away[0])
				Print(usr, "<yellow>%s<green> is away from the terminal for a while;<yellow> %s\n", u->name, u->away);
			else
				Print(usr, "<yellow>%s<green> is away from the terminal for a while\n", u->name);
			Return;
		}
/*
	in case a user is idling, print it
	(hardcoded) default is after 2 minutes
*/
		tdiff = (unsigned long)rtc - (unsigned long)u->idle_time;
		if (tdiff >= 2UL * SECS_IN_MIN) {
			char total_buf[MAX_LINE];

			Print(usr, "<yellow>%s<green> has been idle for %s\n", u->name, print_total_time(tdiff, total_buf, MAX_LINE));
			Return;
		}
		if (u->runtime_flags & RTF_BUSY) {
			if ((u->runtime_flags & RTF_BUSY_SENDING)
				&& in_StringQueue(u->recipients, usr->name) != NULL) {
/*
	the warn follow-up mode feature was donated by Richard of MatrixBBS
*/
				Print(usr, "<yellow>%s<green> is busy sending you a message%s\n",
					u->name, (PARAM_HAVE_FOLLOWUP && (u->flags & USR_FOLLOWUP)) ? " in follow-up mode" : "");
				Return;
			}
			if ((u->runtime_flags & RTF_BUSY_MAILING)
				&& u->new_message != NULL
				&& in_MailToQueue(u->new_message->to, usr->name) != NULL) {
				Print(usr, "<yellow>%s<green> is busy mailing you a message\n", u->name);
				Return;
			}
		}
		if (PARAM_HAVE_HOLD && (u->runtime_flags & RTF_HOLD)) {
			if (u->away != NULL && u->away[0])
				Print(usr, "<yellow>%s<green> has put messages on hold;<yellow> %s\n", u->name, u->away);
			else
				Print(usr, "<yellow>%s<green> has put messages on hold\n", u->name);
		} else
			Print(usr, "<yellow>%s<green> is %sbusy\n", u->name, (u->runtime_flags & RTF_BUSY) ? "" : "not ");
	}
	Return;
}

/*
	flags are not really flags, but it's either PROFILE_LONG or PROFILE_SHORT
*/
void print_profile(User *usr, int flags) {
User *u = NULL;
int allocated = 0, visible;
char total_buf[MAX_LINE], *hidden;

	if (usr == NULL)
		return;

	Enter(print_profile);

	if (!usr->edit_buf[0]) {
		RET(usr);
		Return;
	}
	if (!strcmp(usr->edit_buf, "Sysop") || !strcmp(usr->edit_buf, PARAM_NAME_SYSOP)) {
		if (su_passwd == NULL)
			Print(usr, "<red>There are no %ss on this BBS <white>(!)\n", PARAM_NAME_SYSOP);
		else {
			if (su_passwd->next == NULL)
				Print(usr, "<yellow>%s is: %s\n", PARAM_NAME_SYSOP, su_passwd->key);
			else {
				KVPair *su;

				Print(usr, "<yellow>%ss are: ", PARAM_NAME_SYSOP);
				for(su = su_passwd; su != NULL && su->next != NULL; su = su->next)
					Print(usr, "%s, ", su->key);
				Print(usr, "%s\n", su->key);
			}
		}
		RET(usr);
		Return;
	}
	if (is_guest(usr->edit_buf)) {
		Print(usr, "<green>The <yellow>%s<green> user is a visitor from far away\n", PARAM_NAME_GUEST);

		if (flags == PROFILE_LONG) {
			if ((u = is_online(usr->edit_buf)) != NULL) {
				Print(usr, "<green>Online for <cyan>%s", print_total_time((unsigned long)rtc - (unsigned long)u->login_time, total_buf, MAX_LINE));
				if (u == usr || (usr->runtime_flags & RTF_SYSOP)) {
					if (usr->runtime_flags & RTF_SYSOP)
						Print(usr, "<green> from<yellow> %s<white> [%s]\n", u->conn->hostname, u->conn->ipnum);
					else
						Print(usr, "<green> from<yellow> %s\n", u->conn->hostname);
				} else
					Put(usr, "\n");

				if (site_description(u->conn->ipnum, total_buf, MAX_LINE) != NULL)
					Print(usr, "<yellow>%s<green> is connected from<yellow> %s\n", usr->edit_buf, total_buf);
			}
		}
		RET(usr);
		Return;
	}
	if (!user_exists(usr->edit_buf)) {
		Put(usr, "<red>No such user\n");
		deinit_StringQueue(usr->recipients);
		RET(usr);
		Return;
	}
	if ((u = is_online(usr->edit_buf)) == NULL) {
		deinit_StringQueue(usr->recipients);	/* don't put name in history */

		if ((u = new_User()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((u->conn = new_Conn()) == NULL) {
			destroy_User(u);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		allocated = 1;

		if (load_User(u, usr->edit_buf,
			LOAD_USER_ALL & ~(LOAD_USER_ROOMS | LOAD_USER_PASSWORD | LOAD_USER_QUICKLIST))) {
			Print(usr, "<red>Error loading user <yellow>%s\n", usr->edit_buf);
			log_err("state_profile_user(): failed to load user %s", usr->edit_buf);
			RET(usr);
			Return;
		}
/* load the proper hostname */
		cstrcpy(u->conn->hostname, u->tmpbuf[TMP_FROM_HOST], MAX_LINE);
	} else {
		load_profile_info(u);

		deinit_StringQueue(usr->recipients);		/* place entered name in history */
		(void)add_StringQueue(usr->recipients, new_StringList(usr->edit_buf));
	}
/*
	make the profile
*/
	buffer_text(usr);
	Put(usr, "<white>");

	if (PARAM_HAVE_VANITY && u->vanity != NULL && u->vanity[0]) {
		char fmt[16];

		bufprintf(fmt, sizeof(fmt), "%%-%ds", MAX_NAME + 10);
		Print(usr, fmt, u->name);
		Print(usr, "<magenta>* <white>%s <magenta>*", u->vanity);
	} else
		Put(usr, u->name);

	Put(usr, "\n");

	if (flags == PROFILE_LONG) {
		visible = 1;
		hidden = "";
		if (((u->flags & USR_HIDE_ADDRESS) && in_StringList(u->friends, usr->name) == NULL)
			|| ((u->flags & USR_HIDE_INFO) && in_StringList(u->enemies, usr->name) != NULL)) {
			if ((usr->runtime_flags & RTF_SYSOP) || u == usr)
				hidden = "<white>hidden> ";
			else
				visible = 0;
		}
		if (visible) {
			if (u->real_name != NULL && u->real_name[0])
				Print(usr, "%s<yellow>%s\n", hidden, u->real_name);

			if (u->street != NULL && u->street[0])
				Print(usr, "%s<yellow>%s\n", hidden, u->street);

			if (u->zipcode != NULL && u->zipcode[0]) {
				if (u->city != NULL && u->city[0])
					Print(usr, "%s<yellow>%s<yellow>  %s\n", hidden, u->city, u->zipcode);
				else
					Print(usr, "%s<yellow>%s\n", hidden, u->zipcode);
			} else
				if (u->city != NULL && u->city[0])
					Print(usr, "%s<yellow>%s\n", hidden, u->city);

			if (u->state != NULL && u->state[0]) {
				if (u->country != NULL && u->country[0])
					Print(usr, "%s<yellow>%s, %s\n", hidden, u->state, u->country);
				else
					Print(usr, "%s<yellow>%s\n", hidden, u->state);
			} else
				if (u->country != NULL && u->country[0])
					Print(usr, "%s<yellow>%s\n", hidden, u->country);

			if (u->phone != NULL && u->phone[0])
				Print(usr, "%s<green>Phone: <yellow>%s\n", hidden, u->phone);

			if (u->email != NULL && u->email[0])
				Print(usr, "%s<green>E-mail: <cyan>%s\n", hidden, u->email);

			if (u->www != NULL && u->www[0])
				Print(usr, "%s<green>WWW: <cyan>%s\n", hidden, u->www);
		}
		if (u->doing != NULL && u->doing[0]) {
			if (allocated)
				Print(usr, "<green>Was doing: <yellow>%s <cyan>%s\n", u->name, u->doing);
			else
				Print(usr, "<green>Doing: <yellow>%s <cyan>%s\n", u->name, u->doing);
		}
	}
	if (allocated) {
		char date_buf[MAX_LINE], online_for[MAX_LINE+10];

		if (u->last_online_time > 0UL) {
			int l;

			l = bufprintf(online_for, sizeof(online_for), "%c for %c", color_by_name("green"), color_by_name("yellow"));
			print_total_time(u->last_online_time, online_for+l, sizeof(online_for) - l);
		} else
			online_for[0] = 0;

		Print(usr, "<green>Last online: <cyan>%s%s", print_date(usr, (time_t)u->last_logout, date_buf, MAX_LINE), online_for);
		if (usr->runtime_flags & RTF_SYSOP)
			Print(usr, "<green> from<yellow> %s<white> [%s]\n", u->tmpbuf[TMP_FROM_HOST], u->tmpbuf[TMP_FROM_IP]);
		else
			Put(usr, "\n");

		if (flags == PROFILE_LONG) {
			if (site_description(u->tmpbuf[TMP_FROM_IP], total_buf, MAX_LINE) != NULL)
				Print(usr, "<yellow>%s<green> was connected from<yellow> %s\n", u->name, total_buf);
		}
	} else {
		if (flags == PROFILE_LONG) {
/*
	display for how long someone is online
*/
			Print(usr, "<green>Online for <cyan>%s", print_total_time(rtc - u->login_time, total_buf, MAX_LINE));
			if (u == usr || (usr->runtime_flags & RTF_SYSOP)) {
				if (usr->runtime_flags & RTF_SYSOP)
					Print(usr, "<green> from<yellow> %s<white> [%s]\n", u->conn->hostname, u->conn->ipnum);
				else
					Print(usr, "<green> from<yellow> %s\n", u->conn->hostname);
			} else
				Put(usr, "\n");

			if (site_description(u->conn->ipnum, total_buf, MAX_LINE) != NULL)
				Print(usr, "<yellow>%s<green> is connected from<yellow> %s\n", u->name, total_buf);
		}
	}
	if (!allocated)
		update_stats(u);

	if (flags == PROFILE_LONG) {
		if (usr->runtime_flags & RTF_SYSOP)
			Print(usr, "<green>First login was on <cyan>%s\n", print_date(usr, u->birth, total_buf, MAX_LINE));
		Print(usr, "<green>Total online time: <yellow>%s\n", print_total_time(u->total_time, total_buf, MAX_LINE));
	}
	if (u->flags & USR_X_DISABLED) {
		Print(usr, "<red>%s has message reception turned off", u->name);
		if ((!(u->flags & USR_BLOCK_FRIENDS) && in_StringList(u->friends, usr->name) != NULL)
			|| in_StringList(u->override, usr->name) != NULL)
			Put(usr, ", but is accepting messages from you");
		Put(usr, "\n");
	}
	if (flags == PROFILE_LONG) {
		if (in_StringList(u->friends, usr->name) != NULL) {
			char namebuf[MAX_NAME+20];

			bufprintf(namebuf, sizeof(namebuf), "<yellow>%s<green>", u->name);
			Print(usr, "<green>You are on %s\n", possession(namebuf, "friend list", total_buf, sizeof(total_buf)));
		}
	}
	visible = 1;
	if (!(usr->runtime_flags & RTF_SYSOP) && usr != u
		&& (u->flags & USR_HIDE_INFO) && in_StringList(u->enemies, usr->name) != NULL)
		visible = 0;

	if (visible && in_StringList(u->enemies, usr->name) != NULL)
		Print(usr, "<yellow>%s<red> does not wish to receive any messages from you\n", u->name);

	if (visible && u->info != NULL && u->info->buf != NULL) {
		Put(usr, "<green>\n");
		concat_StringIO(usr->text, u->info);
	}
	if (!PARAM_HAVE_RESIDENT_INFO) {
		destroy_StringIO(u->info);
		u->info = NULL;
	}
	if (usr->message != NULL && usr->message->anon != NULL && usr->message->anon[0]
		&& !strcmp(usr->message->from, u->name)
		&& strcmp(usr->message->from, usr->name))
		log_msg("%s profiled anonymous post", usr->name);

	if (allocated) {
		destroy_Conn(u->conn);
		u->conn = NULL;
		destroy_User(u);
	}
	POP(usr);
	read_text(usr);
	Return;
}

void state_short_profile(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_short_profile);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
/*
	mind that a long profile is default
*/
	if (r == EDIT_RETURN)
		print_profile(usr, (usr->flags & USR_SHORT_PROFILE) ? PROFILE_LONG : PROFILE_SHORT);

	Return;
}

void state_long_profile(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_long_profile);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN)
		print_profile(usr, (usr->flags & USR_SHORT_PROFILE) ? PROFILE_SHORT : PROFILE_LONG);

	Return;
}

/*
	send X messages in a loop
	expects stack arguments: copy of the recipient list as StringList

	every time it loops, it takes one recipient from the list on the stack
	it loops by setting the loop counter to just 1, but does so multiple times
*/
void loop_send_msg(User *usr, char c) {
StringList *recipients, *sl;
User *u;
int recv_it;

	if (usr == NULL)
		return;

	Enter(loop_send_msg);

	if (c != INIT_STATE && c != LOOP_STATE) {
		Perror(usr, "This is unexpected ...");
		Return;
	}
	loop_Conn(usr->conn, 0UL);
/*
	EDIT_INIT resets the edit line, which is important when sending yourself messages
	from within a chat room ... (otherwise the line annoyingly gets reprinted)
*/
	edit_line(usr, EDIT_INIT);

	PEEK_ARG(usr, &recipients, sizeof(StringList *));

	if (recipients == NULL) {
		POP_ARG(usr, &recipients, sizeof(StringList *));

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = NULL;

		RET(usr);
		Return;
	}
	if (usr->send_msg == NULL) {
		Perror(usr, "The message has disappeared!");
		POP_ARG(usr, &recipients, sizeof(StringList *));
		RET(usr);
		Return;
	}
	sl = pop_StringList(&recipients);
	if (sl == NULL) {
		POP_ARG(usr, &recipients, sizeof(StringList *));

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = NULL;

		RET(usr);
		Return;
	}
	POKE_ARG(usr, &recipients, sizeof(StringList *));

	if (sl->str == NULL || !sl->str[0]) {
		destroy_StringList(sl);
		loop_Conn(usr->conn, 1UL);			/* return here once more */
		Return;
	}
	recv_it = 1;
	if (!strcmp(sl->str, usr->name)) {
		Put(usr, "<green>Talking to yourself, are you?\n");
		u = usr;
	} else {
		if ((u = is_online(sl->str)) == NULL) {
			Print(usr, "<red>Sorry, but <yellow>%s<red> left before you could finish typing!\n", sl->str);
			recv_it = 0;
		} else {
			if (u->runtime_flags & RTF_LOCKED) {
				if (u->away != NULL && u->away[0])
					Print(usr, "<red>Sorry, but <yellow>%s<red> suddenly locked the terminal;<yellow> %s\n", sl->str, u->away);
				else
					Print(usr, "<red>Sorry, but <yellow>%s<red> suddenly locked the terminal\n", sl->str);
				recv_it = 0;
			} else {
				if (!(usr->runtime_flags & RTF_SYSOP)
					&& (u->flags & USR_X_DISABLED)
					&& ((u->flags & USR_BLOCK_FRIENDS) || in_StringList(u->friends, usr->name) == NULL)
					&& in_StringList(u->override, usr->name) == NULL) {
					Print(usr, "<red>Sorry, but <yellow>%s<red> suddenly disabled message reception\n", sl->str);
					recv_it = 0;
				}
			}
		}
	}
	if (recv_it) {
		recvMsg(u, usr, usr->send_msg);			/* deliver the message */
		destroy_StringList(sl);
		loop_Conn(usr->conn, 1UL);
	} else {
		if (PARAM_HAVE_MAILROOM) {
			char *name;

			name = sl->str;
			sl->str = NULL;
			destroy_StringList(sl);

			PUSH_ARG(usr, &name, sizeof(char *));

/* user is already gone, so remove from recipient list */
			if ((sl = in_StringQueue(usr->recipients, name)) != NULL) {
				(void)remove_StringQueue(usr->recipients, sl);
				destroy_StringList(sl);
			}
			CALL(usr, STATE_MAIL_SEND_MSG);
		}
	}
	Return;
}

/*
	Note: this state expects the recipients name on the stack as char*
*/
void state_mail_send_msg(User *usr, char c) {
char *name;

	if (usr == NULL)
		return;

	Enter(state_mail_send_msg);

	if (c == INIT_STATE) {
		usr->runtime_flags |= RTF_BUSY;
		Put(usr, "<cyan>Do you wish to <yellow>Mail><cyan> the message? (Y/n): <white>");
		Return;
	}
	switch(yesno(usr, c, 'Y')) {
		case YESNO_YES:
			POP_ARG(usr, &name, sizeof(char *));
			POP(usr);						/* don't return here */
			mail_msg(usr, usr->send_msg, name);
			Return;

		case YESNO_NO:
			POP_ARG(usr, &name, sizeof(char *));
			Free(name);
			POP(usr);						/* don't return here */
			loop_Conn(usr->conn, 1UL);
			Return;

		case YESNO_UNDEF:
			Put(usr, "<cyan>Send the message as <yellow>Mail>, <hotkey>yes or <hotkey>no? (Y/n): <white>");
	}
	Return;
}

/*
	Send an eXpress message as Mail> to user 'name'
*/
void mail_msg(User *usr, BufferedMsg *msg, char *name) {
MailTo *mailto;
char buf[PRINT_BUF], c;
unsigned int flags;

	if (usr == NULL)
		return;

	Enter(mail_msg);

	if (msg == NULL) {
		Perror(usr, "The message is gone all of a sudden!");
		Return;
	}
	destroy_Message(usr->new_message);
	if ((usr->new_message = new_Message()) == NULL) {
		Perror(usr, "Out of memory");
		Return;
	}
	cstrcpy(usr->new_message->from, usr->name, MAX_NAME);

	if ((usr->new_message->to = new_MailToQueue()) == NULL) {
		Perror(usr, "Out of memory");
		destroy_Message(usr->new_message);
		usr->new_message = NULL;
		Return;
	}
/*
	send mail to the recipient who failed
*/
	if ((mailto = new_MailTo()) == NULL) {
		Perror(usr, "Out of memory");
		destroy_Message(usr->new_message);
		usr->new_message = NULL;
		Return;
	}
	mailto->name = name;
	(void)add_MailToQueue(usr->new_message->to, mailto);

	usr->new_message->subject = cstrdup("<lost message>");

	free_StringIO(usr->text);
	put_StringIO(usr->text, "<cyan>Delivery of this message was impossible. You do get it this way.\n \n");
/*
	This is the most ugly hack ever; temporarily reset name to get a correct
	msg header out of buffered_msg_header();
	Temporarily turn off X message numbering too
*/
	c = usr->name[0];
	usr->name[0] = 0;
	flags = usr->flags;
	usr->flags &= ~USR_XMSG_NUM;
	buffered_msg_header(usr, msg, buf, PRINT_BUF);
	usr->name[0] = c;
	usr->flags = flags;

	put_StringIO(usr->text, buf);
	concat_StringIO(usr->text, msg->msg);

/* save the Mail> message */

	PUSH(usr, STATE_RETURN_MAIL_MSG);
	PUSH(usr, STATE_DUMMY);					/* it will JMP, overwriting this state */
	save_message_room(usr, usr->mail);
	Return;
}

void state_return_mail_msg(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_return_mail_msg);

	if (c == INIT_STATE) {
		destroy_Message(usr->new_message);
		usr->new_message = NULL;
		loop_Conn(usr->conn, 1UL);
	}
	Return;
}

void state_edit_emote(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_edit_emote);

	if (c == INIT_STATE) {
		edit_line(usr, EDIT_INIT);
		usr->runtime_flags |= RTF_BUSY_SENDING;
		Print(usr, "<cyan>* <green>%s <yellow>", usr->name);
		Return;
	}
	if (c == KEY_CTRL('A') && (usr->flags & USR_FOLLOWUP)) {
		usr->flags &= ~USR_FOLLOWUP;
		Put(usr, "\n<magenta>Follow up mode aborted\n");
		c = KEY_CTRL('C');
	}
	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		Put(usr, "<red>Message not sent\n");
		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		BufferedMsg *msg;
		StringList *recipients;

		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		if (empty_emote(usr->edit_buf)) {
			Put(usr, "<red>Nothing entered, so no message was sent\n");
			RET(usr);
			Return;
		}
		if ((msg = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((msg->to = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		put_StringIO(msg->msg, usr->edit_buf);
		write_StringIO(msg->msg, "\n", 1);

		cstrcpy(msg->from, usr->name, MAX_NAME);
		msg->mtime = rtc;

		msg->flags = BUFMSG_EMOTE;
		if (usr->runtime_flags & RTF_SYSOP)
			msg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, msg);
		expire_history(usr);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(msg);
		usr->msg_seq_sent++;

/* update stats */
		if (count_Queue(usr->recipients) > 0
			&& !(count_Queue(usr->recipients) == 1 && !strcmp(usr->name, ((StringList *)usr->recipients->head)->str))) {
			usr->esent++;
			if (usr->esent > stats.esent) {
				stats.esent = usr->esent;
				cstrcpy(stats.most_esent, usr->name, MAX_NAME);
			}
			stats.esent_boot++;
		}
		if ((recipients = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		POP(usr);
		PUSH_ARG(usr, &recipients, sizeof(StringList *));
		CALL(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_edit_x(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_edit_x);

	if (c == INIT_STATE) {
		edit_x(usr, EDIT_INIT);
		usr->runtime_flags |= RTF_BUSY_SENDING;
		Put(usr, "<yellow>>");
		Return;
	}
	if (c == KEY_CTRL('A') && (usr->flags & USR_FOLLOWUP)) {
		usr->flags &= ~USR_FOLLOWUP;
		Put(usr, "\n<magenta>Follow up mode aborted\n");
		c = KEY_CTRL('C');
	}
	r = edit_x(usr, c);

	if (r == EDIT_BREAK) {
		free_StringIO(usr->text);

		Put(usr, "<red>Message not sent\n");
		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		BufferedMsg *xmsg;
		StringList *recipients;

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (empty_xmsg(usr->text)) {
			Put(usr, "<red>Nothing entered, so no message was sent\n");
			RET(usr);
			Return;
		}

/* send X */

		if ((xmsg = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((xmsg->to = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
			destroy_BufferedMsg(xmsg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		copy_StringIO(xmsg->msg, usr->text);

		cstrcpy(xmsg->from, usr->name, MAX_NAME);
		if (PARAM_HAVE_XMSG_HDR && usr->xmsg_header != NULL && usr->xmsg_header[0])
			xmsg->xmsg_header = cstrdup(usr->xmsg_header);

		xmsg->mtime = rtc;

		xmsg->flags = BUFMSG_XMSG;
		if (usr->runtime_flags & RTF_SYSOP)
			xmsg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, xmsg);
		expire_history(usr);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(xmsg);
		usr->msg_seq_sent++;

/* update stats */
		if (count_Queue(usr->recipients) > 0
			&& !(count_Queue(usr->recipients) == 1 && !strcmp(usr->name, ((StringList *)usr->recipients->head)->str))) {
			usr->xsent++;
			if (usr->xsent > stats.xsent) {
				stats.xsent = usr->xsent;
				cstrcpy(stats.most_xsent, usr->name, MAX_NAME);
			}
			stats.xsent_boot++;
		}
		if ((recipients = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		POP(usr);
		PUSH_ARG(usr, &recipients, sizeof(StringList *));
		CALL(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_choose_feeling(User *usr, char c) {
DirList *feelings;
int r;

	if (usr == NULL)
		return;

	Enter(state_choose_feeling);

	PEEK_ARG(usr, &feelings, sizeof(DirList *));
	if (feelings == NULL) {
		Perror(usr, "All feelings have gone ...");
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			Put(usr, "\n<green>Feeling: <yellow>");
			Return;

		case INIT_STATE:
			usr->runtime_flags |= (RTF_BUSY | RTF_BUSY_SENDING);
			edit_number(usr, EDIT_INIT);
/*
	remember the current generation number; this will tell us later if
	Sysop changed the feelings in the meantime
*/
			Free(usr->tmpbuf[TMP_FROM_IP]);
			if ((usr->tmpbuf[TMP_FROM_IP] = (char *)Malloc(MAX_NUMBER, TYPE_CHAR)) == NULL) {
				Perror(usr, "Out of memory");
				POP_ARG(usr, &feelings, sizeof(DirList *));
				destroy_DirList(feelings);
				RET(usr);
				Return;
			}
			bufprintf(usr->tmpbuf[TMP_FROM_IP], MAX_NUMBER, "%d", feelings_generation);

			buffer_text(usr);
			print_columns(usr, (StringList *)feelings->list->tail, FORMAT_NUMBERED|FORMAT_NO_UNDERSCORES);
			read_menu(usr);
			Return;

		default:
			r = edit_number(usr, c);

			if (r == EDIT_RETURN && !usr->edit_buf[0])
				r = EDIT_BREAK;

			if (r == EDIT_BREAK) {
				usr->runtime_flags &= ~(RTF_BUSY | RTF_BUSY_SENDING);
				Free(usr->tmpbuf[TMP_FROM_IP]);
				usr->tmpbuf[TMP_FROM_IP] = NULL;
				POP_ARG(usr, &feelings, sizeof(DirList *));
				destroy_DirList(feelings);
				RET(usr);
				Return;
			}
			if (r == EDIT_RETURN) {
				BufferedMsg *msg;
				StringList *sl, *recipients;
				File *file;
				char filename[MAX_PATHLEN];
				int num, gen;

				usr->runtime_flags &= ~RTF_BUSY_SENDING;

				num = atoi(usr->edit_buf);
				if (num <= 0) {
					Put(usr, "<red>No such feeling\n");
					Free(usr->tmpbuf[TMP_FROM_IP]);
					usr->tmpbuf[TMP_FROM_IP] = NULL;
					POP_ARG(usr, &feelings, sizeof(DirList *));
					destroy_DirList(feelings);
					RET(usr);
					Return;
				}
				gen = atoi(usr->tmpbuf[TMP_FROM_IP]);
				Free(usr->tmpbuf[TMP_FROM_IP]);
				usr->tmpbuf[TMP_FROM_IP] = NULL;

				if (gen != feelings_generation) {
					Print(usr, "<red>In the meantime, a %s changed something ...\n\n", PARAM_NAME_SYSOP);

/* re-read the Feelings directory */
					destroy_DirList(feelings);
					if ((feelings = list_DirList(PARAM_FEELINGSDIR, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_DIRS)) == NULL) {
						log_err("state_choose_feeling(): list_DirList(%s) failed", PARAM_FEELINGSDIR);
						Put(usr, "<red>The Feelings are offline for now, please try again later\n");

						POP_ARG(usr, &feelings, sizeof(DirList *));
/* don't destroy; it's already destroyed, but we still needed to get the container off the stack */
						RET(usr);
						Return;
					}
					POKE_ARG(usr, &feelings, sizeof(DirList *));

					CURRENT_STATE(usr);
					Return;
				}
				POP_ARG(usr, &feelings, sizeof(DirList *));

				num--;
				for(sl = (StringList *)feelings->list->tail; num > 0 && sl != NULL; sl = sl->next)
					num--;

				if (sl == NULL || sl->str == NULL) {
					Put(usr, "<red>No such feeling\n");
					destroy_DirList(feelings);
					RET(usr);
					Return;
				}

/* send feeling */

				if ((msg = new_BufferedMsg()) == NULL) {
					Perror(usr, "Out of memory");
					destroy_DirList(feelings);
					RET(usr);
					Return;
				}
				if ((msg->to = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
					destroy_BufferedMsg(msg);
					Perror(usr, "Out of memory");
					destroy_DirList(feelings);
					RET(usr);
					Return;
				}
				bufprintf(filename, sizeof(filename), "%s/%s", PARAM_FEELINGSDIR, sl->str);
				if ((file = Fopen(filename)) == NULL) {
					destroy_BufferedMsg(msg);
					Perror(usr, "The feeling has passed");
					destroy_DirList(feelings);
					RET(usr);
					Return;
				}
				if (Fget_StringIO(file, msg->msg) < 0) {
					Fclose(file);
					destroy_BufferedMsg(msg);
					Perror(usr, "Out of memory");
					destroy_DirList(feelings);
					RET(usr);
					Return;
				}
				Fclose(file);

				cstrcpy(msg->from, usr->name, MAX_NAME);
				msg->mtime = rtc;

				msg->flags = BUFMSG_FEELING;
				if (usr->runtime_flags & RTF_SYSOP)
					msg->flags |= BUFMSG_SYSOP;

				add_BufferedMsg(&usr->history, msg);
				expire_history(usr);

				destroy_BufferedMsg(usr->send_msg);
				usr->send_msg = ref_BufferedMsg(msg);
				usr->msg_seq_sent++;

/* update stats */
				if (count_Queue(usr->recipients) > 0
					&& !(count_Queue(usr->recipients) == 1 && !strcmp(usr->name, ((StringList *)usr->recipients->head)->str))) {
					usr->fsent++;
					if (usr->fsent > stats.fsent) {
						stats.fsent = usr->fsent;
						cstrcpy(stats.most_fsent, usr->name, MAX_NAME);
					}
					stats.fsent_boot++;
				}
				destroy_DirList(feelings);

				if ((recipients = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				}
				POP(usr);
				PUSH_ARG(usr, &recipients, sizeof(StringList *));
				CALL(usr, LOOP_SEND_MSG);
				Return;
			}
	}
	Return;
}

void state_edit_question(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_edit_question);

	if (c == INIT_STATE) {
		if (get_helper(usr, 0) == NULL) {
			RET(usr);
			Return;
		}
		edit_x(usr, EDIT_INIT);
		usr->runtime_flags |= RTF_BUSY_SENDING;
		Put(usr, "<yellow>>");
		Return;
	}
	if (c == KEY_CTRL('A') && (usr->flags & USR_FOLLOWUP)) {
		usr->flags &= ~USR_FOLLOWUP;
		Put(usr, "\n<magenta>Follow up mode aborted\n");
		c = KEY_CTRL('C');
	}
	r = edit_x(usr, c);

	if (r == EDIT_BREAK) {
		Put(usr, "<red>Question not asked\n");
		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		User *u;
		BufferedMsg *question;

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (empty_xmsg(usr->text)) {
			Put(usr, "<red>Nothing entered, so no question was asked\n");
			RET(usr);
			Return;
		}

/* send question */

		if ((u = get_helper(usr, GH_SILENT)) == NULL) {
			RET(usr);
			Return;
		}
		if ((question = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((question->to = new_StringList(usr->question_asked)) == NULL) {
			destroy_BufferedMsg(question);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		copy_StringIO(question->msg, usr->text);

		cstrcpy(question->from, usr->name, MAX_NAME);
		question->mtime = rtc;

		question->flags = BUFMSG_QUESTION;
		if (usr->runtime_flags & RTF_SYSOP)
			question->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, question);
		expire_history(usr);

		recvMsg(u, usr, question);				/* the question is asked! */
		stats.qsent_boot++;
		usr->msg_seq_sent++;
		usr->qsent++;
		if (usr->qsent > stats.qsent) {
			stats.qsent = usr->qsent;
			cstrcpy(stats.most_qsent, usr->name, MAX_NAME);
		}
		RET(usr);
	}
	Return;
}

/*
	just the same as state_edit_x(), but with subtle change
*/
void state_edit_answer(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_edit_answer);

	if (c == INIT_STATE) {
		char prompt[3];

		edit_x(usr, EDIT_INIT);
		usr->runtime_flags |= RTF_BUSY_SENDING;

		prompt[0] = KEY_CTRL('Q');
		prompt[1] = '>';
		prompt[2] = 0;
		Put(usr, prompt);
		Return;
	}
	if (c == KEY_CTRL('A') && (usr->flags & USR_FOLLOWUP)) {
		usr->flags &= ~USR_FOLLOWUP;
		Put(usr, "\n<magenta>Follow up mode aborted\n");
		c = KEY_CTRL('C');
	}
	r = edit_x(usr, c);

	if (r == EDIT_BREAK) {
		free_StringIO(usr->text);

		Put(usr, "<red>Question not answered\n");
		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		BufferedMsg *xmsg;
		StringList *recipients;

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (empty_xmsg(usr->text)) {
			Put(usr, "<red>Nothing entered, so no answer given\n");
			RET(usr);
			Return;
		}

/* send X */

		if ((xmsg = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((xmsg->to = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
			destroy_BufferedMsg(xmsg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		copy_StringIO(xmsg->msg, usr->text);

		cstrcpy(xmsg->from, usr->name, MAX_NAME);

/* Note: no custom X headers here */

		xmsg->mtime = rtc;

		xmsg->flags = BUFMSG_ANSWER;
		if (usr->runtime_flags & RTF_SYSOP)
			xmsg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, xmsg);
		expire_history(usr);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(xmsg);
		usr->msg_seq_sent++;

		if (count_Queue(usr->recipients) > 0
			&& !(count_Queue(usr->recipients) == 1 && !strcmp(usr->name, ((StringList *)usr->recipients->head)->str))) {
			usr->qansw++;
			if (usr->qansw > stats.qansw) {
				stats.qansw = usr->qansw;
				cstrcpy(stats.most_qansw, usr->name, MAX_NAME);
			}
			stats.qansw_boot++;
		}
		if ((recipients = copy_StringList((StringList *)usr->recipients->tail)) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		POP(usr);
		PUSH_ARG(usr, &recipients, sizeof(StringList *));
		CALL(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_su_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_su_prompt);

	if (c == INIT_STATE)
		Print(usr, "<green>Enter %s password: ", PARAM_NAME_SYSOP);

	r = edit_password(usr, c);
	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd;

		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {
			Print(usr, "\n\n<red>You are not allowed to become <yellow>%s<red> any longer\n", PARAM_NAME_SYSOP);
			clear_password_buffer(usr);
			RET(usr);
			Return;
		}
		if (!verify_phrase(usr->edit_buf, pwd)) {
			clear_password_buffer(usr);
			usr->runtime_flags |= RTF_SYSOP;
			Print(usr, "\n<red>*** <white>NOTE: You are now in <yellow>%s<white> mode <red>***\n", PARAM_NAME_SYSOP);

			log_msg("SYSOP %s entered %s mode", usr->name, PARAM_NAME_SYSOP);
		} else {
			clear_password_buffer(usr);
			Put(usr, "<red>Wrong password\n");

			log_msg("SYSOP %s entered wrong %s mode password", usr->name, PARAM_NAME_SYSOP);
		}
		RET(usr);
		Return;
	}
	Return;
}


/* who list sorting functions */

int sort_who_asc_byname(void *v1, void *v2) {
PList *p1, *p2;
User *u1, *u2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	p1 = *(PList **)v1;
	p2 = *(PList **)v2;

	if (p1 == NULL || p2 == NULL)
		return 0;

	u1 = (User *)p1->p;
	u2 = (User *)p2->p;

	if (u1 == NULL || u2 == NULL)
		return 0;

	return strcmp(u1->name, u2->name);
}

int sort_who_desc_byname(void *v1, void *v2) {
PList *p1, *p2;
User *u1, *u2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	p1 = *(PList **)v1;
	p2 = *(PList **)v2;

	if (p1 == NULL || p2 == NULL)
		return 0;

	u1 = (User *)p1->p;
	u2 = (User *)p2->p;

	if (u1 == NULL || u2 == NULL)
		return 0;

	return -strcmp(u1->name, u2->name);
}

int sort_who_asc_bytime(void *v1, void *v2) {
PList *p1, *p2;
User *u1, *u2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	p1 = *(PList **)v1;
	p2 = *(PList **)v2;

	if (p1 == NULL || p2 == NULL)
		return 0;

	u1 = (User *)p1->p;
	u2 = (User *)p2->p;

	if (u1 == NULL || u2 == NULL)
		return 0;

	if (u1->login_time == u2->login_time)
		return 0;

	if (u1->login_time > u2->login_time)
		return 1;

	return -1;
}

int sort_who_desc_bytime(void *v1, void *v2) {
PList *p1, *p2;
User *u1, *u2;

	if (v1 == NULL || v2 == NULL)
		return 0;

	p1 = *(PList **)v1;
	p2 = *(PList **)v2;

	if (p1 == NULL || p2 == NULL)
		return 0;

	u1 = (User *)p1->p;
	u2 = (User *)p2->p;

	if (u1 == NULL || u2 == NULL)
		return 0;

	if (u1->login_time == u2->login_time)
		return 0;

	if (u1->login_time < u2->login_time)
		return 1;

	return -1;
}

void who_list(User *usr, int format) {
PQueue *pq;
User *u;
int (*sort_func)(void *, void *);
int total;

	if (usr == NULL)
		return;

	Enter(who_list);

/* construct PQueue of Users */

	if ((pq = new_PQueue()) == NULL) {
		Perror(usr, "Out of memory");
		Return;
	}
	if (usr->curr_room != NULL && (usr->curr_room->flags & ROOM_CHATROOM)
		&& !(usr->flags & USR_SHOW_ALL)) {
		PList *p;

		for(p = (PList *)usr->curr_room->inside->tail; p != NULL; p = p->next) {
			u = (User *)p->p;
			if (u == NULL)
				continue;

			if (!u->name[0])
				continue;

			if (u->curr_room != usr->curr_room)
				continue;

			if (!(usr->flags & USR_SHOW_ENEMIES) && in_StringList(usr->enemies, u->name) != NULL)
				continue;

			(void)add_PQueue(pq, new_PList(u));
		}
	} else {
		for(u = AllUsers; u != NULL; u = u->next) {
			if (!u->name[0])
				continue;

			if (!(usr->flags & USR_SHOW_ENEMIES) && in_StringList(usr->enemies, u->name) != NULL)
				continue;

			(void)add_PQueue(pq, new_PList(u));
		}
	}

/* sort the list */

	if (usr->flags & USR_SORT_DESCENDING) {
		if (usr->flags & USR_SORT_BYNAME)
			sort_func = sort_who_desc_byname;
		else
			sort_func = sort_who_desc_bytime;
	} else {
		if (usr->flags & USR_SORT_BYNAME)
			sort_func = sort_who_asc_byname;
		else
			sort_func = sort_who_asc_bytime;
	}
	sort_PQueue(pq, sort_func);

	buffer_text(usr);

	total = count_Queue(pq);
	who_list_header(usr, total, format);

	if (format & WHO_LIST_LONG)
		long_who_list(usr, pq);
	else
		short_who_list(usr, pq);

	destroy_PQueue(pq);			/* destroy temp list */

	read_text(usr);					/* display the who list */
	Return;
}

/*
	helper function for long_ and short_who_list()
	it sets the status flag and the color of the who list entry
*/
void who_list_status(User *usr, User *u, char *stat, char *col) {
	if (usr == NULL || u == NULL || stat == NULL || col == NULL)
		return;

	*stat = ' ';
	*col = (char)color_by_name("yellow");

	if (u == usr)
		*col = (char)color_by_name("white");
	else {
		if (in_StringList(u->enemies, usr->name) != NULL
			|| ((usr->flags & USR_SHOW_ENEMIES)
			&& in_StringList(usr->enemies, u->name) != NULL)) {
			*col = (char)color_by_name("red");
			if (!(usr->flags & USR_ANSI))
				*stat = '-';				/* indicate enemy /wo colors */
		} else {
			if (in_StringList(usr->friends, u->name) != NULL) {
				*col = (char)color_by_name("green");
				if (!(usr->flags & USR_ANSI))
					*stat = '+';			/* indicate friend /wo colors */
			}
		}
	}
	if (u->flags & USR_HELPING_HAND)
		*stat = '%';

	if (u->runtime_flags & RTF_SYSOP)
		*stat = '$';

	if (u->runtime_flags & RTF_HOLD)
		*stat = 'b';

	if ((u->flags & USR_X_DISABLED) && !(usr->runtime_flags & RTF_SYSOP)
		&& ((u->flags & USR_BLOCK_FRIENDS) || in_StringList(u->friends, usr->name) == NULL)
		&& in_StringList(u->override, usr->name) == NULL)
		*stat = '*';

	if (u->runtime_flags & RTF_LOCKED)
		*stat = '#';

	if ((u->runtime_flags & RTF_BUSY_SENDING) && in_StringQueue(u->recipients, usr->name) != NULL)
		*stat = 'x';

	if ((u->runtime_flags & RTF_BUSY_MAILING) && u->new_message != NULL && in_MailToQueue(u->new_message->to, usr->name) != NULL)
		*stat = 'm';
}

/*
	construct a long format who list
*/
int long_who_list(User *usr, PQueue *pq) {
PList *pl;
int hrs, mins, l, c, width;
unsigned long time_now;
time_t t;
char buf[PRINT_BUF], buf2[PRINT_BUF], col, stat;
User *u;

	if (usr == NULL || pq == NULL || pq->tail == NULL)
		return 0;

	Enter(long_who_list);

	time_now = rtc;

	pl = (PList *)pq->tail;
	while(pl != NULL) {
		u = (User *)pl->p;

		pl = pl->next;

		if (u == NULL)
			continue;

		who_list_status(usr, u, &stat, &col);

		t = time_now - u->login_time;
		hrs = t / SECS_IN_HOUR;
		t %= SECS_IN_HOUR;
		mins = t / SECS_IN_MIN;

/* 36 is the length of the string with the stats that's to be added at the end, plus margin */
		width = (usr->display->term_width > (PRINT_BUF-36)) ? (PRINT_BUF-36) : usr->display->term_width;

		if (u->doing == NULL || !u->doing[0])
			bufprintf(buf, sizeof(buf), "%c%s<cyan>", col, u->name);
		else {
			bufprintf(buf, sizeof(buf), "%c%s <cyan>%s", col, u->name, u->doing);
			expand_center(buf, buf2, sizeof(buf2) - 36, width);
			expand_hline(buf2, buf, sizeof(buf) - 36, width);
		}
		l = color_index(buf, width - 9);
		buf[l] = 0;

		c = color_strlen(buf);
		while(c < (width-9) && l < (PRINT_BUF-36)) {
			buf[l++] = ' ';
			c++;
		}
		buf[l] = 0;
		Print(usr, "%s <white>%c <yellow>%2d:%02d\n", buf, stat, hrs, mins);
	}
	Return 0;
}

/*
	construct a short format who list with sorted columns
*/
int short_who_list(User *usr, PQueue *pq) {
int i, j, buflen = 0, cols, rows, total;
char buf[PRINT_BUF], col, stat;
User *u;
PList *pl, *pl_cols[16];

	if (usr == NULL || pq == NULL || pq->tail == NULL)
		return 0;

	Enter(short_who_list);

	cols = usr->display->term_width / (MAX_NAME+2);
	if (cols < 1)
		cols = 1;
	else
		if (cols > 15)
			cols = 15;

	total = count_Queue(pq);
	rows = total / cols;
	if (total % cols)
		rows++;

	memset(pl_cols, 0, sizeof(PList *) * cols);

/* fill in array of pointers to columns */

	pl = (PList *)pq->tail;
	for(i = 0; i < cols; i++) {
		pl_cols[i] = pl;
		for(j = 0; j < rows; j++) {
			if (pl == NULL)
				break;

			pl = pl->next;
		}
	}

/* make the who list in sorted columns */

	for(j = 0; j < rows; j++) {
		buf[0] = 0;
		buflen = 0;
		for(i = 0; i < cols; i++) {
			if (pl_cols[i] == NULL || pl_cols[i]->p == NULL)
				continue;

			u = (User *)pl_cols[i]->p;

			who_list_status(usr, u, &stat, &col);

			bufprintf(buf+buflen, sizeof(buf) - buflen, "<white>%c%c%-18s", stat, col, u->name);
			buflen = strlen(buf);

			if ((i+1) < cols) {
				buf[buflen++] = ' ';
				buf[buflen] = 0;
			}
			pl_cols[i] = pl_cols[i]->next;
		}
		Put(usr, buf);
		Put(usr, "\n");
	}
	Return 0;
}

void who_list_header(User *usr, int total, int drawline) {
struct tm *tm;

	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	if ((usr->curr_room->flags & ROOM_CHATROOM) && !(usr->flags & USR_SHOW_ALL)) {
		if (total == 1)
			Print(usr, "<green>You are the only one in <yellow>%s>\n", usr->curr_room->name);
		else
			Print(usr, "<magenta>There %s <yellow>%d<magenta> user%s in <yellow>%s><magenta> at <yellow>%02d:%02d\n",
				(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
				usr->curr_room->name, tm->tm_hour, tm->tm_min);
	} else {
		if (total == 1)
			Put(usr, "<green>You are the only one online right now\n");
		else
			Print(usr, "<magenta>There %s <yellow>%d<magenta> user%s online at <yellow>%02d:%02d\n",
				(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
				tm->tm_hour, tm->tm_min);
	}
/* draw a line across the full screen width */
	if (drawline)
		Put(usr, "<white><hline>-\n");
}

void reply_x(User *usr, int all) {
PList *pl;
BufferedMsg *m;
StringList *sl;
int msgtype;

	if (usr == NULL)
		return;

	Enter(reply_x);

	pl = unwind_BufferedMsg(usr->history);
	while(pl != NULL) {
		m = (BufferedMsg *)pl->p;
		msgtype = m->flags & BUFMSG_TYPE;
		if ((msgtype == BUFMSG_XMSG || msgtype == BUFMSG_EMOTE || msgtype == BUFMSG_FEELING
			|| msgtype == BUFMSG_QUESTION || msgtype == BUFMSG_ANSWER)
			&& strcmp(m->from, usr->name))
			break;

		pl = pl->prev;
	}
	if (pl == NULL) {
		Put(usr, "<red>No last message to reply to\n");
		CURRENT_STATE(usr);
		Return;
	}
	deinit_StringQueue(usr->recipients);

	if (add_StringQueue(usr->recipients, new_StringList(m->from)) == NULL) {
		Perror(usr, "Out of memory");
	} else
		check_recipients(usr);

	if (all == REPLY_X_ALL) {
		StringList *reply_to;

		if ((reply_to = copy_StringList(m->to)) == NULL) {
			Perror(usr, "Out of memory");
			deinit_StringQueue(usr->recipients);
		} else {
			StringList *sl_next;

			for(sl = reply_to; sl != NULL; sl = sl_next) {
				sl_next = sl->next;

/* don't add the same user multiple times */

				if (in_StringQueue(usr->recipients, sl->str) == NULL)
					(void)add_StringQueue(usr->recipients, sl);
				else
					destroy_StringList(sl);
			}
			if ((sl = in_StringQueue(usr->recipients, usr->name)) != NULL) {
				(void)remove_StringQueue(usr->recipients, sl);
				destroy_StringList(sl);
			}
			check_recipients(usr);
		}
	}
	if (count_Queue(usr->recipients) <= 0) {
		CURRENT_STATE(usr);
		Return;
	}
	do_reply_x(usr, m->flags);
	Return;
}

void do_reply_x(User *usr, int flags) {
char numbuf[MAX_NUMBER], many_buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(do_reply_x);

	usr->edit_pos = 0;
	usr->runtime_flags |= RTF_BUSY;
	usr->edit_buf[0] = 0;

	if (usr->flags & USR_XMSG_NUM)
		bufprintf(numbuf, sizeof(numbuf), "(#%d) ", usr->msg_seq_sent+1);
	else
		numbuf[0] = 0;

/* replying to just one person? */
	if (count_Queue(usr->recipients) == 1) {
		usr->runtime_flags &= ~RTF_MULTI;

		Print(usr, "<green>Replying %sto%s\n", numbuf, print_many(usr, many_buf, MAX_LINE));

		if ((flags & BUFMSG_TYPE) == BUFMSG_QUESTION) {
			CALL(usr, STATE_EDIT_ANSWER);
		} else {
			if ((flags & BUFMSG_TYPE) == BUFMSG_EMOTE) {
				CALL(usr, STATE_EDIT_EMOTE);
			} else {
				CALL(usr, STATE_EDIT_X);
			}
		}
	} else {
/* replying to <many>, edit the recipient list */
		Print(usr, "<green>Replying %sto%s", numbuf, print_many(usr, many_buf, MAX_LINE));

		if ((flags & BUFMSG_TYPE) == BUFMSG_EMOTE) {
			PUSH(usr, STATE_EMOTE_PROMPT);
		} else {
			PUSH(usr, STATE_X_PROMPT);
		}
		edit_recipients(usr, INIT_STATE, NULL);
	}
	Return;
}

void state_lock_password(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_lock_password);

	if (c == INIT_STATE) {
		clear_screen(usr);

		Print(usr, "\n<white>%s terminal locked", PARAM_BBS_NAME);
		if (usr->away != NULL && usr->away[0])
			Print(usr, "; <yellow>%s", usr->away);

		Put(usr, "\n<red>Enter password to unlock: ");

		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
		usr->runtime_flags |= (RTF_BUSY | RTF_LOCKED);

		if (usr->idle_timer != NULL) {
			usr->idle_timer->maxtime = PARAM_LOCK_TIMEOUT * SECS_IN_MIN;
			set_Timer(&usr->timerq, usr->idle_timer, usr->idle_timer->maxtime);
		}
		notify_locked(usr);
		Return;
	}
	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);

		Put(usr, "\n<red>Enter password to unlock: ");

		if (usr->idle_timer != NULL) {
			usr->idle_timer->maxtime = PARAM_LOCK_TIMEOUT * SECS_IN_MIN;
			set_Timer(&usr->timerq, usr->idle_timer, usr->idle_timer->maxtime);
		}
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd;

/* sysops unlock with their sysop password */
		if (usr->runtime_flags & RTF_SYSOP) {
			if ((pwd = get_su_passwd(usr->name)) == NULL) {
				usr->runtime_flags &= ~RTF_SYSOP;
				pwd = usr->passwd;
			}
		} else
			pwd = usr->passwd;

		if (!verify_phrase(usr->edit_buf, pwd)) {
			clear_password_buffer(usr);
			Put(usr, "<yellow>Unlocked\n\n");

			usr->runtime_flags &= ~(RTF_BUSY | RTF_LOCKED);

			if ((usr->runtime_flags & (RTF_WAS_HH|RTF_HOLD)) == RTF_WAS_HH) {
				usr->runtime_flags &= ~RTF_WAS_HH;
				usr->flags |= USR_HELPING_HAND;
				add_helper(usr);
			}
			if (usr->idle_timer != NULL) {
				usr->idle_timer->maxtime = PARAM_IDLE_TIMEOUT * SECS_IN_MIN;
				set_Timer(&usr->timerq, usr->idle_timer, usr->idle_timer->maxtime);
			}
			print_user_status(usr);

			notify_unlocked(usr);
			RET(usr);
		} else {
			clear_password_buffer(usr);
			Put(usr, "Wrong password\n"
				"\n"
				"Enter password to unlock: ");
		}
	}
	Return;
}

/*
	state_boss() is for when the boss walks in
*/
void state_boss(User *usr, char c) {
int r;

	Enter(state_boss);

	if (c == INIT_STATE) {
		edit_line(usr, EDIT_INIT);

		if (usr->flags & USR_HELPING_HAND) {		/* this is inconvenient right now */
			usr->flags &= ~USR_HELPING_HAND;
			remove_helper(usr);
			usr->runtime_flags |= RTF_WAS_HH;
		}
		if (usr->flags & (USR_ANSI|USR_BOLD)) {
			clear_screen(usr);
			Print(usr, "%c", KEY_CTRL('D'));
		} else
			Put(usr, "\n\n");

		POP(usr);
		PUSH_ARG(usr, &usr->flags, sizeof(unsigned int));
		PUSH(usr, STATE_BOSS);
		usr->flags &= ~(USR_ANSI|USR_BOLD);			/* turn off colors */

		if (load_screen(usr->text, PARAM_BOSS_SCREEN) >= 0) {
			char buf[PRINT_BUF];

			while(gets_StringIO(usr->text, buf, PRINT_BUF) != NULL) {
				Put(usr, buf);
				Put(usr, "\n");
			}
		} else {
			switch(rand() & 3) {
				case 0:
					Print(usr, "%c ls\n", (usr->runtime_flags & RTF_SYSOP) ? '#' : '$');
					cmd_line(usr, "ls");
					break;

				case 1:
					Print(usr, "%c date\n", (usr->runtime_flags & RTF_SYSOP) ? '#' : '$');
					cmd_line(usr, "date");
					break;

				case 2:
					Print(usr, "%c uname\n", (usr->runtime_flags & RTF_SYSOP) ? '#' : '$');
					cmd_line(usr, "uname");
					break;

				case 3:
					Print(usr, "%c uptime\n", (usr->runtime_flags & RTF_SYSOP) ? '#' : '$');
					cmd_line(usr, "uptime");
					break;
			}
		}
		Put(usr, (usr->runtime_flags & RTF_SYSOP) ? "\n# " : "\n$ ");
	}
	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		cstrcpy(usr->edit_buf, "exit", MAX_LINE);
		r = EDIT_RETURN;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			Put(usr, (usr->runtime_flags & RTF_SYSOP) ? "# " : "$ ");
			edit_line(usr, EDIT_INIT);
			Return;
		}
		if (!strcmp(usr->edit_buf, "exit") || !strcmp(usr->edit_buf, "logout")) {
			POP_ARG(usr, &usr->flags, sizeof(unsigned int));	/* restore flags */

			usr->runtime_flags &= ~RTF_BUSY;
			if ((usr->runtime_flags & (RTF_WAS_HH|RTF_HOLD)) == RTF_WAS_HH) {
				usr->runtime_flags &= ~RTF_WAS_HH;
				usr->flags |= USR_HELPING_HAND;
				add_helper(usr);
			}
			Put(usr, "\n");
			print_user_status(usr);

			RET(usr);
			Return;
		}
		if (cmd_line(usr, usr->edit_buf) < 0)
			Put(usr, "\ntype 'exit' to return");

		Put(usr, (usr->runtime_flags & RTF_SYSOP) ? "\n# " : "\n$ ");
		edit_line(usr, EDIT_INIT);
		Return;
	}
	Return;
}

/*
	this is more or less a joke
*/
int cmd_line(User *usr, char *cmd) {
int i;
char buf[MAX_LONGLINE], *p;

	if (usr == NULL)
		return -1;

	Enter(cmd_line);

	if ((p = cstrchr(cmd, ' ')) != NULL)
		*p = 0;

	if (!strcmp(cmd, "ls")) {
		unsigned int flags;
		StringList *ls = NULL;

		char *sourcefiles[] = {
			"List.c",
			"PList.c",
			"Hash.c",
			"StringList.c",
			"cstring.c",
			"CallStack.c",
			"log.c",
			"inet.c",
			"util.c",
			"screens.c",
			"edit.c",
			"edit_param.c",
			"access.c",
			"User.c",
			"OnlineUser.c",
			"Wrapper.c",
			"Process.c",
			"CachedFile.c",
			"AtomicFile.c",
			"Signal.c",
			"SignalVector.c",
			"Timer.c",
			"timeout.c",
			"Feeling.c",
			"state.c",
			"state_login.c",
			"state_msg.c",
			"state_friendlist.c",
			"state_config.c",
			"state_room.c",
			"state_roomconfig.c",
			"state_sysop.c",
			"Message.c",
			"Room.c",
			"Joined.c",
			"Stats.c",
			"BufferedMsg.c",
			"passwd.c",
			"SU_Passwd.c",
			"Param.c",
			"copyright.c",
			"Memory.c",
			"debug.c",
			"SymbolTable.c",
			"Types.c",
			"HostMap.c",
			"FileFormat.c",
			"Timezone.c",
			"Worldclock.c",
			"locale_system.c",
			"locales.c",
			"Category.c",
			"crc32.c",
			"Conn.c",
			"ConnUser.c",
			"ConnResolv.c",
			"Telnet.c",
			"KVPair.c",
			"StringIO.c",
			"Display.c",
			"patchlist.c",
			"helper.c",
			"Slub.c",
			"DirList.c",
			"NewUserLog.c",
			"main.c",
			NULL
		};

		for(i = 0; sourcefiles[i] != NULL; i++)
			(void)prepend_StringList(&ls, new_StringList(sourcefiles[i]));

		(void)sort_StringList(&ls, alphasort_StringList);

		flags = usr->flags;
		usr->flags &= ~(USR_ANSI|USR_BOLD);

		print_columns(usr, ls, 0);
		
		usr->flags = flags;
		listdestroy_StringList(ls);
		Return 0;
	}
	if (!strcmp(cmd, "uptime")) {
		Print(usr, "up %s, ", print_total_time(rtc - stats.uptime, buf, MAX_LONGLINE));
		i = count_List(AllUsers);
		Print(usr, "%d user%s\n", i, (i == 1) ? "" : "s");
		Return 0;
	}
	if (!strcmp(cmd, "date")) {
		Print(usr, "%s %s\n", print_date(usr, (time_t)0UL, buf, MAX_LONGLINE), name_Timezone(usr->tz));
		Return 0;
	}
	if (!strcmp(cmd, "uname")) {
		Print(usr, "%s %s", PARAM_BBS_NAME, print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL, buf, MAX_LONGLINE));
		Return 0;
	}
	if (!strcmp(cmd, "whoami")) {
		cstrcpy(buf, usr->name, MAX_LONGLINE);
		cstrlwr(buf);
		while((p = cstrchr(buf, ' ')) != NULL)
			*p = '_';
		Print(usr, "%s\n", buf);
		Return 0;
	}
	Print(usr, "-bbs: %s: command not found\n", cmd);
	Return -1;
}


void state_ask_away_reason(User *usr, char c) {
	Enter(state_ask_away_reason);

	if (c == INIT_STATE) {
		Free(usr->away);
		usr->away = NULL;
	}
	change_config(usr, c, &usr->away, "Enter reason: <yellow>");
	Return;
}

void online_friends_list(User *usr) {
User *u;
struct tm *tm;
StringList *sl;
PQueue *pq;
int total;

	if (usr == NULL)
		return;

	Enter(online_friends_list);

	if (usr->friends == NULL) {
		Put(usr, "<red>You have no friends\n");
		CURRENT_STATE(usr);
		Return;
	}
	if ((pq = new_PQueue()) == NULL) {
		Perror(usr, "Out of memory");
		Return;
	}
	for(sl = usr->friends; sl != NULL; sl = sl->next) {
		if ((u = is_online(sl->str)) == NULL)
			continue;

		(void)add_PQueue(pq, new_PList(u));
	}
	if (count_Queue(pq) <= 0) {
		Put(usr, "<red>None of your friends are online\n");
		destroy_PQueue(pq);
		CURRENT_STATE(usr);
		Return;
	}
	sort_PQueue(pq, (usr->flags & USR_SORT_DESCENDING) ? sort_who_desc_byname : sort_who_asc_byname);
	total = count_Queue(pq);

/* construct header */
	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	buffer_text(usr);
	Print(usr, "<magenta>There %s <yellow>%d<magenta> friend%s online at <yellow>%02d:%02d\n",
		(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
		tm->tm_hour, tm->tm_min);

	short_who_list(usr, pq);

	destroy_PQueue(pq);

	read_text(usr);
	Return;
}

/*
	originally implemented by Richard of MatrixBBS
	(basically the same as the online friendslist)

	Note how I use make_talked_to() ... the people we talked to
	are stored in the X history buffer, so use that to construct
	a talked_to list
*/
void talked_list(User *usr) {
User *u;
struct tm *tm;
StringList *talked_to, *sl;
PQueue *pq;
int total;

	if (usr == NULL)
		return;

	Enter(talked_list);

	if (usr->history == NULL || (talked_to = make_talked_to(usr)) == NULL) {
		Put(usr, "<red>You haven't talked to anyone yet\n");
		CURRENT_STATE(usr);
		Return;
	}
	if ((pq = new_PQueue()) == NULL) {
		Perror(usr, "Out of memory");
		Return;
	}
	for(sl = talked_to; sl != NULL; sl = sl->next) {
		if ((u = is_online(sl->str)) == NULL)
			continue;

		(void)add_PQueue(pq, new_PList(u));
	}
	if (count_Queue(pq) <= 0) {
		Put(usr, "<red>Nobody you talked to is online anymore\n");
		destroy_PQueue(pq);
		CURRENT_STATE(usr);
		Return;
	}
	sort_PQueue(pq, (usr->flags & USR_SORT_DESCENDING) ? sort_who_desc_byname : sort_who_asc_byname);
	total = count_Queue(pq);

/* construct header */
	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	buffer_text(usr);
	Print(usr, "<magenta>There %s <yellow>%d<magenta> %s you talked to online at <yellow>%02d:%02d\n",
		(total == 1) ? "is" : "are", total, (total == 1) ? "person" : "people",
		tm->tm_hour, tm->tm_min);

	short_who_list(usr, pq);

	destroy_PQueue(pq);
	listdestroy_StringList(talked_to);

	read_text(usr);
	Return;
}

void print_quicklist(User *usr) {
int i;

	Enter(print_quicklist);

	for(i = 1; i < 5; i++) {
		if (usr->quick[i-1] == NULL)
			Print(usr, "<hotkey>%d<red> %-26s ", i, "<empty>");
		else
			Print(usr, "<hotkey>%d<white> %-26s ", i, usr->quick[i-1]);

		i += 5;
		if (usr->quick[i-1] == NULL)
			Print(usr, "<hotkey>%d<red> %s\n", i, "<empty>");
		else
			Print(usr, "<hotkey>%d<white> %s\n", i, usr->quick[i-1]);
		i -= 5;
	}
	if (usr->quick[i-1] == NULL)
		Print(usr, "<hotkey>%d<red> %-26s ", i, "<empty>");
	else
		Print(usr, "<hotkey>%d<white> %-26s ", i, usr->quick[i-1]);

	if (usr->quick[9] == NULL)
		Print(usr, "<hotkey>0<red> %s\n", "<empty>");
	else
		Print(usr, "<hotkey>0<white> %s\n", usr->quick[i-1]);
	Return;
}

static int print_worldclock(User *usr, int item, char *buf, int buflen) {
struct tm *t, ut;
char zone_color[MAX_COLORBUF], zone_color2[MAX_COLORBUF];

/*
	light up the timezone that this user is in
	btw, it is possible that the time is the same but they are not
	exactly in the same zone. Light it up anyway, because it shows
	the correct current time for this user
*/
	memcpy(&ut, user_time(usr, (time_t)0UL), sizeof(struct tm));

	t = tz_time(worldclock[item].tz, (time_t)0UL);
	if (worldclock[item].tz == NULL) {
		t->tm_hour = 0;			/* reset, don't even display GMT because this is the world clock */
		t->tm_min = 0;
	}
	if (t->tm_hour == ut.tm_hour && t->tm_min == ut.tm_min) {
		cstrcpy(zone_color, "white", MAX_COLORBUF);
		cstrcpy(zone_color2, "white", MAX_COLORBUF);
	} else {
		cstrcpy(zone_color, "cyan", MAX_COLORBUF);
		cstrcpy(zone_color2, "yellow", MAX_COLORBUF);
	}
	if (usr->flags & USR_12HRCLOCK) {
		char am_pm = 'A';

		if (t->tm_hour >= 12) {
			am_pm = 'P';
			if (t->tm_hour > 12)
				t->tm_hour -= 12;
		}
		return bufprintf(buf, buflen, "<cyan>%-15s <%s>%02d:%02d %cM",
			(worldclock[item].name == NULL) ? "" : worldclock[item].name,
			zone_color2, t->tm_hour, t->tm_min, am_pm);
	}
	return bufprintf(buf, buflen, "<cyan>%-15s <%s>%02d:%02d",
		(worldclock[item].name == NULL) ? "" : worldclock[item].name,
		zone_color2, t->tm_hour, t->tm_min);
}

void print_calendar(User *usr) {
time_t t;
struct tm *tmp;
int w, d, today, today_month, today_year, bday_day, bday_mon, bday_year, old_month, green_color, l;
char date_buf[MAX_LINE], line[PRINT_BUF];

	if (usr == NULL)
		return;

	Enter(print_calendar);

	Print(usr, "<magenta>Current time is<yellow> %s %s\n", print_date(usr, (time_t)0UL, date_buf, MAX_LINE), name_Timezone(usr->tz));

	if (!PARAM_HAVE_CALENDAR && !PARAM_HAVE_WORLDCLOCK) {
		Return;
	}
	tmp = user_time(usr, (time_t)0UL);
	w = tmp->tm_wday;
	today = tmp->tm_mday;
	today_month = tmp->tm_mon;
	today_year = tmp->tm_year;

	tmp = user_time(usr, usr->birth);
	bday_day = tmp->tm_mday;
	bday_mon = tmp->tm_mon;
	bday_year = tmp->tm_year;

	Put(usr, "\n");
	l = 0;
	if (PARAM_HAVE_CALENDAR)
		l += bufprintf(line, sizeof(line), "<magenta>  S  M Tu  W Th  F  S");

	if (PARAM_HAVE_WORLDCLOCK) {
		if (PARAM_HAVE_CALENDAR)
			l += bufprintf(line+l, sizeof(line) - l, "    ");

		l += print_worldclock(usr, 0, line+l, sizeof(line) - l);
		l += bufprintf(line+l, sizeof(line) - l, "    ");
		l += print_worldclock(usr, 1, line+l, sizeof(line) - l);
	}
	line[l++] = '\n';
	line[l] = 0;
	Put(usr, line);

	line[0] = 0;
	l = 0;

	t = rtc - (14 + w) * SECS_IN_DAY;
	tmp = user_time(usr, t);
	old_month = tmp->tm_mon;
	green_color = 1;

	if (PARAM_HAVE_CALENDAR)
		l += bufprintf(line, sizeof(line), "<green>");

	for(w = 0; w < 5; w++) {
		if (PARAM_HAVE_CALENDAR) {
			l += bufprintf(line+l, sizeof(line) - l, (green_color == 0) ? "<yellow>" : "<green>");

			for(d = 0; d < 7; d++) {
				tmp = user_time(usr, t);

/* highlight today and bbs birthday */
				if  (tmp->tm_mday == today && tmp->tm_mon == today_month && tmp->tm_year == today_year)
					l += bufprintf(line+l, sizeof(line) - l, "<white> %2d<%s>", tmp->tm_mday, (green_color == 0) ? "yellow" : "green");
				else {
					if (tmp->tm_mday == bday_day && tmp->tm_mon == bday_mon && tmp->tm_year > bday_year)
						l += bufprintf(line+l, sizeof(line) - l, "<magenta> %2d<%s>", tmp->tm_mday, (green_color == 0) ? "yellow" : "green");
					else {
						if (old_month != tmp->tm_mon) {
							green_color ^= 1;
							l += bufprintf(line+l, sizeof(line) - l, (green_color == 0) ? "<yellow>" : "<green>");
							old_month = tmp->tm_mon;
						}
						l += bufprintf(line+l, sizeof(line) - l, " %2d", tmp->tm_mday);
					}
				}
				t += SECS_IN_DAY;
			}
		}
		if (PARAM_HAVE_WORLDCLOCK) {
			if (PARAM_HAVE_CALENDAR)
				l += bufprintf(line+l, sizeof(line) - l, "    ");

			l += print_worldclock(usr, w+2, line+l, sizeof(line) - l);
			l += bufprintf(line+l, sizeof(line) - l, "    ");
			l += print_worldclock(usr, w+7, line+l, sizeof(line) - l);
		}
		line[l++] = '\n';
		line[l] = 0;

		Put(usr, line);

		line[0] = 0;
		l = 0;
	}
	Return;
}

void drop_sysop_privs(User *usr) {
	if (usr->runtime_flags & RTF_SYSOP) {
		usr->runtime_flags &= ~RTF_SYSOP;
		Print(usr, "<white>Exiting %s mode", PARAM_NAME_SYSOP);

		log_msg("SYSOP %s left %s mode", usr->name, PARAM_NAME_SYSOP);

/* silently disable roomaide mode, if not allowed to be roomaide here */
		if ((usr->runtime_flags & RTF_ROOMAIDE) && in_StringList(usr->curr_room->room_aides, usr->name) == NULL)
			usr->runtime_flags &= ~RTF_ROOMAIDE;
	}
}

void state_download_text(User *usr, char c) {
char buf[MAX_LONGLINE], *p;
int cpos, lines;

	if (usr == NULL)
		return;

	Enter(state_download_text);

	switch(c) {
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case 'q':
		case 'Q':
			wipe_line(usr);
			Put(usr, "<red>Download aborted\n");
			RET(usr);
			Return;

		case INIT_STATE:
			rewind_StringIO(usr->text);
			Put(usr, "<green>");

		default:
			if (c != INIT_STATE && !(usr->flags & USR_NOPAGE_DOWNLOADS))
				Put(usr, "<green>\n");

			cpos = lines = 0;
			while(read_StringIO(usr->text, buf, MAX_LONGLINE) > 0) {
				p = buf;
				while(*p) {
					switch(*p) {
						case '\n':
							cpos = 0;
							lines++;
							write_StringIO(usr->conn->output, "\r\n", 2);
							break;

						case '^':
							write_StringIO(usr->conn->output, "^^", 2);
							cpos += 2;
							break;

						default:
							if (*p < ' ') {
								char colorbuf[MAX_COLORBUF];

								short_color_to_long(*p, colorbuf, MAX_COLORBUF, usr->flags & USR_SHORT_DL_COLORS);
								put_StringIO(usr->conn->output, colorbuf);
								cpos += strlen(colorbuf);
							} else {
								write_StringIO(usr->conn->output, p, 1);
								cpos++;
							}
					}
					if (cpos >= usr->display->term_width) {
						cpos -= usr->display->term_width;
						lines++;
					}
					p++;
					if (!(usr->flags & USR_NOPAGE_DOWNLOADS) && lines >= usr->display->term_height-1) {
						seek_StringIO(usr->text, -strlen(p), STRINGIO_CUR);
						Put(usr, "<white>");
						Put(usr, "[Press a key]");
						Return;
					}
				}
			}
			JMP(usr, STATE_PRESS_ANY_KEY);
	}
	Return;
}

void print_reboot_status(User *usr) {
char timebuf[MAX_LINE];
int secs, secs2;
Timer *t;

	if (usr == NULL)
		return;

	Enter(print_reboot_status);

	secs = -1;
	if (shutdown_timer != NULL) {
		t = shutdown_timer;
		secs = time_to_dd(shutdown_timer);

		if (reboot_timer != NULL) {
			secs2 = time_to_dd(reboot_timer);
			if (secs2 < secs) {
				t = reboot_timer;
				secs = secs2;
			}
		}
	} else {
		if (reboot_timer != NULL) {
			t = reboot_timer;
			secs = time_to_dd(reboot_timer);

			if (shutdown_timer != NULL) {
				secs2 = time_to_dd(shutdown_timer);
				if (secs2 < secs) {
					t = shutdown_timer;
					secs = secs2;
				}
			}
		} else {
			Return;
		}
	}
	if (secs < 0)
		timebuf[0] = 0;
	else {
		cstrcpy(timebuf, "in ", MAX_LINE);
		print_total_time((unsigned long)secs, timebuf+3, MAX_LINE-3);
		if (!timebuf[3])
			timebuf[0] = 0;
	}
	if (t == shutdown_timer)
		Print(usr, "\n<white>NOTE: <red>The system is shutting down %s\n", timebuf);
	else
		Print(usr, "\n<white>NOTE: <red>The system is rebooting %s\n", timebuf);

	Return;
}

/* EOB */
