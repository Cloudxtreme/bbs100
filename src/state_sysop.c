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
	state_sysop.c	WJ99

	Sysop menu
*/

#include "config.h"
#include "debug.h"
#include "state_sysop.h"
#include "state_msg.h"
#include "state_login.h"
#include "state_roomconfig.h"
#include "state.h"
#include "edit.h"
#include "util.h"
#include "log.h"
#include "inet.h"
#include "Stats.h"
#include "Timer.h"
#include "screens.h"
#include "passwd.h"
#include "SU_Passwd.h"
#include "timeout.h"
#include "Room.h"
#include "screens.h"
#include "mkdir.h"
#include "Param.h"
#include "main.h"
#include "CachedFile.h"
#include "copyright.h"
#include "cstring.h"
#include "Feeling.h"
#include "Memory.h"
#include "HostMap.h"
#include "OnlineUser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

void state_sysop_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_sysop_menu);

	switch(c) {
		case INIT_STATE:
/* I had to put the save_Wrapper() code here due to needed strange construction.. :P */
			if (usr->runtime_flags & RTF_WRAPPER_EDITED) {
				if (save_Wrapper(wrappers, PARAM_HOSTS_ACCESS_FILE)) {
					Perror(usr, "failed to save wrappers");
				}
				log_msg("SYSOP %s edited wrappers", usr->name);
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
			}
			usr->runtime_flags |= RTF_BUSY;
			Put(usr, "<magenta>\n"
				"<hotkey>Create new room");

			if (usr->curr_room->number > 2)
				Put(usr, "                   <white>Ctrl-<hotkey>D<magenta>elete Room\n");
			else
				Put(usr, "\n");

			Put(usr,
				"<hotkey>Disconnect user                   <white>Ctrl-<hotkey>N<magenta>uke User\n"
				"<hotkey>Banish user                       Edit <hotkey>Wrappers\n"
			);
			Put(usr,
				"<hotkey>Uncache file                      <hotkey>Memory allocation status\n"
				"\n"
				"<white>Ctrl-<hotkey>P<magenta>arameters                   <hotkey>Password\n"
				"\n"
			);
			if (reboot_timer != NULL)
				Print(usr, "<white>Ctrl-<hotkey>R<magenta>eboot <white>(in progress)<magenta>         Cancel <hotkey>Reboot\n");
			else
				Put(usr, "<white>Ctrl-<hotkey>R<magenta>eboot\n");

			if (shutdown_timer != NULL)
				Print(usr, "<white>Ctrl-<hotkey>S<magenta>hutdown <white>(in progress)<magenta>       Cancel <hotkey>Shutdown\n");
			else
				Put(usr, "<white>Ctrl-<hotkey>S<magenta>hutdown\n");

			Print(usr, "%sctivate <hotkey>Nologin\n", (nologin_screen == NULL) ? "A" : "De-a");
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
		case KEY_CTRL('C'):
			Put(usr, "\n");
			RET(usr);
			Return;

		case 'h':
		case 'H':
		case '?':
			Put(usr, "<white>Help\n");

			if (help_sysop == NULL)
				Put(usr, "<red>No help available\n");
			else {
				listdestroy_StringList(usr->more_text);
				if ((usr->more_text = copy_StringList(help_sysop)) == NULL) {
					Perror(usr, "Out of memory");
					break;
				}
				PUSH(usr, STATE_PRESS_ANY_KEY);
				read_more(usr);
				Return;
			}
			break;

		case 'c':
		case 'C':
			Put(usr, "<white>Create room\n");
			CALL(usr, STATE_CREATE_ROOM);
			Return;

		case KEY_CTRL('D'):
			if (usr->curr_room->number > 2) {
				Put(usr, "<white>Delete room\n");
				CALL(usr, STATE_DELETE_ROOM_NAME);
				Return;
			}
			break;

		case 'd':
		case 'D':
			Put(usr, "<white>Disconnect user\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			enter_name(usr, STATE_DISCONNECT_USER);
			Return;

		case KEY_CTRL('N'):
			Put(usr, "<white>Nuke user\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			enter_name(usr, STATE_NUKE_USER);
			Return;

		case 'b':
		case 'B':
			Put(usr, "<white>Banish user\n");
			CALL(usr, STATE_BANISH_USER);
			Return;

		case 'w':
		case 'W':
			Put(usr, "<white>Edit wrappers\n");
			usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
			CALL(usr, STATE_ADD_WRAPPER);
			Return;

		case 'u':
		case 'U':
			Put(usr, "<white>Uncache file\n");
			CALL(usr, STATE_UNCACHE_FILE);
			Return;

		case 'm':
		case 'M':
			Put(usr, "Memory allocation status\n");
			CALL(usr, STATE_MALLOC_STATUS);
			Return;

		case KEY_CTRL('P'):
			Put(usr, "<white>Parameters\n");
			CALL(usr, STATE_PARAMETERS_MENU);
			Return;

		case 'p':
		case 'P':
			Put(usr, "<white>Password\n");
			CALL(usr, STATE_SU_PASSWD);
			Return;

		case KEY_CTRL('R'):
			Put(usr, "<white>Reboot\n");
			CALL(usr, STATE_REBOOT_TIME);
			Return;

		case 'r':
		case 'R':
			if (reboot_timer != NULL) {
				Put(usr, "<white>Cancel reboot\n"
					"<red>Reboot cancelled\n"
				);
				remove_Timer(&timerq, reboot_timer);
				destroy_Timer(reboot_timer);
				reboot_timer = NULL;

				system_broadcast(0, "Reboot cancelled");
				log_msg("SYSOP %s cancelled reboot", usr->name);
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case KEY_CTRL('S'):
			Put(usr, "<white>Shutdown\n");
			CALL(usr, STATE_SHUTDOWN_TIME);
			Return;

		case 's':
		case 'S':
			if (shutdown_timer != NULL) {
				Put(usr, "<white>Cancel shutdown\n"
					"<red>Shutdown cancelled\n"
				);
				remove_Timer(&timerq, shutdown_timer);
				destroy_Timer(shutdown_timer);
				shutdown_timer = NULL;

				system_broadcast(0, "Shutdown cancelled");
				log_msg("SYSOP %s cancelled shutdown", usr->name);
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case 'n':
		case 'N':
			if (nologin_screen != NULL) {
				Put(usr, "<white>Deactivate nologin\n");

				listdestroy_StringList(nologin_screen);
				nologin_screen = NULL;

				Put(usr, "Deactivated\n");
				log_msg("SYSOP %s deactivated nologin", usr->name);
				CURRENT_STATE(usr);
				Return;
			} else {
				StringList *sl;

				Put(usr, "<white>Activate nologin\n");

				if ((sl = load_StringList(PARAM_NOLOGIN_SCREEN)) == NULL) {
					Perror(usr, "Failed to load nologin_screen");
				} else {
					listdestroy_StringList(nologin_screen);
					nologin_screen = sl;

					Put(usr, "Activated\n");
					log_msg("SYSOP %s activated nologin", usr->name);
					CURRENT_STATE(usr);
					Return;
				}
			}
			break;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] # ", PARAM_NAME_SYSOP);
	Return;
}


void state_disconnect_user(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_disconnect_user);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		User *u;

		if (!usr->edit_buf) {
			RET(usr);
			Return;
		}
		if (!strcmp(usr->edit_buf, usr->name)) {
			Put(usr, "<red>That's not a very good idea\n");
			RET(usr);
			Return;
		}
		if ((u = is_online(usr->edit_buf)) != NULL) {
			Put(u, "<red>\n"
				"\n"
				"<yellow>*** <red>Sorry, but you are being disconnected <white>NOW <yellow>***\n"
				"\n"
				"<normal>\n"
			);
			close_connection(u, "user is disconnected by %s", usr->name);
			u = NULL;
			Print(usr, "<yellow>%s<green> was disconnected\n", usr->edit_buf);
		} else {
			if (!user_exists(usr->edit_buf))
				Put(usr, "<red>No such user\n");
			else
				Print(usr, "<yellow>%s<white> is not online\n", usr->edit_buf);
		}
		RET(usr);
	}
	Return;
}

void state_nuke_user(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_nuke_user);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		User *u;
		char path[MAX_PATHLEN], newpath[MAX_PATHLEN];
		SU_Passwd *su;

		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");
			RET(usr);
			Return;
		}
		for(su = su_passwd; su != NULL; su = su->next) {
			if (!strcmp(su->name, usr->edit_buf)) {
				Print(usr, "<red>You can't nuke someone who has %s access!\n", PARAM_NAME_SYSOP);
				RET(usr);
				Return;
			}
		}
		if ((u = is_online(usr->edit_buf)) != NULL) {
			Put(u, "<red>\n"
				"\n"
				"<yellow>*** <red>Sorry, but you are being disconnected <white>NOW <yellow>***\n"
				"\n"
				"<normal>\n"
			);
			close_connection(u, "user is being nuked by %s", usr->name);
			u = NULL;
		}
		sprintf(path, "%s/%c/%s", PARAM_USERDIR, usr->edit_buf[0], usr->edit_buf);
		path_strip(path);
		sprintf(newpath, "%s/%s", PARAM_TRASHDIR, path);
		path_strip(newpath);
