/*
    bbs100 2.2 WJ105
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
#include "edit_param.h"
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
#include "Category.h"
#include "Wrapper.h"

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
				if (save_Wrapper(AllWrappers, PARAM_HOSTS_ACCESS_FILE))
					Perror(usr, "failed to save wrappers");

				log_msg("SYSOP %s edited wrappers", usr->name);
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
			}
			usr->runtime_flags |= RTF_BUSY;
			Put(usr, "<magenta>\n"
				"Create new <hotkey>Room");

			if (usr->curr_room->number >= SPECIAL_ROOMS)
				Put(usr, "                   <white>Ctrl-<hotkey>D<magenta>elete Room\n");
			else
				Put(usr, "\n");

			if (PARAM_HAVE_CATEGORY)
				Put(usr, "Manage <hotkey>categories\n");

			Put(usr,
				"<hotkey>Disconnect user                   <white>Ctrl-<hotkey>N<magenta>uke User\n"
				"<hotkey>Banish user                       Edit <hotkey>wrappers\n"
			);
			Put(usr,
				"<hotkey>Uncache file                      <hotkey>Memory allocation status\n"
				"\n"
				"<white>Ctrl-<hotkey>P<magenta>arameters                   Sysop <hotkey>Password\n"
				"\n"
			);
			if (reboot_timer != NULL)
				Print(usr, "<white>Ctrl-<hotkey>R<magenta>eboot <white>(in progress)<magenta>         Cancel Reb<hotkey>oot\n");
			else
				Put(usr, "<white>Ctrl-<hotkey>R<magenta>eboot\n");

			if (shutdown_timer != NULL)
				Print(usr, "<white>Ctrl-<hotkey>S<magenta>hutdown <white>(in progress)<magenta>       Cancel <hotkey>Shutdown\n");
			else
				Put(usr, "<white>Ctrl-<hotkey>S<magenta>hutdown\n");

			if (!nologin_active)
				Put(usr, "Activate <hotkey>nologin                  <hotkey>Help\n");
			else
				Put(usr, "Deactivate <hotkey>nologin                <hotkey>Help\n");
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
			Put(usr, "Help\n");
			if (load_screen(usr->text, PARAM_HELP_SYSOP) < 0) {
				Put(usr, "<red>No help available\n");
				break;
			}
			PUSH(usr, STATE_PRESS_ANY_KEY);
			read_text(usr);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Create room\n");
			CALL(usr, STATE_CREATE_ROOM);
			Return;

		case KEY_CTRL('D'):
			if (usr->curr_room->number >= SPECIAL_ROOMS) {
				Put(usr, "Delete room\n");
				CALL(usr, STATE_DELETE_ROOM_NAME);
				Return;
			}
			break;

		case 'c':
		case 'C':
			if (PARAM_HAVE_CATEGORY) {
				Put(usr, "Categories\n");
				CALL(usr, STATE_CATEGORIES_MENU);
				Return;
			}
			break;

		case 'd':
		case 'D':
			Put(usr, "Disconnect user\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			enter_name(usr, STATE_DISCONNECT_USER);
			Return;

		case KEY_CTRL('N'):
			Put(usr, "Nuke user\n");

			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			enter_name(usr, STATE_NUKE_USER);
			Return;

		case 'b':
		case 'B':
			Put(usr, "Banish user\n");
			CALL(usr, STATE_BANISH_USER);
			Return;

		case 'w':
		case 'W':
			Put(usr, "Edit wrappers\n");
			usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
			CALL(usr, STATE_ADD_WRAPPER);
			Return;

		case 'u':
		case 'U':
			Put(usr, "Uncache file\n");
			CALL(usr, STATE_UNCACHE_FILE);
			Return;

		case 'm':
		case 'M':
			Put(usr, "Memory allocation status\n");
			CALL(usr, STATE_MALLOC_STATUS);
			Return;

		case KEY_CTRL('P'):
			Put(usr, "Parameters\n");
			CALL(usr, STATE_PARAMETERS_MENU);
			Return;

		case 'p':
		case 'P':
			Put(usr, "Password\n");
			CALL(usr, STATE_SU_PASSWD);
			Return;

		case KEY_CTRL('R'):
			Put(usr, "Reboot\n");
			CALL(usr, STATE_REBOOT_TIME);
			Return;

		case 'o':
		case 'O':
			if (reboot_timer != NULL) {
				Put(usr, "Cancel reboot\n"
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
			Put(usr, "Shutdown\n");
			CALL(usr, STATE_SHUTDOWN_TIME);
			Return;

		case 's':
		case 'S':
			if (shutdown_timer != NULL) {
				Put(usr, "Cancel shutdown\n"
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
			if (nologin_active) {
				Put(usr, "Deactivate nologin\n"
					"Deactivated\n");

				nologin_active = 0;
				log_msg("SYSOP %s deactivated nologin", usr->name);
				CURRENT_STATE(usr);
				Return;
			} else {
				Put(usr, "Activate nologin\n"
					"Activated\n");

				nologin_active = 1;
				log_msg("SYSOP %s activated nologin", usr->name);
				CURRENT_STATE(usr);
				Return;
			}
			break;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] # ", PARAM_NAME_SYSOP);
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

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
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
		if ((room = new_Room()) == NULL) {
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		if ((room->name = cstrdup(usr->edit_buf)) == NULL) {
			Perror(usr, "Out of memory");
			destroy_Room(room);
			RET(usr);
			Return;
		}
		room->generation = (unsigned long)rtc;
		room->flags = (ROOM_HIDDEN | ROOM_READONLY | ROOM_SUBJECTS | ROOM_INVITE_ONLY);

/* find a room number */
		room->number = SPECIAL_ROOMS;			/* lowest possible new room number */
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

		if (PARAM_HAVE_CATEGORY)
			AllRooms = sort_Room(AllRooms, room_sort_by_category);
		else
			AllRooms = sort_Room(AllRooms, room_sort_by_number);

		JMP(usr, STATE_ROOM_CONFIG_MENU);
		usr->runtime_flags |= RTF_ROOM_EDITED;
	}
	Return;
}

