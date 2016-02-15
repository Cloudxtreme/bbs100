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
#include "make_dir.h"
#include "Param.h"
#include "main.h"
#include "CachedFile.h"
#include "copyright.h"
#include "cstring.h"
#include "access.h"
#include "Feeling.h"
#include "Memory.h"
#include "HostMap.h"
#include "OnlineUser.h"
#include "Category.h"
#include "Wrapper.h"
#include "Signals.h"
#include "Slub.h"
#include "memset.h"
#include "bufprintf.h"
#include "NewUserLog.h"
#include "DirList.h"
#include "coredump.h"
#include "my_fcntl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


void state_sysop_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_sysop_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"Create new <hotkey>room                   <white>Ctrl-<hotkey>D<magenta>elete room\n");

			Put(usr, "\n"
				"<hotkey>Disconnect user                   <white>Ctrl-<hotkey>N<magenta>uke user\n"
				"<hotkey>Banish user                       Edit <hotkey>wrappers\n"
			);
			if (PARAM_HAVE_FEELINGS)
				Put(usr, "Manage <hotkey>feelings                   ");
			if (PARAM_HAVE_CATEGORY)
				Put(usr, "Manage <hotkey>categories\n");
			else
				Put(usr, "\n");

			Put(usr,
				"Manage <hotkey>screens and help files     View <hotkey>log files\n");

			if (PARAM_HAVE_FILECACHE)
				Put(usr, "<hotkey>Uncache file                      ");
#ifdef USE_SLUB
			Print(usr, "<hotkey>Memory allocation status\n");
#else
			Put(usr, "\n");
#endif	/* USE_SLUB */
			Print(usr, "\n"
				"<white>Ctrl-<hotkey>P<magenta>arameters                   %s <hotkey>password\n"
				"\n",
				PARAM_NAME_SYSOP
			);
			Print(usr, "<white>%sCtrl-<hotkey>R<magenta>eboot %s            ",
				(reboot_timer == NULL) ? "" : "Cancel ",
				(reboot_timer == NULL) ? "          " : "<white>[!]"
			);
			Print(usr, "<white>%sCtrl-<hotkey>S<magenta>hutdown%s\n",
				(shutdown_timer == NULL) ? "" : "Cancel ",
				(shutdown_timer == NULL) ? "" : " <white>[!]<magenta>"
			);
#ifdef DEBUG
			Put(usr, "<white>Ctrl-<hotkey>C<magenta>ore dump\n\n");
#endif
			if (!nologin_active)
				Put(usr, "Activate <hotkey>nologin                  <hotkey>Help\n");
			else
				Put(usr, "Deactivate <hotkey>nologin <white>[!]<magenta>            <hotkey>Help\n");

			read_menu(usr);
			Return;

		case '$':
			drop_sysop_privs(usr);

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'h':
		case 'H':
		case '?':
			if (sysop_help(usr))
				break;

			Return;

		case 'r':
		case 'R':
			Put(usr, "Create room\n");
			CALL(usr, STATE_CREATE_ROOM);
			Return;

		case KEY_CTRL('D'):
			Put(usr, "Delete room\n");
			CALL(usr, STATE_DELETE_ROOM_NAME);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Disconnect user\n");

			deinit_StringQueue(usr->recipients);

			enter_name(usr, STATE_DISCONNECT_USER);
			Return;

		case KEY_CTRL('N'):
			Put(usr, "Nuke user\n");

			deinit_StringQueue(usr->recipients);

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

#ifdef USE_SLUB
		case 'm':
		case 'M':
			Put(usr, "Memory allocation status\n");
			CALL(usr, STATE_MALLOC_STATUS);
			Return;
#endif	/* USE_SLUB */

		case 's':
		case 'S':
			Put(usr, "Screens and help files\n");
			CALL(usr, STATE_SCREENS_MENU);
			Return;

		case 'l':
		case 'L':
			Put(usr, "Log files\n");
			CALL(usr, STATE_VIEW_LOGS);
			Return;

		case 'f':
		case 'F':
			if (PARAM_HAVE_FEELINGS) {
				DirList *feelings;

				Put(usr, "Feelings\n");

				if ((feelings = list_DirList(PARAM_FEELINGSDIR, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_DIRS)) == NULL) {
					log_err("state_sysop_menu(): list_DirList(%s) failed", PARAM_FEELINGSDIR);
					Put(usr, "<red>Failed to read the Feelings directory\n");
					CURRENT_STATE(usr);
					Return;
				}
				PUSH_ARG(usr, &feelings, sizeof(DirList *));
				CALL(usr, STATE_FEELINGS_MENU);
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
			if (reboot_timer == NULL) {
				Put(usr, "Reboot\n");
				CALL(usr, STATE_REBOOT_TIME);
			} else {
				Put(usr, "Cancel reboot\n"
					"<red>Reboot cancelled\n"
				);
				remove_Timer(&timerq, reboot_timer);
				destroy_Timer(reboot_timer);
				reboot_timer = NULL;

				system_broadcast(0, "Reboot cancelled");
				log_msg("SYSOP %s cancelled reboot", usr->name);
				CURRENT_STATE(usr);
			}
			Return;

		case KEY_CTRL('S'):
			if (shutdown_timer == NULL) {
				Put(usr, "Shutdown\n");
				CALL(usr, STATE_SHUTDOWN_TIME);
			} else {
				Put(usr, "Cancel shutdown\n"
					"<red>Shutdown cancelled\n"
				);
				remove_Timer(&timerq, shutdown_timer);
				destroy_Timer(shutdown_timer);
				shutdown_timer = NULL;

				system_broadcast(0, "Shutdown cancelled");
				log_msg("SYSOP %s cancelled shutdown", usr->name);
				CURRENT_STATE(usr);
			}
			Return;

#ifdef DEBUG
		case KEY_CTRL('C'):
			Put(usr, "Core dump\n");
			CALL(usr, STATE_COREDUMP_YESNO);
			Return;
#endif

		case 'n':
		case 'N':
			if (nologin_active) {
				char filename[MAX_PATHLEN];

				Put(usr, "Deactivate nologin\n");
				bufprintf(filename, sizeof(filename), "%s/%s", PARAM_CONFDIR, NOLOGIN_FILE);
				unlink(filename);
				nologin_active = 0;
				log_msg("SYSOP %s deactivated nologin", usr->name);
				CURRENT_STATE(usr);
				Return;
			} else {
				CALL(usr, STATE_NOLOGIN_YESNO);
				Return;
			}
			break;
	}
	Print(usr, "<yellow>\n[%s] # <white>", PARAM_NAME_SYSOP);
	Return;
}


/*
	read Help for the sysop menu
*/
int sysop_help(User *usr) {
char filename[MAX_PATHLEN];

	Put(usr, "Help\n");
	bufprintf(filename, sizeof(filename), "%s/sysop", PARAM_HELPDIR);
	if (load_screen(usr->text, filename) < 0) {
		Put(usr, "<red>No help available\n");
		return 1;
	}
	PUSH(usr, STATE_PRESS_ANY_KEY);
	read_text(usr);
	return 0;
}


void state_create_room(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_create_room);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter new room name: <yellow>");

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

/*
	find the lowest possible unused room number
	this only works when the rooms are sorted by room number, so if we have categories,
	we must resort the list
*/
		if (PARAM_HAVE_CATEGORY)
			(void)sort_Room(&AllRooms, room_sort_by_number);

		room->number = SPECIAL_ROOMS;			/* lowest possible new room number */
		for(rm = AllRooms; rm != NULL; rm = rm->next) {
			if (room->number == rm->number)
				room->number++;
			else
				if (room->number < rm->number)
					break;
		}
		if (PARAM_HAVE_CATEGORY)
			(void)sort_Room(&AllRooms, room_sort_by_category);

		bufprintf(buf, sizeof(buf), "%s/%u", PARAM_ROOMDIR, room->number);
		path_strip(buf);
		if (make_dir(buf, (mode_t)0750) < 0) {
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
				(void)prepend_Joined(&usr->rooms, j);
			}
		} else {
			j->zapped = 0;
			j->generation = room->generation;
			j->last_read = 0UL;
		}
		Print(usr, "<yellow>The room has been assigned number <white>%u\n", room->number);
		log_msg("SYSOP %s created room %u %s", usr->name, room->number, room->name);

		(void)prepend_Room(&AllRooms, room);		/* add room to all rooms list */

		if (PARAM_HAVE_CATEGORY)
			(void)sort_Room(&AllRooms, room_sort_by_category);
		else
			(void)sort_Room(&AllRooms, room_sort_by_number);

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
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			if (category != NULL) {
				Put(usr, "\n");
				print_columns(usr, category, FORMAT_NUMBERED);
			}
			Print(usr, "<magenta>\n"
				"<hotkey>Add category\n"
			);
			if (category != NULL)
				Put(usr, "<hotkey>Remove category\n");

			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);

			if (usr->runtime_flags & RTF_CATEGORY_EDITED) {
				if (save_Category()) {
					Perror(usr, "failed to save categories file");
				}
				usr->runtime_flags &= ~RTF_CATEGORY_EDITED;
			}
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Add category\n");
			CALL(usr, STATE_ADD_CATEGORY);
			Return;

		case 'r':
		case 'R':
			if (category == NULL)
				break;

			Put(usr, "Remove category\n");
			CALL(usr, STATE_REMOVE_CATEGORY);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Categories# <white>", PARAM_NAME_SYSOP);
	Return;
}

void state_add_category(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_add_category);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter new category: <yellow>");

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
int n;

	if (usr == NULL)
		return;

	Enter(state_remove_category);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of category to remove: <yellow>");

	n = edit_number(usr, c);
	if (n == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (n == EDIT_RETURN) {
		int i;
		StringList *sl;
		Room *r;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		i = atoi(usr->edit_buf) - 1;
		if (i < 0) {
			Put(usr, "<red>No such category\n");
			RET(usr);
			Return;
		}
		for(sl = category; sl != NULL; sl = sl->next) {
			if (i <= 0)
				break;
			i--;
		}
		if (sl == NULL) {
			Put(usr, "<red>No such category\n");
			RET(usr);
			Return;
		}
/*
	remove this category from the rooms
*/
		for(r = AllRooms; r != NULL; r = r->next) {
			if (r->category != NULL && !strcmp(r->category, sl->str)) {
				Free(r->category);
				r->category = NULL;
				r->flags |= ROOM_DIRTY;
			}
		}
		for(r = HomeRooms; r != NULL; r = r->next) {
			if (r->category != NULL && !strcmp(r->category, sl->str)) {
				Free(r->category);
				r->category = NULL;
				r->flags |= ROOM_DIRTY;
			}
		}
		(void)remove_StringList(&category, sl);
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
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if (!strcmp(usr->edit_buf, usr->name)) {
			Put(usr, "<red>That's not a very good idea\n");
			RET(usr);
			Return;
		}
		if (is_online(usr->edit_buf) == NULL) {
			if (!user_exists(usr->edit_buf))
				Put(usr, "<red>No such user\n");
			else
				Print(usr, "<yellow>%s<red> is not online\n", usr->edit_buf);
			RET(usr);
			Return;
		}
		POP(usr);
		CALL(usr, STATE_DISCONNECT_YESNO);
		Return;
	}
	Return;
}

void state_disconnect_yesno(User *usr, char c) {
User *u;

	Enter(state_disconnect_yesno);

	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? (y/N): <white>");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			if ((u = is_online(usr->edit_buf)) != NULL) {
				if (is_sysop(u->name)) {
					Print(usr, "<red>You are not allowed to disconnect a fellow %s\n", PARAM_NAME_SYSOP);
					RET(usr);
					Return;
				}
				Put(u, "<red>\n"
					"\n"
					"<yellow>*** <red>Sorry, but you are being disconnected <white>NOW <yellow>***\n"
					"\n"
					"<normal>\n"
				);
				log_msg("SYSOP %s disconnected user %s", usr->name, usr->edit_buf);
				notify_linkdead(u);
				close_connection(u, "user was disconnected by %s", usr->name);
				u = NULL;
				Print(usr, "<yellow>%s<green> was disconnected\n", usr->edit_buf);
			} else {
				if (!user_exists(usr->edit_buf))
					Print(usr, "<yellow>%s<red> was already nuked by another %s\n", usr->edit_buf, PARAM_NAME_SYSOP);
				else
					Print(usr, "<yellow>%s<red> is not online anymore\n", usr->edit_buf);
			}
			RET(usr);
			Return;

		case YESNO_NO:
			RET(usr);
			Return;

		default:
			Print(usr, "<cyan>Disconnect user <white>%s<cyan>? (y/N): <white>", usr->edit_buf);
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
		if (!user_exists(usr->edit_buf)) {
			Put(usr, "<red>No such user\n");
			RET(usr);
			Return;
		}
		if (!strcmp(usr->name, usr->edit_buf)) {
			Put(usr, "<red>Suicide is not painless...\n");
			RET(usr);
			Return;
		}
		if (is_sysop(usr->edit_buf)) {
			Print(usr, "<red>You can't nuke someone who has <yellow>%s<red> access!\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;
		}
		POP(usr);
		CALL(usr, STATE_NUKE_YESNO);
	}
	Return;
}

