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
	state_roomconfig.c	WJ99

	The Room Config menu
*/

#include "config.h"
#include "debug.h"
#include "state_roomconfig.h"
#include "state_msg.h"
#include "state.h"
#include "edit.h"
#include "util.h"
#include "log.h"
#include "screens.h"
#include "CachedFile.h"
#include "Param.h"
#include "OnlineUser.h"

#include <stdio.h>

void state_room_config_menu(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_room_config_menu);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;

			Put(usr, "\n"
				"<hotkey>E<magenta>dit room info              <hotkey>Help\n");

			if (usr->curr_room->flags & ROOM_INVITE_ONLY)
				Put(usr, "<hotkey>Invite/uninvite             Show <hotkey>invited\n");

			Put(usr, "<hotkey>Kickout/unkick              Show <hotkey>kicked\n");

			if (!(usr->curr_room->flags & ROOM_HOME)) {
				Put(usr, "\n");

				if (usr->runtime_flags & RTF_SYSOP)
					Print(usr, "<hotkey>Assign/unassign %s\n", PARAM_NAME_ROOMAIDE);

				if (usr->curr_room->number != 1 && usr->curr_room->number != 2)
					Put(usr, "Change room <hotkey>name\n");

				if (usr->runtime_flags & RTF_SYSOP) {
					Print(usr, "Reset <hotkey>creation date (all users unjoin)\n");
					Put(usr, "\n"
						"<white>Ctrl-<hotkey>R<magenta>emove all posts\n"
						"<white>Ctrl-<hotkey>D<magenta>elete room\n"
					);
				}
				Print(usr, "\n"
					"Flags\n"
					"<hotkey>1 <magenta>Room has subject lines    <white>[<yellow>%-3s<white>]\n"
					"<hotkey>2 <magenta>Allow anonymous posts     <white>[<yellow>%-3s<white>]\n",
					(usr->curr_room->flags & ROOM_SUBJECTS)  ? "Yes" : "No",
					(usr->curr_room->flags & ROOM_ANONYMOUS) ? "Yes" : "No"
				);
				if (usr->runtime_flags & RTF_SYSOP) {
					Print(usr,
						"<hotkey>3 <magenta>Room is invite-only       <white>[<yellow>%-3s<white>]\n"
						"<hotkey>4 <magenta>Room is read-only         <white>[<yellow>%-3s<white>]\n",
						(usr->curr_room->flags & ROOM_INVITE_ONLY) ? "Yes" : "No",
						(usr->curr_room->flags & ROOM_READONLY)    ? "Yes" : "No"
					);
					Print(usr,
						"<hotkey>5 <magenta>Room is not zappable      <white>[<yellow>%-3s<white>]\n"
						"<hotkey>6 <magenta>Room is hidden            <white>[<yellow>%-3s<white>]\n"
						"<hotkey>7 <magenta>Room is a chat room       <white>[<yellow>%-3s<white>]\n",
						(usr->curr_room->flags & ROOM_NOZAP)    ? "Yes" : "No",
						(usr->curr_room->flags & ROOM_HIDDEN)   ? "Yes" : "No",
						(usr->curr_room->flags & ROOM_CHATROOM) ? "Yes" : "No"
					);
				}
			}
			break;

		case ' ':
		case KEY_RETURN:
		case KEY_CTRL('C'):
		case KEY_BS:
			Put(usr, "\n");

			if (usr->runtime_flags & RTF_ROOM_EDITED) {
				save_Room(usr->curr_room);
				usr->runtime_flags &= ~RTF_ROOM_EDITED;
			}
