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
			Put(usr, "<white>GNU General Public License\n");
			if (load_screen(usr->text, PARAM_GPL_SCREEN) < 0) {
				Put(usr, "<red>The GPL file is missing\n");		/* or out of memory! */
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
				Put(usr, "\n<red>The local mods file is missing\n");		/* or out of memory! */
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
			if (load_StringIO(usr->text, PARAM_CREDITS_SCREEN) < 0) {
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
				listdestroy_StringList(usr->recipients);
				if (usr->message->anon[0]) {
					if (usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE))
						usr->recipients = new_StringList(usr->message->from);
					else
						usr->recipients = NULL;
				} else
					usr->recipients = new_StringList(usr->message->from);
			}
			enter_name(usr, STATE_PROFILE_USER);
			Return 1;

		case 'X':
			Put(usr, "<white>Toggle message reception\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot enable message reception\n", PARAM_NAME_GUEST);
				break;
			}
			usr->flags ^= USR_X_DISABLED;
			Print(usr, "<magenta>Message reception is now turned <yellow>%s\n", (usr->flags & USR_X_DISABLED) ? "off" : "on");

			if (usr->flags & USR_X_DISABLED) {
				if (usr->flags & USR_HELPING_HAND) {
					usr->flags &= ~USR_HELPING_HAND;
					Put(usr, "<magenta>You are no longer available to help others\n");
					usr->runtime_flags |= RTF_WAS_HH;
				}
			} else {
				if (usr->runtime_flags & RTF_WAS_HH) {
					usr->flags |= USR_HELPING_HAND;
					usr->runtime_flags &= ~RTF_WAS_HH;
					Put(usr, "<magenta>You are now available to help others\n");
				}
			}
			break;

		case '$':
			if (usr->runtime_flags & RTF_SYSOP) {
				drop_sysop_privs(usr);
				Put(usr, "\n");
				break;
			}
			if (get_su_passwd(usr->name) != NULL) {
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
char version_buf[256];

	Print(usr, "<yellow>This is <white>%s<yellow>, %s", PARAM_BBS_NAME,
		print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL, version_buf));

	if (*patchlist)
		Print(usr, "<green>Patches: <white>%s\n", patchlist);
}