/*
	usr->edit_buf is user to nuke
*/
void state_nuke_yesno(User *usr, char c) {
User *u;
char path[MAX_PATHLEN], newpath[MAX_PATHLEN];

	Enter(state_nuke_yesno);

	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? (y/N): <white>");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			if (!user_exists(usr->edit_buf)) {
				Print(usr, "<red>No such user, already nuked by another %s!\n", PARAM_NAME_SYSOP);
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
				close_connection(u, "user is being nuked by %s", usr->name);
				u = NULL;
			}
			bufprintf(path, sizeof(path), "%s/%c/%s", PARAM_USERDIR, usr->edit_buf[0], usr->edit_buf);
			path_strip(path);
			bufprintf(newpath, sizeof(newpath), "%s/%s", PARAM_TRASHDIR, path);
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
			Return;

		case YESNO_NO:
			RET(usr);
			Return;

		default:
			Print(usr, "<cyan>Delete user <white>%s<cyan>? (y/N): <white>", usr->edit_buf);
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
				Put(usr, "\n<magenta>Banished are:\n\n");
				print_columns(usr, banished, 0);
			} else
				Print(usr, "\n<magenta>Banished is: <yellow>%s\n", banished->str);
		}
		Put(usr, "\n");
		POP(usr);

		deinit_StringQueue(usr->recipients);

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
				(void)remove_StringList(&banished, sl);
				destroy_StringList(sl);
				Print(usr, "<green>Unbanished <yellow>%s\n", usr->edit_buf);
				log_msg("SYSOP %s unbanished user %s", usr->name, usr->edit_buf);
			} else {
				if (is_sysop(usr->edit_buf)) {
					Print(usr, "<red>You are not allowed to banish a fellow %s\n", PARAM_NAME_SYSOP);
					RET(usr);
					Return;
				}
				if ((sl = new_StringList(usr->edit_buf)) == NULL) {
					Perror(usr, "Out of memory");
					RET(usr);
					Return;
				} else {
					(void)add_StringList(&banished, sl);
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

	if (c == INIT_PROMPT) {
		Put(usr, "\n"
			"<green>Enter number: <yellow>");

		edit_number(usr, EDIT_INIT);
		Return;
	}
	if (c == INIT_STATE) {
		char buf[MAX_LONGLINE], addr_buf[MAX_LINE], mask_buf[MAX_LINE];

		buffer_text(usr);

		if (PARAM_HAVE_WRAPPER_ALL && !allow_Wrapper(usr->conn->ipnum, WRAPPER_ALL_USERS))
			Put(usr, "\n<red>WARNING: You are currently locked out yourself\n");

		Print(usr, "\n<yellow> 1 <magenta>Add new wrapper\n");
		i = 2;
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (PARAM_HAVE_WRAPPER_ALL)
				bufprintf(buf, sizeof(buf), "<yellow>%2d <white>%s%s %s/%s",
					i, (w->flags & WRAPPER_ALLOW) ? "allow" : "deny",
					(w->flags & WRAPPER_APPLY_ALL) ? "_all" : "",
					print_inet_addr(&w->addr, addr_buf, MAX_LINE, w->flags),
					print_inet_mask(&w->mask, mask_buf, MAX_LINE, w->flags));
			else
				bufprintf(buf, sizeof(buf), "<yellow>%2d <white>%s %s/%s",
					i, (w->flags & WRAPPER_ALLOW) ? "allow" : "deny",
					print_inet_addr(&w->addr, addr_buf, MAX_LINE, w->flags),
					print_inet_mask(&w->mask, mask_buf, MAX_LINE, w->flags));

			if (w->comment != NULL)
				Print(usr, "%-40s <cyan># %s\n", buf, w->comment);
			else
				Print(usr, "%s\n", buf);
			i++;
		}
		read_menu(usr);
		Return;
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
			IP_addr addr, mask;

			Put(usr, "Add wrapper\n");
			if ((w = new_Wrapper()) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			memset(&addr, 0, sizeof(IP_addr));
			memset(&mask, 0xff, sizeof(IP_addr));

/* by default, start with a wrapper for IPv4 */
			set_Wrapper(w, WRAPPER_IP4, &addr, &mask, NULL);
			(void)add_Wrapper(&AllWrappers, w);
			i = count_List(AllWrappers) - 1;
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
			i -= 2;
		}
		PUSH_ARG(usr, &i, sizeof(int));
		CALL(usr, STATE_EDIT_WRAPPER);
	}
	Return;
}

/*
	index to the wrapper to edit is on the stack
*/
void state_edit_wrapper(User *usr, char c) {
Wrapper *w;
int i, idx;
char buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(state_edit_wrapper);

	PEEK_ARG(usr, &idx, sizeof(int));
	i = 0;
	for(w = AllWrappers; w != NULL; w = w->next) {
		if (i == idx)
			break;
		i++;
	}
	if (w == NULL) {
		Perror(usr, "The wrapper to edit has gone up in smoke");
		POP_ARG(usr, &idx, sizeof(int));
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			if (PARAM_HAVE_WRAPPER_ALL && !allow_one_Wrapper(w, usr->conn->ipnum, WRAPPER_ALL_USERS))
				Put(usr, "\n<red>WARNING: You are locking yourself out\n");

			Print(usr, "<magenta>\n"
				"<hotkey>Allow/deny connection        <white>%s<magenta>\n",
				(w->flags & WRAPPER_ALLOW) ? "Allow" : "Deny");

			if (PARAM_HAVE_WRAPPER_ALL)
				Print(usr, "This rule applies <hotkey>to ...     <white>%s<magenta>\n",
					(w->flags & WRAPPER_APPLY_ALL) ? "All users" : "New users only");

			Print(usr, "<hotkey>IP address                   <white>%s<magenta>\n",
				print_inet_addr(&w->addr, buf, MAX_LINE, w->flags));

			Print(usr, "IP <hotkey>mask                      <white>%s<magenta>\n",
				print_inet_mask(&w->mask, buf, MAX_LINE, w->flags));

			Print(usr,
				"<hotkey>Comment                      <cyan>%s<magenta>\n"
				"\n"
				"<hotkey>Delete this wrapper\n",

				(w->comment == NULL) ? "" : w->comment
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);

			if (usr->runtime_flags & RTF_WRAPPER_EDITED) {
				if (save_Wrapper(AllWrappers, PARAM_HOSTS_ACCESS_FILE))
					Perror(usr, "failed to save wrappers");

				log_msg("SYSOP %s edited wrappers", usr->name);
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
			}
			POP_ARG(usr, &idx, sizeof(int));
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
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
			PUSH_ARG(usr, &idx, sizeof(int));
			CALL(usr, STATE_IPADDR_WRAPPER);
			Return;

		case 'm':
		case 'M':
			Put(usr, "IP mask\n");
			PUSH_ARG(usr, &idx, sizeof(int));
			CALL(usr, STATE_IPMASK_WRAPPER);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Comment\n");
			PUSH_ARG(usr, &idx, sizeof(int));
			CALL(usr, STATE_COMMENT_WRAPPER);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Delete\n");
			(void)remove_Wrapper(&AllWrappers, w);
			destroy_Wrapper(w);
			usr->runtime_flags |= RTF_WRAPPER_EDITED;
			POP_ARG(usr, &idx, sizeof(int));
			RET(usr);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Wrappers# <white>", PARAM_NAME_SYSOP);
	Return;
}

/*
	index to wrapper to edit is on the stack
*/
void state_ipaddr_wrapper(User *usr, char c) {
int r, idx;

	if (usr == NULL)
		return;

	Enter(state_ipaddr_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter IP address: <yellow>");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		POP_ARG(usr, &idx, sizeof(int));
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i;

		POP_ARG(usr, &idx, sizeof(int));
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		i = 0;
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (i == idx)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		if (read_wrapper_addr(w, usr->edit_buf)) {
			Put(usr, "<red>Bad IP net address (use numeric notation)\n");
			RET(usr);
			Return;
		}
		usr->runtime_flags |= RTF_WRAPPER_EDITED;
		RET(usr);
	}
	Return;
}

/*
	index to wrapper to edit is on the stack
*/
void state_ipmask_wrapper(User *usr, char c) {
int r, idx;

	if (usr == NULL)
		return;

	Enter(state_ipmask_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter IP mask: <yellow>");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		POP_ARG(usr, &idx, sizeof(int));
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i;

		POP_ARG(usr, &idx, sizeof(int));
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		i = 0;
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (i == idx)
				break;
			i++;
		}
		if (w == NULL) {
			Perror(usr, "The wrapper you were editing has gone up in smoke");
			RET(usr);
			Return;
		}
		if (read_wrapper_mask(w, usr->edit_buf)) {
			Put(usr, "<red>Bad IP mask\n");
			RET(usr);
			Return;
		}
		usr->runtime_flags |= RTF_WRAPPER_EDITED;
		RET(usr);
	}
	Return;
}

/*
	the index to the wrapper to edit is on the stack
*/
void state_comment_wrapper(User *usr, char c) {
int r, idx;

	if (usr == NULL)
		return;

	Enter(state_comment_wrapper);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter comment: <yellow>");

	r = edit_line(usr, c);
	if (r == EDIT_BREAK) {
		POP_ARG(usr, &idx, sizeof(int));
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Wrapper *w;
		int i = 0;

		POP_ARG(usr, &idx, sizeof(int));
		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		for(w = AllWrappers; w != NULL; w = w->next) {
			if (i == idx)
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
		Put(usr, "<green>Enter full room name: <yellow>");

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
		if ((room = find_Room(usr, usr->edit_buf)) == NULL) {
			Put(usr, "<red>No such room\n");
			RET(usr);
			Return;
		}
		POP(usr);
		CALL(usr, STATE_DELETE_ROOM_YESNO);
	}
	Return;
}

void state_delete_room_yesno(User *usr, char c) {
Room *r;

	Enter(state_delete_room_yesno);

	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? (y/N): <white>");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			if ((r = find_Room(usr, usr->edit_buf)) == NULL) {
				Print(usr, "<red>No such room, already deleted by another %s!\n", PARAM_NAME_SYSOP);
				RET(usr);
				Return;
			}
			delete_room(usr, r);
			RET(usr);
			Return;

		case YESNO_NO:
			RET(usr);
			Return;

		default:
			Print(usr, "<cyan>Delete room <white>%s<cyan>? (y/N): <white>", usr->edit_buf);
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
			r = 4 * SECS_IN_MIN;
		else
			r = atoi(usr->edit_buf);

		POP(usr);
		PUSH_ARG(usr, &r, sizeof(int));
		CALL(usr, STATE_REBOOT_PASSWORD);
	}
	Return;
}

/*
	Note: amount of seconds till reboot is on the stack
*/
void state_reboot_password(User *usr, char c) {
int r;
char total_buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(state_reboot_password);

	if (c == INIT_STATE) {
		PEEK_ARG(usr, &r, sizeof(int));

		Print(usr, "\n"
			"<yellow>*** <white>WARNING <yellow>***\n"
			"\n"
			"<red>This is serious. Enter the reboot password and the system will reboot\n"
			"in %s (including one minute grace period)\n"
			"\n"
			"Enter reboot password: ", print_total_time((unsigned long)r + (unsigned long)SECS_IN_MIN, total_buf, MAX_LINE));
	}
	r = edit_password(usr, c);
	if (r == EDIT_BREAK || (r == EDIT_RETURN && !usr->edit_buf[0])) {
		clear_password_buffer(usr);
		if (reboot_timer != NULL)
			Put(usr, "<red>Aborted, but note that another reboot procedure is already running\n");
		else
			Put(usr, "<red>Reboot cancelled\n");

		POP_ARG(usr, &r, sizeof(int));
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd, buf[PRINT_BUF];

		POP_ARG(usr, &r, sizeof(int));

		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {
			clear_password_buffer(usr);
			Put(usr, "<red>Wrong password\n");
			usr->runtime_flags &= ~RTF_SYSOP;
			POP(usr);				/* get out of sysop menu */
			RET(usr);
			Return;
		}
		if (verify_phrase(usr->edit_buf, pwd)) {
			clear_password_buffer(usr);
			Put(usr, "<red>Wrong password\n");
			RET(usr);
			Return;
		}
		clear_password_buffer(usr);

		if (reboot_timer != NULL) {
/* this code is never reached any more */
			reboot_timer->maxtime = r;
			reboot_timer->restart = TIMEOUT_REBOOT;
			set_Timer(&timerq, reboot_timer, reboot_timer->maxtime);

			Print(usr, "<red>Reboot time altered to %s (including one minute grace period)\n",
				print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));

			bufprintf(buf, sizeof(buf), "The system is now rebooting in %s",
				print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));
			system_broadcast(0, buf);
			RET(usr);
			Return;
		}
		if ((reboot_timer = new_Timer(r, reboot_timeout, TIMEOUT_REBOOT)) == NULL) {
			Perror(usr, "Out of memory, reboot cancelled");
			RET(usr);
			Return;
		}
		add_Timer(&timerq, reboot_timer);