/* user was unassigned as roomaide */
			if (in_StringList(usr->curr_room->room_aides, usr->name) == NULL)
				usr->runtime_flags &= ~RTF_ROOMAIDE;
			RET(usr);
			Return;

		case 'h':
		case 'H':
		case '?':
			Put(usr, "<white>Help\n");
			listdestroy_StringList(usr->more_text);
			if ((usr->more_text = load_screen(PARAM_HELP_ROOMCONFIG)) == NULL) {
				Put(usr, "<red>No help available\n");
				break;
			}
			PUSH(usr, STATE_PRESS_ANY_KEY);
			read_more(usr);
			Return;

		case 'e':
		case 'E':
		case KEY_CTRL('E'):
			Put(usr, "<white>Edit room info\n");

			listdestroy_StringList(usr->more_text);
			usr->more_text = usr->textp = NULL;

			if (usr->curr_room->info != NULL) {
				StringList *sl;

				if ((usr->more_text = new_StringList("<cyan>The room info currently is<white>:\n<green>")) == NULL
					|| (sl = copy_StringList(usr->curr_room->info)) == NULL) {
					Perror(usr, "Out of memory");
					break;
				}
				concat_StringList(&usr->more_text, sl);

				PUSH(usr, STATE_CHANGE_ROOMINFO);
				read_more(usr);
			} else {
				Put(usr, "<cyan>The room info currently is empty\n<green>");
				CALL(usr, STATE_CHANGE_ROOMINFO);
			}
			Return;

		case 'I':
			if (usr->curr_room->flags & ROOM_INVITE_ONLY) {
				Put(usr, "<white>Invite\n");
				enter_name(usr, STATE_INVITE_PROMPT);
				Return;
			}
			break;

		case 'i':
			if (usr->curr_room->flags & ROOM_INVITE_ONLY) {
				Put(usr, "<white>Show Invited\n");
				if (usr->curr_room->invited == NULL)
					Put(usr, "<red>No one is invited here\n");
				else
					show_namelist(usr, usr->curr_room->invited);
			}
			break;

		case 'K':
			Put(usr, "<white>Kickout\n");
			if (usr->curr_room == Lobby_room) {
				Print(usr, "<red>You can't kick anyone from the <yellow>%s<white>>\n", Lobby_room->name);
				break;
			}
			if (usr->curr_room == usr->mail) {
				Put(usr, "<red>You can't kick anyone from the <yellow>Mail<white>><red> room\n");
				break;
			}
			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			enter_name(usr, STATE_KICKOUT_PROMPT);
			Return;

		case 'k':
			Put(usr, "<white>Show kicked\n");
			if (usr->curr_room->kicked == NULL)
				Put(usr, "<red>No one has been kicked out\n");
			else
				show_namelist(usr, usr->curr_room->kicked);
			break;

		case 'a':
		case 'A':
			if (!(usr->runtime_flags & RTF_SYSOP) || (usr->curr_room->flags & ROOM_HOME))
				break;

			Print(usr, "<white>Assign/unassign %s\n\n", PARAM_NAME_ROOMAIDE);
			if (usr->curr_room->room_aides != NULL) {
				StringList *sl;

				if (usr->curr_room->room_aides->next == NULL)
					Print(usr, "<cyan>%s is<white>: ", PARAM_NAME_ROOMAIDE);
				else
					Print(usr, "<cyan>%ss are<white>: ", PARAM_NAME_ROOMAIDE);

				for(sl = usr->curr_room->room_aides; sl != NULL; sl = sl->next)
					if (sl->next != NULL)
						Print(usr, "<cyan>%s<white>, ", sl->str);
					else
						Print(usr, "<cyan>%s\n\n", sl->str);
			} else
				Print(usr, "<red>Currently, there are no %ss in this room\n\n", PARAM_NAME_ROOMAIDE);
			enter_name(usr, STATE_ASSIGN_ROOMAIDE);
			Return;

		case 'n':
		case 'N':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->curr_room->number != 1 && usr->curr_room->number != 2) {
				Put(usr, "<white>Change room name\n");
				CALL(usr, STATE_CHANGE_ROOMNAME);
				Return;
			}
			break;

		case 'c':
		case 'C':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "<white>Reset creation date\n");
				usr->curr_room->generation = (unsigned long)rtc;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
			}
			Return;

		case KEY_CTRL('R'):
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "<white>Remove all posts\n");
				CALL(usr, STATE_REMOVE_ALL_POSTS);
				Return;
			}
			break;

		case KEY_CTRL('D'):
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "<white>Delete room\n");
				CALL(usr, STATE_DELETE_ROOM);
				Return;
			}
			break;

		case '1':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			Put(usr, "<white>1 (subjects)\n");
			usr->curr_room->flags ^= ROOM_SUBJECTS;
			usr->runtime_flags |= RTF_ROOM_EDITED;
			CURRENT_STATE(usr);
			Return;

		case '2':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			Put(usr, "<white>2 (anonymous)\n");
			usr->curr_room->flags ^= ROOM_ANONYMOUS;
			usr->runtime_flags |= RTF_ROOM_EDITED;
			CURRENT_STATE(usr);
			Return;

		case '3':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "<white>3 (invite-only)\n");
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
				Put(usr, "<white>4 (read-only)\n");
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
				Put(usr, "<white>5 (not zappable)\n");
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
				Put(usr, "<white>6 (hidden)\n");
				usr->curr_room->flags ^= ROOM_HIDDEN;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;

		case '7':
			if (usr->curr_room->flags & ROOM_HOME)
				break;

			if (usr->runtime_flags & RTF_SYSOP) {
				Put(usr, "<white>7 (chat)\n");
				usr->curr_room->flags ^= ROOM_CHATROOM;
				usr->runtime_flags |= RTF_ROOM_EDITED;
				CURRENT_STATE(usr);
				Return;
			}
			break;
	}
	if (usr->flags & USR_ROOMNUMBERS)
		Print(usr, "\n<white>[%u <yellow>%s<white>]<yellow> Room Config<white>> ", usr->curr_room->number, usr->curr_room->name);
	else
		Print(usr, "\n<white>[<yellow>%s<white>]<yellow> Room Config<white>> ", usr->curr_room->name);
	Return;
}


