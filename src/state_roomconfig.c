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
	state_roomconfig.c	WJ99

	The Room Config menu
*/

#include "config.h"
#include "debug.h"
#include "state_roomconfig.h"
#include "state_room.h"
#include "state_msg.h"
#include "state.h"
#include "edit.h"
#include "edit_param.h"
#include "util.h"
#include "log.h"
#include "cstring.h"
#include "screens.h"
#include "CachedFile.h"
#include "Param.h"
#include "OnlineUser.h"
#include "Category.h"
#include "Memory.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>

void state_room_config_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_room_config_menu);

	switch(c) {
		case INIT_PROMPT:
			break;

		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			if (usr->runtime_flags & RTF_ROOM_RESIZED) {
				if (usr->read_lines != usr->curr_room->max_msgs)
					resize_Room(usr->curr_room, usr->read_lines, NULL);

				usr->runtime_flags &= ~RTF_ROOM_RESIZED;
			}
			buffer_text(usr);

			Put(usr, "<magenta>\n"
				"<hotkey>Room info                      <hotkey>Help\n");

			if (usr->curr_room->flags & ROOM_INVITE_ONLY)
				Put(usr, "<hotkey>Invite/uninvite                Show <hotkey>invited\n");

			Put(usr, "<hotkey>Kickout/unkick                 Show <hotkey>kicked\n");

			if (!(usr->curr_room->flags & ROOM_HOME)) {
				Put(usr, "\n");

				if (usr->runtime_flags & RTF_SYSOP) {
					StringList *sl;

					Print(usr, "<hotkey>Assign/unassign %-15s", PARAM_NAME_ROOMAIDE);

					for(sl = usr->curr_room->room_aides; sl != NULL; sl = sl->next) {
						Print(usr, "<cyan>%s", sl->str);
						if (sl->next != NULL)
							Put(usr, ", ");
					}
					Put(usr, "<magenta>\n");
				}
				if (usr->curr_room->number != MAIL_ROOM && usr->curr_room->number != HOME_ROOM)
					Print(usr,
						"Change room <hotkey>name               <white>%s<magenta>\n", usr->curr_room->name);

				if (PARAM_HAVE_CATEGORY && category != NULL) {
					Put(usr, "<hotkey>Category");

					if (usr->curr_room->category != NULL)
						Print(usr, "                       <white>%s<magenta>", usr->curr_room->category);

					Put(usr, "\n");
				}
				if (usr->runtime_flags & RTF_SYSOP) {
					if (!(usr->curr_room->flags & ROOM_CHATROOM))
						Print(usr,
							"<hotkey>Maximum amount of messages     <white>%d<magenta>\n", usr->curr_room->max_msgs);

					Put(usr, "\n"
						"Reset creation <hotkey>date            (all users unjoin)\n"
						"<white>Ctrl-<hotkey>R<magenta>emove all posts          <white>Ctrl-<hotkey>D<magenta>elete room\n"
					);
				}
				Print(usr, "<magenta>\n"
					"<hotkey>1 Room has subject lines       <white>%s<magenta>\n"
					"<hotkey>2 Allow anonymous posts        <white>%s<magenta>\n",

					(usr->curr_room->flags & ROOM_SUBJECTS)  ? "Yes" : "No",
					(usr->curr_room->flags & ROOM_ANONYMOUS) ? "Yes" : "No"
				);
				if (usr->runtime_flags & RTF_SYSOP) {
					Print(usr,
						"<hotkey>3 Room is invite-only          <white>%s<magenta>\n"
						"<hotkey>4 Room is read-only            <white>%s<magenta>\n",

						(usr->curr_room->flags & ROOM_INVITE_ONLY) ? "Yes" : "No",
						(usr->curr_room->flags & ROOM_READONLY)    ? "Yes" : "No"
					);
					Print(usr,
						"<hotkey>5 Room is not zappable         <white>%s<magenta>\n"
						"<hotkey>6 Room is hidden               <white>%s<magenta>\n",

						(usr->curr_room->flags & ROOM_NOZAP)    ? "Yes" : "No",
						(usr->curr_room->flags & ROOM_HIDDEN)   ? "Yes" : "No"
					);
					if (PARAM_HAVE_CHATROOMS)
						Print(usr,
							"<hotkey>7 Room is a chat room          <white>%s<magenta>\n",

							(usr->curr_room->flags & ROOM_CHATROOM) ? "Yes" : "No"
						);
				}
			}
			read_menu(usr);
			Return;

		case '$':
			if (usr->runtime_flags & RTF_SYSOP)
				drop_sysop_privs(usr);
			else
				break;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_BS:
			Put(usr, "\n");

			if (usr->runtime_flags & RTF_ROOM_EDITED) {
				usr->curr_room->flags |= ROOM_DIRTY;		/* make sure it is saved! */
				save_Room(usr->curr_room);
				usr->runtime_flags &= ~RTF_ROOM_EDITED;
			}
/* user was unassigned as roomaide */
			if (in_StringList(usr->curr_room->room_aides, usr->name) == NULL)
				usr->runtime_flags &= ~RTF_ROOMAIDE;
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
			Put(usr, "Help\n");
			if (load_screen(usr->text, PARAM_HELP_ROOMCONFIG) < 0) {
				Put(usr, "<red>No help available\n");
				break;
			}
			PUSH(usr, STATE_PRESS_ANY_KEY);
			read_text(usr);
			Return;

		case 'r':
		case 'R':
			Put(usr, "Room info\n");
			CALL(usr, STATE_CHANGE_ROOMINFO);
			Return;

		case 'I':
			if (usr->curr_room->flags & ROOM_INVITE_ONLY) {
				Put(usr, "Invite\n");
				enter_name(usr, STATE_INVITE_PROMPT);
				Return;
			}
			break;

		case 'i':
			if (usr->curr_room->flags & ROOM_INVITE_ONLY) {
				Put(usr, "Show Invited\n");
				if (usr->curr_room->invited == NULL)
					Put(usr, "<red>No one is invited here\n");
				else {
					Put(usr, "\n");
					print_columns(usr, usr->curr_room->invited, 0);
				}
			}
			break;

		case 'K':
			Put(usr, "Kickout\n");
			if (usr->curr_room == Lobby_room) {
				Print(usr, "<red>You can't kick anyone from the <yellow>%s>\n", Lobby_room->name);
				break;
			}
			if (usr->curr_room == usr->mail) {
				Put(usr, "<red>You can't kick anyone from the <yellow>Mail><red> room\n");
				break;
			}
			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			enter_name(usr, STATE_KICKOUT_PROMPT);
			Return;

		case 'k':
			Put(usr, "Show kicked\n");
			if (usr->curr_room->kicked == NULL)
				Put(usr, "<red>No one has been kicked out\n");
			else {
				print_columns(usr, usr->curr_room->kicked, 0);
			}
			break;

		case 'a':
		case 'A':
			if (!(usr->runtime_flags & RTF_SYSOP) || (usr->curr_room->flags & ROOM_HOME))
				break;

			Print(usr, "Assign/unassign %s\n\n", PARAM_NAME_ROOMAIDE);
			if (usr->curr_room->room_aides != NULL) {
				StringList *sl;

				if (usr->curr_room->room_aides->next == NULL)
					Print(usr, "<cyan>%s is: ", PARAM_NAME_ROOMAIDE);
				else
					Print(usr, "<cyan>%ss are: ", PARAM_NAME_ROOMAIDE);

				for(sl = usr->curr_room->room_aides; sl != NULL; sl = sl->next) {
					Print(usr, "%s", sl->str);
					if (sl->next != NULL)
						Put(usr, ", ");
				}
				Put(usr, "\n\n");
			} else
				Print(usr, "<red>Currently, there are no %ss in this room\n\n", PARAM_NAME_ROOMAIDE);
			enter_name(usr, STATE_ASSIGN_ROOMAIDE);
			Return;

		case 'n':
		case 'N':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->curr_room->number != MAIL_ROOM && usr->curr_room->number != HOME_ROOM) {
				Put(usr, "Change room name\n");
				CALL(usr, STATE_CHANGE_ROOMNAME);
				Return;
			}
			break;

		case 'c':
		case 'C':
			if (!PARAM_HAVE_CATEGORY || (usr->curr_room->flags & ROOM_HOME))
				break;

			Put(usr, "Category\n");
			CALL(usr, STATE_CHOOSE_CATEGORY);
			Return;

		case 'm':
		case 'M':
			if ((usr->runtime_flags & RTF_SYSOP) && !(usr->curr_room->flags & ROOM_CHATROOM)) {
				Put(usr, "Maximum amount of messages\n");

				usr->runtime_flags |= RTF_ROOM_RESIZED;
				usr->read_lines = usr->curr_room->max_msgs;
				if (usr->read_lines < 1)
					usr->read_lines = PARAM_MAX_MESSAGES;

				CALL(usr, STATE_MAX_MESSAGES);
				Return;
			}
			break;

		case 'd':
		case 'D':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "Reset creation date\n");
				CALL(usr, STATE_RESET_CREATION_DATE);
				Return;
			}
			Return;

		case KEY_CTRL('R'):
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "Remove all posts\n");
				CALL(usr, STATE_REMOVE_ALL_POSTS);
				Return;
			}
			break;

		case KEY_CTRL('D'):
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "Delete room\n");
				CALL(usr, STATE_DELETE_ROOM);
				Return;
			}
			break;

		case '1':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			Put(usr, "1 (subjects)\n");
			usr->curr_room->flags ^= ROOM_SUBJECTS;
			usr->runtime_flags |= RTF_ROOM_EDITED;
			CURRENT_STATE(usr);
			Return;

		case '2':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			Put(usr, "2 (anonymous)\n");
			usr->curr_room->flags ^= ROOM_ANONYMOUS;
			usr->runtime_flags |= RTF_ROOM_EDITED;
			CURRENT_STATE(usr);
			Return;

		case '3':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "3 (invite-only)\n");
				usr->curr_room->flags ^= ROOM_INVITE_ONLY;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case '4':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "4 (read-only)\n");
				usr->curr_room->flags ^= ROOM_READONLY;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case '5':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "5 (not zappable)\n");
				usr->curr_room->flags ^= ROOM_NOZAP;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case '6':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "6 (hidden)\n");
				usr->curr_room->flags ^= ROOM_HIDDEN;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case '7':
			if (PARAM_HAVE_CHATROOMS) {
				if (usr->curr_room->flags & ROOM_HOME)
					break;

				if (usr->runtime_flags & RTF_SYSOP) {
					Put(usr, "7 (chat)\n");
					usr->curr_room->flags ^= ROOM_CHATROOM;
					usr->runtime_flags |= RTF_ROOM_EDITED;
					CURRENT_STATE(usr);
					Return;
				}
			}
			break;
	}
	if (usr->flags & USR_ROOMNUMBERS)
		Print(usr, "<yellow>\n[%u %s] Room Config%c <white>", usr->curr_room->number, usr->curr_room->name, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	else
		Print(usr, "<yellow>\n[%s] Room Config%c <white>", usr->curr_room->name, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
	Return;
}

void state_choose_category(User *usr, char c) {
int r, n;
StringList *sl;

	if (usr == NULL)
		return;

	Enter(state_choose_category);

	if (c == INIT_PROMPT) {
		Put(usr, "<green>Choose category: <yellow>");
		edit_number(usr, EDIT_INIT);
		Return;
	}
	if (c == INIT_STATE) {
		buffer_text(usr);
		Put(usr, "\n<green>  0<yellow> (none)\n");

		print_columns(usr, category, FORMAT_NUMBERED);

		Put(usr, "\n");
		if (usr->curr_room->category != NULL)
			Print(usr, "<cyan>The current category is: <white>%s\n", usr->curr_room->category);

		read_menu(usr);
		Return;
	}
	r = edit_number(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *p;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		n = atoi(usr->edit_buf);
		if (n < 0) {
			Put(usr, "<red>No such category\n");
			RET(usr);
			Return;
		}
		if (!n) {
			if (usr->curr_room->category != NULL) {
				Free(usr->curr_room->category);
				usr->curr_room->category = NULL;
				usr->runtime_flags |= RTF_ROOM_EDITED;

				if (PARAM_HAVE_CATEGORY)
					AllRooms = sort_Room(AllRooms, room_sort_by_category);
				else
					AllRooms = sort_Room(AllRooms, room_sort_by_number);
			}
			RET(usr);
			Return;
		}
		n--;
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
		if ((p = cstrdup(sl->str)) == NULL)
			Perror(usr, "failed to set category");
		else {
			Free(usr->curr_room->category);
			usr->curr_room->category = p;
			usr->runtime_flags |= RTF_ROOM_EDITED;

			if (PARAM_HAVE_CATEGORY)
				AllRooms = sort_Room(AllRooms, room_sort_by_category);
			else
				AllRooms = sort_Room(AllRooms, room_sort_by_number);
		}
		RET(usr);
		Return;
	}
	Return;
}

void state_change_roominfo(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_change_roominfo);

	switch(c) {
		case INIT_STATE:
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_BS:
			Put(usr, "Room config menu\n");

			if (!PARAM_HAVE_RESIDENT_INFO) {
				destroy_StringIO(usr->curr_room->info);
				usr->curr_room->info = NULL;
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

		case 'v':
		case 'V':
			Put(usr, "View\n");
			load_roominfo(usr->curr_room, usr->name);
			if (usr->curr_room->info == NULL || usr->curr_room->info->buf == NULL) {
				Put(usr, "<cyan>The room info is currently empty\n");
				CURRENT_STATE(usr);
				Return;
			}
			copy_StringIO(usr->text, usr->curr_room->info);
			Put(usr, "<green>");
			read_text(usr);
			Return;

		case 'e':
		case 'E':
			Put(usr, "Edit\n"
				"<green>\n"
				"Enter new room info, press <yellow><return><green> twice or press <yellow><Ctrl-C><green> to end\n"
			);
			edit_text(usr, save_roominfo, abort_roominfo);
			Return;

		case 'u':
		case 'U':
			Put(usr, "Upload\n"
				"<green>\n"
				"Upload new room info, press <yellow><Ctrl-C><green> to end\n"
			);
			usr->runtime_flags |= RTF_UPLOAD;
			edit_text(usr, save_roominfo, abort_roominfo);
			Return;

		case 'd':
		case 'D':
			Put(usr, "Download\n");
			load_roominfo(usr->curr_room, usr->name);
			if (usr->curr_room->info == NULL || usr->curr_room->info->buf == NULL) {
				Put(usr, "<cyan>The room info is currently empty\n");
				CURRENT_STATE(usr);
				Return;
			}
			copy_StringIO(usr->text, usr->curr_room->info);
			CALL(usr, STATE_DOWNLOAD_TEXT);
			Return;
	}
	Put(usr, "<magenta>\n<hotkey>View, <hotkey>Edit, <hotkey>Upload, <hotkey>Download: <white>");
	Return;
}

void save_roominfo(User *usr, char c) {
StringIO *tmp;

	if (usr == NULL)
		return;

	Enter(save_roominfo);

	if (usr->curr_room == NULL) {
		Perror(usr, "I'm sorry, but you have been sucked into a black hole");
		RET(usr);
		Return;
	}
	destroy_StringIO(usr->curr_room->info);
	usr->curr_room->info = NULL;

	if ((tmp = new_StringIO()) == NULL) {
		Perror(usr, "Failed to save room info");
		RET(usr);
		Return;
	}
	usr->curr_room->info = usr->text;
	usr->text = tmp;

	usr->curr_room->roominfo_changed++;			/* room info has been updated */

/*
	save it now, or else have problems with PARAM_HAVE_RESIDENT_INFO
*/
	usr->curr_room->flags |= ROOM_DIRTY;			/* make sure it is saved! */
	save_Room(usr->curr_room);
	usr->runtime_flags &= ~RTF_ROOM_EDITED;

	RET(usr);
	Return;
}

void abort_roominfo(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(abort_roominfo);

	free_StringIO(usr->text);
	RET(usr);
	Return;
}


void state_invite_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_invite_prompt);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *sl;
		User *u;
		Joined *j;
		char roomname_buf[MAX_LINE];

		if (!usr->edit_buf[0]) {
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
		if ((sl = in_StringList(usr->curr_room->kicked, usr->edit_buf)) != NULL) {
			remove_StringList(&usr->curr_room->kicked, sl);
			add_StringList(&usr->curr_room->invited, sl);

			Print(usr, "<yellow>%s<green> was kicked, but is now invited\n", sl->str);
			log_msg("%s invited %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

			if ((u = is_online(sl->str)) != NULL) {
				if (u != usr)
					Tell(u, "\n<magenta>You have been invited into %s<magenta> by <yellow>%s\n",
						room_name(u, usr->curr_room, roomname_buf, MAX_LINE), usr->name);

				if ((j = in_Joined(u->rooms, usr->curr_room->number)) == NULL)
					if ((j = new_Joined()) != NULL)
						add_Joined(&u->rooms, j);
				if (j != NULL) {
					j->zapped = 0;
					j->number = usr->curr_room->number;
					j->generation = usr->curr_room->generation;
				}
			}
		} else {
			if ((sl = in_StringList(usr->curr_room->invited, usr->edit_buf)) != NULL) {
				remove_StringList(&usr->curr_room->invited, sl);
				destroy_StringList(sl);

				Print(usr, "<yellow>%s<green> is uninvited\n", usr->edit_buf);
				log_msg("%s uninvited %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

				if ((u = is_online(usr->edit_buf)) != NULL) {
					if (u != usr)
						Tell(u, "\n<magenta>You have been uninvited from %s<magenta> by <yellow>%s\n",
							room_name(u, usr->curr_room, roomname_buf, MAX_LINE), usr->name);

					if ((usr->curr_room->flags & ROOM_HIDDEN)
						&& (j = in_Joined(u->rooms, usr->curr_room->number)) != NULL)
						j->zapped = 1;
				}
			} else {
				add_StringList(&usr->curr_room->invited, new_StringList(usr->edit_buf));
				Print(usr, "<yellow>%s<green> is invited\n", usr->edit_buf);
				log_msg("%s invited %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

				if ((u = is_online(usr->edit_buf)) != NULL) {
					if (u != usr)
						Tell(u, "\n<magenta>You have been invited in %s<magenta> by <yellow>%s\n",
							room_name(u, usr->curr_room, roomname_buf, MAX_LINE), usr->name);

					if ((j = in_Joined(u->rooms, usr->curr_room->number)) == NULL)
						if ((j = new_Joined()) != NULL)
							add_Joined(&u->rooms, j);
					if (j != NULL) {
						j->zapped = 0;
						j->number = usr->curr_room->number;
						j->generation = usr->curr_room->generation;
					}
				}
			}
		}
		usr->runtime_flags |= RTF_ROOM_EDITED;
		RET(usr);
	}
	Return;
}

void state_kickout_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_kickout_prompt);

	if (c == INIT_STATE) {
		if (usr->curr_room == Lobby_room) {
			Print(usr, "<red>You can't kick anyone out of the <yellow>%s><red>!\n", Lobby_room->name);
			RET(usr);
			Return;
		}
		if (usr->curr_room->number == MAIL_ROOM) {
			Put(usr, "<red>You can't kick anyone out of the <yellow>Mail><red> room!\n");
			RET(usr);
			Return;
		}
	}
	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *sl;
		User *u;
		Joined *j;
		char roomname_buf[MAX_LINE];

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if (!strcmp(usr->name, usr->edit_buf)) {
			Put(usr, "<red>You can't kick yourself out..!\n");
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
		if (in_StringList(usr->curr_room->room_aides, usr->edit_buf) != NULL
			&& !(usr->runtime_flags & RTF_SYSOP)) {
			if (in_StringList(usr->curr_room->kicked, usr->edit_buf) != NULL)
				Print(usr, "<red>You are not allowed to unkick the kicked %s\n", PARAM_NAME_ROOMAIDE);
			else
				Print(usr, "<red>You are not allowed to kick out your co-%s\n", PARAM_NAME_ROOMAIDE);

			RET(usr);
			Return;
		}
		if ((sl = in_StringList(usr->curr_room->invited, usr->edit_buf)) != NULL) {
			remove_StringList(&usr->curr_room->invited, sl);
			add_StringList(&usr->curr_room->kicked, sl);

			Print(usr, "<yellow>%s<green> was invited, but is now kicked\n", sl->str);
			log_msg("%s kicked %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

			if ((u = is_online(usr->edit_buf)) != NULL && u != usr) {
				Tell(u, "\n<magenta>You have been kicked out of %s<magenta> by <yellow>%s\n",
					room_name(u, usr->curr_room, roomname_buf, MAX_LINE), usr->name);

				if (u->curr_room == usr->curr_room) {
					Tell(u, "<green>You are being dropped off in the <yellow>%s>", Lobby_room->name);

					if ((u->runtime_flags & RTF_ROOMAIDE)
						&& in_StringList(Lobby_room->room_aides, usr->name) == NULL)
						u->runtime_flags &= ~RTF_ROOMAIDE;
					goto_room(u, Lobby_room);
				}
				if ((usr->curr_room->flags & ROOM_HIDDEN)
					&& (j = in_Joined(u->rooms, usr->curr_room->number)) != NULL) {
					j->zapped = 1;
					j->generation = usr->curr_room->generation;
				}
			}
		} else {
			if ((sl = in_StringList(usr->curr_room->kicked, usr->edit_buf)) != NULL) {
				remove_StringList(&usr->curr_room->kicked, sl);
				destroy_StringList(sl);

				Print(usr, "<yellow>%s<green> was kicked, but not anymore\n", usr->edit_buf);
				log_msg("%s unkicked %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

				if ((u = is_online(usr->edit_buf)) != NULL && u != usr) {
					Tell(u, "\n<magenta>You have been allowed access to %s<magenta> again by <yellow>%s\n",
						room_name(u, usr->curr_room, roomname_buf, MAX_LINE), usr->name);
				}
			} else {
				add_StringList(&usr->curr_room->kicked, new_StringList(usr->edit_buf));

				Print(usr, "<yellow>%s<green> has been kicked out\n", usr->edit_buf);
				log_msg("%s kicked %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

				if ((u = is_online(usr->edit_buf)) != NULL && u != usr) {
					Tell(u, "\n<magenta>You have been kicked out of %s<magenta> by <yellow>%s\n",
						room_name(u, usr->curr_room, roomname_buf, MAX_LINE), usr->name);

					if (u->curr_room == usr->curr_room) {
						Tell(u, "<green>You are being dropped off in the <yellow>%s>\n", Lobby_room->name);

						if ((u->runtime_flags & RTF_ROOMAIDE)
							&& in_StringList(Lobby_room->room_aides, usr->name) == NULL)
							u->runtime_flags &= ~RTF_ROOMAIDE;

						goto_room(u, Lobby_room);
					}
					if ((usr->curr_room->flags & ROOM_HIDDEN)
						&& (j = in_Joined(u->rooms, usr->curr_room->number)) != NULL) {
						j->zapped = 1;
						j->generation = usr->curr_room->generation;
					}
				}
			}
		}
		usr->runtime_flags |= RTF_ROOM_EDITED;
		RET(usr);
	}
	Return;
}

void state_assign_roomaide(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_assign_roomaide);

	r = edit_tabname(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		StringList *sl;
		User *u;

		if (!usr->edit_buf[0]) {
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
		if ((sl = in_StringList(usr->curr_room->room_aides, usr->edit_buf)) != NULL) {
			if (!(usr->runtime_flags & RTF_SYSOP)) {
				if (!strcmp(usr->edit_buf, usr->name)) {
					Print(usr, "<red>You can't unassign yourself as %s. Please ask a sysop to do so.\n", PARAM_NAME_ROOMAIDE);

					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;

					RET(usr);
					Return;
				}
				Print(usr, "<red>You are not allowed to unassign <yellow>%s<red> as %s.\n"
					"Please ask a sysop to do so.\n", usr->edit_buf, PARAM_NAME_ROOMAIDE);

				listdestroy_StringList(usr->recipients);
				usr->recipients = NULL;

				RET(usr);
				Return;
			}
			remove_StringList(&usr->curr_room->room_aides, sl);
			Print(usr, "<yellow>%s<green> is no longer %s of this room\n", usr->edit_buf, PARAM_NAME_ROOMAIDE);
			destroy_StringList(sl);

			if ((u = is_online(usr->edit_buf)) != NULL && u != usr) {
				Tell(u, "\n<magenta>You are no longer %s of <yellow>%s>\n", PARAM_NAME_ROOMAIDE, usr->curr_room->name);
				if (u->curr_room == usr->curr_room)
					u->runtime_flags &= ~RTF_ROOMAIDE;
			}
		} else {
			add_StringList(&usr->curr_room->room_aides, new_StringList(usr->edit_buf));
			Print(usr, "<yellow>%s<green> assigned as %s\n", usr->edit_buf, PARAM_NAME_ROOMAIDE);

			if ((u = is_online(usr->edit_buf)) != NULL && u != usr)
				Tell(u, "\n<magenta>You have been assigned as %s of <yellow>%s>\n", PARAM_NAME_ROOMAIDE, usr->curr_room->name);
		}
		usr->runtime_flags |= RTF_ROOM_EDITED;
		RET(usr);
	}
	Return;
}

void state_change_roomname(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_change_roomname);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter new room name: <yellow>");

	r = edit_roomname(usr, c);

	if (r == EDIT_BREAK) {
		Put(usr, "<red>Room name not changed\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		char *p;

		if (!usr->edit_buf[0] || !strcmp(usr->curr_room->name, usr->edit_buf)) {
			RET(usr);
			Return;
		}
		if (usr->edit_buf[0] >= '0' && usr->edit_buf[0] <= '9') {
			Put(usr, "<red>Room names cannot start with a digit\n");
			RET(usr);
			Return;
		}
		if (room_exists(usr->edit_buf)) {
			Put(usr, "<red>That room already exists\n");
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
		if ((p = cstrdup(usr->edit_buf)) != NULL) {
			Free(usr->curr_room->name);
			usr->curr_room->name = p;
			usr->runtime_flags |= RTF_ROOM_EDITED;
		} else
			Perror(usr, "failed to set new room name");
		RET(usr);
	}
	Return;
}

void state_max_messages(User *usr, char c) {
	Enter(state_max_messages);
	change_int_param(usr, c, &usr->read_lines);		/* abuse read_lines for this */
	Return;
}

void state_reset_creation_date(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_remove_all_posts);

	if (c == INIT_STATE) {
		if (usr->curr_room->number == MAIL_ROOM) {
			Put(usr, "<red>You can<yellow>'<red>t reset the creation date of the <yellow>Mail><red> room\n");
			RET(usr);
			Return;
		}
		Put(usr, "\n"
			"<yellow>Resetting the creation date makes all users unjoin the room.\n"
			"\n"
			"<cyan>Are you sure you wish to reset the creation date? (y/N): "
		);
	} else {
		switch(yesno(usr, c, 'N')) {
			case YESNO_YES:
				usr->curr_room->generation = (unsigned long)rtc;
				usr->runtime_flags |= RTF_ROOM_EDITED;

			case YESNO_NO:
				RET(usr);
				break;

			case YESNO_UNDEF:
				Put(usr, "<cyan>Reset creation date, <hotkey>Yes or <hotkey>No? (y/N): ");
		}
	}
	Return;
}

void state_remove_all_posts(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_remove_all_posts);

	if (c == INIT_STATE) {
		if (usr->curr_room->number == MAIL_ROOM) {
			Put(usr, "<red>You can't remove any posts from the <yellow>Mail><red> room\n");
			RET(usr);
			Return;
		}
		Put(usr, "\n"
"<red>Warning:<yellow> After removing all posts, you should also reset the creation date of\n"
"the room. This makes everyone unjoin (bad for invite-only rooms)\n");

		Put(usr, "\n<cyan>Are you sure you wish to remove all posts? (y/N): ");
	} else {
		switch(yesno(usr, c, 'N')) {
			case YESNO_YES:
				remove_all_posts(usr->curr_room);
				log_msg("SYSOP room %d %s cleaned out by %s",
					usr->curr_room->number, usr->curr_room->name, usr->name);
													/* fall through */
			case YESNO_NO:
				RET(usr);
				break;

			case YESNO_UNDEF:
				Put(usr, "<cyan>Remove all posts, <hotkey>Yes or <hotkey>No? (y/N): ");
		}
	}
	Return;
}

void remove_all_posts(Room *room) {
char buf[MAX_PATHLEN];
long num;
User *u;

	if (room == NULL)
		return;

	Enter(remove_all_posts);

	for(num = room->tail_msg; num <= room->head_msg; num++) {
		bufprintf(buf, MAX_PATHLEN, "%s/%u/%lu", PARAM_ROOMDIR, room->number, num);
		path_strip(buf);
		unlink_file(buf);

	}
	for(u = AllUsers; u != NULL; u = u->next) {
		if (u->curr_room == room)
			u->curr_msg = -1L;
	}
	Return;
}

void state_delete_room(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_delete_room);

	if (c == INIT_STATE)
		Put(usr, "<cyan>Are you sure? (y/N): ");
	else {
		switch(yesno(usr, c, 'N')) {
			case YESNO_YES:
				delete_room(usr, usr->curr_room);

			case YESNO_NO:
				RET(usr);
				break;

			case YESNO_UNDEF:
				Put(usr, "<cyan>Delete this room, <hotkey>Yes or <hotkey>No? (y/N): ");
		}
	}
	Return;
}

void delete_room(User *usr, Room *room) {
Joined *j;
User *u;
char path[MAX_PATHLEN], newpath[MAX_PATHLEN];
void (*func)(User *, char *, ...);

	if (room == NULL)
		return;

	Enter(delete_room);

	if (room->number == LOBBY_ROOM || room->number == MAIL_ROOM || room->number == HOME_ROOM) {
		Print(usr, "<red>The<yellow> %s><red> room is special and cannot be deleted\n", room->name);
		Return;
	}
	remove_Room(&AllRooms, room);

	for(u = AllUsers; u != NULL; u = u->next) {
		if (u->name[0] && u->curr_room == room) {
			if (u == usr)
				func = Print;
			else
				func = Tell;

			func(u, "\n<beep><red>The current room is being removed from the BBS.\n"
				"You are dropped off in the <yellow>%s>\n\n", Lobby_room->name);
/* unjoin */
			if ((j = in_Joined(u->rooms, u->curr_room->number)) != NULL) {
				remove_Joined(&u->rooms, j);
				destroy_Joined(j);
			}
			u->curr_room = Lobby_room;
			usr->runtime_flags &= ~RTF_ROOMAIDE;
			u->curr_msg = -1L;
		}
	}
/* move room to trash/ directory */
	bufprintf(path, MAX_PATHLEN, "%s/%u", PARAM_ROOMDIR, room->number);
	path_strip(path);
	bufprintf(newpath, MAX_PATHLEN, "%s/%s", PARAM_TRASHDIR, path);
	path_strip(newpath);

	rm_rf_trashdir(newpath);	/* make sure trash/newpath is empty or rename() will fail */
	rename_dir(path, newpath);

	log_msg("SYSOP room %d %s removed by %s", room->number, room->name, usr->name);
	Print(usr, "<yellow>%s><red> deleted\n", room->name);

	destroy_Room(room);
	Return;
}

/* EOB */