void state_categories_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_categories_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			if (category != NULL) {
				StringList *sl;
				int n;

				Put(usr, "\n");
				n = 1;
				for(sl = category; sl != NULL; sl = sl->next) {
					Print(usr, "<white>%2d<green> %s\n", n, sl->str);
					n++;
				}
			}
			Print(usr, "<magenta>\n"
				"<hotkey>Add category\n"
				"<hotkey>Remove category\n"
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			if (usr->runtime_flags & RTF_CATEGORY_EDITED) {
				if (save_Category()) {
					Perror(usr, "failed to save categories file");
				}
				usr->runtime_flags &= ~RTF_CATEGORY_EDITED;
			}
			RET(usr);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Add category\n");
			CALL(usr, STATE_ADD_CATEGORY);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Remove category\n");
			CALL(usr, STATE_REMOVE_CATEGORY);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Categories<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_add_category(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_add_category);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter new category<yellow>: ");

	r = edit_roomname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if (in_Category(usr->edit_buf))
			Put(usr, "<red>Category already exists\n");
		else {
			add_Category(usr->edit_buf);
			usr->runtime_flags |= RTF_CATEGORY_EDITED;
		}
		RET(usr);
		Return;
	}
	Return;
}

void state_remove_category(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_remove_category);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of category to remove<yellow>: ");

	r = edit_number(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		int n;
		StringList *sl;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		n = atoi(usr->edit_buf) - 1;
		if (n < 0) {
			Put(usr, "<red>No such category\n");
			RET(usr);
			Return;
		}
		for(sl = category; sl != NULL; sl = sl->next) {
			if (n <= 0)
				break;
			n--;
		}
		if (sl == NULL) {
			Put(usr, "<red>No such category\n");
			RET(usr);
			Return;
		}
		remove_StringList(&category, sl);
		destroy_StringList(sl);

		usr->runtime_flags |= RTF_CATEGORY_EDITED;

		RET(usr);
		Return;
	}
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
		KVPair *su;

		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");
			RET(usr);
			Return;
		}
		for(su = su_passwd; su != NULL; su = su->next) {
			if (!strcmp(su->key, usr->edit_buf)) {
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
		char buf[MAX_LINE*3], addr_buf[MAX_LINE], mask_buf[MAX_LINE];

		if (PARAM_HAVE_WRAPPER_ALL && !allow_Wrapper(usr->conn->ipnum, WRAPPER_ALL_USERS))
			Put(usr, "\n<red>WARNING<yellow>:<red> You are currently locked out yourself\n");

		Print(usr, "\n<yellow> 1 <white>Add new wrapper\n");
		i = 2;
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (PARAM_HAVE_WRAPPER_ALL)
				sprintf(buf, "<yellow>%2d <white>%s%s %s/%s",
					i, (w->flags & WRAPPER_ALLOW) ? "allow" : "deny",
					(w->flags & WRAPPER_APPLY_ALL) ? "_all" : "",
					print_inet_addr(w->addr, addr_buf, w->flags), print_inet_mask(w->mask, mask_buf, w->flags));
			else
				sprintf(buf, "<yellow>%2d <white>%s %s/%s",
					i, (w->flags & WRAPPER_ALLOW) ? "allow" : "deny",
					print_inet_addr(w->addr, addr_buf, w->flags), print_inet_mask(w->mask, mask_buf, w->flags));

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
		int j;

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
			int addr[8], mask[8];

			Put(usr, "Add wrapper\n");
			if ((w = new_Wrapper()) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			memset(addr, 0, sizeof(int)*8);

			for(j = 0; j < 8; j++)
				mask[j] = 0xffff;

			set_Wrapper(w, 0, addr, mask, NULL);
			add_Wrapper(&AllWrappers, w);
			usr->read_lines = list_Count(AllWrappers) - 1;
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
		} else {
			j = 2;
			for(w = AllWrappers; w != NULL && j < i; w = w->next)
				j++;

			if (w == NULL) {
				Put(usr, "<red>Invalid entry\n");
				CURRENT_STATE(usr);
				Return;
			}
			usr->read_lines = i-2;
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
int i;
char buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(state_edit_wrapper);

	i = 0;
	for(w = AllWrappers; w != NULL; w = w->next) {
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

			if (PARAM_HAVE_WRAPPER_ALL && !allow_one_Wrapper(w, usr->conn->ipnum, WRAPPER_ALL_USERS))
				Put(usr, "\n<red>WARNING<yellow>:<red> You are locking yourself out\n");

			Print(usr, "<magenta>\n"
				"<hotkey>Allow/deny connection        <white>%s<magenta>\n",
				(w->flags & WRAPPER_ALLOW) ? "Allow" : "Deny");

			if (PARAM_HAVE_WRAPPER_ALL)
				Print(usr, "This rule applies <hotkey>to ...     <white>%s<magenta>\n",
					(w->flags & WRAPPER_APPLY_ALL) ? "All users" : "New users only");

			Print(usr, "<hotkey>IP address                   <white>%s<magenta>\n",
				print_inet_addr(w->addr, buf, w->flags));

			Print(usr, "IP <hotkey>mask                      <white>%s<magenta>\n",
				print_inet_mask(w->mask, buf, w->flags));

			Print(usr,
				"<hotkey>Comment                      <cyan>%s<magenta>\n"
				"\n"
				"<hotkey>Delete this wrapper\n",

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
			Put(usr, "Allow/deny connection\n");
			w->flags ^= WRAPPER_ALLOW;
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 't':
		case 'T':
			if (PARAM_HAVE_WRAPPER_ALL) {
				w->flags ^= WRAPPER_APPLY_ALL;
				Print(usr, "Apply to %s\n", (w->flags & WRAPPER_APPLY_ALL) ? "All users" : "New users only");
				usr->runtime_flags |= RTF_WRAPPER_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case 'i':
		case 'I':
			Put(usr, "IP address\n");
			CALL(usr, STATE_IPADDR_WRAPPER);
			Return;

		case 'm':
		case 'M':
			Put(usr, "IP mask\n");
			CALL(usr, STATE_IPMASK_WRAPPER);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Comment\n");
			CALL(usr, STATE_COMMENT_WRAPPER);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Delete\n");
			remove_Wrapper(&AllWrappers, w);
			destroy_Wrapper(w);
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
			RET(usr);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Wrapper<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_ipaddr_wrapper(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_ipaddr_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter IP address<yellow>: ");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		i = 0;
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (i == usr->read_lines)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		if (read_inet_addr(usr->edit_buf, w->addr, &w->flags)) {
			Put(usr, "<red>Bad IP net address (use numeric notation)\n");
			RET(usr);
			Return;
		}
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
		int i;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		i = 0;
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (i == usr->read_lines)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		if (read_inet_mask(usr->edit_buf, w->mask, w->flags)) {
			Put(usr, "<red>Bad IP mask\n");
			RET(usr);
			Return;
		}
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
		for(w = AllWrappers; w != NULL; w = w->next) {
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
			usr->read_lines = 4 * SECS_IN_MIN;
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
char total_buf[MAX_LINE];

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
"Enter reboot password: ", print_total_time((unsigned long)usr->read_lines + (unsigned long)SECS_IN_MIN, total_buf));

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

			Print(usr, "<red>Reboot time altered to %s (including one minute grace period)\n", print_total_time((unsigned long)usr->read_lines + (unsigned long)SECS_IN_MIN, total_buf));

			sprintf(buf, "The system is now rebooting in %s",
				print_total_time((unsigned long)reboot_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
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
				print_total_time((unsigned long)reboot_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
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
			usr->read_lines = 4 * SECS_IN_MIN;
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
char total_buf[MAX_LINE];

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
"Enter shutdown password: ", print_total_time((unsigned long)usr->read_lines + (unsigned long)SECS_IN_MIN, total_buf));

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
			Print(usr, "<red>Shutdown time altered to %s (including one minute grace period)\n", print_total_time((unsigned long)usr->read_lines + (unsigned long)SECS_IN_MIN, total_buf));

			sprintf(buf, "The system is now shutting down in %s",
				print_total_time((unsigned long)shutdown_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
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
				print_total_time((unsigned long)shutdown_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
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
				char crypted[MAX_CRYPTED], *p;
				KVPair *su;

				crypt_phrase(usr->edit_buf, crypted);

				if (verify_phrase(usr->edit_buf, crypted)) {
					Perror(usr, "bug in password encryption -- please choose an other password");
					CURRENT_STATE(usr);
					Return;
				}
				for(su = su_passwd; su != NULL; su = su->next) {
					if (!strcmp(su->key, usr->name)) {
						if ((p = cstrdup(crypted)) == NULL) {
							Perror(usr, "Out of memory, password NOT changed");
							RET(usr);
							Return;
						}
						Free(su->value.s);
						su->value.s = p;

						if (save_SU_Passwd(PARAM_SU_PASSWD_FILE)) {
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
		int i, len = 0, l;
		char num_buf[25], line[PRINT_BUF];

		Print(usr, "\n<green>Total memory in use: <yellow>%s <green>bytes\n\n", print_number(memory_total, num_buf));

		for(i = 0; i < NUM_TYPES+1; i++) {
			if (strlen(Types_table[i].type) > len)
				len = strlen(Types_table[i].type);
		}
		l = 0;
		line[0] = 0;
		for(i = 0; i < NUM_TYPES+1; i++) {
			if (i & 1)
				l += sprintf(line+l, "      ");

			l += sprintf(line+l, "<green>%-*s <yellow>:<white> %12s ", len, Types_table[i].type, print_number(mem_stats[i], num_buf));

			if (i & 1) {
				Print(usr, "%s\n", line);
				l = 0;
				line[0] = 0;
			} else {
				line[l++] = ' ';
				line[l] = 0;
			}
		}
		if (!(i & 1))
			Put(usr, "\n");

		Put(usr, "\n"
			"<white>[Press a key]");
	} else {
		wipe_line(usr);
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
				"System <hotkey>Configuration\n"
				"Configure locations of <hotkey>Files\n"
				"Configure <hotkey>Maximums and timeouts\n"
				"Configure <hotkey>Strings and messages\n"
			);
			Print(usr,
				"Configure <hotkey>Log rotation\n"
				"<hotkey>Toggle features\n"
				"<hotkey>Reload screens and help files\n"
				"\n"
				"<white>Ctrl-<hotkey>R<magenta>eload param file <white>%s\n", param_file);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			if (usr->runtime_flags & RTF_PARAM_EDITED) {
				if (save_Param(param_file))
					Perror(usr, "failed to save param file");

				usr->runtime_flags &= ~RTF_PARAM_EDITED;
			}
			RET(usr);
			Return;


		case 'c':
		case 'C':
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
			Put(usr, "Configure maximums and timeouts\n");
			CALL(usr, STATE_MAXIMUMS_MENU);
			Return;

		case 's':
		case 'S':
			Put(usr, "Configure strings and messages\n");
			CALL(usr, STATE_STRINGS_MENU);
			Return;

		case 'l':
		case 'L':
			Put(usr, "Configure log rotation\n");
			CALL(usr, STATE_LOG_MENU);
			Return;

		case 't':
		case 'T':
			Put(usr, "Toggle features\n");
			CALL(usr, STATE_FEATURES_MENU);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Reload screens and help files\n");
			CALL(usr, STATE_RELOAD_FILES_MENU);
			Return;

		case KEY_CTRL('R'):
			Put(usr, "Reload param file\n");

			usr->runtime_flags &= ~RTF_PARAM_EDITED;

			path_strip(param_file);

			if (load_Param(param_file))
				Perror(usr, "Failed to load param file");
			else {
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

			PARAM_UMASK &= 0777;
			umask(PARAM_UMASK);

			Print(usr, "<magenta>\n"
				"BBS <hotkey>Name            <white>%s<magenta>\n"
				"P<hotkey>ort number         <white>%s<magenta>\n"
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
				"<hotkey>Zoneinfo directory  <white>%s<magenta>\n",
				PARAM_ZONEINFODIR
			);
			Print(usr,
				"<hotkey>User directory      <white>%s<magenta>\n"
				"<hotkey>Room directory      <white>%s<magenta>\n"
				"<hotkey>Trash directory     <white>%s<magenta>\n"
				"umas<hotkey>k               <white>0%02o<magenta>\n",
				PARAM_USERDIR,
				PARAM_ROOMDIR,
				PARAM_TRASHDIR,
				PARAM_UMASK
			);
			Print(usr, "\n"
				"<hotkey>Main program        <white>%s<magenta>\n"
				"Resol<hotkey>ver program    <white>%s<magenta>\n",
				PARAM_PROGRAM_MAIN,
				PARAM_PROGRAM_RESOLVER
			);
			Print(usr, "\n"
				"Default time<hotkey>zone    <white>%s<magenta>\n",
				PARAM_DEFAULT_TIMEZONE
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

		case 'Z':
			Put(usr, "Zoneinfo directory\n");
			CALL(usr, STATE_PARAM_ZONEINFODIR);
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

		case 'k':
		case 'K':
			Put(usr, "umask\n");
			CALL(usr, STATE_PARAM_UMASK);
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

		case 'z':
			Put(usr, "Default timezone\n");
			CALL(usr, STATE_PARAM_DEF_TIMEZONE);
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
	change_string_param(usr, c, &PARAM_PORT_NUMBER, "<green>Enter service name or port number<yellow>: ");
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

void state_param_zoneinfodir(User *usr, char c) {
	Enter(state_param_zoneinfodir);
	change_string_param(usr, c, &PARAM_ZONEINFODIR, "<green>Enter zoneinfo directory<yellow>: ");
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

void state_param_umask(User *usr, char c) {
	Enter(state_param_umask);
	change_octal_param(usr, c, &PARAM_UMASK);
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

			Print(usr, "Cred<hotkey>its          <white>%-22s<magenta>\n"
				"\n"
				"Standard <hotkey>help    <white>%-22s<magenta>  Room config h<hotkey>elp <white>%s<magenta>\n",
				PARAM_CREDITS_SCREEN, PARAM_HELP_STD, PARAM_HELP_ROOMCONFIG);
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

		case 'i':
		case 'I':
			Put(usr, "Credits screen\n");
			CALL(usr, STATE_PARAM_CREDITS_SCREEN);
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

		case 'm':
		case 'M':
			Put(usr, "Host map file\n");
			CALL(usr, STATE_PARAM_HOSTMAP_FILE);
			Return;

		case 'y':
		case 'Y':
			Put(usr, "Symbol table file\n");
			CALL(usr, STATE_PARAM_SYMTAB_FILE);
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

void state_param_credits_screen(User *usr, char c) {
	Enter(state_param_first_login);
	change_string_param(usr, c, &PARAM_CREDITS_SCREEN, "<green>Enter credits screen<yellow>: ");
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

void state_param_def_timezone(User *usr, char c) {
	Enter(state_param_def_timezone);
	change_string_param(usr, c, &PARAM_DEFAULT_TIMEZONE, "<green>Enter default timezone<yellow>: ");
	Return;
}


void state_reload_files_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			Put(usr, "<magenta>\n"
				"Reload log<hotkey>in screen             Reload <hotkey>1st login screen\n"
				"Reload log<hotkey>out screen            Reload <hotkey>user help\n"
				"Reload <hotkey>motd screen              Reload <hotkey>config menu help\n"
			);
			Put(usr,
				"Reload cr<hotkey>edits screen           Reload <hotkey>room config menu help\n"
				"Reload <hotkey>boss screen              Reload <hotkey>sysop menu help\n"
				"Reload cr<hotkey>ash screen             Reload <hotkey>nologin screen\n"
			);
			Put(usr,
				"\n"
				"Reload <hotkey>hostmap                  Reload <hotkey>feelings\n"
				"Reload <hotkey>local mods               Reload <hotkey>GPL\n"
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

#define UNCACHE_FILE(x)								\
	remove_Cache_filename(x);						\
	Print(usr, "File %s uncached\n", (x));			\
	CURRENT_STATE(usr);								\
	Return

#define RELOAD_FILE(x,y)											\
	free_StringIO(y);												\
	if (load_StringIO((y), (x)) < 0)								\
		Print(usr, "<red>Failed to load file <white>%s\n", (x));	\
	else															\
		Print(usr, "Reloaded file %s\n", (x));						\
	CURRENT_STATE(usr);												\
	Return

		case 'i':
		case 'I':
			Put(usr, "Reload login screen\n");
			UNCACHE_FILE(PARAM_LOGIN_SCREEN);

		case 'o':
		case 'O':
			Put(usr, "Reload logout screen\n");
			UNCACHE_FILE(PARAM_LOGOUT_SCREEN);

		case 'm':
		case 'M':
			Put(usr, "Reload motd\n");
			UNCACHE_FILE(PARAM_MOTD_SCREEN);

		case 'e':
		case 'E':
			Put(usr, "Reload credits screen\n");
			UNCACHE_FILE(PARAM_CREDITS_SCREEN);

		case 'b':
		case 'B':
			Put(usr, "Reload boss screen\n");
			UNCACHE_FILE(PARAM_BOSS_SCREEN);
/*
	the crash screen is not cached
	(just in case the cache becomes corrupted as well)
*/
		case 'a':
		case 'A':
			Put(usr, "Reload crash screen\n");
			RELOAD_FILE(PARAM_CRASH_SCREEN, crash_screen);

		case 'n':
		case 'N':
			Put(usr, "Reload nologin screen\n");
			UNCACHE_FILE(PARAM_NOLOGIN_SCREEN);

		case '1':
			Put(usr, "Reload 1st login screen\n");
			UNCACHE_FILE(PARAM_FIRST_LOGIN);

		case 'u':
		case 'U':
			Put(usr, "Reload user help\n");
			UNCACHE_FILE(PARAM_HELP_STD);

		case 'c':
		case 'C':
			Put(usr, "Reload config menu help\n");
			UNCACHE_FILE(PARAM_HELP_CONFIG);

		case 'r':
		case 'R':
			Put(usr, "Reload room config menu help\n");
			UNCACHE_FILE(PARAM_HELP_ROOMCONFIG);

		case 's':
		case 'S':
			Put(usr, "Reload sysop menu help\n");
			UNCACHE_FILE(PARAM_HELP_SYSOP);

		case 'g':
		case 'G':
		case KEY_CTRL('G'):
			Put(usr, "Reload GNU General Public License\n");
			UNCACHE_FILE(PARAM_GPL_SCREEN);

		case 'l':
		case ']':
			Put(usr, "Reload local modifications file\n");
			UNCACHE_FILE(PARAM_MODS_SCREEN);

		case 'f':
		case 'F':
			Put(usr, "Reload feelings\n");
			if (init_Feelings())
				Perror(usr, "Failed to load Feelings");
			else
				Print(usr, "loading feelings from %s ... Ok\n", PARAM_FEELINGSDIR);

			CURRENT_STATE(usr);
			Return;

		case 'h':
		case 'H':
			Put(usr, "Reload hostmap\n");
			if (load_HostMap(PARAM_HOSTMAP_FILE))
				Perror(usr, "Failed to load hostmap");
			else
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
				"Max number of <hotkey>cached files            <white>%6u<magenta>\n"
				"Max number of messages kept in a <hotkey>Room <white>%6u<magenta>\n"
				"Max number of messages kept in <hotkey>Mail>  <white>%6u<magenta>\n"
				"Max number of lines in an <hotkey>X message   <white>%6u<magenta>\n"
				"Max number of <hotkey>lines in a message      <white>%6u<magenta>\n",
				PARAM_MAX_CACHED,
				PARAM_MAX_MESSAGES,
				PARAM_MAX_MAIL_MSGS,
				PARAM_MAX_XMSG_LINES,
				PARAM_MAX_MSG_LINES
			);
			Print(usr,
				"Max lines in ch<hotkey>at room history        <white>%6u<magenta>\n"
				"Max number of messages in X <hotkey>history   <white>%6u<magenta>\n"
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
				"Cache expire <hotkey>time                     <white>%6u %s<magenta>\n",
				PARAM_IDLE_TIMEOUT, (PARAM_IDLE_TIMEOUT == 1) ? "minute" : "minutes",
				PARAM_LOCK_TIMEOUT, (PARAM_LOCK_TIMEOUT == 1) ? "minute" : "minutes",
				PARAM_SAVE_TIMEOUT, (PARAM_SAVE_TIMEOUT == 1) ? "minute" : "minutes",
				PARAM_CACHE_TIMEOUT, (PARAM_CACHE_TIMEOUT == 1) ? "minute" : "minutes"
			);
			Print(usr,
				"Minimum helper a<hotkey>ge                    <white>%6u %s<magenta>\n",
				PARAM_HELPER_AGE, (PARAM_HELPER_AGE == 1) ? "day" : "days"
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
/*
	PARAM_CACHE_TIMEOUT was changed, so we need to reset the timer
	I'm actually abusing the RTF_WRAPPER_EDITED flag for this
*/
			if (usr->runtime_flags & RTF_WRAPPER_EDITED) {
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;

				if (expire_timer != NULL) {
					remove_Timer(&timerq, expire_timer);
					destroy_Timer(expire_timer);
					expire_timer = NULL;
				}
				if ((expire_timer = new_Timer(PARAM_CACHE_TIMEOUT * SECS_IN_MIN, cache_expire_timerfunc, TIMER_RESTART)) == NULL)
					log_err("state_maximums_menu(): failed to allocate a new cache_expire Timer");
				else
					add_Timer(&timerq, expire_timer);
			}
			RET(usr);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Max Cached\n");
			CALL(usr, STATE_PARAM_CACHED);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Max messages in a room\n"
				"<green>This is a default value that applies for new rooms only\n"
			);
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

		case 't':
		case 'T':
			Put(usr, "Cache expire time\n");
			CALL(usr, STATE_PARAM_CACHE_TIMEOUT);
			Return;

		case 'g':
		case 'G':
			Put(usr, "Minimum helper age\n");
			CALL(usr, STATE_PARAM_HELPER_AGE);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Maximums<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_param_cached(User *usr, char c) {
	Enter(state_param_cached);
	change_int0_param(usr, c, &PARAM_MAX_CACHED);

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

void state_param_cache_timeout(User *usr, char c) {
	Enter(state_param_cache_timeout);
	change_int_param(usr, c, &PARAM_CACHE_TIMEOUT);
	usr->runtime_flags |= RTF_WRAPPER_EDITED;		/* flag change in timer (see code above) */
	Return;
}

void state_param_helper_age(User *usr, char c) {
	Enter(state_param_helper_age);
	change_int0_param(usr, c, &PARAM_HELPER_AGE);
	Return;
}


void state_strings_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_strings_menu);

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


#define TOGGLE_FEAT(x, y)	do {	\
		(x) ^= PARAM_TRUE;			\
		Print(usr, "<white>%s %s\n", ((x) == PARAM_FALSE) ? "Disabling" : "Enabling", (y));		\
		usr->runtime_flags |= RTF_PARAM_EDITED;		\
		CURRENT_STATE(usr);			\
	} while(0)

#define TOGGLE_FEATURE(x, y)	do {	\
		TOGGLE_FEAT((x), (y));			\
		Return;							\
	} while(0)

void state_features_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_features_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Print(usr, "\n<magenta>"
				"e<hotkey>Xpress Messages      <white>%-3s<magenta>        Quic<hotkey>k X messaging     <white>%s<magenta>\n"
				"<hotkey>Emotes                <white>%-3s<magenta>        <hotkey>Talked To list        <white>%s<magenta>\n"
				"<hotkey>Feelings              <white>%-3s<magenta>        H<hotkey>old message mode     <white>%s<magenta>\n"
				"<hotkey>Questions             <white>%-3s<magenta>        Follow-<hotkey>up mode        <white>%s<magenta>\n",

				(PARAM_HAVE_XMSGS == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_QUICK_X == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_EMOTES == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_TALKEDTO == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_FEELINGS == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_HOLD == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_QUESTIONS == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_FOLLOWUP == PARAM_FALSE) ? "off" : "on"
			);
			Print(usr,
				"X <hotkey>Reply               <white>%-3s<magenta>        Ch<hotkey>at rooms            <white>%s<magenta>\n"
				"<hotkey>Home> room            <white>%-3s<magenta>        <hotkey>Mail> room            <white>%s<magenta>\n"
				"<hotkey>Calendar              <white>%-3s<magenta>        <hotkey>World clock           <white>%s<magenta>\n"
				"Cate<hotkey>gories            <white>%-3s<magenta>        C<hotkey>ycle unread rooms    <white>%s<magenta>\n",

				(PARAM_HAVE_X_REPLY == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_CHATROOMS == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_HOMEROOM == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_MAILROOM == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_CALENDAR == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_WORLDCLOCK == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_CATEGORY == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_CYCLE_ROOMS == PARAM_FALSE) ? "off" : "on"
			);
			Print(usr,
				"Mem o<hotkey>bject cache      <white>%-3s<magenta>        F<hotkey>ile cache            <white>%s<magenta>\n"
				"Wrapper a<hotkey>pply to All  <white>%-3s<magenta>        <hotkey>Display warnings      <white>%s<magenta>\n",

				(PARAM_HAVE_MEMCACHE == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_FILECACHE == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_WRAPPER_ALL == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_DISABLED_MSG == PARAM_FALSE) ? "off" : "on"
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

		case 'x':
		case 'X':
			TOGGLE_FEATURE(PARAM_HAVE_XMSGS, "eXpress Messages");

		case 'e':
		case 'E':
			TOGGLE_FEATURE(PARAM_HAVE_EMOTES, "Emotes");

		case 'f':
		case 'F':
			TOGGLE_FEATURE(PARAM_HAVE_FEELINGS, "Feelings");

		case 'q':
		case 'Q':
			TOGGLE_FEATURE(PARAM_HAVE_QUESTIONS, "Questions");

		case 'k':
		case 'K':
			TOGGLE_FEATURE(PARAM_HAVE_QUICK_X, "Quick X messaging");

		case 't':
		case 'T':
			TOGGLE_FEATURE(PARAM_HAVE_TALKEDTO, "Talked To lists");

		case 'o':
		case 'O':
			TOGGLE_FEATURE(PARAM_HAVE_HOLD, "Hold message mode");

		case 'u':
		case 'U':
			TOGGLE_FEATURE(PARAM_HAVE_FOLLOWUP, "Follow-up mode");

		case 'r':
		case 'R':
			TOGGLE_FEATURE(PARAM_HAVE_X_REPLY, "X Reply");

		case 'c':
		case 'C':
			TOGGLE_FEATURE(PARAM_HAVE_CALENDAR, "Calendar");

		case 'w':
		case 'W':
			TOGGLE_FEATURE(PARAM_HAVE_WORLDCLOCK, "World clock");

		case 'a':
		case 'A':
			TOGGLE_FEATURE(PARAM_HAVE_CHATROOMS, "Chat rooms");

		case 'm':
		case 'M':
			TOGGLE_FEATURE(PARAM_HAVE_MAILROOM, "the Mail> room");

		case 'h':
		case 'H':
			TOGGLE_FEATURE(PARAM_HAVE_HOMEROOM, "the Home> room");

		case 'g':
		case 'G':
			PARAM_HAVE_CATEGORY ^= PARAM_TRUE;
			Print(usr, "%s categories\n", (PARAM_HAVE_CATEGORY == PARAM_FALSE) ? "Disabling" : "Enabling");
			usr->runtime_flags |= RTF_PARAM_EDITED;

			if (PARAM_HAVE_CATEGORY)
				AllRooms = sort_Room(AllRooms, room_sort_by_category);
			else
				AllRooms = sort_Room(AllRooms, room_sort_by_number);

			CURRENT_STATE(usr);
			Return;

		case 'y':
		case 'Y':
			TOGGLE_FEATURE(PARAM_HAVE_CYCLE_ROOMS, "Cycle unread rooms");

		case 'b':
		case 'B':
			TOGGLE_FEATURE(PARAM_HAVE_MEMCACHE, "Object cache");

		case 'i':
		case 'I':
			TOGGLE_FEAT(PARAM_HAVE_FILECACHE, "File cache");
			if (PARAM_HAVE_FILECACHE == PARAM_FALSE)
				deinit_FileCache();
			else
				init_FileCache();
			Return;

		case 'p':
		case 'P':
			TOGGLE_FEATURE(PARAM_HAVE_WRAPPER_ALL, "Wrapper apply to All");

		case 'd':
		case 'D':
			TOGGLE_FEATURE(PARAM_HAVE_DISABLED_MSG, "warnings");
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Features<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_log_menu(User *usr, char c) {
char *new_val;

	if (usr == NULL)
		return;

	Enter(state_log_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			Print(usr, "\n<magenta>"
				"<hotkey>Syslog              <white>%s<magenta>\n"
				"<hotkey>Authlog             <white>%s<magenta>\n"
				"<hotkey>Rotate              <white>%s<magenta>\n"
				"Arch<hotkey>ive directory   <white>%s<magenta>\n",
				PARAM_SYSLOG,
				PARAM_AUTHLOG,
				PARAM_LOGROTATE,
				PARAM_ARCHIVEDIR
			);
			Print(usr,
				"\n"
				"<hotkey>On crash            <white>%s<magenta>\n"
				"<hotkey>Core dump directory <white>%s<magenta>\n",
				PARAM_ONCRASH,
				PARAM_CRASHDIR
			);
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
/*
	if edited, re-initialize logging
*/
			if (usr->runtime_flags & RTF_WRAPPER_EDITED) {
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
				init_log();
			}
			RET(usr);
			Return;


		case 's':
		case 'S':
			Put(usr, "Syslog\n");
			CALL(usr, STATE_PARAM_SYSLOG);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Authlog\n");
			CALL(usr, STATE_PARAM_AUTHLOG);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Rotate\n");
			if (!strcmp(PARAM_LOGROTATE, "never"))
				new_val = "daily";
			else
				if (!strcmp(PARAM_LOGROTATE, "daily"))
					new_val = "weekly";
				else
					if (!strcmp(PARAM_LOGROTATE, "weekly"))
						new_val = "monthly";
					else
						if (!strcmp(PARAM_LOGROTATE, "monthly"))
							new_val = "yearly";
						else
							new_val = "never";

			Free(PARAM_LOGROTATE);
			PARAM_LOGROTATE = cstrdup(new_val);
			CURRENT_STATE(usr);
			Return;

		case 'i':
		case 'I':
			Put(usr, "Archive directory\n");
			CALL(usr, STATE_PARAM_ARCHIVEDIR);
			Return;

		case 'o':
		case 'O':
			Put(usr, "On crash\n");
			if (!strcmp(PARAM_ONCRASH, "recover"))
				new_val = "dumpcore";
			else
				new_val = "recover";

			Free(PARAM_ONCRASH);
			PARAM_ONCRASH = cstrdup(new_val);
			CURRENT_STATE(usr);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Core dump directory\n");
			CALL(usr, STATE_PARAM_CRASHDIR);
			Return;
	}
	Print(usr, "\n<white>[<yellow>%s<white>] <yellow>Logrotate<white># ", PARAM_NAME_SYSOP);
	Return;
}

void state_param_syslog(User *usr, char c) {
	Enter(state_param_syslog);
	change_string_param(usr, c, &PARAM_SYSLOG, "<green>Enter syslog file<yellow>: ");
	usr->runtime_flags |= RTF_WRAPPER_EDITED;
	Return;
}

void state_param_authlog(User *usr, char c) {
	Enter(state_param_authlog);
	change_string_param(usr, c, &PARAM_AUTHLOG, "<green>Enter authlog file<yellow>: ");
	usr->runtime_flags |= RTF_WRAPPER_EDITED;
	Return;
}

void state_param_archivedir(User *usr, char c) {
	Enter(state_param_archivedir);
	change_string_param(usr, c, &PARAM_ARCHIVEDIR, "<green>Enter archive directory<yellow>: ");
	Return;
}

void state_param_crashdir(User *usr, char c) {
	Enter(state_param_crashdir);
	change_string_param(usr, c, &PARAM_CRASHDIR, "<green>Enter core dump directory<yellow>: ");
	Return;
}

/* EOB */