void state_change_roominfo(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_change_roominfo);

	if (c == INIT_STATE) {
		Put(usr, "\n<cyan>Are you sure you wish to change this? <white>(<cyan>Y<white>/<cyan>n<white>): ");
		usr->runtime_flags |= RTF_BUSY;
	} else {
		switch(yesno(usr, c, 'Y')) {
			case YESNO_YES:
				POP(usr);
				usr->runtime_flags &= ~RTF_ROOM_EDITED;
				usr->runtime_flags |= RTF_UPLOAD;
				Print(usr, "\n<green>Upload new room info, press <white><<yellow>Ctrl-C<white>><green> to end\n");
				edit_text(usr, save_roominfo, abort_roominfo);
				break;

			case YESNO_NO:
				RET(usr);
				break;

			case YESNO_UNDEF:
				CURRENT_STATE(usr);
		}
	}
	Return;
}

void save_roominfo(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(save_roominfo);

	if (usr->curr_room == NULL) {
		Perror(usr, "I'm sorry, but you have been sucked into a black hole");
		RET(usr);
		Return;
	}
	usr->runtime_flags |= RTF_ROOM_EDITED;

	listdestroy_StringList(usr->curr_room->info);
	usr->more_text = rewind_StringList(usr->more_text);
	usr->curr_room->info = usr->more_text;
	usr->more_text = NULL;

	usr->curr_room->roominfo_changed++;		/* room info has been updated */
	RET(usr);
	Return;
}

void abort_roominfo(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(abort_roominfo);

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;
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
					Tell(u, "\n<magenta>You have been invited into %s<magenta> by <yellow>%s\n", room_name(u, usr->curr_room), usr->name);

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
						Tell(u, "\n<magenta>You have been uninvited from %s<magenta> by <yellow>%s\n", room_name(u, usr->curr_room), usr->name);

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
						Tell(u, "\n<magenta>You have been invited in %s<magenta> by <yellow>%s\n", room_name(u, usr->curr_room), usr->name);

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
			Print(usr, "<red>You can't kick anyone out of the <yellow>%s<white>><red>!\n", Lobby_room->name);
			RET(usr);
			Return;
		}
		if (usr->curr_room->number == 1) {
			Put(usr, "<red>You can't kick anyone out of the <yellow>Mail<white>><red> room!\n");
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
				Tell(u, "\n<magenta>You have been kicked out of %s<magenta> by <yellow>%s\n", room_name(u, usr->curr_room), usr->name);

				if (u->curr_room == usr->curr_room) {
					Tell(u, "<green>You are being dropped off in the <yellow>%s<white>>", Lobby_room->name);

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
					Tell(u, "\n<magenta>You have been allowed access to %s<magenta> again by <yellow>%s\n", room_name(u, usr->curr_room), usr->name);
				}
			} else {
				add_StringList(&usr->curr_room->kicked, new_StringList(usr->edit_buf));

				Print(usr, "<yellow>%s<green> has been kicked out\n", usr->edit_buf);
				log_msg("%s kicked %s (room %d %s>)", usr->name, usr->edit_buf, usr->curr_room->number, usr->curr_room->name);

				if ((u = is_online(usr->edit_buf)) != NULL && u != usr) {
					Tell(u, "\n<magenta>You have been kicked out of %s<magenta> by <yellow>%s\n", room_name(u, usr->curr_room), usr->name);

					if (u->curr_room == usr->curr_room) {
						Tell(u, "<green>You are being dropped off in the <yellow>%s<white>>\n", Lobby_room->name);

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
				Tell(u, "\n<magenta>You are no longer %s of <yellow>%s<white>>\n", PARAM_NAME_ROOMAIDE, usr->curr_room->name);
				if (u->curr_room == usr->curr_room)
					u->runtime_flags &= ~RTF_ROOMAIDE;
			}
		} else {
			add_StringList(&usr->curr_room->room_aides, new_StringList(usr->edit_buf));
			Print(usr, "<yellow>%s<green> assigned as %s\n", usr->edit_buf, PARAM_NAME_ROOMAIDE);

			if ((u = is_online(usr->edit_buf)) != NULL && u != usr)
				Tell(u, "\n<magenta>You have been assigned as %s of <yellow>%s<white>>\n", PARAM_NAME_ROOMAIDE, usr->curr_room->name);
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
		Put(usr, "<green>Enter new room name<yellow>: ");

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
		strcpy(usr->curr_room->name, usr->edit_buf);
		usr->runtime_flags |= RTF_ROOM_EDITED;
		RET(usr);
	}
	Return;
}