/*
	Note: at this point, reboot_timer->sleeptime can be a smaller value, because
	      this timerqueue is sorted by relative time
	use time_to_dd() to get the correct value
*/
		log_msg("SYSOP %s initiated reboot", usr->name);

		Put(usr, "\n<red>Reboot procedure started\n");

		if (reboot_timer->sleeptime > 0) {
			bufprintf(buf, sizeof(buf), "The system is rebooting in %s",
				print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));
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
			r = 4 * SECS_IN_MIN;
		else
			r = atoi(usr->edit_buf);

		POP(usr);
		PUSH_ARG(usr, &r, sizeof(int));
		CALL(usr, STATE_SHUTDOWN_PASSWORD);
	}
	Return;
}

/*
	Note: amount of seconds till shutdown is on the stack
*/
void state_shutdown_password(User *usr, char c) {
int r;
char total_buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(state_shutdown_password);

	if (c == INIT_STATE) {
		PEEK_ARG(usr, &r, sizeof(int));

		Print(usr, "\n"
			"<yellow>*** <white>WARNING <yellow>***\n"
			"\n"
			"<red>This is serious. Enter the shutdown password and the system will shut\n"
			"down in %s (including one minute grace period)\n"
			"\n"
			"Enter shutdown password: ", print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));
	}
	r = edit_password(usr, c);
	if (r == EDIT_BREAK || (r == EDIT_RETURN && !usr->edit_buf[0])) {
		clear_password_buffer(usr);
		if (shutdown_timer != NULL)
			Put(usr, "<red>Aborted, but note that another shutdown procedure is already running\n");
		else
			Put(usr, "<red>Shutdown cancelled\n");

		POP_ARG(usr, &r, sizeof(int));
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *pwd, buf[PRINT_BUF];

		POP_ARG(usr, &r, sizeof(int));

		pwd = get_su_passwd(usr->name);
		if (pwd == NULL) {				/* not allowed to enter sysop commands! (how did we get here?) */
			clear_password_buffer(usr);
			Put(usr, "<red>Wrong password\n");
			usr->runtime_flags &= ~RTF_SYSOP;
			POP(usr);					/* drop out of sysop menu */
			RET(usr);
			Return;
		}
		if (verify_phrase(usr->edit_buf, pwd)) {
			clear_password_buffer(usr);
			Put(usr, "<red>Wrong password\n");
			RET(usr);
			Return;
		}
		clear_password_buffer(usr);

		if (shutdown_timer != NULL) {
/* this code is never reached any more */
			shutdown_timer->maxtime = r;
			shutdown_timer->restart = TIMEOUT_SHUTDOWN;
			set_Timer(&timerq, shutdown_timer, shutdown_timer->maxtime);
			Print(usr, "<red>Shutdown time altered to %s (including one minute grace period)\n",
				print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));

			bufprintf(buf, sizeof(buf), "The system is now shutting down in %s",
				print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));
			system_broadcast(0, buf);
			RET(usr);
			Return;
		}
		if ((shutdown_timer = new_Timer(r, shutdown_timeout, TIMEOUT_SHUTDOWN)) == NULL) {
			Perror(usr, "Out of memory, shutdown cancelled");
			RET(usr);
			Return;
		}
		add_Timer(&timerq, shutdown_timer);

		log_msg("SYSOP %s initiated shutdown", usr->name);

		Put(usr, "\n<red>Shutdown sequence initiated\n");

		if (shutdown_timer->sleeptime > 0) {
			bufprintf(buf, sizeof(buf), "The system is shutting down in %s",
				print_total_time((unsigned long)(r + SECS_IN_MIN), total_buf, MAX_LINE));
			system_broadcast(0, buf);
		}
		RET(usr);
	}
	Return;
}

#ifdef DEBUG
/*
	request a live core dump
*/
void state_coredump_yesno(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_coredump_yesno);

	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? (y/N): <white>");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			dumpcore(usr);
			RET(usr);
			Return;

		case YESNO_NO:
			RET(usr);
			Return;

		default:
			Put(usr, "<cyan>Dump core, <hotkey>yes or <hotkey>no? (y/N): <white>");
	}
	Return;
}
#endif	/* DEBUG */

void state_su_passwd(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_su_passwd);

	if (c == INIT_STATE)
		Print(usr, "<red>Enter <yellow>%s<red> mode password: ", PARAM_NAME_SYSOP);

	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
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
			clear_password_buffer(usr);
			Print(usr, "\n\n<red>You are not allowed to become <yellow>%s<red> any longer\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;
		}
		if (!verify_phrase(usr->edit_buf, pwd)) {
			clear_password_buffer(usr);
			JMP(usr, STATE_CHANGE_SU_PASSWD);
		} else {
			clear_password_buffer(usr);
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
		Print(usr, "<red>Enter new <yellow>%s<red> mode password: ", PARAM_NAME_SYSOP);

		Free(usr->tmpbuf[TMP_PASSWD]);
		usr->tmpbuf[TMP_PASSWD] = NULL;
	}
	r = edit_password(usr, c);

	if (r == EDIT_BREAK) {
		clear_password_buffer(usr);
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
				clear_password_buffer(usr);
				Put(usr, "<red>That password is too short\n");
				CURRENT_STATE(usr);
				Return;
			}
			Put(usr, "<red>Enter it again <white>(<red>for verification<white>):<red> ");

			if ((usr->tmpbuf[TMP_PASSWD] = cstrdup(usr->edit_buf)) == NULL) {
				clear_password_buffer(usr);
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			clear_password_buffer(usr);
		} else {
			if (!strcmp(usr->edit_buf, usr->tmpbuf[TMP_PASSWD])) {
				char crypted[MAX_CRYPTED], *p;
				KVPair *su;

				crypt_phrase(usr->edit_buf, crypted);

				if (verify_phrase(usr->edit_buf, crypted)) {
					clear_password_buffer(usr);
					Perror(usr, "bug in password encryption -- please choose an other password");
					CURRENT_STATE(usr);
					Return;
				}
				clear_password_buffer(usr);

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
						} else
							Print(usr, "<red>%s mode password changed\n", PARAM_NAME_SYSOP);

						Free(usr->tmpbuf[TMP_PASSWD]);
						usr->tmpbuf[TMP_PASSWD] = NULL;

						RET(usr);
						Return;
					}
				}
				Print(usr, "<red>You are not allowed to change the <yellow>%s<red> mode password anymore\n", PARAM_NAME_SYSOP);
				usr->runtime_flags &= ~RTF_SYSOP;
				POP(usr);
			} else {
				clear_password_buffer(usr);
				Print(usr, "<red>Passwords didn't match; <yellow>%s<red> mode password NOT changed\n", PARAM_NAME_SYSOP);
			}
			Free(usr->tmpbuf[TMP_PASSWD]);
			usr->tmpbuf[TMP_PASSWD] = NULL;

			RET(usr);
		}
	}
	Return;
}

void state_nologin_yesno(User *usr, char c) {
char filename[MAX_PATHLEN];

	Enter(state_nologin_yesno);

	if (c == INIT_STATE) {
		Put(usr, "Activate nologin\n"
			"<cyan>Are you sure? (y/N): <white>");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			bufprintf(filename, sizeof(filename), "%s/%s", PARAM_CONFDIR, NOLOGIN_FILE);
			close(open(filename, O_CREAT|O_WRONLY|O_TRUNC, (mode_t)0660));
			nologin_active = 1;
			log_msg("SYSOP %s activated nologin", usr->name);
			RET(usr);
			Return;

		case YESNO_NO:
			RET(usr);
			Return;

		default:
			Put(usr, "<cyan>Activate nologin, <hotkey>yes or <hotkey>no? (y/N): <white>");
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

#ifdef USE_SLUB
void state_malloc_status(User *usr, char c) {
int i, j, num_columns, idx;
char num_buf[MAX_NUMBER];

	if (usr == NULL)
		return;

	Enter(state_malloc_status);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			Put(usr, "\n");
			buffer_text(usr);

			Put(usr, "<yellow>Slab cache statistics\n<green>");

			/* print in 4 columns */
			num_columns = NUM_MEMCACHES / 4;
			for(j = 0; j < num_columns; j++) {
				for(i = 0; i < 4; i++) {
					idx = i * 4 + j;
					if (idx >= NUM_MEMCACHES) {
						continue;
					}
					Print(usr, " cache<yellow>[%3d]<white> %4d<green> ", (idx+1) * SLUB_SIZESTEP,
						memcache_info.nr_cache[idx]);
				}
				Put(usr, "\n");
			}
			Put(usr, "\n");

			Print(usr, " # of cache allocations<yellow>   %8d    %s kB<green>\n", memcache_info.nr_cache_all,
				print_number((unsigned long)memcache_info.cache_bytes / 1024UL, num_buf, sizeof(num_buf)));
			Print(usr, " # of allocated pages<yellow>     %8d    %s kB<green>\n", memcache_info.nr_pages,
				print_number((unsigned long)memcache_info.nr_pages * SLUB_PAGESIZE / 1024UL, num_buf, sizeof(num_buf)));
			Print(usr, " # of foreign allocations<yellow> %8d\n", memcache_info.nr_foreign);

			read_menu(usr);
			Return;
		
		default:
			wipe_line(usr);
			RET(usr);
			Return;
	}
	Put(usr, "<white>\n"
		"[Press a key]");
	Return;
}
#endif	/* USE_SLUB */

void state_screens_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_screens_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"Log<hotkey>in screen                      <hotkey>1st login screen\n"
				"Log<hotkey>out screen                     <hotkey>Motd screen\n"
				"Reboo<hotkey>t screen                     <hotkey>Nologin screen\n"
			);
			Put(usr,
				"Shut<hotkey>down screen                   <hotkey>Boss screen\n"
				"Cr<hotkey>ash screen                      Hostma<hotkey>p\n"
				"\n"
				"<hotkey>Local modifications               <hotkey>Help files\n"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'i':
		case 'I':
			screen_menu(usr, "Login screen", PARAM_LOGIN_SCREEN);
			Return;

		case 'o':
		case 'O':
			screen_menu(usr, "Logout screen", PARAM_LOGOUT_SCREEN);
			Return;

		case 'm':
		case 'M':
			screen_menu(usr, "Message of the day", PARAM_MOTD_SCREEN);
			Return;

		case 'b':
		case 'B':
			screen_menu(usr, "Boss screen", PARAM_BOSS_SCREEN);
			Return;

		case 't':
		case 'T':
			screen_menu(usr, "Reboot screen", PARAM_REBOOT_SCREEN);
			Return;

		case 'd':
		case 'D':
			screen_menu(usr, "Shutdown screen", PARAM_SHUTDOWN_SCREEN);
			Return;

		case 'a':
		case 'A':
			screen_menu(usr, "Crash screen", PARAM_CRASH_SCREEN);
			Return;

		case 'n':
		case 'N':
			screen_menu(usr, "Nologin screen", PARAM_NOLOGIN_SCREEN);
			Return;

		case '1':
			screen_menu(usr, "1st login screen", PARAM_FIRST_LOGIN);
			Return;

		case 'p':
		case 'P':
			screen_menu(usr, "Hostmap", PARAM_HOSTMAP_FILE);
			Return;

		case 'l':
		case 'L':
		case ']':
			screen_menu(usr, "Local modifications file", PARAM_MODS_SCREEN);
			Return;

		case 'h':
		case 'H':
			Put(usr, "Help files\n");
			CALL(usr, STATE_HELP_FILES);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Screens# <white>", PARAM_NAME_SYSOP);
	Return;
}