/*
	Move the user directory
	Note that this enables nuked users to recreate their account instantly,
	which is something I do not really want :P

	Perhaps I should just reset the password to zero or something...
*/
		rm_rf_trashdir(newpath);		/* make sure trash/newpath does not exist */

		if (rename_dir(path, newpath) < 0) {
			log_err("rename() failed for %s -> %s", path, newpath);
			Put(usr, "<red>Failed to remove user directory\n");
		} else
			Print(usr, "<yellow>%s<red> nuked\n", usr->edit_buf);

		log_msg("SYSOP %s nuked user %s", usr->name, usr->edit_buf);
		RET(usr);
	}
	Return;
}

void state_banish_user(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_banish_user);

	if (c == INIT_STATE) {
		if (banished != NULL) {
			if (banished->next != NULL) {
				Put(usr, "\n<magenta>Banished are<yellow>:\n");
				show_namelist(usr, banished);
			} else
				Print(usr, "\n<magenta>Banished is<yellow>: %s\n", banished->str);
		}
		Put(usr, "\n");
		POP(usr);

		listdestroy_StringList(usr->recipients);
		usr->recipients = NULL;

		enter_name(usr, STATE_BANISH_USER);
		Return;
	}
	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			StringList *sl;

			if ((sl = in_StringList(banished, usr->edit_buf)) != NULL) {
				remove_StringList(&banished, sl);
				destroy_StringList(sl);
				Print(usr, "<green>Unbanished <yellow>%s\n", usr->edit_buf);
				log_msg("SYSOP %s unbanished user %s", usr->name, usr->edit_buf);
			} else {
				if ((sl = new_StringList(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				} else {
					add_StringList(&banished, sl);
					Print(usr, "<yellow>%s<green> banished\n", usr->edit_buf);
					log_msg("SYSOP %s banished user %s", usr->name, usr->edit_buf);
				}
			}
			if (save_StringList(banished, PARAM_BANISHED_FILE)) {
				Perror(usr, "failed to save banished_file");
			}
			CURRENT_STATE(usr);
			Return;
		}
		RET(usr);
	}
	Return;
}