void state_remove_all_posts(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_remove_all_posts);

	if (c == INIT_STATE) {
		if (usr->curr_room->number == 1) {
			Put(usr, "<red>You can't remove any posts from the <yellow>Mail<white>><red> room\n");
			RET(usr);
			Return;
		}
		Put(usr, "\n"
"<red>Warning<white>:<yellow> After removing all posts<white>,<yellow> you should also reset the creation date of\n"
"the room<white>.<yellow> This makes everyone unjoin <white>(<yellow>bad for invite<white>-<yellow>only rooms<white>)\n");

		Put(usr, "\n<cyan>Are you sure you wish to remove all posts? <white>(<cyan>y<white>/<cyan>N<white>): ");
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
				Put(usr, "<cyan>Remove all posts, <hotkey>Yes or <hotkey>No? <white>(<cyan>y<white>/<cyan>N<white>): ");
		}
	}
	Return;
}

void remove_all_posts(Room *room) {
char buf[MAX_PATHLEN];
MsgIndex *idx, *idx_next;
User *u;

	if (room == NULL)
		return;

	Enter(remove_all_posts);

	for(idx = room->msgs; idx != NULL; idx = idx_next) {
		idx_next = idx->next;

		sprintf(buf, "%s/%u/%lu", PARAM_ROOMDIR, room->number, idx->number);
		path_strip(buf);
		unlink_file(buf);

		for(u = AllUsers; u != NULL; u = u->next)
			if (u->curr_msg == idx)
				u->curr_msg = NULL;

		destroy_MsgIndex(idx);
	}
	room->msgs = NULL;
	Return;
}

void state_delete_room(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_delete_room);

	if (c == INIT_STATE)
		Put(usr, "<cyan>Are you sure? <white>(<cyan>y<white>/<cyan>N<white>): ");
	else {
		switch(yesno(usr, c, 'N')) {
			case YESNO_YES:
				delete_room(usr, usr->curr_room);

			case YESNO_NO:
				RET(usr);
				break;

			case YESNO_UNDEF:
				Put(usr, "<cyan>Delete this room, <hotkey>Yes or <hotkey>No? <white>(<cyan>y<white>/<cyan>N<white>): ");
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

	if (room->number <= 2) {
		Print(usr, "<red>The <yellow>%s<white>><red> room is special and cannot be deleted\n", room->name);
		Return;
	}
	remove_Room(&AllRooms, room);

	for(u = AllUsers; u != NULL; u = u->next) {
		if (u->socket > 0 && u->curr_room == room) {
			if (u == usr)
				func = Print;
			else
				func = Tell;

			func(u, "\n<beep><red>The current room is being removed from the BBS.\n"
				"You are dropped off in the <yellow>%s<white>>\n\n", Lobby_room->name);
/* unjoin */
			if ((j = in_Joined(u->rooms, u->curr_room->number)) != NULL) {
				remove_Joined(&u->rooms, j);
				destroy_Joined(j);
			}
			u->curr_room = Lobby_room;
			usr->runtime_flags &= ~RTF_ROOMAIDE;
			u->curr_msg = NULL;
		}
	}
/* move room to trash/ directory */
	sprintf(path, "%s/%u", PARAM_ROOMDIR, room->number);
	path_strip(path);
	sprintf(newpath, "%s/%s", PARAM_TRASHDIR, path);
	path_strip(newpath);

	rm_rf_trashdir(newpath);	/* make sure trash/newpath is empty or rename() will fail */
	rename_dir(path, newpath);

	log_msg("SYSOP room %d %s removed by %s", room->number, room->name, usr->name);
	Print(usr, "<yellow>%s<white>><red> deleted\n", room->name);

	destroy_Room(room);
	Return;
}

/* EOB */