/*
	Note: 'desc' is const, but I don't use the qualifier
*/
void screen_menu(User *usr, char *desc, char *filename) {
	if (usr == NULL || desc == NULL || filename == NULL)
		return;

	Enter(screen_menu);

	Print(usr, "<white>%s\n", desc);
	Free(usr->tmpbuf[TMP_NAME]);
	if ((usr->tmpbuf[TMP_NAME] = cstrdup(filename)) == NULL) {
		Perror(usr, "Out of memory?");
		CURRENT_STATE(usr);
		Return;
	}
	Free(usr->tmpbuf[TMP_PASSWD]);
	usr->tmpbuf[TMP_PASSWD] = desc;

	Free(usr->tmpbuf[TMP_FROM_HOST]);
	usr->tmpbuf[TMP_FROM_HOST] = cstrdup(desc);

	CALL(usr, STATE_SCREEN_ACTION);
	Return;
}

void state_screen_action(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_screen_action);

	switch(c) {
		case INIT_STATE:
			Put(usr, "<magenta>\n"
				"<hotkey>View                     <hotkey>Upload\n"
				"<hotkey>Reload                   <hotkey>Download\n"
			);
			break;

		case ' ':
		case KEY_BS:
		case KEY_RETURN:
			Put(usr, "Screens menu\n");

			Free(usr->tmpbuf[TMP_NAME]);
			usr->tmpbuf[TMP_NAME] = NULL;
/*			Free(usr->tmpbuf[TMP_PASSWD]);		don't free the 'const' pointer */
			usr->tmpbuf[TMP_PASSWD] = NULL;
			Free(usr->tmpbuf[TMP_FROM_HOST]);
			usr->tmpbuf[TMP_FROM_HOST] = NULL;
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'v':
		case 'V':
			Put(usr, "View\n");
			view_file(usr, usr->tmpbuf[TMP_PASSWD], usr->tmpbuf[TMP_NAME]);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Reload\n");
			reload_file(usr, usr->tmpbuf[TMP_PASSWD], usr->tmpbuf[TMP_NAME]);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Download\n");
			download_file(usr, usr->tmpbuf[TMP_PASSWD], usr->tmpbuf[TMP_NAME]);
			Return;

		case 'u':
		case 'U':
			Put(usr, "Upload\n");
			upload_file(usr, usr->tmpbuf[TMP_PASSWD], usr->tmpbuf[TMP_NAME]);
			Return;
	}
	if (usr->tmpbuf[TMP_FROM_HOST] != NULL)
		Print(usr, "<yellow>\n[%s] %s# <white>", PARAM_NAME_SYSOP, usr->tmpbuf[TMP_FROM_HOST]);
	else
		Print(usr, "<yellow>\n[%s] Screens# <white>", PARAM_NAME_SYSOP);
	Return;
}

void view_file(User *usr, char *desc, char *filename) {
	if (usr == NULL || desc == NULL || filename == NULL)
		return;

	Enter(download_file);

	free_StringIO(usr->text);

	if (!*filename) {
		Print(usr, "<red>The filename for the <yellow>%s<red> has not been configured yet\n", desc);
		CURRENT_STATE_X(usr, KEY_RETURN);
		Return;
	}
	if (load_StringIO(usr->text, filename) < 0) {
		Print(usr, "<red>Failed to load the %s\n", desc);
		CURRENT_STATE_X(usr, KEY_RETURN);
		Return;
	}
	PUSH(usr, STATE_PRESS_ANY_KEY);
	Put(usr, "<green>");
	read_text(usr);
	Return;
}

void reload_file(User *usr, char *desc, char *filename) {
	if (usr == NULL || desc == NULL || filename == NULL)
		return;

	Enter(download_file);

	if (!*filename) {
		Print(usr, "<red>The filename for the <yellow>%s<red> has not been configured yet\n", desc);
		CURRENT_STATE_X(usr, KEY_RETURN);
		Return;
	}
	if (filename == PARAM_CRASH_SCREEN) {
		free_StringIO(crash_screen);
		if (load_StringIO(crash_screen, PARAM_CRASH_SCREEN) < 0)
			Perror(usr, "Failed to load crash screen");
		else
			Print(usr, "Reloaded file %s\n", PARAM_CRASH_SCREEN);
	} else {
		remove_Cache_filename(filename);
		Print(usr, "File %s uncached\n", filename);
	}
	CURRENT_STATE(usr);
	Return;
}

void download_file(User *usr, char *desc, char *filename) {
	if (usr == NULL || desc == NULL || filename == NULL)
		return;

	Enter(download_file);

	if (!*filename) {
		Print(usr, "<red>The filename for the <yellow>%s<red> has not been configured yet\n", desc);
		CURRENT_STATE_X(usr, KEY_RETURN);
		Return;
	}
	free_StringIO(usr->text);
	if (load_StringIO(usr->text, filename) < 0) {
		Print(usr, "<red>Failed to load the %s\n", desc);
		CURRENT_STATE_X(usr, KEY_RETURN);
		Return;
	}
	CALL(usr, STATE_DOWNLOAD_TEXT);
	Return;
}

/*
	requirement:
	tmpbuf[TMP_NAME] is the filename
	tmpbuf[TMP_PASSWD] is the description
*/
void upload_file(User *usr, char *desc, char *filename) {
	if (usr == NULL || desc == NULL || filename == NULL)
		return;

	Enter(upload_file);

	if (!*filename) {
		Print(usr, "<red>The filename for the <yellow>%s<red> has not been configured yet\n", desc);
		CURRENT_STATE_X(usr, KEY_RETURN);
		Return;
	}
	usr->runtime_flags |= RTF_UPLOAD;
	Print(usr, "\n<green>Upload new %s, press<yellow> <Ctrl-C><green> to end\n", usr->tmpbuf[TMP_PASSWD]);
	edit_text(usr, upload_save, upload_abort);
	Return;
}

/*
	tmpbuf[TMP_NAME] is the filename
	tmpbuf[TMP_PASSWD] is the description
*/
void upload_save(User *usr, char c) {
	if (usr == NULL)
		return;

	if (usr->tmpbuf[TMP_NAME] == NULL || !usr->tmpbuf[TMP_NAME][0]) {
		Perror(usr, "The filename has disappeared, unable to save");
		RET(usr);
		return;
	}
	remove_Cache_filename(usr->tmpbuf[TMP_NAME]);
	if (save_StringIO(usr->text, usr->tmpbuf[TMP_NAME]) < 0) {
		Print(usr, "<red>Failed to save file<white> %s\n", usr->tmpbuf[TMP_NAME]);
		log_err("upload_save(): failed to save file %s", usr->tmpbuf[TMP_NAME]);
	} else
		log_msg("SYSOP %s uploaded new %s", usr->name, usr->tmpbuf[TMP_PASSWD]);

	if (usr->tmpbuf[TMP_NAME] == PARAM_CRASH_SCREEN) {
		free_StringIO(crash_screen);
		if (load_StringIO(crash_screen, PARAM_CRASH_SCREEN) < 0)
			Perror(usr, "Failed to reload crash screen");
	}
	RET(usr);
}

void upload_abort(User *usr, char c) {
	free_StringIO(usr->text);
	RET(usr);
}


void state_help_files(User *usr, char c) {
char filename[MAX_PATHLEN];

	if (usr == NULL)
		return;

	Enter(state_help_files);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"<hotkey>Introduction                 Editing recipient <hotkey>lists\n"
				"e<hotkey>Xpress messages             Editing with <hotkey>colors\n"
				"<hotkey>Friends and enemies          <hotkey>Navigating the --More-- prompt\n"
			);
			Put(usr,
				"Reading and posting <hotkey>messages\n"
				"The <hotkey>room system\n"
				"Customizing your <hotkey>profile     <hotkey>Other commands\n"
			);
			Put(usr, "\n"
				"Confi<hotkey>g menu help             Room <hotkey>Aide help\n"
				"<hotkey>Sysop menu help\n"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'i':
		case 'I':
			bufprintf(filename, sizeof(filename), "%s/intro", PARAM_HELPDIR);
			screen_menu(usr, "Help Introduction", filename);
			Return;

		case 'x':
		case 'X':
			bufprintf(filename, sizeof(filename), "%s/xmsg", PARAM_HELPDIR);
			screen_menu(usr, "Help X messages", filename);
			Return;

		case 'f':
		case 'F':
			bufprintf(filename, sizeof(filename), "%s/friends", PARAM_HELPDIR);
			screen_menu(usr, "Help Friends & enemies", filename);
			Return;

		case 'm':
		case 'M':
			bufprintf(filename, sizeof(filename), "%s/msgs", PARAM_HELPDIR);
			screen_menu(usr, "Help on Messages", filename);
			Return;

		case 'r':
		case 'R':
			bufprintf(filename, sizeof(filename), "%s/rooms", PARAM_HELPDIR);
			screen_menu(usr, "Help on Room commands", filename);
			Return;

		case 'p':
		case 'P':
			bufprintf(filename, sizeof(filename), "%s/profile", PARAM_HELPDIR);
			screen_menu(usr, "Help on Profile info", filename);
			Return;

		case 'l':
		case 'L':
			bufprintf(filename, sizeof(filename), "%s/recipient", PARAM_HELPDIR);
			screen_menu(usr, "Help on recipient lists", filename);
			Return;

		case 'c':
		case 'C':
			bufprintf(filename, sizeof(filename), "%s/colors", PARAM_HELPDIR);
			screen_menu(usr, "Help on colors", filename);
			Return;

		case 'n':
		case 'N':
			bufprintf(filename, sizeof(filename), "%s/more", PARAM_HELPDIR);
			screen_menu(usr, "Help on the --More-- prompt", filename);
			Return;

		case 'o':
		case 'O':
			bufprintf(filename, sizeof(filename), "%s/other", PARAM_HELPDIR);
			screen_menu(usr, "Help on other commands", filename);
			Return;

		case 'g':
		case 'G':
			bufprintf(filename, sizeof(filename), "%s/config", PARAM_HELPDIR);
			screen_menu(usr, "Config Menu Help", filename);
			Return;

		case 'a':
		case 'A':
			bufprintf(filename, sizeof(filename), "%s/roomconfig", PARAM_HELPDIR);
			screen_menu(usr, "Room Aide Menu Help", filename);
			Return;

		case 's':
		case 'S':
			bufprintf(filename, sizeof(filename), "%s/sysop", PARAM_HELPDIR);
			screen_menu(usr, "Sysop Menu Help", filename);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Help files# <white>", PARAM_NAME_SYSOP);
	Return;
}