void enter_recipients(User *usr, void (*state_func)(User *, char)) {
	if (usr == NULL)
		return;

	Enter(enter_recipients);

	if (usr->recipients == NULL)
		Put(usr, "<green>Enter recipient: <yellow>");
	else {
		if (usr->recipients->next == NULL)
			Print(usr, "<green>Enter recipient <white>[<yellow>%s<white>]:<yellow> ", usr->recipients->str);
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

	if (usr->recipients == NULL)
		Put(usr, "<green>Enter name: <yellow>");
	else {
		if (usr->recipients->next != NULL) {
			usr->recipients->next->prev = NULL;
			listdestroy_StringList(usr->recipients->next);
			usr->recipients->next = NULL;
		}
		Print(usr, "<green>Enter name <white>[<yellow>%s<white>]:<yellow> ", usr->recipients->str);
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
			if (usr->recipients == NULL) {
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
			if (usr->recipients == NULL) {
				RET(usr);
				break;
			}
			JMP(usr, STATE_EDIT_EMOTE);
	}
	Return;
}

void state_feelings_prompt(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_feelings_prompt);

	switch(edit_recipients(usr, c, multi_x_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			check_recipients(usr);
			if (usr->recipients == NULL) {
				RET(usr);
				break;
			}
			JMP(usr, STATE_CHOOSE_FEELING);
	}
	Return;
}

void state_ping_prompt(User *usr, char c) {
StringList *sl;

	if (usr == NULL)
		return;

	Enter(state_ping_prompt);

	switch(edit_recipients(usr, c, multi_ping_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			if ((sl = in_StringList(usr->recipients, usr->name)) != NULL) {
				remove_StringList(&usr->recipients, sl);
				destroy_StringList(sl);

				if (usr->recipients == NULL)
					Put(usr, "<red>You are keeping yourself busy pinging yourself\n");
			}
			if (usr->recipients == NULL) {
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
		usr->conn->loop_counter = list_Count(usr->recipients);
		usr->conn->state |= CONN_LOOPING;
	} else {
		StringList *sl;
		User *u;
		unsigned long i, tdiff;

		sl = usr->recipients;
		if (sl == NULL) {
			usr->conn->loop_counter = 0UL;
			Return;
		}
		for(i = 0UL; i < usr->conn->loop_counter; i++) {		/* do the next recipient */
			sl = sl->next;
			if (sl == NULL) {
				usr->conn->loop_counter = 0UL;
				Return;
			}
		}
		if ((u = is_online(sl->str)) == NULL) {
			Print(usr, "<yellow>%s <red>suddenly logged off!\n", sl->str);
			remove_StringList(&usr->recipients, sl);
			destroy_StringList(sl);
			Return;
		}
		if (u->runtime_flags & RTF_LOCKED) {
			if (u->away != NULL && u->away[0])
				Print(usr, "<yellow>%s<green> is away from the terminal for a while; %s\n", u->name, u->away);
			else
				Print(usr, "<yellow>%s<green> is away from the terminal for a while\n", u->name);
		} else {
			if (u->runtime_flags & RTF_BUSY) {
				if ((u->runtime_flags & RTF_BUSY_SENDING)
					&& in_StringList(u->recipients, usr->name) != NULL)
/*
	the warn follow-up mode feature was donated by Richard of MatrixBBS
*/
					Print(usr, "<yellow>%s<green> is busy sending you a message%s\n",
						u->name, (PARAM_HAVE_FOLLOWUP && (u->flags & USR_FOLLOWUP)) ? " in follow-up mode" : "");
				else {
					if ((u->runtime_flags & RTF_BUSY_MAILING)
						&& u->new_message != NULL
						&& in_StringList(u->new_message->to, usr->name) != NULL)
						Print(usr, "<yellow>%s<green> is busy mailing you a message\n", u->name);
					else
						if (PARAM_HAVE_HOLD && (u->runtime_flags & RTF_HOLD)) {
							if (u->away != NULL && u->away[0])
								Print(usr, "<yellow>%s<green> has put messages on hold; %s\n", u->name, u->away);
							else
								Print(usr, "<yellow>%s<green> has put messages on hold\n", u->name);
						} else
							Print(usr, "<yellow>%s<green> is busy\n", u->name);
				}
			} else
				if (PARAM_HAVE_HOLD && (u->runtime_flags & RTF_HOLD)) {
					if (u->away != NULL && u->away[0])
						Print(usr, "<yellow>%s<green> has put messages on hold; %s\n", u->name, u->away);
					else
						Print(usr, "<yellow>%s<green> has put messages on hold\n", u->name);
				} else
					Print(usr, "<yellow>%s<green> is not busy\n", u->name);

/*
	in case a user is idling, print it
	(hardcoded) default is after 2 minutes
*/
			tdiff = (unsigned long)rtc - (unsigned long)u->idle_time;
			if (tdiff >= 2UL * SECS_IN_MIN) {
				char total_buf[MAX_LINE];

				Print(usr, "<yellow>%s<green> is idle for %s\n", u->name, print_total_time(tdiff, total_buf));
			}
		}
/*
		if (in_StringList(u->friends, usr->name) != NULL)
			Print(usr, "You are on %s\n", possession(u->name, "friend list", name_buf));
		if (in_StringList(u->enemies, usr->name) != NULL)
			Print(usr, "<red>You are on %s\n", possession(u->name, "enemy list", name_buf));
*/
	}
	Return;
}

void state_profile_user(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_profile_user);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		User *u = NULL;
		int allocated = 0, visible;
		char total_buf[MAX_LINE], *p, *hidden;

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

			if ((u = is_online(usr->edit_buf)) != NULL) {
				Print(usr, "<green>Online for <cyan>%s\n", print_total_time((unsigned long)rtc - (unsigned long)u->login_time, total_buf));
				if (u == usr || (usr->runtime_flags & RTF_SYSOP)) {
					if (usr->runtime_flags & RTF_SYSOP)
						Print(usr, "<green>From host: <yellow>%s <white>[%s]\n", u->conn->hostname, u->conn->ipnum);
					else
						Print(usr, "<green>From host: <yellow>%s\n", u->conn->hostname);
				}
				if ((p = HostMap_desc(u->conn->ipnum)) != NULL)
					Print(usr, "<yellow>%s<green> is connected from <yellow>%s\n", usr->edit_buf, p);
			}
			RET(usr);
			Return;
		}
		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			RET(usr);
			Return;
		}
		if ((u = is_online(usr->edit_buf)) == NULL) {
			listdestroy_StringList(usr->recipients);	/* don't put name in history */
			usr->recipients = NULL;

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
			strcpy(u->conn->hostname, u->tmpbuf[TMP_FROM_HOST]);
		} else {
			load_profile_info(u);

			listdestroy_StringList(usr->recipients);		/* place entered name in history */
			usr->recipients = new_StringList(usr->edit_buf);
		}
/*
	make the profile
*/
		buffer_text(usr);
		Put(usr, "<white>");

		if (PARAM_HAVE_VANITY && u->vanity != NULL && u->vanity[0]) {
			char fmt[16];

			sprintf(fmt, "%%-%ds", MAX_NAME + 10);
			Print(usr, fmt, u->name);
			Print(usr, "<magenta>* <white>%s <magenta>*", u->vanity);
		} else
			Put(usr, u->name);

		Put(usr, "\n");

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
		if (allocated) {
			char date_buf[MAX_LINE], online_for[MAX_LINE+10];

			if (u->last_online_time > 0UL) {
				int l;

				l = sprintf(online_for, "%c for %c", color_by_name("green"), color_by_name("yellow"));
				print_total_time(u->last_online_time, online_for+l);
			} else
				online_for[0] = 0;

			Print(usr, "<green>Last online: <cyan>%s%s\n", print_date(usr, (time_t)u->last_logout, date_buf), online_for);
			if (usr->runtime_flags & RTF_SYSOP)
				Print(usr, "<green>From host: <yellow>%s <white>[%s]\n", u->conn->hostname, u->tmpbuf[TMP_FROM_IP]);

			if ((p = HostMap_desc(u->conn->ipnum)) != NULL)
				Print(usr, "<yellow>%s<green> was connected from <yellow>%s\n", u->name, p);
		} else {
/*
	display for how long someone is online
*/
			Print(usr, "<green>Online for <cyan>%s\n", print_total_time(rtc - u->login_time, total_buf));
			if (u == usr || (usr->runtime_flags & RTF_SYSOP)) {
				if (usr->runtime_flags & RTF_SYSOP)
					Print(usr, "<green>From host: <yellow>%s <white>[%s]\n", u->conn->hostname, u->conn->ipnum);
				else
					Print(usr, "<green>From host: <yellow>%s\n", u->conn->hostname);
			}
			if ((p = HostMap_desc(u->conn->ipnum)) != NULL)
				Print(usr, "<yellow>%s<green> is connected from <yellow>%s\n", u->name, p);
		}
		if (!allocated)
			update_stats(u);
		Print(usr, "<green>Total online time: <yellow>%s\n", print_total_time(u->total_time, total_buf));

		if (u->flags & USR_X_DISABLED)
			Print(usr, "<red>%s has message reception turned off\n", u->name);

		if (in_StringList(u->friends, usr->name) != NULL) {
			char namebuf[MAX_NAME+20];

			sprintf(namebuf, "<yellow>%s<green>", u->name);
			Print(usr, "<green>You are on %s\n", possession(namebuf, "friend list", total_buf));
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
		if (usr->message != NULL && usr->message->anon[0]
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
	}
	Return;
}

void loop_send_msg(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(loop_send_msg);

	if (c == INIT_STATE) {
		usr->conn->state |= CONN_LOOPING;
		usr->conn->loop_counter = list_Count(usr->recipients);
/*
	the message that you send to yourself won't be received... but you do
	get a copy in your X history buffer
*/
		if (usr->conn->loop_counter == 1 && !strcmp(usr->recipients->str, usr->name))
			Print(usr, "<green>Talking to yourself, are you?\n");
	} else {
		StringList *sl;
		User *u;
		unsigned long i;

		if (usr->send_msg == NULL) {
			Perror(usr, "The message has disappeared!");
			usr->conn->loop_counter = 0UL;
			Return;
		}
		sl = usr->recipients;
		if (sl != NULL) {
			for(i = 0UL; i < usr->conn->loop_counter; i++) {		/* do the next recipient */
				sl = sl->next;
				if (sl == NULL)
					break;
			}
		}
		if (sl != NULL) {
			if ((u = is_online(sl->str)) == NULL) {
				Print(usr, "<red>Sorry, but <yellow>%s<red> left before you could finish typing!\n", sl->str);
				if (PARAM_HAVE_MAILROOM)
					CALL(usr, STATE_MAIL_SEND_MSG);
				Return;
			} else {
				if (u->runtime_flags & RTF_LOCKED) {
					Print(usr, "<red>Sorry, but <yellow>%s<red> suddenly locked the terminal\n", sl->str);
					if (PARAM_HAVE_MAILROOM)
						CALL(usr, STATE_MAIL_SEND_MSG);
					Return;
				} else {
					if (!(usr->runtime_flags & RTF_SYSOP)
						&& (u->flags & USR_X_DISABLED)
						&& (in_StringList(u->friends, usr->name) == NULL)) {
						Print(usr, "<red>Sorry, but <yellow>%s<red> suddenly disabled message reception\n", sl->str);
						if (PARAM_HAVE_MAILROOM)
							CALL(usr, STATE_MAIL_SEND_MSG);
						Return;
					}
				}
				recvMsg(u, usr, usr->send_msg);			/* deliver the message */
			}
		}
		if (!usr->conn->loop_counter) {
/*
	destroy the copy of the message we just sent
	there are conditions in which this code is never reached... while this is
	considered a bug, it doesn't really matter because usr->send_msg is
	destroyed in other places as well
*/
			destroy_BufferedMsg(usr->send_msg);
			usr->send_msg = NULL;
		}
	}
	Return;
}

/*
	Note: this state is called from within a loop
*/
void state_mail_send_msg(User *usr, char c) {
StringList *sl;

	if (usr == NULL)
		return;

	Enter(state_mail_send_msg);

	if (c == INIT_STATE) {
		usr->conn->state &= ~CONN_LOOPING;
		usr->runtime_flags |= RTF_BUSY;

		Put(usr, "<cyan>Do you wish to <yellow>Mail><cyan> the message? (Y/n): ");
		Return;
	}
	switch(yesno(usr, c, 'Y')) {
		case YESNO_YES:
			mail_msg(usr, usr->send_msg);
			break;

		case YESNO_NO:
			break;

		case YESNO_UNDEF:
			CURRENT_STATE(usr);
			Return;
	}
/*
	The user was already gone, so remove it from the recipient list
	This actually works and does not give any problems, but only because the
	loop is done from end to beginning
*/
	sl = usr->recipients;
	if (sl != NULL) {
		unsigned long i;

		for(i = 0UL; i < usr->conn->loop_counter; i++) {
			sl = sl->next;
			if (sl == NULL)
				break;
		}
		if (sl != NULL) {
			remove_StringList(&usr->recipients, sl);
			destroy_StringList(sl);
		}
	}
	usr->conn->state |= CONN_LOOPING;
	POP(usr);
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

		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		if (!usr->edit_buf[0]) {
			Put(usr, "<red>Nothing entered, so no message was sent\n");
			RET(usr);
			Return;
		}
		if ((msg = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((msg->to = copy_StringList(usr->recipients)) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		put_StringIO(msg->msg, usr->edit_buf);
		write_StringIO(msg->msg, "\n", 1);

		strcpy(msg->from, usr->name);
		msg->mtime = rtc;

		msg->flags = BUFMSG_EMOTE;
		if (usr->runtime_flags & RTF_SYSOP)
			msg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, msg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(msg);

/* update stats */
		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->esent++;
			if (usr->esent > stats.esent) {
				stats.esent = usr->esent;
				strcpy(stats.most_esent, usr->name);
			}
			stats.esent_boot++;
		}
		JMP(usr, LOOP_SEND_MSG);
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

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (!usr->edit_buf[0] && usr->text->buf == NULL) {
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
		if ((xmsg->to = copy_StringList(usr->recipients)) == NULL) {
			destroy_BufferedMsg(xmsg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		copy_StringIO(xmsg->msg, usr->text);

		strcpy(xmsg->from, usr->name);
		if (PARAM_HAVE_XMSG_HDR && usr->xmsg_header != NULL && usr->xmsg_header[0])
			xmsg->xmsg_header = cstrdup(usr->xmsg_header);

		xmsg->mtime = rtc;

		xmsg->flags = BUFMSG_XMSG;
		if (usr->runtime_flags & RTF_SYSOP)
			xmsg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, xmsg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(xmsg);

/* update stats */
		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->xsent++;
			if (usr->xsent > stats.xsent) {
				stats.xsent = usr->xsent;
				strcpy(stats.most_xsent, usr->name);
			}
			stats.xsent_boot++;
		}
		JMP(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_choose_feeling(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_choose_feeling);

	if (c == INIT_STATE) {
		usr->runtime_flags |= (RTF_BUSY | RTF_BUSY_SENDING);

		make_feelings_screen(usr->text, usr->display->term_width);
		if (usr->text == NULL || usr->text->buf == NULL || !usr->text->len) {
			Perror(usr, "The feelings are temporarily unavailable");
			RET(usr);
			Return;
		}
		display_text(usr, usr->text);
		free_StringIO(usr->text);
		Put(usr, "\n<green>Feeling: <yellow>");
	}
	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		usr->runtime_flags &= ~(RTF_BUSY | RTF_BUSY_SENDING);
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		BufferedMsg *msg;
		KVPair *f;
		File *file;
		char *filename;
		int num;

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		num = atoi(usr->edit_buf);
		if (num <= 0) {
			Put(usr, "<red>No such feeling\n");
			RET(usr);
			Return;
		}
		f = feelings;
		num--;
		while(num > 0 && f != NULL) {
			num--;
			f = f->next;
		}
		if (f == NULL) {
			Put(usr, "<red>No such feeling\n");
			RET(usr);
			Return;
		}

/* send feeling */

		if ((msg = new_BufferedMsg()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((msg->to = copy_StringList(usr->recipients)) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((filename = (char *)KVPair_getpointer(f)) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "This feeling has vanished");
			RET(usr);
			Return;
		}
		if ((file = Fopen(filename)) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "The feeling has passed");
			RET(usr);
			Return;
		}
		if (Fget_StringIO(file, msg->msg) < 0) {
			Fclose(file);
			destroy_BufferedMsg(msg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		Fclose(file);

		strcpy(msg->from, usr->name);
		msg->mtime = rtc;

		msg->flags = BUFMSG_FEELING;
		if (usr->runtime_flags & RTF_SYSOP)
			msg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, msg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(msg);

/* update stats */
		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->fsent++;
			if (usr->fsent > stats.fsent) {
				stats.fsent = usr->fsent;
				strcpy(stats.most_fsent, usr->name);
			}
			stats.fsent_boot++;
		}
		JMP(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_edit_question(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_edit_question);

	if (c == INIT_STATE) {
		if (check_helping_hand(usr) == NULL) {
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

		if (!usr->edit_buf[0] && usr->text->buf == NULL) {
			Put(usr, "<red>Nothing entered, so no question was asked\n");
			RET(usr);
			Return;
		}

/* send question */

		if ((u = check_helping_hand(usr)) == NULL) {
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

		strcpy(question->from, usr->name);
		question->mtime = rtc;

		question->flags = BUFMSG_QUESTION;
		if (usr->runtime_flags & RTF_SYSOP)
			question->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, question);
		recvMsg(u, usr, question);				/* the question is asked! */
		stats.qsent_boot++;

		usr->qsent++;
		if (usr->qsent > stats.qsent) {
			stats.qsent = usr->qsent;
			strcpy(stats.most_qsent, usr->name);
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

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (!usr->edit_buf[0] && usr->text->buf == NULL) {
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
		if ((xmsg->to = copy_StringList(usr->recipients)) == NULL) {
			destroy_BufferedMsg(xmsg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		copy_StringIO(xmsg->msg, usr->text);

		strcpy(xmsg->from, usr->name);

/* Note: no custom X headers here */

		xmsg->mtime = rtc;

		xmsg->flags = BUFMSG_ANSWER;
		if (usr->runtime_flags & RTF_SYSOP)
			xmsg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, xmsg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = ref_BufferedMsg(xmsg);

		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->qansw++;
			if (usr->qansw > stats.qansw) {
				stats.qansw = usr->qansw;
				strcpy(stats.most_qansw, usr->name);
			}
			stats.qansw_boot++;
		}
		JMP(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_su_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_su_prompt);

	if (c == INIT_STATE)
		Print(usr, "<green>Enter password: ", PARAM_NAME_SYSOP);

	r = edit_password(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd;

		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {
			Print(usr, "\n\n<red>You are not allowed to become <yellow>%s<red> any longer\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;
		}
		if (!verify_phrase(usr->edit_buf, pwd)) {
			usr->runtime_flags |= RTF_SYSOP;
			Print(usr, "\n<red>*** <white>NOTE: You are now in <yellow>%s<white> mode <red>***\n", PARAM_NAME_SYSOP);

			log_msg("SYSOP %s entered %s mode", usr->name, PARAM_NAME_SYSOP);
		} else {
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
PList *proot = NULL;
User *u;
int (*sort_func)(void *, void *);
int total;

	if (usr == NULL)
		return;

	Enter(who_list);

/* construct PList of Users (optimized for speed) */

	if ((format & WHO_LIST_ROOM) || (usr->curr_room != NULL
		&& (usr->curr_room->flags & ROOM_CHATROOM) && !(usr->flags & USR_SHOW_ALL))) {
		PList *p;

		for(p = usr->curr_room->inside; p != NULL; p = p->next) {
			u = (User *)p->p;
			if (u == NULL)
				continue;

			if (!u->name[0])
				continue;

			if (u->curr_room != usr->curr_room)
				continue;

			if (!(usr->flags & USR_SHOW_ENEMIES) && in_StringList(usr->enemies, u->name) != NULL)
				continue;

			proot = add_PList(&proot, new_PList(u));
		}
	} else {
		for(u = AllUsers; u != NULL; u = u->next) {
			if (!u->name[0])
				continue;

			if (!(usr->flags & USR_SHOW_ENEMIES) && in_StringList(usr->enemies, u->name) != NULL)
				continue;

			proot = add_PList(&proot, new_PList(u));
		}
	}
	proot = rewind_PList(proot);

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
	proot = sort_PList(proot, sort_func);

	buffer_text(usr);

	total = list_Count(proot);
	who_list_header(usr, total, format);

	if (format & WHO_LIST_LONG)
		long_who_list(usr, proot);
	else
		short_who_list(usr, proot, total);

	listdestroy_PList(proot);		/* destroy temp list */

	read_text(usr);					/* display the who list */
	Return;
}

/*
	construct a long format who list
*/
int long_who_list(User *usr, PList *pl) {
int hrs, mins, l, c, width;
unsigned long time_now;
time_t t;
char buf[PRINT_BUF], buf2[PRINT_BUF], col, stat;
User *u;

	if (usr == NULL)
		return 0;

	Enter(long_who_list);

	time_now = rtc;

	while(pl != NULL) {
		u = (User *)pl->p;

		pl = pl->next;

		if (u == NULL)
			continue;

		stat = ' ';
		col = (char)color_by_name("yellow");

		if (u == usr)
			col = (char)color_by_name("white");
		else
			if (in_StringList(u->enemies, usr->name) != NULL
				|| ((usr->flags & USR_SHOW_ENEMIES)
					&& in_StringList(usr->enemies, u->name) != NULL)) {

				col = (char)color_by_name("red");
				if (!(usr->flags & USR_ANSI))
					stat = '-';				/* indicate enemy /wo colors */
			} else
				if (in_StringList(usr->friends, u->name) != NULL) {
					col = (char)color_by_name("green");
					if (!(usr->flags & USR_ANSI))
						stat = '+';			/* indicate friend /wo colors */
				}

		if (u->flags & USR_HELPING_HAND)
			stat = '%';

		if (u->runtime_flags & RTF_SYSOP)
			stat = '$';

		if (u->runtime_flags & RTF_HOLD)
			stat = 'b';

		if (u->flags & USR_X_DISABLED)
			stat = '*';

		if (u->runtime_flags & RTF_LOCKED)
			stat = '#';
/*
	you can see it when someone is X-ing or Mail>-ing you
	this is after a (good) idea by Richard of MatrixBBS
*/
		if ((u->runtime_flags & RTF_BUSY_SENDING) && in_StringList(u->recipients, usr->name) != NULL)
			stat = 'x';

		if ((u->runtime_flags & RTF_BUSY_MAILING) && u->new_message != NULL && in_StringList(u->new_message->to, usr->name) != NULL)
			stat = 'm';

		t = time_now - u->login_time;
		hrs = t / SECS_IN_HOUR;
		t %= SECS_IN_HOUR;
		mins = t / SECS_IN_MIN;

/* 36 is the length of the string with the stats that's to be added at the end, plus margin */
		width = (usr->display->term_width > (PRINT_BUF-36)) ? (PRINT_BUF-36) : usr->display->term_width;

		if (u->doing == NULL || !u->doing[0])
			sprintf(buf, "%c%s<cyan>", col, u->name);
		else {
			sprintf(buf, "%c%s <cyan>%s", col, u->name, u->doing);
			expand_center(buf, buf2, PRINT_BUF - 36, width);
			expand_hline(buf2, buf, PRINT_BUF - 36, width);
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

	total should equal list_Count(pl)
*/
int short_who_list(User *usr, PList *pl, int total) {
int i, j, buflen = 0, cols, rows;
char buf[PRINT_BUF], col, stat;
User *u;
PList *pl_cols[16];

	if (usr == NULL || pl == NULL || pl->p == NULL)
		return 0;

	Enter(short_who_list);

	cols = usr->display->term_width / (MAX_NAME+2);
	if (cols < 1)
		cols = 1;
	else
		if (cols > 15)
			cols = 15;

	rows = total / cols;
	if (total % cols)
		rows++;

	memset(pl_cols, 0, sizeof(PList *) * cols);

/* fill in array of pointers to columns */

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

/* add who-list entry for User *u */
			stat = ' ';
			col = (char)color_by_name("yellow");

			if (u == usr)
				col = (char)color_by_name("white");
			else {
				if (in_StringList(u->enemies, usr->name) != NULL
					|| ((usr->flags & USR_SHOW_ENEMIES)
						&& in_StringList(usr->enemies, u->name) != NULL)) {

					col = (char)color_by_name("red");
					if (!(usr->flags & USR_ANSI))
						stat = '-';				/* indicate enemy /wo colors */
				} else {
					if (in_StringList(usr->friends, u->name) != NULL) {
						col = (char)color_by_name("green");
						if (!(usr->flags & USR_ANSI))
							stat = '+';			/* indicate friend /wo colors */
					}
				}
			}
			if (u->flags & USR_HELPING_HAND)
				stat = '%';

			if (u->runtime_flags & RTF_SYSOP)
				stat = '$';

			if (u->runtime_flags & RTF_HOLD)
				stat = 'b';

			if (u->flags & USR_X_DISABLED)
				stat = '*';

			if (u->runtime_flags & RTF_LOCKED)
				stat = '#';

			if ((u->runtime_flags & RTF_BUSY_SENDING) && in_StringList(u->recipients, usr->name) != NULL)
				stat = 'x';

			if ((u->runtime_flags & RTF_BUSY_MAILING) && u->new_message != NULL && in_StringList(u->new_message->to, usr->name) != NULL)
				stat = 'm';

			sprintf(buf+buflen, "<white>%c%c%-18s", stat, col, u->name);
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
StringList *sl;
struct tm *tm;

	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	sl = NULL;
	if ((drawline & WHO_LIST_ROOM) || ((usr->curr_room->flags & ROOM_CHATROOM) && !(usr->flags & USR_SHOW_ALL))) {
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
		if ((msgtype == BUFMSG_XMSG || msgtype == BUFMSG_EMOTE || msgtype == BUFMSG_FEELING)
			&& strcmp(m->from, usr->name))
			break;

		pl = pl->prev;
	}
	if (pl == NULL) {
		Put(usr, "<red>No last message to reply to\n");
		CURRENT_STATE(usr);
		Return;
	}
	listdestroy_StringList(usr->recipients);
	usr->recipients = NULL;

	if ((usr->recipients = new_StringList(m->from)) == NULL) {
		Perror(usr, "Out of memory");
	} else
		check_recipients(usr);

	if (all == REPLY_X_ALL) {
		StringList *reply_to;

		if ((reply_to = copy_StringList(m->to)) == NULL) {
			Perror(usr, "Out of memory");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;
		} else {
			StringList *sl_next;

			for(sl = reply_to; sl != NULL; sl = sl_next) {
				sl_next = sl->next;

/* don't add the same user multiple times */

				if (in_StringList(usr->recipients, sl->str) == NULL)
					add_StringList(&usr->recipients, sl);
				else
					destroy_StringList(sl);
			}
			if ((sl = in_StringList(usr->recipients, usr->name)) != NULL) {
				remove_StringList(&usr->recipients, sl);
				destroy_StringList(sl);
			}
			check_recipients(usr);
		}
	}
	if (usr->recipients == NULL) {
		CURRENT_STATE(usr);
		Return;
	}
	do_reply_x(usr, m->flags);
	Return;
}

void do_reply_x(User *usr, int flags) {
char many_buf[MAX_LINE*3];

	if (usr == NULL)
		return;

	Enter(do_reply_x);

	usr->edit_pos = 0;
	usr->runtime_flags |= RTF_BUSY;
	usr->edit_buf[0] = 0;

/* replying to just one person? */
	if (usr->recipients->next == NULL && usr->recipients->prev == NULL) {
		usr->runtime_flags &= ~RTF_MULTI;
		Print(usr, "<green>Replying to%s\n", print_many(usr, many_buf));

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
		Print(usr, "<green>Replying to%s", print_many(usr, many_buf));

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

		if (usr->idle_timer != NULL)
			usr->idle_timer->sleeptime = usr->idle_timer->maxtime = PARAM_LOCK_TIMEOUT * SECS_IN_MIN;

		notify_locked(usr);
		Return;
	}
	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		usr->edit_buf[0] = 0;
		usr->edit_pos = 0;

		Put(usr, "\n<red>Enter password to unlock: ");

		if (usr->idle_timer != NULL)
			usr->idle_timer->sleeptime = usr->idle_timer->maxtime = PARAM_LOCK_TIMEOUT * SECS_IN_MIN;

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
			Put(usr, "<yellow>Unlocked\n\n");

			usr->runtime_flags &= ~(RTF_BUSY | RTF_LOCKED);

			if (usr->idle_timer != NULL)
				usr->idle_timer->sleeptime = usr->idle_timer->maxtime = PARAM_IDLE_TIMEOUT * SECS_IN_MIN;

			print_user_status(usr);

			notify_unlocked(usr);
			RET(usr);
		} else
			Put(usr, "Wrong password\n"
				"\n"
				"Enter password to unlock: ");

		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
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
			usr->runtime_flags |= RTF_WAS_HH;
		}
		if (usr->flags & (USR_ANSI|USR_BOLD)) {
			clear_screen(usr);
			Print(usr, "%c", KEY_CTRL('D'));
		} else
			Put(usr, "\n\n");

		usr->read_lines = (unsigned int)usr->flags;
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
		strcpy(usr->edit_buf, "exit");
		r = EDIT_RETURN;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			Put(usr, (usr->runtime_flags & RTF_SYSOP) ? "# " : "$ ");
			edit_line(usr, EDIT_INIT);
			Return;
		}
		if (!strcmp(usr->edit_buf, "exit") || !strcmp(usr->edit_buf, "logout")) {
			usr->runtime_flags &= ~RTF_BUSY;
			if (usr->runtime_flags & RTF_WAS_HH) {
				usr->runtime_flags &= ~RTF_WAS_HH;
				usr->flags |= USR_HELPING_HAND;
			}
			usr->flags = (unsigned int)usr->read_lines;		/* restore color flags */
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
int i, pos;
char buf[MAX_LINE*3], *p;

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
			"main.c",
			NULL
		};

		pos = 0;
		for(i = 0; sourcefiles[i] != NULL; i++)
			ls = add_StringList(&ls, new_StringList(sourcefiles[i]));

		ls = sort_StringList(rewind_StringList(ls), alphasort_StringList);

		flags = usr->flags;
		usr->flags &= ~(USR_ANSI|USR_BOLD);

		print_columns(usr, ls, 0);
		
		usr->flags = flags;
		listdestroy_StringList(ls);
		Return 0;
	}
	if (!strcmp(cmd, "uptime")) {
		Print(usr, "up %s, ", print_total_time(rtc - stats.uptime, buf));
		i = list_Count(AllUsers);
		Print(usr, "%d user%s\n", i, (i == 1) ? "" : "s");
		Return 0;
	}
	if (!strcmp(cmd, "date")) {
		Print(usr, "%s %s\n", print_date(usr, (time_t)0UL, buf), name_Timezone(usr->tz));
		Return 0;
	}
	if (!strcmp(cmd, "uname")) {
		Print(usr, "%s %s", PARAM_BBS_NAME, print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL, buf));
		Return 0;
	}
	if (!strcmp(cmd, "whoami")) {
		strcpy(buf, usr->name);
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
PList *pl = NULL;
int total;

	if (usr == NULL)
		return;

	Enter(online_friends_list);

	if (usr->friends == NULL) {
		Put(usr, "<red>You have no friends\n");
		CURRENT_STATE(usr);
		Return;
	}
	for(sl = usr->friends; sl != NULL; sl = sl->next) {
		if ((u = is_online(sl->str)) == NULL)
			continue;
/*
		if (in_StringList(usr->enemies, u->name) != NULL)
			continue;
*/
		pl = add_PList(&pl, new_PList(u));
	}
	if (pl == NULL) {
		Put(usr, "<red>None of your friends are online\n");
		CURRENT_STATE(usr);
		Return;
	}
	pl = rewind_PList(pl);
	pl = sort_PList(pl, (usr->flags & USR_SORT_DESCENDING) ? sort_who_desc_byname : sort_who_asc_byname);
	total = list_Count(pl);

/* construct header */
	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	buffer_text(usr);
	Print(usr, "<magenta>There %s <yellow>%d<magenta> friend%s online at <yellow>%02d:%02d\n",
		(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
		tm->tm_hour, tm->tm_min);

	short_who_list(usr, pl, total);

	listdestroy_PList(pl);

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
PList *pl = NULL;
int total;

	if (usr == NULL)
		return;

	Enter(talked_list);

	if (usr->history == NULL || (talked_to = make_talked_to(usr)) == NULL) {
		Put(usr, "<red>You haven't talked to anyone yet\n");
		CURRENT_STATE(usr);
		Return;
	}
	for(sl = talked_to; sl != NULL; sl = sl->next) {
		if ((u = is_online(sl->str)) == NULL)
			continue;

		pl = add_PList(&pl, new_PList(u));
	}
	if (pl == NULL) {
		Put(usr, "<red>Nobody you talked to is online anymore\n");
		CURRENT_STATE(usr);
		Return;
	}
	pl = rewind_PList(pl);
	pl = sort_PList(pl, (usr->flags & USR_SORT_DESCENDING) ? sort_who_desc_byname : sort_who_asc_byname);
	total = list_Count(pl);

/* construct header */
	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	buffer_text(usr);
	Print(usr, "<magenta>There %s <yellow>%d<magenta> %s you talked to online at <yellow>%02d:%02d\n",
		(total == 1) ? "is" : "are", total, (total == 1) ? "person" : "people",
		tm->tm_hour, tm->tm_min);

	short_who_list(usr, pl, total);

	listdestroy_PList(pl);
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

/*
	mind that 'buf' must be large enough to contain all data
*/
static int print_worldclock(User *usr, int item, char *buf) {
struct tm *t, ut;
char zone_color[16], zone_color2[16];

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
		strcpy(zone_color, "white");
		strcpy(zone_color2, "white");
	} else {
		strcpy(zone_color, "cyan");
		strcpy(zone_color2, "yellow");
	}
	if (usr->flags & USR_12HRCLOCK) {
		char am_pm = 'A';

		if (t->tm_hour >= 12) {
			am_pm = 'P';
			if (t->tm_hour > 12)
				t->tm_hour -= 12;
		}
		return sprintf(buf, "<cyan>%-15s <%s>%02d:%02d %cM",
			(worldclock[item].name == NULL) ? "" : worldclock[item].name,
			zone_color2, t->tm_hour, t->tm_min, am_pm);
	}
	return sprintf(buf, "<cyan>%-15s <%s>%02d:%02d",
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

	Print(usr, "<magenta>Current time is<yellow> %s %s\n", print_date(usr, (time_t)0UL, date_buf), name_Timezone(usr->tz));

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
		l += sprintf(line, "<magenta>  S  M Tu  W Th  F  S");

	if (PARAM_HAVE_WORLDCLOCK) {
		if (PARAM_HAVE_CALENDAR)
			l += sprintf(line+l, "    ");

		l += print_worldclock(usr, 0, line+l);
		l += sprintf(line+l, "    ");
		l += print_worldclock(usr, 1, line+l);
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
		l += sprintf(line, "<green>");

	for(w = 0; w < 5; w++) {
		if (PARAM_HAVE_CALENDAR) {
			l += sprintf(line+l, (green_color == 0) ? "<yellow>" : "<green>");

			for(d = 0; d < 7; d++) {
				tmp = user_time(usr, t);

/* highlight today and bbs birthday */
				if  (tmp->tm_mday == today && tmp->tm_mon == today_month && tmp->tm_year == today_year)
					l += sprintf(line+l, "<white> %2d<%s>", tmp->tm_mday, (green_color == 0) ? "yellow" : "green");
				else {
					if (tmp->tm_mday == bday_day && tmp->tm_mon == bday_mon && tmp->tm_year > bday_year)
						l += sprintf(line+l, "<magenta> %2d<%s>", tmp->tm_mday, (green_color == 0) ? "yellow" : "green");
					else {
						if (old_month != tmp->tm_mon) {
							green_color ^= 1;
							l += sprintf(line+l, (green_color == 0) ? "<yellow>" : "<green>");
							old_month = tmp->tm_mon;
						}
						l += sprintf(line+l, " %2d", tmp->tm_mday);
					}
				}
				t += SECS_IN_DAY;
			}
		}
		if (PARAM_HAVE_WORLDCLOCK) {
			if (PARAM_HAVE_CALENDAR)
				l += sprintf(line+l, "    ");

			l += print_worldclock(usr, w+2, line+l);
			l += sprintf(line+l, "    ");
			l += print_worldclock(usr, w+7, line+l);
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

/* EOB */