void state_add_wrapper(User *usr, char c) {
int r;
Wrapper *w;
int i;

	if (usr == NULL)
		return;

	Enter(state_add_wrapper);

	if (c == INIT_STATE) {
		unsigned long n, m;
		char buf[MAX_LINE];

		Print(usr, "\n<yellow> 1 <white>Add new wrapper\n");
		i = 2;
		for(w = wrappers; w != NULL; w = w->next) {
			n = w->net;
			m = w->mask;
			sprintf(buf, "<yellow>%2d <white>%s %lu.%lu.%lu.%lu/%lu.%lu.%lu.%lu",
				i, (w->allow == 0) ? "deny" : "allow",
				(n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255,
				(m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255, m & 255);

			if (w->comment != NULL)
				Print(usr, "%-40s <cyan># %s\n", buf, w->comment);
			else
				Print(usr, "%s\n", buf);
			i++;
		}
		Put(usr, "\n"
			"<green>Enter number<yellow>: ");
	}
	r = edit_number(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		i = atoi(usr->edit_buf);
		if (i < 1) {
			Put(usr, "<red>Invalid entry\n");
			CURRENT_STATE(usr);
			Return;
		}
		if (i == 1) {
			Put(usr, "<white>Add wrapper\n");
			if ((w = new_Wrapper(0, 0UL, 0xffffffffUL, NULL)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			add_Wrapper(&wrappers, w);
			usr->read_lines = list_Count(wrappers) - 1;
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
		} else {
			int j = 2;

			for(w = wrappers; w != NULL && j < i; w = w->next)
				j++;

			if (w == NULL) {
				Put(usr, "<red>Invalid entry\n");
				CURRENT_STATE(usr);
				Return;
			}
			usr->read_lines = i-2;

			Put(usr, "<white>Edit wrapper\n");
		}
		CALL(usr, STATE_EDIT_WRAPPER);
	}
	Return;
}

/*
	Note: usr->read_lines is wrapper to edit
*/
void state_edit_wrapper(User *usr, char c) {
Wrapper *w;
int i = 0;
unsigned long n, m;

	if (usr == NULL)
		return;

	Enter(state_edit_wrapper);

	for(w = wrappers; w != NULL; w = w->next) {
		if (i == usr->read_lines)
			break;
		i++;
	}
	if (w == NULL) {
		Perror(usr, "The wrapper to edit has gone up in smoke");
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			n = w->net;
			m = w->mask;
			Print(usr, "<magenta>\n"
				"<hotkey>Allow/deny access            <white>[%s]<magenta>\n"
				"<hotkey>IP net address               <white>%lu.%lu.%lu.%lu<magenta>\n",
				(w->allow == 0) ? "Deny" : "Allow",
				(n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255
			);
			Print(usr,
				"IP <hotkey>mask                      <white>%lu.%lu.%lu.%lu<magenta>\n"
				"<hotkey>Comment                      <cyan>%s<magenta>\n"
				"\n"
				"Add <hotkey>new wrapper              <hotkey>Delete this wrapper\n"
				"\n"
				"<white>[<yellow>Edit wrapper<white>] # ",
				(m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255, m & 255,
				(w->comment == NULL) ? "" : w->comment
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

		case 'a':
		case 'A':
			Put(usr, "<white>Allow/deny\n");
			w->allow ^= 1;
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 'i':
		case 'I':
			Put(usr, "<white>IP net address\n");
			CALL(usr, STATE_IPADDR_WRAPPER);
			Return;

		case 'm':
		case 'M':
			Put(usr, "<white>IP mask\n");
			CALL(usr, STATE_IPMASK_WRAPPER);
			Return;

		case 'c':
		case 'C':
			Put(usr, "<white>Comment\n");
			CALL(usr, STATE_COMMENT_WRAPPER);
			Return;

		case 'n':
		case 'N':
			Put(usr, "<white>Add new wrapper\n");
			if ((w = new_Wrapper(0, 0UL, 0xffffffffUL, NULL)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			add_Wrapper(&wrappers, w);
			usr->read_lines = list_Count(wrappers) - 1;
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 'd':
		case 'D':
			Put(usr, "<white>Delete\n");
			remove_Wrapper(&wrappers, w);
			destroy_Wrapper(w);
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
			RET(usr);
			Return;
	}
	Return;
}

void state_ipaddr_wrapper(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_ipaddr_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter IP net address<yellow>: ");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i = 0, n1, n2, n3, n4;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		for(w = wrappers; w != NULL; w = w->next) {
			if (i == usr->read_lines)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		if (sscanf(usr->edit_buf, "%d.%d.%d.%d", &n1, &n2, &n3, &n4) != 4) {
			Put(usr, "<red>Malformed IP net address (should be in standard dot notation)\n");
			RET(usr);
			Return;
		}
		if (n1 < 0 || n1 > 255
			|| n2 < 0 || n2 > 255
			|| n3 < 0 || n3 > 255
			|| n4 < 0 || n4 > 255) {
			Put(usr, "<red>Malformed IP net address\n");
			RET(usr);
			Return;
		}
		w->net = n1;
		w->net <<= 8;
		w->net |= n2;
		w->net <<= 8;
		w->net |= n3;
		w->net <<= 8;
		w->net |= n4;

		usr->runtime_flags |= RTF_WRAPPER_EDITED;
		RET(usr);
	}
	Return;
}

void state_ipmask_wrapper(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_ipmask_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter IP mask<yellow>: ");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i = 0, m1, m2, m3, m4;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		for(w = wrappers; w != NULL; w = w->next) {
			if (i == usr->read_lines)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		if (sscanf(usr->edit_buf, "%d.%d.%d.%d", &m1, &m2, &m3, &m4) != 4) {
			Put(usr, "<red>Malformed IP mask (should be in standard dot notation)\n");
			RET(usr);
			Return;
		}
		if (m1 < 0 || m1 > 255
			|| m2 < 0 || m2 > 255
			|| m3 < 0 || m3 > 255
			|| m4 < 0 || m4 > 255) {
			Put(usr, "<red>Malformed IP mask\n");
			RET(usr);
			Return;
		}
		w->mask = m1;
		w->mask <<= 8;
		w->mask |= m2;
		w->mask <<= 8;
		w->mask |= m3;
		w->mask <<= 8;
		w->mask |= m4;

		usr->runtime_flags |= RTF_WRAPPER_EDITED;
		RET(usr);
	}
	Return;
}

void state_comment_wrapper(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_comment_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter comment<yellow>: ");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i = 0;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		for(w = wrappers; w != NULL; w = w->next) {
			if (i == usr->read_lines)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		Free(w->comment);
		if ((w->comment = cstrdup(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
		}
		usr->runtime_flags |= RTF_WRAPPER_EDITED;
		RET(usr);
	}
	Return;
}


void state_create_room(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_create_room);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter new room name<yellow>: ");

	r = edit_roomname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Room *room, *rm;
		Joined *j;
		char buf[MAX_PATHLEN], *p;

		if (usr->edit_buf[0] >= '0' && usr->edit_buf[0] <= '9') {
			Put(usr, "<red>Room names cannot start with a digit\n");
			RET(usr);
			Return;
		}
		if (room_exists(usr->edit_buf)) {
			Put(usr, "<red>Room already exists\n");
			RET(usr);
			Return;
		}
		if (!strcmp(usr->edit_buf, "Mail") || !strcmp(usr->edit_buf, "Home")) {
			Put(usr, "<red>The room names <white>Home<red> and <white>Mail<red> are reserved and cannot be used\n");
			RET(usr);
			Return;
		}		
		if ((p = cstrchr(usr->edit_buf, '\'')) != NULL) {
			if (!strcmp(p, "'s Mail") || !strcmp(p, "'s Home")
				|| !strcmp(p, "' Mail") || !strcmp(p, "' Home")) {
				Put(usr, "<red>The room names <white>Home<red> and <white>Mail<red> are reserved and cannot be used\n");
				RET(usr);
				Return;
			}		
		}
		if ((room = new_Room()) == NULL) {			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		strcpy(room->name, usr->edit_buf);
		room->generation = (unsigned long)rtc;
		room->flags = (ROOM_HIDDEN | ROOM_READONLY | ROOM_SUBJECTS | ROOM_INVITE_ONLY);

/* find a room number */
		room->number = 3;			/* lowest possible new room number is 3 */
		for(rm = AllRooms; rm != NULL; rm = rm->next) {
			if (room->number == rm->number)
				room->number++;
			else
				if (room->number < rm->number)
					break;
		}
		sprintf(buf, "%s/%u", PARAM_ROOMDIR, room->number);
		path_strip(buf);
		if (mkdir(buf, (mode_t)0750) < 0) {
			log_err("failed to create new room directory %s", buf);
			Perror(usr, "failed to create room directory");
			destroy_Room(room);
			RET(usr);
			Return;
		}
		usr->curr_room = room;

/* join this room or problems will occur */
		if ((j = in_Joined(usr->rooms, room->number)) == NULL) {
			if ((j = new_Joined()) == NULL) {
				Perror(usr, "Out of memory");
			} else {
				j->number = room->number;
				j->generation = room->generation;
				j->last_read = 0UL;

				add_Joined(&usr->rooms, j);
			}
		} else {
			j->zapped = 0;
			j->generation = room->generation;
			j->last_read = 0UL;
		}
		Print(usr, "<yellow>The room has been assigned number <white>%u\n", room->number);
		log_msg("SYSOP %s created room %u %s", usr->name, room->number, room->name);

		add_Room(&AllRooms, room);					/* add room to all rooms list */
		AllRooms = sort_Room(AllRooms, room_sort_func);		/* re-sort the list */

		JMP(usr, STATE_ROOM_CONFIG_MENU);
		usr->runtime_flags |= RTF_ROOM_EDITED;
	}
	Return;
}

void state_delete_room_name(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_delete_room_name);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter room name<yellow>: ");

	r = edit_roomname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Room *room;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if ((room = find_Room(usr, usr->edit_buf)) == NULL)
			Put(usr, "<red>No such room\n");
		else
			delete_room(usr, room);
		RET(usr);
	}
	Return;
}
	


void state_reboot_time(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_reboot_time);

	if (c == INIT_STATE)
		Print(usr, "<red>Enter reboot time in seconds <white>[<yellow>240<white>]: ");

	r = edit_number(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0])
			usr->read_lines = 4*60;
		else
			usr->read_lines = atoi(usr->edit_buf);

		JMP(usr, STATE_REBOOT_PASSWORD);
	}
	Return;
}

/*
	Note: usr->read_lines is amount of seconds till reboot
*/
void state_reboot_password(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_reboot_password);

	if (c == INIT_STATE)
		Print(usr, "\n"
"<yellow>*** <white>WARNING<yellow> ***\n"
"\n"
"<red>This is serious. Enter the reboot password and the system will reboot\n"
"in %s (including one minute grace period)\n"
"\n"
"Enter reboot password: ", print_total_time((unsigned long)usr->read_lines + 60UL));

	r = edit_password(usr, c);
	if (r == EDIT_BREAK) {
		if (reboot_timer != NULL)
			Put(usr, "<red>Aborted, but note that another reboot procedure is already running\n\n");
		else
			Put(usr, "<red>Reboot cancelled\n\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd, buf[256];

		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {
			Put(usr, "<red>Wrong password\n");
			usr->runtime_flags &= ~RTF_SYSOP;
			POP(usr);
			RET(usr);
			Return;
		}
		if (verify_phrase(usr->edit_buf, pwd)) {
			Put(usr, "<red>Wrong password\n");
			RET(usr);
			Return;
		}
		if (reboot_timer != NULL) {
			remove_Timer(&timerq, reboot_timer);
			reboot_timer->sleeptime = reboot_timer->maxtime = usr->read_lines;
			reboot_timer->restart = TIMEOUT_REBOOT;
			add_Timer(&timerq, reboot_timer);

			Print(usr, "<red>Reboot time altered to %s (including one minute grace period)\n", print_total_time((unsigned long)usr->read_lines + 60UL));

			sprintf(buf, "The system is now rebooting in %s",
				print_total_time((unsigned long)reboot_timer->sleeptime + 60UL));
			system_broadcast(0, buf);
			RET(usr);
			Return;
		}
		if ((reboot_timer = new_Timer(usr->read_lines, reboot_timeout, TIMEOUT_REBOOT)) == NULL) {
			Perror(usr, "Out of memory, reboot cancelled");
			RET(usr);
			Return;
		}
		add_Timer(&timerq, reboot_timer);

		log_msg("SYSOP %s initiated reboot", usr->name);

		Put(usr, "\n<red>Reboot procedure started\n");

		if (reboot_timer->sleeptime > 0) {
			sprintf(buf, "The system is rebooting in %s",
				print_total_time((unsigned long)reboot_timer->sleeptime + 60UL));
			system_broadcast(0, buf);
		}
		RET(usr);
	}
	Return;
}



void state_shutdown_time(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_shutdown_time);

	if (c == INIT_STATE)
		Print(usr, "<red>Enter shutdown time in seconds <white>[<yellow>240<white>]: ");

	r = edit_number(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0])
			usr->read_lines = 4*60;
		else
			usr->read_lines = atoi(usr->edit_buf);

		JMP(usr, STATE_SHUTDOWN_PASSWORD);
	}
	Return;
}