void state_view_logs(User *usr, char c) {
DirList *dl;

	if (usr == NULL)
		return;

	Enter(state_view_logs);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			buffer_text(usr);
			Put(usr, "<magenta>\n"
				"View <hotkey>bbslog                       View <hotkey>yesterday's bbslog\n"
				"View <hotkey>authlog                      View <hotkey>Yesterday's authlog\n"
				"View <hotkey>newusers log\n"
				"\n"
				"Access <hotkey>old logs\n"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_BS:
		case KEY_RETURN:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'b':
		case 'B':
			Put(usr, "View bbslog\n<green>");
			if (load_logfile(usr->text, PARAM_SYSLOG) < 0) {
				Perror(usr, "failed to load bbslog");
				CURRENT_STATE(usr);
				Return;
			}
			PUSH(usr, STATE_PRESS_ANY_KEY);
			read_text(usr);
			Return;

		case 'y':
			Put(usr, "View yesterday's bbslog\n<green>");
			yesterdays_log(usr, PARAM_SYSLOG);
			Return;

		case 'a':
		case 'A':
			Put(usr, "View authlog\n<green>");
			if (load_logfile(usr->text, PARAM_AUTHLOG) < 0) {
				Perror(usr, "failed to load authlog");
				CURRENT_STATE(usr);
				Return;
			}
			PUSH(usr, STATE_PRESS_ANY_KEY);
			read_text(usr);
			Return;

		case 'Y':
			Put(usr, "View yesterday's authlog\n<green>");
			yesterdays_log(usr, PARAM_AUTHLOG);
			Return;

		case 'n':
		case 'N':
			Put(usr, "View newusers log\n");
			if (load_newuserlog(usr) < 0) {
				Perror(usr, "failed to load newusers log");
				CURRENT_STATE(usr);
				Return;
			}
			PUSH(usr, STATE_PRESS_ANY_KEY);
			read_text(usr);
			Return;

		case 'o':
		case 'O':
			Put(usr, "Access old logs\n");

			if ((dl = list_DirList(PARAM_ARCHIVEDIR, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_SLASHES)) == NULL) {
				Perror(usr, "failed to access the archive");
				destroy_DirList(dl);
				CURRENT_STATE(usr);
				Return;
			}
			if (count_Queue(dl->list) <= 0) {
				Put(usr, "<red>There are no files in the archive\n");
				destroy_DirList(dl);
				CURRENT_STATE(usr);
				Return;
			}
			PUSH_ARG(usr, &dl, sizeof(DirList *));
			CALL(usr, STATE_OLD_LOGS_YEAR);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Logs# <white>", PARAM_NAME_SYSOP);
	Return;
}

/*
	view a log file with colors
*/
int load_logfile(StringIO *s, char *filename) {
File *f;
char buf[PRINT_BUF];
int color;

	if (s == NULL || filename == NULL || !*filename || (f = Fopen(filename)) == NULL)
		return -1;

	color = 0;
	while(Fgets(f, buf, PRINT_BUF - 16) != NULL) {
		if (cstrmatch(buf, "Aaa ?d dd:dd:dd *")) {
			buf[15] = 0;
			put_StringIO(s, "<cyan>");
			put_StringIO(s, buf);

			buf[15] = ' ';
			switch(buf[16]) {
				case 'I':
					put_StringIO(s, "<yellow>");
					color = 1;
					break;

				case 'E':
				case 'W':
					put_StringIO(s, "<red>");
					color = 1;
					break;

				case 'D':
					put_StringIO(s, "<magenta>");
					color = 1;
					break;

				case 'A':
				default:
					put_StringIO(s, "<green>");
					color = 0;
			}
			put_StringIO(s, buf+15);
		} else {
			if (color) {
				color = 0;
				put_StringIO(s, "<yellow>");
			}
			put_StringIO(s, buf);
		}
		write_StringIO(s, "\n", 1);
	}
	Fclose(f);
	return 0;
}

/*
	View yesterday's logfile
	logfile is either PARAM_SYSLOG or PARAM_AUTHLOG
*/
void yesterdays_log(User *usr, char *logfile) {
char *p, filename[MAX_PATHLEN];
struct tm *tm;
time_t t;

	if (usr == NULL)
		return;

	Enter(yesterdays_log);

	if (logfile == NULL || !*logfile) {
		Perror(usr, "The log's filename not been defined yet");
		Return;
	}
/* 'basename' logfile */
	if ((p = cstrrchr(logfile, '/')) == NULL)
		p = logfile;
	else {
		p++;
		if (!*p)
			p = logfile;
	}
	t = rtc;
	t -= SECS_IN_DAY;			/* yesterday */
	tm = localtime(&t);

	bufprintf(filename, sizeof(filename), "%s/%04d/%02d/%s.%04d%02d%02d", PARAM_ARCHIVEDIR, tm->tm_year+1900, tm->tm_mon+1,
		p, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	path_strip(filename);

	if (!file_exists(filename)) {
		Put(usr, "<red>Yesterday's log file is missing\n");
		CURRENT_STATE(usr);
		Return;
	}
	if (load_logfile(usr->text, filename) < 0) {
		Perror(usr, "failed to load yesterday's log");
		CURRENT_STATE(usr);
		Return;
	}
	PUSH(usr, STATE_PRESS_ANY_KEY);
	read_text(usr);
	Return;
}

int load_newuserlog(User *usr) {
NewUserQueue *q;
NewUserLog *l;
char date_buf[MAX_LINE];

	if (usr == NULL || (q = load_NewUserQueue(PARAM_NEWUSERLOG)) == NULL)
		return -1;

	buffer_text(usr);

	for(l = (NewUserLog *)q->tail; l != NULL; l = l->next)
		Print(usr, "<cyan>%-40s  <yellow>%s\n", print_date(usr, l->timestamp, date_buf, MAX_LINE), l->name);
	Put(usr, "\n");
	destroy_NewUserQueue(q);
	return 0;
}

void state_old_logs_year(User *usr, char c) {
DirList *dl;
int r;

	if (usr == NULL)
		return;

	Enter(state_old_logs_year);

	PEEK_ARG(usr, &dl, sizeof(DirList *));
	if (dl == NULL || dl->list == NULL) {
		Perror(usr, "The directory listing has disappeared");
		destroy_DirList(dl);
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			edit_number(usr, EDIT_INIT);

			buffer_text(usr);
			Put(usr, "\n");
			print_columns(usr, (StringList *)dl->list->tail, 0);
			read_menu(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		default:
			r = edit_number(usr, c);
			if (r == EDIT_RETURN) {
				if (!usr->edit_buf[0])
					r = EDIT_BREAK;
				else {
					char dirname[MAX_PATHLEN];
					DirList *dl2;

					bufprintf(dirname, sizeof(dirname), "%s/%s", dl->name, usr->edit_buf);
					if ((dl2 = list_DirList(dirname, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_SLASHES)) == NULL) {
						Print(usr, "<red>Failed to read directory <white>%s\n", dirname);
						CURRENT_STATE(usr);
						Return;
					}
					if (count_Queue(dl2->list) <= 0) {
						Put(usr, "<red>That archive is empty\n");
						destroy_DirList(dl2);
						CURRENT_STATE(usr);
						Return;
					}
					PUSH_ARG(usr, &dl2, sizeof(DirList *));
					CALL(usr, STATE_OLD_LOGS_MONTH);
					Return;
				}
			}
			if (r == EDIT_BREAK) {
				POP_ARG(usr, &dl, sizeof(DirList *));
				destroy_DirList(dl);
				RET(usr);
				Return;
			}
			Return;
	}
	Print(usr, "<yellow>\n[%s] Archive# <green>Enter year: ", PARAM_NAME_SYSOP);
	Return;
}

/*
	practically the same as for a year
*/
void state_old_logs_month(User *usr, char c) {
DirList *dl;
int r;

	if (usr == NULL)
		return;

	Enter(state_old_logs_month);

	PEEK_ARG(usr, &dl, sizeof(DirList *));
	if (dl == NULL || dl->list == NULL) {
		Perror(usr, "The directory listing has disappeared");
		destroy_DirList(dl);
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			edit_number(usr, EDIT_INIT);

			buffer_text(usr);
			Put(usr, "\n");
			print_columns(usr, (StringList *)dl->list->tail, 0);
			read_menu(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		default:
			r = edit_number(usr, c);
			if (r == EDIT_RETURN) {
				if (!usr->edit_buf[0])
					r = EDIT_BREAK;
				else {
					char dirname[MAX_PATHLEN];
					DirList *dl2;

					bufprintf(dirname, sizeof(dirname), "%s/%s", dl->name, usr->edit_buf);
					if ((dl2 = list_DirList(dirname, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_DIRS)) == NULL) {
						Print(usr, "<red>Failed to read directory <white>%s\n", dirname);
						break;
					}
					if (count_Queue(dl2->list) <= 0) {
						Put(usr, "<red>That archive is empty\n");
						destroy_DirList(dl2);
						break;
					}
					PUSH_ARG(usr, &dl2, sizeof(DirList *));
					CALL(usr, STATE_OLD_LOGS_FILES);
					Return;
				}
			}
			if (r == EDIT_BREAK) {
				POP_ARG(usr, &dl, sizeof(DirList *));
				destroy_DirList(dl);
				RET(usr);
				Return;
			}
			Return;
	}
	Print(usr, "<yellow>\n[%s] Archive# <green>Enter month: ", PARAM_NAME_SYSOP);
	Return;
}

void state_old_logs_files(User *usr, char c) {
DirList *dl;
int r;

	if (usr == NULL)
		return;

	Enter(state_old_logs_files);

	PEEK_ARG(usr, &dl, sizeof(DirList *));
	if (dl == NULL || dl->list == NULL) {
		Perror(usr, "The directory listing has disappeared");
		destroy_DirList(dl);
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			edit_number(usr, EDIT_INIT);

			buffer_text(usr);
			Put(usr, "\n");
			print_columns(usr, (StringList *)dl->list->tail, FORMAT_NUMBERED);
			read_menu(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		default:
			r = edit_number(usr, c);
			if (r == EDIT_RETURN) {
				if (!usr->edit_buf[0])
					r = EDIT_BREAK;
				else {
					char filename[MAX_PATHLEN];
					StringList *sl;
					int n;

					n = atoi(usr->edit_buf);
					if (n <= 0) {
						Put(usr, "<red>Invalid number\n");
						CURRENT_STATE(usr);
						Return;
					}
					if (n > count_Queue(dl->list)) {
						Put(usr, "<red>No such log file\n");
						CURRENT_STATE(usr);
						Return;
					}
					n--;
					for(sl = (StringList *)dl->list->tail; n > 0 && sl != NULL; sl = sl->next)
						n--;

					if (sl == NULL) {
						Put(usr, "<red>No such log file\n");
						CURRENT_STATE(usr);
						Return;
					}
					bufprintf(filename, sizeof(filename), "%s/%s", dl->name, sl->str);
/* view the log file */
					free_StringIO(usr->text);
					if (load_logfile(usr->text, filename) < 0) {
						Print(usr, "<red>Failed to load file <white>%s\n", sl->str);
						CURRENT_STATE(usr);
						Return;
					}
					PUSH(usr, STATE_PRESS_ANY_KEY);
					Put(usr, "<green>");
					read_text(usr);
					Return;
				}
			}
			if (r == EDIT_BREAK) {
				POP_ARG(usr, &dl, sizeof(DirList *));
				destroy_DirList(dl);
				RET(usr);
				Return;
			}
			Return;
	}
	Print(usr, "<yellow>\n[%s] Archive# <green>Enter number: ", PARAM_NAME_SYSOP);
	Return;
}


void state_feelings_menu(User *usr, char c) {
DirList *feelings;

	if (usr == NULL)
		return;

	Enter(state_feelings_menu);

	PEEK_ARG(usr, &feelings, sizeof(DirList *));
	if (feelings == NULL) {
		Perror(usr, "Your feelings have ebbed away ...");
		RET(usr);
		Return;
	}
	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			buffer_text(usr);

/* always re-read the directory */
			destroy_DirList(feelings);
			if ((feelings = list_DirList(PARAM_FEELINGSDIR, IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_DIRS)) == NULL) {
				log_err("state_feelings_menu(): list_DirList(%s) failed", PARAM_FEELINGSDIR);
				Put(usr, "<red>Failed to read the Feelings directory\n");
				POP_ARG(usr, &feelings, sizeof(DirList *));
/* don't destroy; it's already destroyed, but we still needed to get the container off the stack */
				RET(usr);
				Return;
			}
			POKE_ARG(usr, &feelings, sizeof(DirList *));

			if (count_Queue(feelings->list) > 0) {
				Put(usr, "\n");
				print_columns(usr, (StringList *)feelings->list->tail, FORMAT_NUMBERED|FORMAT_NO_UNDERSCORES);
			}
			Put(usr, "<magenta>\n"
				"<hotkey>Add feeling"
			);
			if (count_Queue(feelings->list) > 0) {
				Put(usr, "                       <hotkey>View feeling\n"
					"<hotkey>Remove feeling                    <hotkey>Download\n"
				);
			} else
				Put(usr, "\n");

			read_menu(usr);
			Return;

		case ' ':
		case KEY_BS:
		case KEY_RETURN:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);

			POP_ARG(usr, &feelings, sizeof(DirList *));
			destroy_DirList(feelings);
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;
/*
	Note: here we put another pointer to the feelings on the stack, but be sure
	NOT to destroy them in the next state; the feelings are destroyed upon
	return from _this_ (calling) state
*/
		case 'a':
		case 'A':
			Put(usr, "Add\n");
			PUSH_ARG(usr, &feelings, sizeof(DirList *));
			CALL(usr, STATE_ADD_FEELING);
			Return;

		case 'r':
		case 'R':
			if (feelings == NULL)
				break;

			Put(usr, "Remove\n");
			PUSH_ARG(usr, &feelings, sizeof(DirList *));
			CALL(usr, STATE_REMOVE_FEELING);
			Return;

		case 'v':
		case 'V':
			if (feelings == NULL)
				break;

			Put(usr, "View\n");
			PUSH_ARG(usr, &feelings, sizeof(DirList *));
			CALL(usr, STATE_VIEW_FEELING);
			Return;

		case 'd':
		case 'D':
			if (feelings == NULL)
				break;

			Put(usr, "Download\n");
			PUSH_ARG(usr, &feelings, sizeof(DirList *));
			CALL(usr, STATE_DOWNLOAD_FEELING);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Feelings# <white>", PARAM_NAME_SYSOP);
	Return;
}

void state_add_feeling(User *usr, char c) {
int r;
char *p;
DirList *feelings;
StringList *sl;

	Enter(state_add_feeling);

	PEEK_ARG(usr, &feelings, sizeof(DirList *));
	if (feelings == NULL || feelings->list == NULL) {
		Perror(usr, "All feelings have gone!");
		RET(usr);
		Return;
	}
	if (c == INIT_STATE) {
		Put(usr, "<green>Enter name: <yellow>");
		edit_caps_line(usr, EDIT_INIT);
		Return;
	}
	r = edit_caps_line(usr, c);
	switch(r) {
		case EDIT_BREAK:
/*
	pop, but do not destroy
	(see also comment in state_feelings_menu())
*/
			POP_ARG(usr, &feelings, sizeof(DirList *));
			RET(usr);
			break;

		case EDIT_RETURN:
/*
	pop, but do not destroy
	(see also comment in state_feelings_menu())
*/
			POP_ARG(usr, &feelings, sizeof(DirList *));

			cstrip_line(usr->edit_buf);
			if (!usr->edit_buf[0]) {
				RET(usr);
				Return;
			}
			if (cstrchr(usr->edit_buf, '/') != NULL) {
				Put(usr, "<red>Invalid name\n");
				RET(usr);
				Return;
			}
			while((p = cstrchr(usr->edit_buf, ' ')) != NULL)
				*p = '_';
			cstrip_spaces(usr->edit_buf);

			for(sl = (StringList *)feelings->list->tail; sl != NULL; sl = sl->next) {
				if (!cstricmp(usr->edit_buf, sl->str)) {
					Put(usr, "<red>Feeling already exists\n");
					RET(usr);
					Return;
				}
			}
			Free(usr->tmpbuf[TMP_NAME]);
			if ((usr->tmpbuf[TMP_NAME] = cstrdup(usr->edit_buf)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				Return;
			}
			usr->runtime_flags |= RTF_UPLOAD;
			Print(usr, "<green>\n"
				"Upload feeling, press <yellow><Ctrl-C><green> to end\n");
			POP(usr);
			edit_text(usr, save_feeling, abort_feeling);
	}
	Return;
}

void save_feeling(User *usr, char c) {
char filename[MAX_PATHLEN], *p;

	if (usr->tmpbuf[TMP_NAME] == NULL || !usr->tmpbuf[TMP_NAME][0]) {
		Free(usr->tmpbuf[TMP_NAME]);
		usr->tmpbuf[TMP_NAME] = NULL;
		Perror(usr, "The feeling has faded");
		RET(usr);
		Return;
	}
	bufprintf(filename, sizeof(filename), "%s/%s", PARAM_FEELINGSDIR, usr->tmpbuf[TMP_NAME]);

	Free(usr->tmpbuf[TMP_NAME]);
	usr->tmpbuf[TMP_NAME] = NULL;

	if (save_StringIO(usr->text, filename) < 0) {
		Put(usr, "<red>Failed to save feeling\n");
		log_err("failed to save feeling as %s", filename);

		free_StringIO(usr->text);
		RET(usr);
		Return;
	}
	free_StringIO(usr->text);

	feelings_generation++;

	p = filename + strlen(PARAM_FEELINGSDIR);
	if (*p == '/')
		p++;
	log_msg("SYSOP %s added Feeling %s", usr->name, p);
	RET(usr);
}

void abort_feeling(User *usr, char c) {
	Free(usr->tmpbuf[TMP_NAME]);
	usr->tmpbuf[TMP_NAME] = NULL;
	RET(usr);
}

void state_remove_feeling(User *usr, char c) {
	feelings_menu(usr, c, do_remove_feeling);
}

void feelings_menu(User *usr, char c, void (*func)(User *)) {
int r;
DirList *feelings;
StringList *sl;

	if (usr == NULL || func == NULL)
		return;

	Enter(feelings_menu);

	PEEK_ARG(usr, &feelings, sizeof(DirList *));
	if (feelings == NULL || feelings->list == NULL) {
		Perror(usr, "All feelings have gone!");
		RET(usr);
		Return;
	}
	if (c == INIT_STATE) {
		Put(usr, "<green>Enter number: <yellow>");
		edit_number(usr, EDIT_INIT);
		Return;
	}
	r = edit_number(usr, c);
	switch(r) {
		case EDIT_BREAK:
/*
	pop, but do not destroy
	(see also comment in state_feelings_menu())
*/
			POP_ARG(usr, &feelings, sizeof(DirList *));
			RET(usr);
			Return;

		case EDIT_RETURN:
/*
	pop, but do not destroy
	(see also comment in state_feelings_menu())
*/
			POP_ARG(usr, &feelings, sizeof(DirList *));

			if (!usr->edit_buf[0]) {
				RET(usr);
				Return;
			}
			r = atoi(usr->edit_buf);
			if (r <= 0) {
				Put(usr, "<red>No such feeling\n");
				RET(usr);
				Return;
			}
			r--;
			for(sl = (StringList *)feelings->list->tail; r > 0 && sl != NULL; sl = sl->next)
				r--;

			if (sl == NULL) {
				Put(usr, "<red>No such feeling\n");
				RET(usr);
				Return;
			}
			Free(usr->tmpbuf[TMP_NAME]);
			if ((usr->tmpbuf[TMP_NAME] = cstrdup(sl->str)) == NULL) {
				Perror(usr, "Bummer, out of memory");
				RET(usr);
				Return;
			}
/*
	I use this construction with func because this all would be duplicate code
	sl->str is the filename of the Feeling
*/
			func(usr);
			Return;
	}
	Return;
}

/*
	usr->tmpbuf[TMP_NAME] is the filename of the feeling
*/
void do_remove_feeling(User *usr) {
	JMP(usr, STATE_REMOVE_FEELING_YESNO);
}

/*
	usr->tmpbuf[TMP_NAME] is the filename of the feeling
*/
void state_remove_feeling_yesno(User *usr, char c) {
char filename[MAX_PATHLEN], *p;

	Enter(state_remove_feeling_yesno);

	if (usr->tmpbuf[TMP_NAME] == NULL) {
		Perror(usr, "Can't remember what Feeling you wanted to remove..!");
		RET(usr);
		Return;
	}
	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? (y/N): <white>");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			bufprintf(filename, sizeof(filename), "%s/%s", PARAM_FEELINGSDIR, usr->tmpbuf[TMP_NAME]);
			Free(usr->tmpbuf[TMP_NAME]);
			usr->tmpbuf[TMP_NAME] = NULL;

			if (unlink_file(filename) < 0)
				Perror(usr, "Failed to delete feeling");
			else {
				p = filename + strlen(PARAM_FEELINGSDIR);
				if (*p == '/')
					p++;
				log_msg("SYSOP %s deleted Feeling %s", usr->name, p);
				feelings_generation++;
			}
			RET(usr);
			Return;

		case YESNO_NO:
			Free(usr->tmpbuf[TMP_NAME]);
			usr->tmpbuf[TMP_NAME] = NULL;
			RET(usr);
			Return;

		case YESNO_UNDEF:
			Put(usr, "<cyan>Delete this feeling, <hotkey>yes or <hotkey>no? (y/N): <white>");
	}
	Return;
}

void state_view_feeling(User *usr, char c) {
	feelings_menu(usr, c, do_view_feeling);
}

void do_view_feeling(User *usr) {
char filename[MAX_PATHLEN];

	Enter(do_view_feeling);

	if (usr->tmpbuf[TMP_NAME] == NULL) {
		Perror(usr, "Can't remember what Feeling you were talking about..!");
		RET(usr);
		Return;
	}
	bufprintf(filename, sizeof(filename), "%s/%s", PARAM_FEELINGSDIR, usr->tmpbuf[TMP_NAME]);
	Free(usr->tmpbuf[TMP_NAME]);
	usr->tmpbuf[TMP_NAME] = NULL;

	free_StringIO(usr->text);
	if (load_StringIO(usr->text, filename) < 0) {
		Perror(usr, "Failed to load feeling");
		RET(usr);
		Return;
	}
	POP(usr);
	PUSH(usr, STATE_PRESS_ANY_KEY);
	read_text(usr);
	Return;
}

void state_download_feeling(User *usr, char c) {
	feelings_menu(usr, c, do_download_feeling);
}

void do_download_feeling(User *usr) {
char filename[MAX_PATHLEN];

	Enter(do_download_feeling);

	if (usr->tmpbuf[TMP_NAME] == NULL) {
		Perror(usr, "Can't remember what Feeling you were talking about..!");
		RET(usr);
		Return;
	}
	bufprintf(filename, sizeof(filename), "%s/%s", PARAM_FEELINGSDIR, usr->tmpbuf[TMP_NAME]);
	Free(usr->tmpbuf[TMP_NAME]);
	usr->tmpbuf[TMP_NAME] = NULL;

	free_StringIO(usr->text);
	if (load_StringIO(usr->text, filename) < 0) {
		Perror(usr, "Failed to load feeling");
		RET(usr);
		Return;
	}
	JMP(usr, STATE_DOWNLOAD_TEXT);
	Return;
}


void state_parameters_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_parameters_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"System <hotkey>configuration\n"
				"Configure locations of <hotkey>files\n"
				"Configure <hotkey>maximums and timeouts\n"
				"Configure <hotkey>strings and messages\n"
			);
			Print(usr,
				"Configure <hotkey>log rotation\n"
				"<hotkey>Toggle features\n"
				"\n"
				"<white>Ctrl-<hotkey>R<magenta>eload param file<white> %s\n", param_file);

			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Print(usr, "%s menu\n", PARAM_NAME_SYSOP);

			if (usr->runtime_flags & RTF_PARAM_EDITED) {
				if (save_Param(param_file))
					Perror(usr, "failed to save param file");

				usr->runtime_flags &= ~RTF_PARAM_EDITED;
			}
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
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
	Print(usr, "<yellow>\n[%s] Parameters# <white>", PARAM_NAME_SYSOP);
	Return;
}


void state_system_config_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_system_config_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			PARAM_UMASK &= 0777;
			umask(PARAM_UMASK);

			buffer_text(usr);

			Put(usr, "<white>\n"
				"Note: <red>Although these parameters are live, most of them require a reboot "
				"to correctly re-initialize the BBS\n"
			);
			Print(usr, "<magenta>\n"
				"BBS <hotkey>name           <white> %s<magenta>\n"
				"Bind <hotkey>address       <white> %s<magenta>\n"
				"P<hotkey>ort number        <white> %s<magenta>\n"
				"<hotkey>Param file         <white> %s<magenta>\n",

				PARAM_BBS_NAME,
				PARAM_BIND_ADDRESS,
				PARAM_PORT_NUMBER,
				param_file
			);
			Print(usr, "\n"
				"<hotkey>Base directory     <white> %s<magenta>\n"
				"B<hotkey>inary directory   <white> %s<magenta>\n"
				"<hotkey>Config directory   <white> %s<magenta>\n",

				PARAM_BASEDIR,
				PARAM_BINDIR,
				PARAM_CONFDIR
			);
			Print(usr,
				"<hotkey>Help directory     <white> %s<magenta>\n"
				"<hotkey>Feelings directory <white> %s<magenta>\n"
				"<hotkey>Zoneinfo directory <white> %s<magenta>\n",

				PARAM_HELPDIR,
				PARAM_FEELINGSDIR,
				PARAM_ZONEINFODIR
			);
			Print(usr,
				"<hotkey>User directory     <white> %s<magenta>\n"
				"<hotkey>Room directory     <white> %s<magenta>\n"
				"<hotkey>Trash directory    <white> %s<magenta>\n"
				"umas<hotkey>k              <white> 0%02o<magenta>\n",

				PARAM_USERDIR,
				PARAM_ROOMDIR,
				PARAM_TRASHDIR,
				PARAM_UMASK
			);
			Print(usr, "\n"
				"<hotkey>Main program       <white> %s<magenta>\n"
				"Resol<hotkey>ver program   <white> %s<magenta>\n"
				"\n"
				"Default time<hotkey>zone   <white> %s<magenta>\n",

				PARAM_PROGRAM_MAIN,
				PARAM_PROGRAM_RESOLVER,
				PARAM_DEFAULT_TIMEZONE
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Parameters\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'n':
		case 'N':
			Put(usr, "BBS Name\n");
			CALL(usr, STATE_PARAM_BBS_NAME);
			Return;

		case 'a':
		case 'A':
			Put(usr, "Bind address\n");
			CALL(usr, STATE_PARAM_BIND_ADDRESS);
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

		case 'h':
		case 'H':
			Put(usr, "Help directory\n");
			CALL(usr, STATE_PARAM_HELPDIR);
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
	Print(usr, "<yellow>\n[%s] Sysconf# <white>", PARAM_NAME_SYSOP);
	Return;
}


void state_param_bbs_name(User *usr, char c) {
	Enter(state_param_bbs_name);
	change_string_param(usr, c, &PARAM_BBS_NAME, "<green>Enter name: <yellow>");
	Return;
}

void state_param_bind_address(User *usr, char c) {
	Enter(state_param_bbs_name);
	change_string_param(usr, c, &PARAM_BIND_ADDRESS, "<green>Enter address to bind to: <yellow>");
	Return;
}

void state_param_port_number(User *usr, char c) {
	Enter(state_param_port_number);
	change_string_param(usr, c, &PARAM_PORT_NUMBER, "<green>Enter service name or port number: <yellow>");
	Return;
}

void state_param_file(User *usr, char c) {
	Enter(state_param_file);
	change_string_param(usr, c, &param_file, "<green>Enter param file: <yellow>");
	Return;
}

void state_param_basedir(User *usr, char c) {
	Enter(state_param_basedir);
	change_string_param(usr, c, &PARAM_BASEDIR, "<green>Enter base directory: <yellow>");
	Return;
}

void state_param_bindir(User *usr, char c) {
	Enter(state_param_bindir);
	change_string_param(usr, c, &PARAM_BINDIR, "<green>Enter binary directory: <yellow>");
	Return;
}

void state_param_confdir(User *usr, char c) {
	Enter(state_param_confdir);
	change_string_param(usr, c, &PARAM_CONFDIR, "<green>Enter config directory: <yellow>");
	Return;
}

void state_param_helpdir(User *usr, char c) {
	Enter(state_param_helpdir);
	change_string_param(usr, c, &PARAM_HELPDIR, "<green>Enter help directory: <yellow>");
	Return;
}

void state_param_feelingsdir(User *usr, char c) {
	Enter(state_param_feelingsdir);
	change_string_param(usr, c, &PARAM_FEELINGSDIR, "<green>Enter feelings directory: <yellow>");
	Return;
}

void state_param_zoneinfodir(User *usr, char c) {
	Enter(state_param_zoneinfodir);
	change_string_param(usr, c, &PARAM_ZONEINFODIR, "<green>Enter zoneinfo directory: <yellow>");
	Return;
}

void state_param_userdir(User *usr, char c) {
	Enter(state_param_userdir);
	change_string_param(usr, c, &PARAM_USERDIR, "<green>Enter user directory: <yellow>");
	Return;
}

void state_param_roomdir(User *usr, char c) {
	Enter(state_param_roomdir);
	change_string_param(usr, c, &PARAM_ROOMDIR, "<green>Enter room directory: <yellow>");
	Return;
}

void state_param_trashdir(User *usr, char c) {
	Enter(state_param_trashdir);
	change_string_param(usr, c, &PARAM_TRASHDIR, "<green>Enter trash directory: <yellow>");
	Return;
}

void state_param_umask(User *usr, char c) {
	Enter(state_param_umask);
	change_octal_param(usr, c, &PARAM_UMASK);
	Return;
}

void state_param_program_main(User *usr, char c) {
	Enter(state_param_program_main);
	change_string_param(usr, c, &PARAM_PROGRAM_MAIN, "<green>Enter main program: <yellow>");
	Return;
}

void state_param_program_resolver(User *usr, char c) {
	Enter(state_param_program_resolver);
	change_string_param(usr, c, &PARAM_PROGRAM_RESOLVER, "<green>Enter resolver program: <yellow>");
	Return;
}


void state_config_files_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_config_files_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
/*
	I'm hopelessly out of hotkeys here...
*/
			buffer_text(usr);

			Print(usr, "<magenta>\n"
				"<hotkey>GPL             <white> %-22s<magenta>  <hotkey>0 Local mods    <white> %s<magenta>\n",
				PARAM_GPL_SCREEN, PARAM_MODS_SCREEN);
			Print(usr, "<hotkey>Login           <white> %-22s<magenta>  <hotkey>Nologin         <white> %s<magenta>\n",
				PARAM_LOGIN_SCREEN, PARAM_NOLOGIN_SCREEN);
			Print(usr, "Log<hotkey>out          <white> %-22s<magenta>  <hotkey>Reboot          <white> %s<magenta>\n",
				PARAM_LOGIN_SCREEN, PARAM_REBOOT_SCREEN);
			Print(usr, "Mot<hotkey>d            <white> %-22s<magenta>  <hotkey>Z Shutdown      <white> %s<magenta>\n",
				PARAM_MOTD_SCREEN, PARAM_SHUTDOWN_SCREEN);
			Print(usr, "<hotkey>1st login       <white> %-22s<magenta>  <hotkey>K Crash         <white> %s<magenta>\n",
				PARAM_FIRST_LOGIN, PARAM_CRASH_SCREEN);

			Print(usr, "Cred<hotkey>its         <white> %-22s<magenta>\n"
				"\n",
				PARAM_CREDITS_SCREEN);

			Print(usr, "<hotkey>Param file      <white> %-22s<magenta>  Uni<hotkey>x PID file   <white> %s<magenta>\n",
				param_file, PARAM_PID_FILE);
			Print(usr, "<hotkey>Banished        <white> %-22s<magenta>  Sta<hotkey>tistics      <white> %s<magenta>\n",
				PARAM_BANISHED_FILE, PARAM_STAT_FILE);
			Print(usr, "Host <hotkey>access     <white> %-22s<magenta>  S<hotkey>U Passwd       <white> %s<magenta>\n",
				PARAM_HOSTS_ACCESS_FILE, PARAM_SU_PASSWD_FILE);
			Print(usr, "Host <hotkey>map        <white> %-22s<magenta>  S<hotkey>ymbol table    <white> %s<magenta>\n",
				PARAM_HOSTMAP_FILE, PARAM_SYMTAB_FILE);

			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Parameters\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
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
	Print(usr, "<yellow>\n[%s] Files# <white>", PARAM_NAME_SYSOP);
	Return;
}


void state_param_gpl_screen(User *usr, char c) {
	Enter(state_param_gpl_screen);
	change_string_param(usr, c, &PARAM_GPL_SCREEN, "<green>Enter GPL file: <yellow>");
	Return;
}

void state_param_mods_screen(User *usr, char c) {
	Enter(state_param_mods_screen);
	change_string_param(usr, c, &PARAM_MODS_SCREEN, "<green>Enter local mods file: <yellow>");
	Return;
}

void state_param_login_screen(User *usr, char c) {
	Enter(state_param_login_screen);
	change_string_param(usr, c, &PARAM_LOGIN_SCREEN, "<green>Enter login screen: <yellow>");
	Return;
}

void state_param_logout_screen(User *usr, char c) {
	Enter(state_param_logout_screen);
	change_string_param(usr, c, &PARAM_LOGOUT_SCREEN, "<green>Enter logout screen: <yellow>");
	Return;
}

void state_param_nologin_screen(User *usr, char c) {
	Enter(state_param_nologin_screen);
	change_string_param(usr, c, &PARAM_NOLOGIN_SCREEN, "<green>Enter nologin screen: <yellow>");
	Return;
}

void state_param_motd_screen(User *usr, char c) {
	Enter(state_param_motd_screen);
	change_string_param(usr, c, &PARAM_MOTD_SCREEN, "<green>Enter motd screen: <yellow>");
	Return;
}

void state_param_reboot_screen(User *usr, char c) {
	Enter(state_param_reboot_screen);
	change_string_param(usr, c, &PARAM_REBOOT_SCREEN, "<green>Enter reboot screen: <yellow>");
	Return;
}

void state_param_shutdown_screen(User *usr, char c) {
	Enter(state_param_shutdown_screen);
	change_string_param(usr, c, &PARAM_SHUTDOWN_SCREEN, "<green>Enter shutdown screen: <yellow>");
	Return;
}

void state_param_crash_screen(User *usr, char c) {
	Enter(state_param_crash_screen);
	change_string_param(usr, c, &PARAM_CRASH_SCREEN, "<green>Enter crash screen: <yellow>");
	Return;
}

void state_param_first_login(User *usr, char c) {
	Enter(state_param_first_login);
	change_string_param(usr, c, &PARAM_FIRST_LOGIN, "<green>Enter first login screen: <yellow>");
	Return;
}

void state_param_credits_screen(User *usr, char c) {
	Enter(state_param_first_login);
	change_string_param(usr, c, &PARAM_CREDITS_SCREEN, "<green>Enter credits screen: <yellow>");
	Return;
}

void state_param_hosts_access(User *usr, char c) {
	Enter(state_param_hosts_access);
	change_string_param(usr, c, &PARAM_HOSTS_ACCESS_FILE, "<green>Enter hosts_access file: <yellow>");
	Return;
}

void state_param_banished_file(User *usr, char c) {
	Enter(state_param_banished_file);
	change_string_param(usr, c, &PARAM_BANISHED_FILE, "<green>Enter banished file: <yellow>");
	Return;
}

void state_param_stat_file(User *usr, char c) {
	Enter(state_param_stat_file);
	change_string_param(usr, c, &PARAM_STAT_FILE, "<green>Enter statistics file: <yellow>");
	Return;
}

void state_param_su_passwd_file(User *usr, char c) {
	Enter(state_param_su_passwd_file);
	change_string_param(usr, c, &PARAM_SU_PASSWD_FILE, "<green>Enter su_passwd file: <yellow>");
	Return;
}

void state_param_pid_file(User *usr, char c) {
	Enter(state_param_pid_file);
	change_string_param(usr, c, &PARAM_PID_FILE, "<green>Enter PID file: <yellow>");
	Return;
}

void state_param_symtab_file(User *usr, char c) {
	Enter(state_param_symtab_file);
	change_string_param(usr, c, &PARAM_SYMTAB_FILE, "<green>Enter symtab file: <yellow>");
	Return;
}

void state_param_hostmap_file(User *usr, char c) {
	Enter(state_param_hostmap_file);
	change_string_param(usr, c, &PARAM_HOSTMAP_FILE, "<green>Enter hostmap file: <yellow>");
	Return;
}

void state_param_def_timezone(User *usr, char c) {
	Enter(state_param_def_timezone);
	change_string_param(usr, c, &PARAM_DEFAULT_TIMEZONE, "<green>Enter default timezone: <yellow>");
	Return;
}



void state_maximums_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_maximums_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "<magenta>\n"
				"Max number of <hotkey>cached files            <white>%6u<magenta>\n"
				"Max number of messages kept in a <hotkey>room <white>%6u<magenta>\n"
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
				"Max number of <hotkey>Enemies                 <white>%6u<magenta>\n"
				"Max number of entries in <hotkey>New user log <white>%6u<magenta>\n",

				PARAM_MAX_CHAT_HISTORY,
				PARAM_MAX_HISTORY,
				PARAM_MAX_FRIEND,
				PARAM_MAX_ENEMY,
				PARAM_MAX_NEWUSERLOG
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
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Parameters\n");
/*
	PARAM_CACHE_TIMEOUT was changed, so we need to reset the timer
	I'm actually abusing the RTF_WRAPPER_EDITED flag for this
*/
			if (usr->runtime_flags & RTF_WRAPPER_EDITED) {
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;

				if (expire_timer != NULL) {
					expire_timer->maxtime = PARAM_CACHE_TIMEOUT * SECS_IN_MIN;
					set_Timer(&timerq, expire_timer, expire_timer->maxtime);
				} else {
					if ((expire_timer = new_Timer(PARAM_CACHE_TIMEOUT * SECS_IN_MIN, cache_expire_timerfunc, TIMER_RESTART)) == NULL)
						log_err("state_maximums_menu(): failed to allocate a new cache_expire Timer");
					else
						add_Timer(&timerq, expire_timer);
				}
			}
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
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

		case 'n':
		case 'N':
			Put(usr, "Max entries in New user log\n");
			CALL(usr, STATE_PARAM_MAX_NEWUSERLOG);
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
	Print(usr, "<yellow>\n[%s] Maximums# <white>", PARAM_NAME_SYSOP);
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

void state_param_max_newuserlog(User *usr, char c) {
	Enter(state_param_max_newuserlog);
	change_int_param(usr, c, &PARAM_MAX_NEWUSERLOG);
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
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "\n"
				"<magenta>Name <hotkey>sysop               <white>%s<magenta>\n"
				"<magenta>Name Room <hotkey>aide           <white>%s<magenta>\n"
				"<magenta>Name <hotkey>helper              <white>%s<magenta>\n"
				"<magenta>Name <hotkey>guest               <white>%s<magenta>\n",

				PARAM_NAME_SYSOP,
				PARAM_NAME_ROOMAIDE,
				PARAM_NAME_HELPER,
				PARAM_NAME_GUEST
			);
			Print(usr, "\n"
				"<magenta>Notify logi<hotkey>n             %s\n"
				"<magenta>Notify logou<hotkey>t            %s\n",

				PARAM_NOTIFY_LOGIN,
				PARAM_NOTIFY_LOGOUT
			);
			Print(usr,
				"<magenta>Notify link<hotkey>dead          %s\n"
				"<magenta>Notify <hotkey>idle              %s\n",

				PARAM_NOTIFY_LINKDEAD,
				PARAM_NOTIFY_IDLE
			);
			Print(usr,
				"<magenta>Notify <hotkey>locked            %s\n"
				"<magenta>Notify <hotkey>unlocked          %s\n"
				"<magenta>Notify h<hotkey>old              %s\n"
				"<magenta>Notify <hotkey>release (unhold)  %s\n",

				PARAM_NOTIFY_LOCKED,
				PARAM_NOTIFY_UNLOCKED,
				PARAM_NOTIFY_HOLD,
				PARAM_NOTIFY_UNHOLD
			);
			Print(usr,
				"<magenta>Notify <hotkey>enter chat        %s\n"
				"<magenta>Notify lea<hotkey>ve chat        %s\n",

				PARAM_NOTIFY_ENTER_CHAT,
				PARAM_NOTIFY_LEAVE_CHAT
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Parameters\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
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

		case 't':
		case 'T':
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

		case 'o':
		case 'O':
			Put(usr, "Notify hold\n");
			CALL(usr, STATE_PARAM_NOTIFY_HOLD);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Notify release\n");
			CALL(usr, STATE_PARAM_NOTIFY_UNHOLD);
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
	Print(usr, "<yellow>\n[%s] Strings# <white>", PARAM_NAME_SYSOP);
	Return;
}

void state_param_name_sysop(User *usr, char c) {
	Enter(state_param_name_sysop);
	change_string_param(usr, c, &PARAM_NAME_SYSOP, "<green>Enter name Sysop: ");
	Return;
}

void state_param_name_roomaide(User *usr, char c) {
	Enter(state_param_name_roomaide);
	change_string_param(usr, c, &PARAM_NAME_ROOMAIDE, "<green>Enter name Room Aide: ");
	Return;
}

void state_param_name_helper(User *usr, char c) {
	Enter(state_param_name_helper);
	change_string_param(usr, c, &PARAM_NAME_HELPER, "<green>Enter name Helper: ");
	Return;
}

void state_param_name_guest(User *usr, char c) {
	Enter(state_param_name_guest);
	change_string_param(usr, c, &PARAM_NAME_GUEST, "<green>Enter name Guest: ");
	Return;
}

void state_param_notify_login(User *usr, char c) {
	Enter(state_param_notify_login);
	change_string_param(usr, c, &PARAM_NOTIFY_LOGIN, "<green>Enter login notification: ");
	Return;
}

void state_param_notify_logout(User *usr, char c) {
	Enter(state_param_notify_logout);
	change_string_param(usr, c, &PARAM_NOTIFY_LOGOUT, "<green>Enter logout notification: ");
	Return;
}

void state_param_notify_linkdead(User *usr, char c) {
	Enter(state_param_notify_linkdead);
	change_string_param(usr, c, &PARAM_NOTIFY_LINKDEAD, "<green>Enter linkdead notification: ");
	Return;
}

void state_param_notify_idle(User *usr, char c) {
	Enter(state_param_notify_idle);
	change_string_param(usr, c, &PARAM_NOTIFY_IDLE, "<green>Enter idle notification: ");
	Return;
}

void state_param_notify_locked(User *usr, char c) {
	Enter(state_param_notify_locked);
	change_string_param(usr, c, &PARAM_NOTIFY_LOCKED, "<green>Enter locked notification: ");
	Return;
}

void state_param_notify_unlocked(User *usr, char c) {
	Enter(state_param_notify_unlocked);
	change_string_param(usr, c, &PARAM_NOTIFY_UNLOCKED, "<green>Enter unlocked notification: ");
	Return;
}

void state_param_notify_hold(User *usr, char c) {
	Enter(state_param_notify_hold);
	change_string_param(usr, c, &PARAM_NOTIFY_HOLD, "<green>Enter hold notification: ");
	Return;
}

void state_param_notify_unhold(User *usr, char c) {
	Enter(state_param_notify_unhold);
	change_string_param(usr, c, &PARAM_NOTIFY_UNHOLD, "<green>Enter release notification: ");
	Return;
}

void state_param_notify_enter_chat(User *usr, char c) {
	Enter(state_param_notify_enter_chat);
	change_string_param(usr, c, &PARAM_NOTIFY_ENTER_CHAT, "<green>Enter chat notification: ");
	Return;
}

void state_param_notify_leave_chat(User *usr, char c) {
	Enter(state_param_notify_leave_chat);
	change_string_param(usr, c, &PARAM_NOTIFY_LEAVE_CHAT, "<green>Leave chat notification: ");
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
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "\n<magenta>"
				"e<hotkey>Xpress Messages      <white>%-3s<magenta>      Quic<hotkey>k X messaging       <white>%s<magenta>\n"
				"<hotkey>Emotes                <white>%-3s<magenta>      <hotkey>Talked To list          <white>%s<magenta>\n"
				"Fee<hotkey>lings              <white>%-3s<magenta>      H<hotkey>old message mode       <white>%s<magenta>\n"
				"<hotkey>Questions             <white>%-3s<magenta>      Follow-<hotkey>up mode          <white>%s<magenta>\n",

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
				"X <hotkey>Reply               <white>%-3s<magenta>      eXpres<hotkey>s message header  <white>%s<magenta>\n"
				"<hotkey>Vanity flag           <white>%-3s<magenta>      Resident <hotkey>info           <white>%s<magenta>\n"
				"Cate<hotkey>gories            <white>%-3s<magenta>\n"
				"Ch<hotkey>at rooms            <white>%-3s<magenta>      Guess <hotkey>name rooms        <white>%s<magenta>\n"
				"<hotkey>Home> room            <white>%-3s<magenta>      <hotkey>Mail> room              <white>%s<magenta>\n",

				(PARAM_HAVE_X_REPLY == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_XMSG_HDR == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_VANITY == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_RESIDENT_INFO == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_CATEGORY == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_CHATROOMS == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_GUESSNAME == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_HOMEROOM == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_MAILROOM == PARAM_FALSE) ? "off" : "on"
			);
			Print(usr,
				"<hotkey>Calendar              <white>%-3s<magenta>      <hotkey>World clock             <white>%s<magenta>\n"
				"<hotkey>File cache            <white>%-3s<magenta>\n"
				"Wrapper a<hotkey>pply to all  <white>%-3s<magenta>      <hotkey>Display warnings        <white>%s<magenta>\n",

				(PARAM_HAVE_CALENDAR == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_WORLDCLOCK == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_FILECACHE == PARAM_FALSE) ? "off" : "on",

				(PARAM_HAVE_WRAPPER_ALL == PARAM_FALSE) ? "off" : "on",
				(PARAM_HAVE_DISABLED_MSG == PARAM_FALSE) ? "off" : "on"
			);
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Parameters\n");
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case 'x':
		case 'X':
			TOGGLE_FEATURE(PARAM_HAVE_XMSGS, "eXpress Messages");

		case 'e':
		case 'E':
			TOGGLE_FEATURE(PARAM_HAVE_EMOTES, "Emotes");

		case 'l':
		case 'L':
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

		case 's':
		case 'S':
			TOGGLE_FEATURE(PARAM_HAVE_XMSG_HDR, "eXpress message header");

		case 'v':
		case 'V':
			TOGGLE_FEATURE(PARAM_HAVE_VANITY, "Vanity flag");

		case 'c':
		case 'C':
			TOGGLE_FEATURE(PARAM_HAVE_CALENDAR, "Calendar");

		case 'w':
		case 'W':
			TOGGLE_FEATURE(PARAM_HAVE_WORLDCLOCK, "World clock");

		case 'a':
		case 'A':
			TOGGLE_FEATURE(PARAM_HAVE_CHATROOMS, "Chat rooms");

		case 'n':
		case 'N':
			TOGGLE_FEATURE(PARAM_HAVE_GUESSNAME, "Guess name rooms");

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
				(void)sort_Room(&AllRooms, room_sort_by_category);
			else
				(void)sort_Room(&AllRooms, room_sort_by_number);

			CURRENT_STATE(usr);
			Return;

		case 'i':
		case 'I':
			PARAM_HAVE_RESIDENT_INFO ^= PARAM_TRUE;
			Print(usr, "%s info in core memory\n", (PARAM_HAVE_RESIDENT_INFO == PARAM_FALSE) ? "Don't keep" : "Keep");
			usr->runtime_flags |= RTF_PARAM_EDITED;
			CURRENT_STATE(usr);
			Return;

		case 'f':
		case 'F':
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
	Print(usr, "<yellow>\n[%s] Features# <white>", PARAM_NAME_SYSOP);
	Return;
}