/*
	Note: usr->read_lines is amount of seconds till shutdown
*/
void state_shutdown_password(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_shutdown_password);

	if (c == INIT_STATE)
		Print(usr, "\n"
"<yellow>*** <white>WARNING<yellow> ***\n"
"\n"
"<red>This is serious. Enter the shutdown password and the system will shut\n"
"down in %s (including one minute grace period)\n"
"\n"
"Enter shutdown password: ", print_total_time((unsigned long)usr->read_lines + 60UL));

	r = edit_password(usr, c);
	if (r == EDIT_BREAK) {
		if (shutdown_timer != NULL)
			Put(usr, "<red>Aborted, but note that another shutdown procedure is already running\n\n");
		else
			Put(usr, "<red>Shutdown cancelled\n\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd, buf[256];

		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {
			Put(usr, "<red>Wrong password\n");
			usr->runtime_flags &= ~RTF_SYSOP;
			POP(usr);
			RET(usr);
			Return;
		}
		if (verify_phrase(usr->edit_buf, pwd)) {
			Put(usr, "<red>Wrong password\n");
			RET(usr);
			Return;
		}
		if (shutdown_timer != NULL) {
			remove_Timer(&timerq, shutdown_timer);
			shutdown_timer->sleeptime = shutdown_timer->maxtime = usr->read_lines;
			shutdown_timer->restart = TIMEOUT_SHUTDOWN;
			add_Timer(&timerq, shutdown_timer);
			Print(usr, "<red>Shutdown time altered to %s (including one minute grace period)\n", print_total_time((unsigned long)usr->read_lines + 60UL));

			sprintf(buf, "The system is now shutting down in %s",
				print_total_time((unsigned long)shutdown_timer->sleeptime + 60UL));
			system_broadcast(0, buf);
			RET(usr);
			Return;
		}
		if ((shutdown_timer = new_Timer(usr->read_lines, shutdown_timeout, TIMEOUT_SHUTDOWN)) == NULL) {
			Perror(usr, "Out of memory, shutdown cancelled");
			RET(usr);
			Return;
		}
		add_Timer(&timerq, shutdown_timer);

		log_msg("SYSOP %s initiated shutdown", usr->name);

		Put(usr, "\n<red>Shutdown sequence initiated\n");

		if (shutdown_timer->sleeptime > 0) {
			sprintf(buf, "The system is shutting down in %s",
				print_total_time((unsigned long)shutdown_timer->sleeptime + 60UL));
			system_broadcast(0, buf);
		}
		RET(usr);
	}
	Return;
}

void state_su_passwd(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_su_passwd);

	if (c == INIT_STATE)
		Print(usr, "<red>Enter <yellow>%s<red> mode password<white>:<red> ", PARAM_NAME_SYSOP);

	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
	}
	if (r == EDIT_RETURN) {
		char *pwd;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {
			Print(usr, "\n\n<red>You are not allowed to become <yellow>%s<red> any longer\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;
		}
		if (!verify_phrase(usr->edit_buf, pwd)) {
			JMP(usr, STATE_CHANGE_SU_PASSWD);
		} else {
			Put(usr, "<red>Wrong password\n");
			RET(usr);
		}
	}
	Return;
}

void state_change_su_passwd(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_change_su_passwd);

	if (c == INIT_STATE) {
		Print(usr, "<red>Enter new <yellow>%s<red> mode password<white>:<red> ", PARAM_NAME_SYSOP);

		Free(usr->tmpbuf[TMP_PASSWD]);
		usr->tmpbuf[TMP_PASSWD] = NULL;
	}
	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		Put(usr, "<red>Password not changed\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if (usr->tmpbuf[TMP_PASSWD] == NULL) {
			if (strlen(usr->edit_buf) < 5) {
				Put(usr, "<red>That password is too short\n");
				CURRENT_STATE(usr);
				Return;
			}
			Put(usr, "<red>Enter it again <white>(<red>for verification<white>):<red> ");

			if ((usr->tmpbuf[TMP_PASSWD] = cstrdup(usr->edit_buf)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			usr->edit_buf[0] = 0;
			usr->edit_pos = 0;
		} else {
			if (!strcmp(usr->edit_buf, usr->tmpbuf[TMP_PASSWD])) {
				char *crypted;
				SU_Passwd *su;

				crypted = crypt_phrase(usr->edit_buf);
				crypted[MAX_CRYPTED_PASSWD-1] = 0;

				if (verify_phrase(usr->edit_buf, crypted)) {
					Perror(usr, "bug in password encryption -- please choose an other password");
					CURRENT_STATE(usr);
					Return;
				}
				for(su = su_passwd; su != NULL; su = su->next) {
					if (!strcmp(su->name, usr->name)) {
						strcpy(su->passwd, crypted);

						if (save_SU_Passwd(su_passwd, PARAM_SU_PASSWD_FILE)) {
							Perror(usr, "failed to save su_passwd_file");
						} else {
							Print(usr, "<red>%s mode password changed\n", PARAM_NAME_SYSOP);
							log_msg("SYSOP %s changed %s mode password", usr->name, PARAM_NAME_SYSOP);
						}
						Free(usr->tmpbuf[TMP_PASSWD]);
						usr->tmpbuf[TMP_PASSWD] = NULL;

						RET(usr);
						Return;
					}
				}
				Print(usr, "<red>You are not allowed to change the <yellow>%s<red> mode password anymore\n", PARAM_NAME_SYSOP);
				usr->runtime_flags &= ~RTF_SYSOP;
				POP(usr);
			} else
				Print(usr, "<red>Passwords didn't match <white>; <yellow>%s<red> mode password NOT changed\n", PARAM_NAME_SYSOP);

			Free(usr->tmpbuf[TMP_PASSWD]);
			usr->tmpbuf[TMP_PASSWD] = NULL;

			RET(usr);
		}
	}
	Return;
}

void state_uncache_file(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_uncache_file);

	if (c == INIT_STATE)
		Put(usr, "\n<green>Enter filename: <white>");

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		cstrip_line(usr->edit_buf);

		if (usr->edit_buf[0]) {
			if (in_Cache(usr->edit_buf) != NULL) {
				remove_Cache_filename(usr->edit_buf);
				Print(usr, "<green>File <white>%s<green> removed from cache\n", usr->edit_buf);
			} else
				Print(usr, "<red>File <white>%s<red> was not cached\n", usr->edit_buf);
		}
		RET(usr);
	}
	Return;
}

void state_malloc_status(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_malloc_status);

	if (c == INIT_STATE) {
		int i, len = 0;

		Print(usr, "\n<green>Total memory in use: <yellow>%s <green>bytes\n\n", print_number(memory_total));

		for(i = 0; i < NUM_TYPES+1; i++) {
			if (strlen(Types_table[i].type) > len)
				len = strlen(Types_table[i].type);
		}
		for(i = 0; i < NUM_TYPES+1; i++) {
			if (i & 1)
				Print(usr, "      ");

			Print(usr, "<green>%-*s <yellow>:<white> %12s %c", len, Types_table[i].type, print_number(mem_stats[i]), (i & 1) ? '\n' : ' ');
		}
		if (i & 1)
			Put(usr, "\n");

		Put(usr, "\n"
			"<white>[Press a key]");
	} else {
		Put(usr, "<cr>              <cr>");
		RET(usr);
	}
	Return;
}


void state_parameters_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_parameters_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Put(usr, "<magenta>\n"
				"S<hotkey>ystem configuration\n"
				"Configure locations of <hotkey>Files\n"
				"Configure <hotkey>Maximums\n"
				"Configure <hotkey>Strings and messages\n"
			);
			Print(usr,
				"<hotkey>Reload screens and help files\n"
				"\n"
				"<white>Ctrl-<hotkey>R<magenta>eload param file <white>%s\n", param_file);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			if (usr->runtime_flags & RTF_PARAM_EDITED) {
				if (save_Param(param_file)) {
					Perror(usr, "failed to save param file");
				}
				usr->runtime_flags &= ~RTF_PARAM_EDITED;
			}
			RET(usr);
			Return;


		case 'y':
		case 'Y':
			Put(usr, "System configuration\n");
			CALL(usr, STATE_SYSTEM_CONFIG_MENU);
			Return;

		case 'f':
		case 'F':
			Put(usr, "Configure locations of files\n");
			CALL(usr, STATE_CONFIG_FILES_MENU);
			Return;

		case 'm':
		case 'M':
			Put(usr, "Configure maximums\n");
			CALL(usr, STATE_MAXIMUMS_MENU);
			Return;

		case 's':
		case 'S':
			Put(usr, "Configure strings and messages\n");
			CALL(usr, STATE_STRINGS_MENU);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Reload screens and help files\n");
			CALL(usr, STATE_RELOAD_FILES_MENU);
			Return;

		case KEY_CTRL('R'):
			Put(usr, "Reload param file\n");

			usr->runtime_flags &= ~RTF_PARAM_EDITED;

			if (load_Param(param_file)) {
				Perror(usr, "Failed to load param file");
			} else {
				Print(usr, "loading %s ... Ok\n", param_file);
				CURRENT_STATE(usr);
			}
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Parameters<white># ", PARAM_NAME_SYSOP);
}




void state_system_config_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_system_config_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Print(usr, "<magenta>\n"
				"BBS <hotkey>Name            <white>%s<magenta>\n"
				"P<hotkey>ort number         <white>%u<magenta>\n"
				"<hotkey>Param file          <white>%s<magenta>\n",
				PARAM_BBS_NAME,
				PARAM_PORT_NUMBER,
				param_file
			);
			Print(usr, "\n"
				"<hotkey>Base directory      <white>%s<magenta>\n"
				"B<hotkey>inary directory    <white>%s<magenta>\n"
				"<hotkey>Config directory    <white>%s<magenta>\n"
				"<hotkey>Feelings directory  <white>%s<magenta>\n",
				PARAM_BASEDIR,
				PARAM_BINDIR,
				PARAM_CONFDIR,
				PARAM_FEELINGSDIR
			);
			Print(usr,
				"<hotkey>User directory      <white>%s<magenta>\n"
				"<hotkey>Room directory      <white>%s<magenta>\n"
				"<hotkey>Trash directory     <white>%s<magenta>\n",
				PARAM_USERDIR,
				PARAM_ROOMDIR,
				PARAM_TRASHDIR
			);
			Print(usr, "\n"
				"<hotkey>Main program        <white>%s<magenta>\n"
				"Resol<hotkey>ver program    <white>%s<magenta>\n",
				PARAM_PROGRAM_MAIN,
				PARAM_PROGRAM_RESOLVER
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;


		case 'n':
		case 'N':
			Put(usr, "BBS Name\n");
			CALL(usr, STATE_PARAM_BBS_NAME);
			Return;

		case 'o':
		case 'O':
			Put(usr, "Port number\n");
			CALL(usr, STATE_PARAM_PORT_NUMBER);
			Return;

		case 'p':
		case 'P':
			Put(usr, "Param file\n");
			CALL(usr, STATE_PARAM_FILE);
			Return;

		case 'b':
		case 'B':
			Put(usr, "Base directory\n");
			CALL(usr, STATE_PARAM_BASEDIR);
			Return;

		case 'i':
		case 'I':
			Put(usr, "Binary directory\n");
			CALL(usr, STATE_PARAM_BINDIR);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Config directory\n");
			CALL(usr, STATE_PARAM_CONFDIR);
			Return;

		case 'f':
		case 'F':
			Put(usr, "Feelings directory\n");
			CALL(usr, STATE_PARAM_FEELINGSDIR);
			Return;

		case 'u':
		case 'U':
			Put(usr, "User directory\n");
			CALL(usr, STATE_PARAM_USERDIR);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Room directory\n");
			CALL(usr, STATE_PARAM_ROOMDIR);
			Return;

		case 't':
		case 'T':
			Put(usr, "Trash directory\n");
			CALL(usr, STATE_PARAM_TRASHDIR);
			Return;


		case 'm':
		case 'M':
			Put(usr, "Main program\n");
			CALL(usr, STATE_PARAM_PROGRAM_MAIN);
			Return;

		case 'v':
		case 'V':
			Put(usr, "Resolver program\n");
			CALL(usr, STATE_PARAM_PROGRAM_RESOLVER);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Sysconf<white># ", PARAM_NAME_SYSOP);
	Return;
}


void state_param_bbs_name(User *usr, char c) {
	Enter(state_param_bbs_name);
	change_string_param(usr, c, &PARAM_BBS_NAME, "<green>Enter name<yellow>: ");
	Return;
}

void state_param_port_number(User *usr, char c) {
	Enter(state_param_port_number);
	change_int_param(usr, c, &PARAM_PORT_NUMBER);
	Return;
}

void state_param_file(User *usr, char c) {
	Enter(state_param_file);
	change_string_param(usr, c, &param_file, "<green>Enter param file<yellow>: ");
	Return;
}

void state_param_basedir(User *usr, char c) {
	Enter(state_param_basedir);
	change_string_param(usr, c, &PARAM_BASEDIR, "<green>Enter base directory<yellow>: ");
	Return;
}

void state_param_bindir(User *usr, char c) {
	Enter(state_param_bindir);
	change_string_param(usr, c, &PARAM_BINDIR, "<green>Enter binary directory<yellow>: ");
	Return;
}

void state_param_confdir(User *usr, char c) {
	Enter(state_param_confdir);
	change_string_param(usr, c, &PARAM_CONFDIR, "<green>Enter config directory<yellow>: ");
	Return;
}

void state_param_feelingsdir(User *usr, char c) {
	Enter(state_param_feelingsdir);
	change_string_param(usr, c, &PARAM_FEELINGSDIR, "<green>Enter feelings directory<yellow>: ");
	Return;
}

void state_param_userdir(User *usr, char c) {
	Enter(state_param_userdir);
	change_string_param(usr, c, &PARAM_USERDIR, "<green>Enter user directory<yellow>: ");
	Return;
}

void state_param_roomdir(User *usr, char c) {
	Enter(state_param_roomdir);
	change_string_param(usr, c, &PARAM_ROOMDIR, "<green>Enter room directory<yellow>: ");
	Return;
}

void state_param_trashdir(User *usr, char c) {
	Enter(state_param_trashdir);
	change_string_param(usr, c, &PARAM_TRASHDIR, "<green>Enter trash directory<yellow>: ");
	Return;
}

void state_param_program_main(User *usr, char c) {
	Enter(state_param_program_main);
	change_string_param(usr, c, &PARAM_PROGRAM_MAIN, "<green>Enter main program<yellow>: ");
	Return;
}

void state_param_program_resolver(User *usr, char c) {
	Enter(state_param_program_resolver);
	change_string_param(usr, c, &PARAM_PROGRAM_RESOLVER, "<green>Enter resolver program<yellow>: ");
	Return;
}



void state_config_files_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_config_files_menu);

/*
	I'm hopelessly out of hotkeys here...
*/
	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Print(usr, "<magenta>\n"
				"<hotkey>GPL              <white>%-22s<magenta>  <hotkey>0 Local mods     <white>%s<magenta>\n",
				PARAM_GPL_SCREEN, PARAM_MODS_SCREEN);
			Print(usr, "<hotkey>Login            <white>%-22s<magenta>  <hotkey>Nologin          <white>%s<magenta>\n",
				PARAM_LOGIN_SCREEN, PARAM_NOLOGIN_SCREEN);
			Print(usr, "Log<hotkey>out           <white>%-22s<magenta>  <hotkey>Reboot           <white>%s<magenta>\n",
				PARAM_LOGIN_SCREEN, PARAM_REBOOT_SCREEN);
			Print(usr, "Mot<hotkey>d             <white>%-22s<magenta>  <hotkey>Z Shutdown       <white>%s<magenta>\n",
				PARAM_MOTD_SCREEN, PARAM_SHUTDOWN_SCREEN);
			Print(usr, "<hotkey>1st login        <white>%-22s<magenta>  <hotkey>K Crash          <white>%s<magenta>\n",
				PARAM_FIRST_LOGIN, PARAM_CRASH_SCREEN);

			Print(usr, "\n"
				"Standard <hotkey>help    <white>%-22s<magenta>  Room config h<hotkey>elp <white>%s<magenta>\n",
				PARAM_HELP_STD, PARAM_HELP_ROOMCONFIG);
			Print(usr, "<hotkey>Config menu help <white>%-22s<magenta>  <hotkey>Sysop menu help  <white>%s<magenta>\n",
				PARAM_HELP_CONFIG, PARAM_HELP_SYSOP);

			Print(usr, "<hotkey>Param file       <white>%-22s<magenta>  Uni<hotkey>x PID file    <white>%s<magenta>\n",
				param_file, PARAM_PID_FILE);
			Print(usr, "<hotkey>Banished         <white>%-22s<magenta>  Sta<hotkey>tistics       <white>%s<magenta>\n",
				PARAM_BANISHED_FILE, PARAM_STAT_FILE);
			Print(usr, "Host <hotkey>access      <white>%-22s<magenta>  S<hotkey>U Passwd        <white>%s<magenta>\n",
				PARAM_HOSTS_ACCESS_FILE, PARAM_SU_PASSWD_FILE);
			Print(usr, "Host <hotkey>map         <white>%-22s<magenta>  S<hotkey>ymbol table     <white>%s<magenta>\n",
				PARAM_HOSTMAP_FILE, PARAM_SYMTAB_FILE);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;


		case 'g':
		case 'G':
			Put(usr, "GNU General Public License\n");
			CALL(usr, STATE_PARAM_GPL_SCREEN);
			Return;

		case '0':
			Put(usr, "Local mods screen\n");
			CALL(usr, STATE_PARAM_MODS_SCREEN);
			Return;

		case 'l':
		case 'L':
			Put(usr, "Login screen\n");
			CALL(usr, STATE_PARAM_LOGIN_SCREEN);
			Return;

		case 'o':
		case 'O':
			Put(usr, "Logout screen\n");
			CALL(usr, STATE_PARAM_LOGOUT_SCREEN);
			Return;

		case 'n':
		case 'N':
			Put(usr, "Nologin screen\n");
			CALL(usr, STATE_PARAM_NOLOGIN_SCREEN);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Motd screen\n");
			CALL(usr, STATE_PARAM_MOTD_SCREEN);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Reboot screen\n");
			CALL(usr, STATE_PARAM_REBOOT_SCREEN);
			Return;

		case 'z':
		case 'Z':
			Put(usr, "Shutdown screen\n");
			CALL(usr, STATE_PARAM_SHUTDOWN_SCREEN);
			Return;

		case 'k':
		case 'K':
			Put(usr, "Crash screen\n");
			CALL(usr, STATE_PARAM_CRASH_SCREEN);
			Return;

		case '1':
			Put(usr, "First login screen\n");
			CALL(usr, STATE_PARAM_FIRST_LOGIN);
			Return;

		case 'h':
		case 'H':
			Put(usr, "Standard help\n");
			CALL(usr, STATE_PARAM_HELP_STD);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Config menu help\n");
			CALL(usr, STATE_PARAM_HELP_CONFIG);
			Return;

		case 'e':
		case 'E':
			Put(usr, "Room config help\n");
			CALL(usr, STATE_PARAM_HELP_ROOMCONFIG);
			Return;

		case 's':
		case 'S':
			Put(usr, "Sysop help\n");
			CALL(usr, STATE_PARAM_HELP_SYSOP);
			Return;

		case 'p':
		case 'P':
			Put(usr, "Param file\n");
			CALL(usr, STATE_PARAM_FILE);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Hosts access file\n");
			CALL(usr, STATE_PARAM_HOSTS_ACCESS);
			Return;

		case 'b':
		case 'B':
			Put(usr, "Banished file\n");
			CALL(usr, STATE_PARAM_BANISHED_FILE);
			Return;

		case 't':
		case 'T':
			Put(usr, "Statistics file\n");
			CALL(usr, STATE_PARAM_STAT_FILE);
			Return;

		case 'u':
		case 'U':
			Put(usr, "SU Passwd file\n");
			CALL(usr, STATE_PARAM_SU_PASSWD_FILE);
			Return;

		case 'x':
		case 'X':
			Put(usr, "Unix PID file\n");
			CALL(usr, STATE_PARAM_PID_FILE);
			Return;

		case 'y':
		case 'Y':
			Put(usr, "Symbol table file\n");
			CALL(usr, STATE_PARAM_SYMTAB_FILE);
			Return;

		case 'm':
		case 'M':
			Put(usr, "Host map file\n");
			CALL(usr, STATE_PARAM_HOSTMAP_FILE);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Files<white># ", PARAM_NAME_SYSOP);
	Return;
}