void state_log_menu(User *usr, char c) {
char *new_val;

	if (usr == NULL)
		return;

	Enter(state_log_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			buffer_text(usr);

			Print(usr, "\n<magenta>"
				"<hotkey>Syslog              <white>%s<magenta>\n"
				"<hotkey>Authlog             <white>%s<magenta>\n"
				"<hotkey>New users log       <white>%s<magenta>\n",

				PARAM_SYSLOG,
				PARAM_AUTHLOG,
				PARAM_NEWUSERLOG
			);
			Print(usr,
				"<hotkey>Rotate              <white>%s<magenta>\n"
				"Arch<hotkey>ive directory   <white>%s<magenta>\n",

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
			read_menu(usr);
			Return;

		case ' ':
		case KEY_RETURN:
		case KEY_BS:
			Put(usr, "Parameters\n");
/*
	if edited, re-initialize logging
*/
			if (usr->runtime_flags & RTF_WRAPPER_EDITED) {
				usr->runtime_flags &= ~RTF_WRAPPER_EDITED;
				init_log(-1);
			}
			RET(usr);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "\n");
			CURRENT_STATE(usr);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
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

		case 'n':
		case 'N':
			Put(usr, "New users log\n");
			CALL(usr, STATE_PARAM_NEWUSERLOG);
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
			usr->runtime_flags |= RTF_PARAM_EDITED;

			CURRENT_STATE(usr);
			Return;

		case 'c':
		case 'C':
			Put(usr, "Core dump directory\n");
			CALL(usr, STATE_PARAM_CRASHDIR);
			Return;
	}
	Print(usr, "<yellow>\n[%s] Logrotate# <white>", PARAM_NAME_SYSOP);
	Return;
}