void state_param_gpl_screen(User *usr, char c) {
	Enter(state_param_gpl_screen);
	change_string_param(usr, c, &PARAM_GPL_SCREEN, "<green>Enter GPL file<yellow>: ");
	Return;
}

void state_param_mods_screen(User *usr, char c) {
	Enter(state_param_mods_screen);
	change_string_param(usr, c, &PARAM_MODS_SCREEN, "<green>Enter local mods file<yellow>: ");
	Return;
}

void state_param_login_screen(User *usr, char c) {
	Enter(state_param_login_screen);
	change_string_param(usr, c, &PARAM_LOGIN_SCREEN, "<green>Enter login screen<yellow>: ");
	Return;
}

void state_param_logout_screen(User *usr, char c) {
	Enter(state_param_logout_screen);
	change_string_param(usr, c, &PARAM_LOGOUT_SCREEN, "<green>Enter logout screen<yellow>: ");
	Return;
}

void state_param_nologin_screen(User *usr, char c) {
	Enter(state_param_nologin_screen);
	change_string_param(usr, c, &PARAM_NOLOGIN_SCREEN, "<green>Enter nologin screen<yellow>: ");
	Return;
}

void state_param_motd_screen(User *usr, char c) {
	Enter(state_param_motd_screen);
	change_string_param(usr, c, &PARAM_MOTD_SCREEN, "<green>Enter motd screen<yellow>: ");
	Return;
}

void state_param_reboot_screen(User *usr, char c) {
	Enter(state_param_reboot_screen);
	change_string_param(usr, c, &PARAM_REBOOT_SCREEN, "<green>Enter reboot screen<yellow>: ");
	Return;
}

void state_param_shutdown_screen(User *usr, char c) {
	Enter(state_param_shutdown_screen);
	change_string_param(usr, c, &PARAM_SHUTDOWN_SCREEN, "<green>Enter shutdown screen<yellow>: ");
	Return;
}

void state_param_crash_screen(User *usr, char c) {
	Enter(state_param_crash_screen);
	change_string_param(usr, c, &PARAM_CRASH_SCREEN, "<green>Enter crash screen<yellow>: ");
	Return;
}

void state_param_first_login(User *usr, char c) {
	Enter(state_param_first_login);
	change_string_param(usr, c, &PARAM_FIRST_LOGIN, "<green>Enter first login screen<yellow>: ");
	Return;
}

void state_param_help_std(User *usr, char c) {
	Enter(state_param_help_std);
	change_string_param(usr, c, &PARAM_HELP_STD, "<green>Enter standard help file<yellow>: ");
	Return;
}

void state_param_help_config(User *usr, char c) {
	Enter(state_param_help_config);
	change_string_param(usr, c, &PARAM_HELP_CONFIG, "<green>Enter Config menu help<yellow>: ");
	Return;
}

void state_param_help_roomconfig(User *usr, char c) {
	Enter(state_param_help_roomconfig);
	change_string_param(usr, c, &PARAM_HELP_ROOMCONFIG, "<green>Enter Room Config help<yellow>: ");
	Return;
}

void state_param_help_sysop(User *usr, char c) {
	Enter(state_param_help_sysop);
	change_string_param(usr, c, &PARAM_HELP_SYSOP, "<green>Enter Sysop menu help<yellow>: ");
	Return;
}

void state_param_hosts_access(User *usr, char c) {
	Enter(state_param_hosts_access);
	change_string_param(usr, c, &PARAM_HOSTS_ACCESS_FILE, "<green>Enter hosts_access file<yellow>: ");
	Return;
}

void state_param_banished_file(User *usr, char c) {
	Enter(state_param_banished_file);
	change_string_param(usr, c, &PARAM_BANISHED_FILE, "<green>Enter banished file<yellow>: ");
	Return;
}

void state_param_stat_file(User *usr, char c) {
	Enter(state_param_stat_file);
	change_string_param(usr, c, &PARAM_STAT_FILE, "<green>Enter statistics file<yellow>: ");
	Return;
}

void state_param_su_passwd_file(User *usr, char c) {
	Enter(state_param_su_passwd_file);
	change_string_param(usr, c, &PARAM_SU_PASSWD_FILE, "<green>Enter su_passwd file<yellow>: ");
	Return;
}

void state_param_pid_file(User *usr, char c) {
	Enter(state_param_pid_file);
	change_string_param(usr, c, &PARAM_PID_FILE, "<green>Enter PID file<yellow>: ");
	Return;
}

void state_param_symtab_file(User *usr, char c) {
	Enter(state_param_symtab_file);
	change_string_param(usr, c, &PARAM_SYMTAB_FILE, "<green>Enter symtab file<yellow>: ");
	Return;
}

void state_param_hostmap_file(User *usr, char c) {
	Enter(state_param_hostmap_file);
	change_string_param(usr, c, &PARAM_HOSTMAP_FILE, "<green>Enter hostmap file<yellow>: ");
	Return;
}


void state_reload_files_menu(User *usr, char c) {
StringList *sl;

	if (usr == NULL)
		return;

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			Put(usr, "<magenta>\n"
				"<hotkey>1 Reload login screen             <hotkey>6 Reload standard help\n"
				"<hotkey>2 Reload logout screen            <hotkey>7 Reload config menu help\n"
				"<hotkey>3 Reload motd screen              <hotkey>8 Reload room config menu help\n"
			);
			Put(usr,
				"<hotkey>4 Reload crash screen             <hotkey>9 Reload sysop menu help\n"
				"<hotkey>5 Reload first login screen\n"
				"<hotkey>g Reload GPL                      <hotkey>l Reload local mods\n"
				"<hotkey>h Reload hostmap                  <hotkey>f Reload feelings\n"
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;


#define RELOAD_FILE(x,y)						\
	if ((sl = load_StringList(x)) == NULL) {	\
		Perror(usr, "Failed to load file");		\
	} else {									\
		listdestroy_StringList(y);				\
		(y) = sl;								\
		Print(usr, "loading %s ... Ok\n", (x));	\
	}											\
	CURRENT_STATE(usr);							\
	Return;										\

		case '1':
			Put(usr, "<white>Reload login screen\n");
			RELOAD_FILE(PARAM_LOGIN_SCREEN, login_screen);

		case '2':
			Put(usr, "<white>Reload logout screen\n");
			RELOAD_FILE(PARAM_LOGOUT_SCREEN, logout_screen);

		case '3':
			Put(usr, "<white>Reload motd\n");
			RELOAD_FILE(PARAM_MOTD_SCREEN, motd_screen);

		case '4':
			Put(usr, "<white>Reload crash screen\n");
			RELOAD_FILE(PARAM_CRASH_SCREEN, crash_screen);

		case '5':
			Put(usr, "<white>Reload first login screen\n");
			RELOAD_FILE(PARAM_FIRST_LOGIN, first_login);

		case '6':
			Put(usr, "Reload standard help\n");
			RELOAD_FILE(PARAM_HELP_STD, help_std);

		case '7':
			Put(usr, "Reload config menu help\n");
			RELOAD_FILE(PARAM_HELP_CONFIG, help_config);

		case '8':
			Put(usr, "Reload room config menu help\n");
			RELOAD_FILE(PARAM_HELP_ROOMCONFIG, help_roomconfig);

		case '9':
			Put(usr, "Reload sysop menu help\n");
			RELOAD_FILE(PARAM_HELP_SYSOP, help_sysop);

		case 'g':
		case 'G':
		case KEY_CTRL('G'):
			Put(usr, "Reload GNU General Public License\n");
			RELOAD_FILE(PARAM_GPL_SCREEN, gpl_screen);

		case 'l':
		case ']':
			Put(usr, "Reload local modifications file\n");
			RELOAD_FILE(PARAM_MODS_SCREEN, mods_screen);

		case 'f':
		case 'F':
			Put(usr, "Reload feelings\n");
			if (init_Feelings()) {
				Perror(usr, "Failed to load Feelings");
			} else
				Print(usr, "loading feelings from %s ... Ok\n", PARAM_FEELINGSDIR);

			CURRENT_STATE(usr);
			Return;

		case 'h':
		case 'H':
			Put(usr, "Reload hostmap\n");
			if (load_HostMap(PARAM_HOSTMAP_FILE)) {
				Perror(usr, "Failed to load hostmap");
			} else
				Print(usr, "loading %s ... Ok\n", PARAM_HOSTMAP_FILE);

			CURRENT_STATE(usr);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Reload<white># ", PARAM_NAME_SYSOP);
}


void state_maximums_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_maximums_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Print(usr, "<magenta>\n"
				"Max number of <hotkey>Cached files            <white>%6u<magenta>\n"
				"Max number of messages kept in a <hotkey>Room <white>%6u<magenta>\n"
				"Max number of messages kept in <hotkey>Mail>  <white>%6u<magenta>\n"
				"Max number of lines in an <hotkey>X message   <white>%6u<magenta>\n"
				"Max number of <hotkey>Lines in a message      <white>%6u<magenta>\n",
				PARAM_MAX_CACHED,
				PARAM_MAX_MESSAGES,
				PARAM_MAX_MAIL_MSGS,
				PARAM_MAX_XMSG_LINES,
				PARAM_MAX_MSG_LINES
			);
			Print(usr,
				"Max lines in ch<hotkey>at room history        <white>%6u<magenta>\n"
				"Max number of messages in X <hotkey>History   <white>%6u<magenta>\n"
				"Max number of <hotkey>Friends                 <white>%6u<magenta>\n"
				"Max number of <hotkey>Enemies                 <white>%6u<magenta>\n",
				PARAM_MAX_CHAT_HISTORY,
				PARAM_MAX_HISTORY,
				PARAM_MAX_FRIEND,
				PARAM_MAX_ENEMY
			);
			Print(usr,
				"<hotkey>Idle timeout                          <white>%6u %s<magenta>\n"
				"Loc<hotkey>k timeout                          <white>%6u %s<magenta>\n"
				"Periodic <hotkey>saving                       <white>%6u %s<magenta>\n"
				"Online <hotkey>user hash size                 <white>%6u<magenta>\n",
				PARAM_IDLE_TIMEOUT, (PARAM_IDLE_TIMEOUT == 1) ? "minute" : "minutes",
				PARAM_LOCK_TIMEOUT, (PARAM_LOCK_TIMEOUT == 1) ? "minute" : "minutes",
				PARAM_SAVE_TIMEOUT, (PARAM_SAVE_TIMEOUT == 1) ? "minute" : "minutes",
				PARAM_USERHASH_SIZE
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Max Cached\n");
			CALL(usr, STATE_PARAM_CACHED);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Max messages in a room\n");
			CALL(usr, STATE_PARAM_MESSAGES);
			Return;

		case 'm':
		case 'M':
			Put(usr, "Max messages in Mail>\n");
			CALL(usr, STATE_PARAM_MAIL_MSGS);
			Return;

		case 'x':
		case 'X':
			Put(usr, "Max lines in an X message\n");
			CALL(usr, STATE_PARAM_XMSG_LINES);
			Return;

		case 'l':
		case 'L':
			Put(usr, "Max lines in a message\n");
			CALL(usr, STATE_PARAM_MSG_LINES);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Max chat room history\n");
			CALL(usr, STATE_PARAM_CHAT_HISTORY);
			Return;

		case 'h':
		case 'H':
			Put(usr, "Max X history\n");
			CALL(usr, STATE_PARAM_HISTORY);
			Return;

		case 'f':
		case 'F':
			Put(usr, "Max friends\n");
			CALL(usr, STATE_PARAM_FRIEND);
			Return;

		case 'e':
		case 'E':
			Put(usr, "Max enemies\n");
			CALL(usr, STATE_PARAM_ENEMY);
			Return;

		case 'i':
		case 'I':
			Put(usr, "Idle timeout\n");
			CALL(usr, STATE_PARAM_IDLE);
			Return;

		case 'k':
		case 'K':
			Put(usr, "Lock timeout\n");
			CALL(usr, STATE_PARAM_LOCK);
			Return;

		case 's':
		case 'S':
			Put(usr, "Periodic saving\n");
			CALL(usr, STATE_PARAM_SAVE);
			Return;

		case 'u':
		case 'U':
			Put(usr, "Online user hash size\n");
			CALL(usr, STATE_PARAM_USERHASH);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Maximums<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_param_cached(User *usr, char c) {
	Enter(state_param_cached);
	change_int_param(usr, c, &PARAM_MAX_CACHED);

	if ((PARAM_MAX_CACHED != cache_size) && (resize_Cache() == -1))
		Print(usr, "<red>Failed to resize the file cache, <yellow>max_cached<red> reset to <white>%d\n", PARAM_MAX_CACHED);
	else
		stats.cache_hit = stats.cache_miss = 0;		/* reset cache stats */
	Return;
}

void state_param_messages(User *usr, char c) {
	Enter(state_param_messages);
	change_int_param(usr, c, &PARAM_MAX_MESSAGES);
	Return;
}

void state_param_mail_msgs(User *usr, char c) {
	Enter(state_param_mail_msgs);
	change_int_param(usr, c, &PARAM_MAX_MAIL_MSGS);
	Return;
}

void state_param_xmsg_lines(User *usr, char c) {
	Enter(state_param_xmsg_lines);
	change_int_param(usr, c, &PARAM_MAX_XMSG_LINES);
	Return;
}

void state_param_msg_lines(User *usr, char c) {
	Enter(state_param_msg_lines);
	change_int_param(usr, c, &PARAM_MAX_MSG_LINES);
	Return;
}

void state_param_chat_history(User *usr, char c) {
	Enter(state_param_chat_history);
	change_int_param(usr, c, &PARAM_MAX_CHAT_HISTORY);
	Return;
}

void state_param_history(User *usr, char c) {
	Enter(state_param_history);
	change_int_param(usr, c, &PARAM_MAX_HISTORY);
	Return;
}

void state_param_friend(User *usr, char c) {
	Enter(state_param_friend);
	change_int_param(usr, c, &PARAM_MAX_FRIEND);
	Return;
}

void state_param_enemy(User *usr, char c) {
	Enter(state_param_enemy);
	change_int_param(usr, c, &PARAM_MAX_ENEMY);
	Return;
}

void state_param_idle(User *usr, char c) {
	Enter(state_param_idle);
	change_int_param(usr, c, &PARAM_IDLE_TIMEOUT);
	Return;
}

void state_param_lock(User *usr, char c) {
	Enter(state_param_lock);
	change_int_param(usr, c, &PARAM_LOCK_TIMEOUT);
	Return;
}

void state_param_save(User *usr, char c) {
	Enter(state_param_save);
	change_int_param(usr, c, &PARAM_SAVE_TIMEOUT);
	Return;
}

void state_param_userhash(User *usr, char c) {
	Enter(state_param_userhash);
	change_int_param(usr, c, &PARAM_USERHASH_SIZE);

	if ((PARAM_USERHASH_SIZE != online_users_size) && (resize_OnlineUser() == -1))
		Print(usr, "<red>Failed to resize the online user cache, <yellow>userhash_size<yellow> reset to <white>%d\n", PARAM_USERHASH_SIZE);
	Return;
}

void state_strings_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_maximums_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Print(usr, "\n"
				"<magenta>Name <hotkey>Sysop          <white>%s<magenta>\n"
				"<magenta>Name Room <hotkey>Aide      <white>%s<magenta>\n"
				"<magenta>Name <hotkey>Helper         <white>%s<magenta>\n"
				"<magenta>Name <hotkey>Guest          <white>%s<magenta>\n",
				PARAM_NAME_SYSOP,
				PARAM_NAME_ROOMAIDE,
				PARAM_NAME_HELPER,
				PARAM_NAME_GUEST
			);
			Print(usr, "\n"
				"<magenta>Notify logi<hotkey>n        %s\n"
				"<magenta>Notify log<hotkey>out       %s\n",
				PARAM_NOTIFY_LOGIN,
				PARAM_NOTIFY_LOGOUT
			);
			Print(usr,
				"<magenta>Notify link<hotkey>dead     %s\n"
				"<magenta>Notify <hotkey>idle         %s\n",
				PARAM_NOTIFY_LINKDEAD,
				PARAM_NOTIFY_IDLE
			);
			Print(usr,
				"<magenta>Notify <hotkey>locked       %s\n"
				"<magenta>Notify <hotkey>unlocked     %s\n",
				PARAM_NOTIFY_LOCKED,
				PARAM_NOTIFY_UNLOCKED
			);
			Print(usr,
				"<magenta>Notify <hotkey>enter chat   %s\n"
				"<magenta>Notify lea<hotkey>ve chat   %s\n",
				PARAM_NOTIFY_ENTER_CHAT,
				PARAM_NOTIFY_LEAVE_CHAT
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

		case 's':
		case 'S':
			Put(usr, "Name Sysop\n");
			CALL(usr, STATE_PARAM_NAME_SYSOP);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Name Room Aide\n");
			CALL(usr, STATE_PARAM_NAME_ROOMAIDE);
			Return;

		case 'h':
		case 'H':
			Put(usr, "Name Helper\n");
			CALL(usr, STATE_PARAM_NAME_HELPER);
			Return;

		case 'g':
		case 'G':
			Put(usr, "Name Guest\n");
			CALL(usr, STATE_PARAM_NAME_GUEST);
			Return;

		case 'n':
		case 'N':
			Put(usr, "Notify login\n");
			CALL(usr, STATE_PARAM_NOTIFY_LOGIN);
			Return;

		case 'o':
		case 'O':
			Put(usr, "Notify logout\n");
			CALL(usr, STATE_PARAM_NOTIFY_LOGOUT);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Notify linkdead\n");
			CALL(usr, STATE_PARAM_NOTIFY_LINKDEAD);
			Return;

		case 'i':
		case 'I':
			Put(usr, "Notify idle\n");
			CALL(usr, STATE_PARAM_NOTIFY_IDLE);
			Return;

		case 'l':
		case 'L':
			Put(usr, "Notify locked\n");
			CALL(usr, STATE_PARAM_NOTIFY_LOCKED);
			Return;

		case 'u':
		case 'U':
			Put(usr, "Notify unlocked\n");
			CALL(usr, STATE_PARAM_NOTIFY_UNLOCKED);
			Return;

		case 'e':
		case 'E':
			Put(usr, "Notify enter chat\n");
			CALL(usr, STATE_PARAM_NOTIFY_ENTER_CHAT);
			Return;

		case 'v':
		case 'V':
			Put(usr, "Notify leave chat\n");
			CALL(usr, STATE_PARAM_NOTIFY_LEAVE_CHAT);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Strings<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_param_name_sysop(User *usr, char c) {
	Enter(state_param_name_sysop);
	change_string_param(usr, c, &PARAM_NAME_SYSOP, "<green>Enter name Sysop<yellow>: ");
	Return;
}

void state_param_name_roomaide(User *usr, char c) {
	Enter(state_param_name_roomaide);
	change_string_param(usr, c, &PARAM_NAME_ROOMAIDE, "<green>Enter name Room Aide<yellow>: ");
	Return;
}

void state_param_name_helper(User *usr, char c) {
	Enter(state_param_name_helper);
	change_string_param(usr, c, &PARAM_NAME_HELPER, "<green>Enter name Helper<yellow>: ");
	Return;
}

void state_param_name_guest(User *usr, char c) {
	Enter(state_param_name_guest);
	change_string_param(usr, c, &PARAM_NAME_GUEST, "<green>Enter name Guest<yellow>: ");
	Return;
}

void state_param_notify_login(User *usr, char c) {
	Enter(state_param_notify_login);
	change_string_param(usr, c, &PARAM_NOTIFY_LOGIN, "<green>Enter login notification<yellow>: ");
	Return;
}

void state_param_notify_logout(User *usr, char c) {
	Enter(state_param_notify_logout);
	change_string_param(usr, c, &PARAM_NOTIFY_LOGOUT, "<green>Enter logout notification<yellow>: ");
	Return;
}

void state_param_notify_linkdead(User *usr, char c) {
	Enter(state_param_notify_linkdead);
	change_string_param(usr, c, &PARAM_NOTIFY_LINKDEAD, "<green>Enter linkdead notification<yellow>: ");
	Return;
}

void state_param_notify_idle(User *usr, char c) {
	Enter(state_param_notify_idle);
	change_string_param(usr, c, &PARAM_NOTIFY_IDLE, "<green>Enter idle notification<yellow>: ");
	Return;
}

void state_param_notify_locked(User *usr, char c) {
	Enter(state_param_notify_locked);
	change_string_param(usr, c, &PARAM_NOTIFY_LOCKED, "<green>Enter locked notification<yellow>: ");
	Return;
}

void state_param_notify_unlocked(User *usr, char c) {
	Enter(state_param_notify_unlocked);
	change_string_param(usr, c, &PARAM_NOTIFY_UNLOCKED, "<green>Enter unlocked notification<yellow>: ");
	Return;
}

void state_param_notify_enter_chat(User *usr, char c) {
	Enter(state_param_notify_enter_chat);
	change_string_param(usr, c, &PARAM_NOTIFY_ENTER_CHAT, "<green>Enter chat notification<yellow>: ");
	Return;
}

void state_param_notify_leave_chat(User *usr, char c) {
	Enter(state_param_notify_leave_chat);
	change_string_param(usr, c, &PARAM_NOTIFY_LEAVE_CHAT, "<green>Leave chat notification<yellow>: ");
	Return;
}



void change_int_param(User *usr, char c, unsigned int *var) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_int_param);

	if (c == INIT_STATE)
		Print(usr, "<green>Enter new value <white>[%u]: <yellow>", *var);

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			r = atoi(usr->edit_buf);
			if (r < 1)
				Put(usr, "<red>Invalid value; not changed\n");
			else {
				*var = (unsigned int)r;
				usr->runtime_flags |= RTF_PARAM_EDITED;
			}
		} else
			Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

/*
	practically copied from change_config() (in state_config.c) :P
	except that this routine sets RTF_PARAM_EDITED
*/
void change_string_param(User *usr, char c, char **var, char *prompt) {
int r;

	if (usr == NULL || var == NULL)
		return;

	Enter(change_string_param);

	if (c == INIT_STATE && prompt != NULL)
		Put(usr, prompt);

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0]) {
			cstrip_line(usr->edit_buf);

			if (!usr->edit_buf[0]) {
				Free(*var);
				*var = NULL;
			} else {
				char *s;

				if ((s = cstrdup(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				}
				Free(*var);
				*var = s;
			}
			usr->runtime_flags |= RTF_PARAM_EDITED;
		} else
			if (var != NULL && *var != NULL && **var)
				Put(usr, "<red>Not changed\n");
		RET(usr);
	}
	Return;
}

/* EOB */