void state_param_syslog(User *usr, char c) {
	Enter(state_param_syslog);
	change_string_param(usr, c, &PARAM_SYSLOG, "<green>Enter syslog file: ");
	usr->runtime_flags |= RTF_WRAPPER_EDITED;
	Return;
}

void state_param_authlog(User *usr, char c) {
	Enter(state_param_authlog);
	change_string_param(usr, c, &PARAM_AUTHLOG, "<green>Enter authlog file: ");
	usr->runtime_flags |= RTF_WRAPPER_EDITED;
	Return;
}

void state_param_newuserlog(User *usr, char c) {
	Enter(state_param_newuserlog);
	change_string_param(usr, c, &PARAM_NEWUSERLOG, "<green>Enter newuser log file: ");
	usr->runtime_flags |= RTF_WRAPPER_EDITED;
	Return;
}

void state_param_archivedir(User *usr, char c) {
	Enter(state_param_archivedir);
	change_string_param(usr, c, &PARAM_ARCHIVEDIR, "<green>Enter archive directory: ");
	Return;
}

void state_param_crashdir(User *usr, char c) {
	Enter(state_param_crashdir);
	change_string_param(usr, c, &PARAM_CRASHDIR, "<green>Enter core dump directory: ");
	Return;
}

/* EOB */
