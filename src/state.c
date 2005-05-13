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
#include "DataCmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#define STRING_CHANCE	((rand() % 20) < 4)

char *Str_enter_chatroom[] = {
	"enters",
	"enters",
	"enters",
	"enters",
	"enters the room",
	"strolls in",
	"walks in",
	"comes walking in",
	"comes strolling in",
	"falls into the room",
	"jumps in",
	"bounces in",
	"hops in",
};

char *Str_leave_chatroom[] = {
	"leaves",
	"leaves",
	"leaves",
	"leaves",
	"leaves the room",
	"walks out the door",
	"is out of here",
	"strolls out",
	"strolls away",
	"walks away",
	"falls out of the room",
	"jumps out",
	"bounces away",
	"hops out",
};


void state_dummy(User *usr, char c) {
	if (usr == NULL)
		return;

	POP(usr);				/* dummy ret; just pop the call off the stack */
}


void state_room_prompt(User *usr, char c) {
int i, idx;

	if (usr == NULL)
		return;

	Enter(state_room_prompt);

/*
	First check for chat rooms (they're a rather special case)
*/
	if (usr->curr_room != NULL) {
		if (usr->curr_room->flags & ROOM_CHATROOM) {
			if (!(usr->runtime_flags & RTF_CHAT_ESCAPE)) {
				switch(c) {
					case INIT_STATE:
						edit_line(usr, EDIT_INIT);
						usr->runtime_flags &= ~(RTF_BUSY | RTF_CHAT_ESCAPE);
						PrintPrompt(usr);
						break;

					case '/':
						if (!usr->edit_pos) {
							Put(usr, "<white>/");
							usr->runtime_flags |= RTF_CHAT_ESCAPE;
							Return;
						}

					default:
						i = edit_line(usr, c);

						if (i == EDIT_RETURN)
							chatroom_say(usr, usr->edit_buf);

						if (i == EDIT_RETURN || i == EDIT_BREAK) {
							edit_line(usr, EDIT_INIT);
							usr->runtime_flags &= ~(RTF_BUSY | RTF_CHAT_ESCAPE);
							PrintPrompt(usr);
						} else
							usr->runtime_flags |= RTF_BUSY;
				}
				Return;
			}
		}
	}
/*
	Now handle the more 'normal' DOC-style rooms
*/
	switch(c) {
		case INIT_STATE:
			break;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

		case KEY_CTRL('Q'):
			if (PARAM_HAVE_QUICK_X) {
				Put(usr, "<white>Quicklist\n");
				print_quicklist(usr);
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Quick X<red> is not enabled on this server\n");
			break;

		case KEY_ESC:
			Put(usr, "<white>Escape\n");
			if (usr->curr_room != NULL && (usr->curr_room->flags & ROOM_CHATROOM))
				Put(usr, "<red>In case you were looking for the exit: Type <yellow>/L<red> to leave\n");
			else
				Put(usr, "<red>In case you were looking for the exit: Press <hotkey>L to logout\n");
			break;

		case 'h':
		case 'H':
		case '?':
			Put(usr, "<white>Help\n");
			listdestroy_StringList(usr->more_text);
			if ((usr->more_text = load_screen(PARAM_HELP_STD)) == NULL) {
				Put(usr, "<red>No help available\n");
				break;
			}
			read_more(usr);
			Return;

		case KEY_CTRL('G'):
			Put(usr, "<white>GNU General Public License\n");
			listdestroy_StringList(usr->more_text);
			if ((usr->more_text = load_screen(PARAM_GPL_SCREEN)) == NULL) {
				Put(usr, "<red>The GPL file is missing\n");		/* or out of memory! */
				break;
			}
			read_more(usr);
			Return;

		case '[':
			Put(usr, "<white>bbs100 version information\n");
			print_version_info(usr);
			break;

		case ']':
			Put(usr, "<white>Local modifications made to bbs100\n");
			listdestroy_StringList(usr->more_text);
			if ((usr->more_text = load_screen(PARAM_MODS_SCREEN)) == NULL) {
				Put(usr, "<red>The local mods file is missing\n");		/* or out of memory! */
				break;
			}
			read_more(usr);
			Return;

		case 'w':
			Put(usr, "<white>Who\n\n");
			if (usr->flags & USR_SHORT_WHO)
				who_list(usr, WHO_LIST_SHORT);
			else
				who_list(usr, WHO_LIST_LONG);
			Return;

		case 'W':
			Put(usr, "<white>Who\n\n");
			if (usr->flags & USR_SHORT_WHO)
				who_list(usr, WHO_LIST_LONG);
			else
				who_list(usr, WHO_LIST_SHORT);
			Return;

		case KEY_CTRL('W'):
			Put(usr, "<white>Customize Who list\n");
			CALL(usr, STATE_CONFIG_WHO);
			Return;

		case KEY_CTRL('F'):
			Put(usr, "<white>Online friends\n");
			online_friends_list(usr);
			Return;

		case KEY_CTRL('T'):
			if (PARAM_HAVE_TALKEDTO) {
				Put(usr, "<white>Talked to list\n");
				talked_list(usr);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Talked To lists<red> are not enabled on this server\n");
			break;

		case 'q':
		case 'Q':
			if (PARAM_HAVE_QUESTIONS) {
				Put(usr, "<white>Question\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot ask questions\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->question_asked == NULL) {			/* this should never happen */
					if (!next_helping_hand(usr)) {
						Put(usr, "<red>Sorry, but currently there is no one available to help you\n");

						listdestroy_StringList(usr->recipients);
						usr->recipients = NULL;
						break;
					}
				}
				Print(usr, "<green>The question goes to <yellow>%s\n", usr->question_asked);

				listdestroy_StringList(usr->recipients);
				usr->recipients = new_StringList(usr->question_asked);

				CALL(usr, STATE_EDIT_QUESTION);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Questions<red> are not enabled on this server\n");
			break;

		case '%':
			if (PARAM_HAVE_QUESTIONS) {
				Put(usr, "<white>Toggle helper status\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot be available to help others\n", PARAM_NAME_GUEST);
					break;
				}
				if ((usr->flags & USR_HELPING_HAND) || (usr->runtime_flags & RTF_WAS_HH)) {
					usr->flags &= ~USR_HELPING_HAND;
					usr->runtime_flags &= ~RTF_WAS_HH;
					Put(usr, "<magenta>You are no longer available to help others\n");
				} else {
					if (usr->flags & USR_X_DISABLED) {
						Put(usr, "<red>You must enable message reception if you want to be available to help others\n");
						break;
					}
					if (usr->runtime_flags & RTF_HOLD) {
						if (PARAM_HAVE_HOLD) {
							Put(usr, "<red>You must not have messages on hold if you want to be available to help others\n");
							break;
						} else
							usr->runtime_flags &= ~RTF_HOLD;
					}
					usr->flags |= USR_HELPING_HAND;
					Put(usr, "<magenta>You now are available to help others\n");
				}
				break;
			} else {
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Questions<red> and <yellow>Helpers<red> are not enabled on this server\n");

				usr->flags &= ~USR_HELPING_HAND;
			}
			break;

		case 'x':
			if (PARAM_HAVE_XMSGS) {
				Put(usr, "<white>eXpress Message\n<green>");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send eXpress Messages\n", PARAM_NAME_GUEST);
					break;
				}
				enter_recipients(usr, STATE_X_PROMPT);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>eXpress Messages<red> are not enabled on this server\n");
			break;

		case '0':
			c = '9'+1;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (PARAM_HAVE_QUICK_X) {
				Put(usr, "<white>Quick X\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send Quick eXpress Messages\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->quick[c - '1'] != NULL) {
					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;

					strcpy(usr->edit_buf, usr->quick[c - '1']);
					usr->edit_pos = strlen(usr->edit_buf);
					usr->runtime_flags |= RTF_BUSY;

					Print(usr, "<green>Enter recipient<yellow>: %s", usr->edit_buf);
					PUSH(usr, STATE_X_PROMPT);
					Return;
				} else
					Put(usr, "<red>That quicklist entry is empty. Press <white><<yellow>Ctrl<white>-<yellow>C<white>><red> to enter the <yellow>Config menu<red>\n"
						"so you can configure your quicklist\n");
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Quick X<red> is not enabled on this server\n");
			break;

		case KEY_CTRL('X'):
			if (PARAM_HAVE_XMSGS) {
				Put(usr, "<white>Message history\n");
				CALL(usr, STATE_HISTORY_PROMPT);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>eXpress Messages<red> are not enabled on this server\n");
			break;

		case ':':
		case ';':
			if (PARAM_HAVE_EMOTES) {
				Put(usr, "<white>Emote\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send emotes\n", PARAM_NAME_GUEST);
					break;
				}
				Put(usr, "<green>");
				enter_recipients(usr, STATE_EMOTE_PROMPT);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Emotes<red> are not enabled on this server\n");
			break;

		case '*':
			if (PARAM_HAVE_FEELINGS) {
				Put(usr, "<white>Feelings\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send feelings\n", PARAM_NAME_GUEST);
					break;
				}
				Put(usr, "<green>");
				enter_recipients(usr, STATE_FEELINGS_PROMPT);
				Return;			
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Feelings<red> are not enabled on this server\n");
			break;

		case KEY_CTRL('P'):
			Put(usr, "<white>Ping\n<green>");
			enter_recipients(usr, STATE_PING_PROMPT);
			Return;

		case 'v':
			if (PARAM_HAVE_X_REPLY) {
				Put(usr, "<white>Reply\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send replies\n", PARAM_NAME_GUEST);
					break;
				}
				reply_x(usr, REPLY_X_ONE);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but the <yellow>X Reply<red> feature is not enabled on this server\n");
			break;

		case 'V':
			if (PARAM_HAVE_X_REPLY) {
				Put(usr, "<white>Reply to all\n");
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send replies\n", PARAM_NAME_GUEST);
					break;
				}
				reply_x(usr, REPLY_X_ALL);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but the <yellow>X Reply<red> feature is not enabled on this server\n");
			break;

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
			Return;

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

		case 'l':
		case 'L':
			if (usr->curr_room != NULL && (usr->curr_room->flags & ROOM_CHATROOM)) {
				Put(usr, "<white>Leave\n");
				goto_room(usr, Lobby_room);
			} else {
				Put(usr, "<white>Logout\n");
				CALL(usr, STATE_LOGOUT_PROMPT);
			}
			Return;

		case '@':
			if ((usr->runtime_flags & RTF_SYSOP)
				|| in_StringList(usr->curr_room->room_aides, usr->name) != NULL) {

				Print(usr, "<white>Toggle %s status\n", PARAM_NAME_ROOMAIDE);

				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot play %s\n", PARAM_NAME_GUEST, PARAM_NAME_ROOMAIDE);
					break;
				}
				usr->runtime_flags ^= RTF_ROOMAIDE;
				Print(usr, "<magenta>%s functions are now <yellow>%sabled\n", PARAM_NAME_ROOMAIDE,
					(usr->runtime_flags & RTF_ROOMAIDE) ? "en" : "dis");
			}
			break;

		case '$':
			if (usr->runtime_flags & RTF_SYSOP) {
				usr->runtime_flags &= ~RTF_SYSOP;
				Print(usr, "<white>Exiting %s mode\n", PARAM_NAME_SYSOP);

				log_msg("SYSOP %s left %s mode", usr->name, PARAM_NAME_SYSOP);

/* silently disable roomaide mode, if not allowed to be roomaide here */
				if ((usr->runtime_flags & RTF_ROOMAIDE) && in_StringList(usr->curr_room->room_aides, usr->name) == NULL)
					usr->runtime_flags &= ~RTF_ROOMAIDE;
				break;
			}
			if (get_su_passwd(usr->name) != NULL) {
				Print(usr, "<white>%s mode\n", PARAM_NAME_SYSOP);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot play %s\n", PARAM_NAME_GUEST, PARAM_NAME_SYSOP);
					break;
				}
				CALL(usr, STATE_SU_PROMPT);
				Return;
			}

		case 't':
		case 'T':
			Put(usr, "Time\n");
			print_calendar(usr);
			break;

		case 'd':
		case 'D':
			Put(usr, "<white>Delete\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot delete messages\n", PARAM_NAME_GUEST);
				break;
			}
			if (usr->message != NULL && usr->message->from[0]) {
/* delete message */
				if (usr->message->deleted != (time_t)0UL) {
					Put(usr, "<red>Message already has been deleted<yellow>..!\n\n");
					break;
				}
				if (
	((usr->message->flags & MSG_FROM_SYSOP) && !(usr->runtime_flags & RTF_SYSOP))
	|| ((usr->message->flags & MSG_FROM_ROOMAIDE) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))
	|| (usr->curr_room != usr->mail && strcmp(usr->name, usr->message->from) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))
				) {
					Put(usr, "<red>You are not allowed to delete this message\n\n");
					break;
				}
				CALL(usr, STATE_DEL_MSG_PROMPT);
				Return;
			} else
				Put(usr, "<red>No message to delete\n");
			break;

		case KEY_CTRL('D'):
			Put(usr, "<white>Doing\n");
			CALL(usr, STATE_CONFIG_DOING);
			Return;

		case 'u':
		case 'U':
			Put(usr, "<white>Undelete\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot undelete messages\n", PARAM_NAME_GUEST);
				break;
			}
			undelete_msg(usr);
			break;

		case 'B':
			usr->flags ^= USR_BEEP;
			if (usr->flags & USR_BEEP)
				usr->flags |= USR_ROOMBEEP;
			else
				usr->flags &= ~USR_ROOMBEEP;

			Print(usr, "<white>Toggle beeping\n"
				"<magenta>Messages will %s beep on arrival\n", (usr->flags & USR_BEEP) ? "now" : "<yellow>not<magenta>");
			break;

		case KEY_CTRL('B'):
			if (PARAM_HAVE_HOLD) {
				Put(usr, "<white>Hold messages\n");

				usr->runtime_flags ^= RTF_HOLD;

				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>Messages will be held until you press "
						"<white><<yellow>Ctrl<white>-<yellow>B<white>> <magenta>again\n");

					if (usr->flags & USR_HELPING_HAND) {		/* this is inconvenient right now */
						usr->flags &= ~USR_HELPING_HAND;
						usr->runtime_flags |= RTF_WAS_HH;
					}
				} else {
					Put(usr, "<magenta>Messages will <yellow>no longer<magenta> be held\n");

					if (usr->held_msgs != NULL) {
						usr->runtime_flags |= RTF_HOLD;			/* keep it on hold for a little longer */
						CALL(usr, STATE_HELD_HISTORY_PROMPT);
						Return;
					}
					if (usr->runtime_flags & RTF_WAS_HH) {
						usr->runtime_flags &= ~RTF_WAS_HH;
						usr->flags |= USR_HELPING_HAND;
					}
				}
			} else {
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Hold Message Mode<red> is not enabled on this server\n");

				usr->runtime_flags &= ~RTF_HOLD;
			}
			break;

		case 'j':
		case 'J':
			Put(usr, "<white>Jump\n");
			CALL(usr, STATE_JUMP_ROOM);
			Return;

		case 'z':
		case 'Z':
			Put(usr, "<white>Zap\n");
			CALL(usr, STATE_ZAP_PROMPT);
			Return;

		case KEY_CTRL('Z'):
			Put(usr, "<white>Zap all rooms\n");
			CALL(usr, STATE_ZAPALL_PROMPT);
			Return;

		case '#':
			CALL(usr, STATE_ENTER_MSG_NUMBER);
			Return;

		case '-':
			Put(usr, "<white>Read last\n");
			CALL(usr, STATE_ENTER_MINUS_MSG);
			Return;

		case '+':
			Put(usr, "<white>Skip forward\n");
			CALL(usr, STATE_ENTER_PLUS_MSG);
			Return;

		case 'm':
		case 'M':
			if (PARAM_HAVE_MAILROOM) {
				if (usr->curr_room != usr->mail)
					goto_room(usr, usr->mail);		/* 'mail anywhere', by Richard of MatrixBBS */
			} else {
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but this server has no <yellow>Mail<white>> <red>room\n");
				break;
			}

		case 'e':
		case 'E':
		case KEY_CTRL('E'):
			if (c == KEY_CTRL('E'))
				usr->runtime_flags |= RTF_UPLOAD;
			else
				usr->runtime_flags &= ~RTF_UPLOAD;

			enter_message(usr);
			Return;

		case 's':
			if (usr->curr_msg >= 0) {
				Put(usr, "<white>Stop\n");
				stop_reading(usr);
			}
			break;

		case 'S':
			Put(usr, "<white>Statistics\n<green>");
			print_stats(usr);
			Return;

/*
	read again
	contributed by Mutation of MatrixBBS
*/
		case 'a':
			if (usr->curr_room->msgs == NULL || usr->curr_room->msg_idx <= 0) {
				Put(usr, "<white>Again\n"
					"<red>No messages\n");
				break;
			}
			if (usr->curr_msg < 0)
				break;

			Put(usr, "<white>Again\n");
			PUSH(usr, STATE_ROOM_PROMPT);
			readMsg(usr);
			Return;
			break;

/*
	read parent message of a Reply
	contributed by Mutation of MatrixBBS
*/
		case '(':
			if (usr->curr_room->msgs == NULL || usr->curr_room->msg_idx <= 0) {
				Put(usr, "<white>Read Parent\n"
					"<red>No messages\n");
				break;
			}
			if (usr->curr_msg < 0)
				break;

			if (!(usr->message->flags & MSG_REPLY)) {
				Put(usr, "<white>Read Parent\n"
					"<red>This is not a reply\n");
				break;
			}
			for(idx = 0; idx < usr->curr_room->msg_idx; idx++) {
				if (usr->curr_room->msgs[idx] == usr->message->reply_number)
					break;

				if (usr->curr_room->msgs[idx] > usr->message->reply_number) {		/* we're not getting there anymore */
					idx = usr->curr_room->msg_idx;
					break;
				}
			}
			if (idx >= usr->curr_room->msg_idx) {
				Put(usr, "<red>Parent message doesn't exist anymore\n");
				break;
			}
			usr->curr_msg = idx;

			Put(usr, "<white>Read Parent\n");
			PUSH(usr, STATE_ROOM_PROMPT);
			readMsg(usr);
			Return;
			break;

		case 'b':
			if (usr->curr_room->msgs == NULL || usr->curr_room->msg_idx <= 0) {
				Put(usr, "<white>Back\n"
					"<red>No messages\n");
				break;
			}
			if (usr->curr_msg < 0)
				usr->curr_msg = usr->curr_room->msg_idx - 1;
			else
				usr->curr_msg--;

			if (usr->curr_msg >= 0) {
				Put(usr, "<white>Back\n");
				PUSH(usr, STATE_ROOM_PROMPT);
				readMsg(usr);
				Return;
			}
			break;

		case 'g':
		case 'G':
			do {
				Room *r;

				r = next_unread_room(usr);
				if (r != usr->curr_room) {
					Print(usr, "<white>Goto <yellow>%s<white>\n", r->name);
					goto_room(usr, r);
					Return;
				}
			} while(0);
			Put(usr, "<red>No new messages");
			break;

		case ' ':
			listdestroy_StringList(usr->more_text);
			usr->more_text = NULL;
			destroy_Message(usr->message);
			usr->message = NULL;

			if (usr->curr_room->flags & ROOM_CHATROOM)
				break;

			if (usr->curr_msg < 0) {
				Joined *j;

				if ((j = in_Joined(usr->rooms, usr->curr_room->number)) == NULL) {
					Perror(usr, "All of a sudden you haven't joined the current room ?");
					break;
				}
				if ((usr->curr_msg = newMsgs(usr->curr_room, j->last_read)) >= 0) {
					Put(usr, "<white>Read New\n");
					PUSH(usr, STATE_ROOM_PROMPT);
					readMsg(usr);
					Return;
				} else {
					Room *r;

					r = next_unread_room(usr);
					if (r != usr->curr_room) {
						Print(usr, "<white>Goto <yellow>%s<white>\n", r->name);
						goto_room(usr, r);
						Return;
					}
				}
			} else {
				usr->curr_msg++;
				if (usr->curr_msg < usr->curr_room->msg_idx) {
					Put(usr, "<white>Read Next\n");
					PUSH(usr, STATE_ROOM_PROMPT);
					readMsg(usr);
					Return;
				} else {
					usr->curr_msg = -1;
					break;
				}
			}
			Put(usr, "<red>No new messages");
			break;

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
					Return;
				}
				break;
			}
			break;

		case 'r':
		case 'R':
		case KEY_CTRL('R'):
			if (c == 'R' && usr->message != NULL && usr->message->to != NULL && usr->message->to->next != NULL)
				Print(usr, "<white>%seply to all\n", (c == KEY_CTRL('R')) ? "Upload R" : "R");
			else
				Print(usr, "<white>%seply\n", (c == KEY_CTRL('R')) ? "Upload R" : "R");

			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot reply to messages\n", PARAM_NAME_GUEST);
				break;
			}
			if (c == KEY_CTRL('R')) {
				usr->runtime_flags |= RTF_UPLOAD;
				c = 'R';
			} else
				usr->runtime_flags &= ~RTF_UPLOAD;

			if (usr->message != NULL && usr->message->from[0]) {
				Message *m;

				if ((usr->curr_room->flags & ROOM_READONLY)
					&& !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE))) {
					Put(usr, "<red>You are not allowed to post in this room\n");
					break;
				}
				if ((m = new_Message()) == NULL) {
					Perror(usr, "Out of memory");
					break;
				}
				strcpy(m->from, usr->name);

				if (usr->curr_room == usr->mail) {
					if (mail_access(usr, usr->message->from))
						break;

					if ((m->to = new_StringList(usr->message->from)) == NULL) {
						Perror(usr, "Out of memory");
						break;
					}
/* reply to all */
					if (c == 'R') {
						StringList *sl;

						for(sl = usr->message->to; sl != NULL; sl = sl->next) {
							if (strcmp(sl->str, usr->name) && !mail_access(usr, sl->str))
								add_StringList(&m->to, new_StringList(sl->str));
						}
					}
				}
				m->mtime = rtc;
				m->flags = MSG_REPLY;
				if (usr->runtime_flags & RTF_SYSOP)
					m->flags |= MSG_FROM_SYSOP;
				else
					if (usr->runtime_flags & RTF_ROOMAIDE)
						m->flags |= MSG_FROM_ROOMAIDE;

				if (usr->message->subject[0])
					strcpy(m->subject, usr->message->subject);

				if (usr->curr_room == usr->mail)
					m->reply_number = 0UL;
				else
					m->reply_number = usr->message->number;
/*
				else
					if (usr->curr_room != usr->mail) {
						char num_buf[25];

						sprintf(m->subject, "<message #%s>", print_number(usr, usr->message->number, num_buf));
					}
*/
				destroy_Message(usr->new_message);
				usr->new_message = m;
				enter_the_message(usr);
				Return;
			} else
				Put(usr, "<red>No message to reply to\n");
			break;

		case 'f':
			Put(usr, "<white>Forward\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot forward messages\n", PARAM_NAME_GUEST);
				break;
			}
			destroy_Message(usr->new_message);
			usr->new_message = copy_Message(usr->message);

			if (usr->curr_room == usr->mail) {
				if (usr->new_message != NULL) {
					if (!usr->new_message->subject[0])
						sprintf(usr->new_message->subject, "<mail message from %s>", usr->new_message->from);
					strcpy(usr->new_message->from, usr->name);

					if (usr->runtime_flags & RTF_SYSOP)
						usr->new_message->flags |= MSG_FROM_SYSOP;

					enter_recipients(usr, STATE_ENTER_FORWARD_RECIPIENTS);
					Return;
				}
			} else {
				if (usr->new_message != NULL) {
					if (!strcmp(usr->new_message->from, usr->name)
						|| (usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE))) {
						CALL(usr, STATE_FORWARD_ROOM);
						Return;
					} else
						Put(usr, "<red>You are not allowed to forward this message to another room\n");
					break;
				}
			}
			Put(usr, "<red>No message to forward\n");
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

		case 'k':
			Put(usr, "<white>Known rooms\n\n");
			known_rooms(usr);
			Return;

		case 'K':
		case KEY_CTRL('K'):
			Put(usr, "<white>All known rooms\n\n");
			allknown_rooms(usr);
			Return;

		case 'i':
		case 'I':
			Put(usr, "<white>Room info\n\n");
			room_info(usr);
			Return;

		case '>':
			Put(usr, "<white>Friends\n");
			CALL(usr, STATE_FRIENDLIST_PROMPT);
			Return;

		case '<':
			Put(usr, "<white>Enemies\n");
			CALL(usr, STATE_ENEMYLIST_PROMPT);
			Return;

		case KEY_CTRL('L'):
			Put(usr, "<white>Lock\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot lock the terminal\n", PARAM_NAME_GUEST);
				break;
			}
			CALL(usr, STATE_LOCK_PASSWORD);
			Return;

		case 'c':
		case 'C':
			Put(usr, "<red>Press <white><<yellow>Ctrl-C<white>><red> or <white><<yellow>Ctrl-O<white>><red> to access the Config menu\n");
			break;

		case KEY_CTRL('C'):				/* this don't work for some people (?) */
		case KEY_CTRL('O'):				/* so I added Ctrl-O by special request */
			Put(usr, "<white>Config menu\n");
			CALL(usr, STATE_CONFIG_MENU);
			Return;

		case KEY_CTRL('S'):
			if (usr->runtime_flags & RTF_SYSOP) {
				Print(usr, "<white>%s menu\n", PARAM_NAME_SYSOP);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot play %s\n", PARAM_NAME_GUEST, PARAM_NAME_SYSOP);
					break;
				}
				CALL(usr, STATE_SYSOP_MENU);
				Return;
			}
			break;
	}
	if (usr->curr_room->flags & ROOM_CHATROOM) {
		edit_line(usr, EDIT_INIT);
		usr->runtime_flags &= ~(RTF_BUSY | RTF_BUSY_SENDING | RTF_BUSY_MAILING | RTF_CHAT_ESCAPE);
	} else {
		usr->runtime_flags &= ~(RTF_BUSY | RTF_BUSY_SENDING | RTF_BUSY_MAILING);

		if (!(usr->runtime_flags & RTF_HOLD) && usr->held_msgs != NULL)
			Put(usr, "\n<green>The following messages were held while you were busy<yellow>:\n");
	}
	PrintPrompt(usr);
	Return;
}


void PrintPrompt(User *usr) {
	if (usr == NULL)
		return;

	Enter(PrintPrompt);

	if (!(usr->runtime_flags & (RTF_BUSY|RTF_HOLD)) && usr->held_msgs != NULL) {
		if (usr->flags & USR_HOLD_BUSY) {
			CALL(usr, STATE_HELD_HISTORY_PROMPT);
			Return;
		} else
			spew_BufferedMsg(usr);
	}
	if (!(usr->runtime_flags & RTF_BUSY)) {

/* print a short prompt for chatrooms */

		if (usr->curr_room == NULL || (usr->curr_room->flags & ROOM_CHATROOM)) {
			StringList *sl;

/* spool the messages we didn't get while we were busy */

			for(sl = usr->chat_history; sl != NULL; sl = sl->next)
				Print(usr, "%s\n", sl->str);

			listdestroy_StringList(usr->chat_history);
			usr->chat_history = NULL;

			if (usr->runtime_flags & RTF_SYSOP)
				Put(usr, "\n<white># ");
			else
				Put(usr, "\n<white>> ");
		} else {
			char roomname[MAX_LINE], name_buf[MAX_NAME+3];

			if (usr->curr_room == usr->mail)
				sprintf(roomname, "%s Mail", name_with_s(usr, usr->name, name_buf));
			else
				strcpy(roomname, usr->curr_room->name);

/* print a long prompt with msg number when reading messages */

			if (usr->curr_msg >= 0) {
				int remaining = -1;
				char num_buf[25];

				remaining = usr->curr_room->msg_idx - 1 - usr->curr_msg;

				if (usr->flags & USR_ROOMNUMBERS)
					Print(usr, "\n<white>[%u <yellow>%s<white>]<green> msg #%s (%d remaining) <white>%c ",
						usr->curr_room->number, roomname, print_number(usr, usr->curr_room->msgs[usr->curr_msg], num_buf), remaining, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
				else
					Print(usr, "\n<white>[<yellow>%s<white>]<green> msg #%s (%d remaining) <white>%c ",
						roomname, print_number(usr, usr->curr_room->msgs[usr->curr_msg], num_buf), remaining, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
			} else {
				destroy_Message(usr->message);
				usr->message = NULL;

/* print a prompt with roomname */

				if (usr->flags & USR_ROOMNUMBERS)
					Print(usr, "\n<white>%u <yellow>%s<white>%c ", usr->curr_room->number, roomname, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
				else
					Print(usr, "\n<yellow>%s<white>%c ", roomname, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
			}
		}
	}
	Return;
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
		Put(usr, "<green>Enter recipient<yellow>: ");
	else {
		if (usr->recipients->next == NULL)
			Print(usr, "<green>Enter recipient <white>[<yellow>%s<white>]:<yellow> ", usr->recipients->str);
		else
			Put(usr, "<green>Enter recipient <white>[<green><many><white>]:<yellow> ");
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
		Put(usr, "<green>Enter name<yellow>: ");
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
		if (u->runtime_flags & RTF_LOCKED)
			Print(usr, "<yellow>%s<green> is away from the terminal for a while\n", u->name);
		else {
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
						if (PARAM_HAVE_HOLD && (u->runtime_flags & RTF_HOLD))
							Print(usr, "<yellow>%s<green> has put messages on hold\n", u->name);
						else
							Print(usr, "<yellow>%s<green> is busy\n", u->name);
				}
			} else
				if (PARAM_HAVE_HOLD && (u->runtime_flags & RTF_HOLD))
					Print(usr, "<yellow>%s<green> has put messages on hold\n", u->name);
				else
					Print(usr, "<yellow>%s<green> is not busy\n", u->name);

/*
	in case a user is idling, print it
	(hardcoded) default is after 2 minutes
*/
			tdiff = (unsigned long)rtc - (unsigned long)u->idle_time;
			if (tdiff >= 120UL) {
				char total_buf[MAX_LINE];

				Print(usr, "<yellow>%s<green> is idle for %s\n", u->name, print_total_time(usr, tdiff, total_buf));
			}
		}
/*
		if (in_StringList(u->friends, usr->name) != NULL)
			Print(usr, "You are on <yellow>%s<green> friend list\n", name_with_s(usr, u->name, name_buf));
		if (in_StringList(u->enemies, usr->name) != NULL)
			Print(usr, "<red>You are on <yellow>%s<red> enemy list\n", name_with_s(usr, u->name, name_buf));
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
					Print(usr, "<yellow>%s is<white>: <yellow>%s\n", PARAM_NAME_SYSOP, su_passwd->key);
				else {
					KVPair *su;

					Print(usr, "<yellow>%ss are<white>: ", PARAM_NAME_SYSOP);
					for(su = su_passwd; su != NULL && su->next != NULL; su = su->next)
						Print(usr, "<yellow>%s<white>, ", su->key);
					Print(usr, "<yellow>%s\n", su->key);
				}
			}
			RET(usr);
			Return;
		}
		if (is_guest(usr->edit_buf)) {
			Print(usr, "<green>The <yellow>%s<green> user is a visitor from far away\n", PARAM_NAME_GUEST);

			if ((u = is_online(usr->edit_buf)) != NULL) {
				Print(usr, "<green>Online for <cyan>%s", print_total_time(usr, (unsigned long)rtc - (unsigned long)u->login_time, total_buf));
				if (u == usr || (usr->runtime_flags & RTF_SYSOP)) {
					if (usr->runtime_flags & RTF_SYSOP)
						Print(usr, "<green>From host: <yellow>%s <white>[%s]\n", u->conn->hostname, u->conn->ipnum);
					else
						Print(usr, "<green>From host: <yellow>%s\n", u->conn->hostname);
				}
				if ((p = HostMap_desc(u->conn->ipnum)) != NULL)
					Print(usr, "<yellow>%s<green> is connected from <yellow>%s\n", usr->edit_buf, p);

				Put(usr, "\n");
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
			listdestroy_StringList(usr->recipients);		/* place entered name in history */
			usr->recipients = new_StringList(usr->edit_buf);
		}
/* make the profile */
		listdestroy_StringList(usr->more_text);
		usr->more_text = NULL;

		usr->more_text = add_String(&usr->more_text, "<white>%s", u->name);

		visible = 1;
		hidden = "";
		if (((u->flags & USR_HIDE_ADDRESS) && in_StringList(u->friends, usr->name) == NULL)
			|| ((u->flags & USR_HIDE_INFO) && in_StringList(u->enemies, usr->name) != NULL)) {
			if ((usr->runtime_flags & RTF_SYSOP) || u == usr)
				hidden = "<white>(hidden) ";
			else
				visible = 0;
		}
		if (u->real_name != NULL && u->real_name[0] && visible)
			usr->more_text = add_String(&usr->more_text, "%s<yellow>%s", hidden, u->real_name);

		if (u->street != NULL && u->street[0] && visible)
			usr->more_text = add_String(&usr->more_text, "%s<yellow>%s", hidden, u->street);

		if (u->zipcode != NULL && u->zipcode[0] && visible) {
			if (u->city != NULL && u->city[0])
				usr->more_text = add_String(&usr->more_text, "%s<yellow>%s %s", hidden, u->zipcode, u->city);
			else
				usr->more_text = add_String(&usr->more_text, "%s<yellow>%s", hidden, u->zipcode);
		} else
			if (u->city != NULL && u->city[0] && visible)
				usr->more_text = add_String(&usr->more_text, "%s<yellow>%s", hidden, u->city);

		if (u->state != NULL && u->state[0] && visible) {
			if (u->country != NULL && u->country[0])
				usr->more_text = add_String(&usr->more_text, "%s<yellow>%s, %s", hidden, u->state, u->country);
			else
				usr->more_text = add_String(&usr->more_text, "%s<yellow>%s", hidden, u->state);
		} else
			if (u->country != NULL && u->country[0] && visible)
				usr->more_text = add_String(&usr->more_text, "%s<yellow>%s", hidden, u->country);

		if (u->phone != NULL && u->phone[0] && visible)
			usr->more_text = add_String(&usr->more_text, "%s<green>Phone: <yellow>%s", hidden, u->phone);

		if (u->email != NULL && u->email[0] && visible)
			usr->more_text = add_String(&usr->more_text, "%s<green>E-mail: <cyan>%s", hidden, u->email);

		if (u->www != NULL && u->www[0] && visible)
			usr->more_text = add_String(&usr->more_text, "%s<green>WWW: <cyan>%s", hidden, u->www);

		if (u->doing != NULL && u->doing[0]) {
			if (allocated)
				usr->more_text = add_String(&usr->more_text, "<green>Was doing: <yellow>%s <cyan>%s", u->name, u->doing);
			else
				usr->more_text = add_String(&usr->more_text, "<green>Doing: <yellow>%s <cyan>%s", u->name, u->doing);
		}
		if (allocated) {
			char date_buf[MAX_LINE], online_for[MAX_LINE+10];

			if (u->last_online_time > 0UL) {
				int l;

				l = sprintf(online_for, "%c for %c", color_by_name("green"), color_by_name("yellow"));
				print_total_time(usr, u->last_online_time, online_for+l);
			} else
				online_for[0] = 0;

			usr->more_text = add_String(&usr->more_text, "<green>Last online: <cyan>%s%s", print_date(usr, (time_t)u->last_logout, date_buf), online_for);
			if (usr->runtime_flags & RTF_SYSOP)
				usr->more_text = add_String(&usr->more_text, "<green>From host: <yellow>%s <white>[%s]", u->conn->hostname, u->tmpbuf[TMP_FROM_IP]);

			if ((p = HostMap_desc(u->conn->ipnum)) != NULL)
				usr->more_text = add_String(&usr->more_text, "<yellow>%s<green> was connected from <yellow>%s", u->name, p);
		} else {
/*
	display for how long someone is online
	by Richard of MatrixBBS
*/
			usr->more_text = add_String(&usr->more_text, "<green>Online for <cyan>%s", print_total_time(usr, rtc - u->login_time, total_buf));
			if (u == usr || (usr->runtime_flags & RTF_SYSOP)) {
				if (usr->runtime_flags & RTF_SYSOP)
					usr->more_text = add_String(&usr->more_text, "<green>From host: <yellow>%s <white>[%s]", u->conn->hostname, u->conn->ipnum);
				else
					usr->more_text = add_String(&usr->more_text, "<green>From host: <yellow>%s", u->conn->hostname);
			}
			if ((p = HostMap_desc(u->conn->ipnum)) != NULL)
				usr->more_text = add_String(&usr->more_text, "<yellow>%s<green> is connected from <yellow>%s", u->name, p);
		}
		if (!allocated)
			update_stats(u);
		usr->more_text = add_String(&usr->more_text, "<green>Total online time: <yellow>%s", print_total_time(usr, u->total_time, total_buf));

		if (u->flags & USR_X_DISABLED)
			usr->more_text = add_String(&usr->more_text, "<red>%s has message reception turned off", u->name);

		if (in_StringList(u->friends, usr->name) != NULL)
			usr->more_text = add_String(&usr->more_text, "<green>You are on <yellow>%s<green> friend list", name_with_s(usr, u->name, total_buf));

		visible = 1;
		if (!(usr->runtime_flags & RTF_SYSOP) && usr != u
			&& (u->flags & USR_HIDE_INFO) && in_StringList(u->enemies, usr->name) != NULL)
			visible = 0;

		if (visible && in_StringList(u->enemies, usr->name) != NULL)
			usr->more_text = add_String(&usr->more_text, "<yellow>%s<red> does not wish to receive any messages from you", u->name);

		if (visible && u->info != NULL) {
			usr->more_text = add_StringList(&usr->more_text, new_StringList("<green>"));
			if ((usr->more_text->next = copy_StringList(u->info)) != NULL)
				usr->more_text->next->prev = usr->more_text;
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
		Put(usr, "\n");

		POP(usr);
		read_more(usr);
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
	this doesn't seem the right place to do this, but we can't do it
	in recvMsg() because that function doesn't know whether this is a
	multi-X or not
	So the message that you send to yourself won't be received... but
	you do get a copy in your X history buffer
	Perhaps this design should be changed and turned the other way around...
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
				usr->send_msg->flags &= ~BUFMSG_SEEN;	/* this is for the recipient */
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

		Put(usr, "<cyan>Do you wish to <yellow>Mail<white>><cyan> the message? <white>(<cyan>Y<white>/<cyan>n<white>): ");
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
		if ((msg->msg = new_StringList(usr->edit_buf)) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		strcpy(msg->from, usr->name);
		msg->mtime = rtc;

		msg->flags |= (BUFMSG_EMOTE | BUFMSG_SEEN);
		if (usr->runtime_flags & RTF_SYSOP)
			msg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, msg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = copy_BufferedMsg(msg);

/* update stats */
		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->esent++;
			if (usr->esent > stats.esent) {
				stats.esent = usr->esent;
				strcpy(stats.most_esent, usr->name);
			}
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
		listdestroy_StringList(usr->more_text);
		usr->more_text = NULL;

		Put(usr, "<red>Message not sent\n");
		usr->runtime_flags &= ~RTF_BUSY_SENDING;
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		BufferedMsg *xmsg;

		usr->runtime_flags &= ~RTF_BUSY_SENDING;

		if (!usr->edit_buf[0] && usr->more_text == NULL) {
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
		if ((xmsg->msg = copy_StringList(usr->more_text)) == NULL) {
			destroy_BufferedMsg(xmsg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		strcpy(xmsg->from, usr->name);
		xmsg->mtime = rtc;

		xmsg->flags |= (BUFMSG_XMSG | BUFMSG_SEEN);
		if (usr->runtime_flags & RTF_SYSOP)
			xmsg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, xmsg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = copy_BufferedMsg(xmsg);

/* update stats */
		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->xsent++;
			if (usr->xsent > stats.xsent) {
				stats.xsent = usr->xsent;
				strcpy(stats.most_xsent, usr->name);
			}
		}
		JMP(usr, LOOP_SEND_MSG);
	}
	Return;
}

void state_choose_feeling(User *usr, char c) {
StringList *sl;
int r;

	if (usr == NULL)
		return;

	Enter(state_choose_feeling);

	if (c == INIT_STATE) {
		usr->runtime_flags |= (RTF_BUSY | RTF_BUSY_SENDING);

		make_feelings_screen(usr->display->term_width);
		for(sl = feelings_screen; sl != NULL; sl = sl->next)
			Print(usr, "%s\n", sl->str);
		Put(usr, "\n<green>Feeling<yellow>: ");
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
		if ((msg->msg = copy_StringList((StringList *)KVPair_getpointer(f))) == NULL) {
			destroy_BufferedMsg(msg);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		strcpy(msg->from, usr->name);
		msg->mtime = rtc;

		msg->flags |= (BUFMSG_FEELING | BUFMSG_SEEN);
		if (usr->runtime_flags & RTF_SYSOP)
			msg->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, msg);

		destroy_BufferedMsg(usr->send_msg);
		usr->send_msg = copy_BufferedMsg(msg);

/* update stats */
		if (usr->recipients != NULL
			&& !(usr->recipients->next == NULL && !strcmp(usr->name, usr->recipients->str))) {
			usr->fsent++;
			if (usr->fsent > stats.fsent) {
				stats.fsent = usr->fsent;
				strcpy(stats.most_fsent, usr->name);
			}
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

		if (!usr->edit_buf[0] && usr->more_text == NULL) {
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
		if ((question->msg = copy_StringList(usr->more_text)) == NULL) {
			destroy_BufferedMsg(question);
			Perror(usr, "Out of memory");
			RET(usr);
			Return;
		}
		strcpy(question->from, usr->name);
		question->mtime = rtc;

		question->flags |= BUFMSG_QUESTION;
		if (usr->runtime_flags & RTF_SYSOP)
			question->flags |= BUFMSG_SYSOP;

		add_BufferedMsg(&usr->history, question);

		recvMsg(u, usr, question);				/* the question is asked! */

		question->flags |= BUFMSG_SEEN;
		RET(usr);
	}
	Return;
}


void state_jump_room(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_jump_room);

	if (c == INIT_STATE) {
		edit_roomname(usr, EDIT_INIT);
		Put(usr, "<green>Enter room name<yellow>: ");
		Return;
	}
	r = edit_roomname(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Room *r;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if ((r = find_abbrevRoom(usr, usr->edit_buf)) == NULL) {
			Put(usr, "<red>No such room\n");
			RET(usr);
			Return;
		}

/* already there */

		if (r == usr->curr_room) {
			Put(usr, "<white>Bounce! Bounce!\n");
			RET(usr);
			Return;
		}
		if (!(usr->runtime_flags & RTF_SYSOP)) {
			switch(room_access(r, usr->name)) {
				case ACCESS_INVITE_ONLY:
					Put(usr, "<red>That room is invite-only, and you have not been invited\n");
					unload_Room(r);
					RET(usr);
					Return;

				case ACCESS_KICKED:
					Put(usr, "<red>You have been kicked from that room\n");
					unload_Room(r);
					RET(usr);
					Return;

				case ACCESS_INVITED:
					if (r != usr->mail)
						Put(usr, "<yellow>You are invited in this room\n");
			}
		}
		POP(usr);
		goto_room(usr, r);
	}
	Return;
}

void state_zap_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_zap_prompt);

	if (c == INIT_STATE) {
		if (usr->curr_room == NULL) {
			Perror(usr, "You have no current room, so you can't Zap it");
			usr->curr_room = Lobby_room;
			usr->runtime_flags &= ~RTF_ROOMAIDE;
			RET(usr);
			Return;
		}
		if (usr->curr_room == usr->mail) {
			Put(usr, "<red>You can't Zap your own <yellow>Mail<white>><red> room\n");
			RET(usr);
			Return;
		}
		if (usr->curr_room->flags & ROOM_NOZAP) {
			Put(usr, "<red>This room cannot be Zapped\n");
			RET(usr);
			Return;
		}
		if (in_StringList(usr->curr_room->room_aides, usr->name) != NULL) {
			Print(usr, "<red>... but you are %s here!\n", PARAM_NAME_ROOMAIDE);
			RET(usr);
			Return;
		}
		Put(usr, "<cyan>Are you sure? <white>(<cyan>Y<white>/<cyan>n<white>): ");
		usr->runtime_flags |= RTF_BUSY;
		Return;
	}
	r = yesno(usr, c, 'Y');
	if (r == YESNO_YES) {
		Room *room;
		Joined *j;

		if ((j = in_Joined(usr->rooms, usr->curr_room->number)) != NULL)
			j->zapped = 1;

		room = next_unread_room(usr);
		if (room != usr->curr_room) {
			POP(usr);
			goto_room(usr, room);
			Return;
		}
	}
	if (r == YESNO_UNDEF) {
		CURRENT_STATE(usr);
		Return;
	}
	RET(usr);
	Return;
}

void state_zapall_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_zapall_prompt);

	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? <white>(<cyan>y<white>/<cyan>N<white>): ");
		usr->runtime_flags |= RTF_BUSY;
		Return;
	}
	r = yesno(usr, c, 'N');
	if (r == YESNO_YES) {
		Joined *j;
		Room *room;

		for(room = AllRooms; room != NULL; room = room->next) {
			if (room->number >= SPECIAL_ROOMS
				&& !(room->flags & ROOM_NOZAP)
				&& in_StringList(room->room_aides, usr->name) == NULL) {

				if ((j = in_Joined(usr->rooms, room->number)) != NULL)
					j->zapped = 1;
				else {
					if ((j = new_Joined()) != NULL) {
						j->number = room->number;
						j->generation = room->generation;
						j->zapped = 1;
						add_Joined(&usr->rooms, j);
					}
				}
			}
		}
		j = in_Joined(usr->rooms, usr->curr_room->number);
		if (j == NULL || j->zapped) {
			POP(usr);
			goto_room(usr, Lobby_room);
			Return;
		}
	}
	if (r == YESNO_UNDEF) {
		CURRENT_STATE(usr);
		Return;
	}
	RET(usr);
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

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

/* make the who list in usr->more_text */
	if (format & WHO_LIST_LONG) {
		total = long_who_list(usr, proot);
		who_list_header(usr, total, format);
	} else {
		total = short_who_list(usr, proot);
		who_list_header(usr, total, format);
	}
	listdestroy_PList(proot);		/* destroy temp list */

	read_more(usr);					/* display the who list */
	Return;
}

/*
	construct a long format who list
*/
int long_who_list(User *usr, PList *pl) {
StringList *s = NULL;
int total = 0, hrs, mins, l, c, width;
unsigned long time_now;
time_t t;
char buf[PRINT_BUF], buf2[PRINT_BUF], col, stat;
User *u;

	if (usr == NULL)
		return 0;

	Enter(long_who_list);

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	time_now = rtc;

	while(pl != NULL) {
		u = (User *)pl->p;

		pl = pl->next;

		if (u == NULL)
			continue;

		total++;

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

		if (u->flags & USR_X_DISABLED)
			stat = '*';

		if (u->runtime_flags & RTF_LOCKED)
			stat = '#';
/*
	you can see it when someone is X-ing or Mail>-ing you
	this is after a (good) idea by Richard of MatrixBBS, who implemented
	lots of flags more on there
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
		s = add_String(&s, "%s <white>%c <yellow>%2d<white>:<yellow>%02d", buf, stat, hrs, mins);
	}
	usr->more_text = rewind_StringList(s);
	Return total;
}

/*
	construct a short format who list
*/
int short_who_list(User *usr, PList *pl) {
StringList *s = NULL, *sl;
int i, j, total, buflen = 0, cols, rows;
char buf[PRINT_BUF], col, stat;
User *u;
PList *pl_cols[16];

	if (pl == NULL || pl->p == NULL)
		return 0;

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	total = list_Count(pl);

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
		if ((sl = new_StringList(buf)) == NULL)
			break;

		s = add_StringList(&s, sl);
	}
	usr->more_text = rewind_StringList(s);
	Return total;
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
			sl = add_String(&sl, "<green>You are the only one in <yellow>%s<white>>", usr->curr_room->name);
		else
			sl = add_String(&sl, "<magenta>There %s <yellow>%d<magenta> user%s in <yellow>%s<white>><magenta> at <yellow>%02d<white>:<yellow>%02d",
				(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
				usr->curr_room->name, tm->tm_hour, tm->tm_min);
	} else {
		if (total == 1)
			sl = add_String(&sl, "<green>You are the only one online right now");
		else
			sl = add_String(&sl, "<magenta>There %s <yellow>%d<magenta> user%s online at <yellow>%02d<white>:<yellow>%02d",
				(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
				tm->tm_hour, tm->tm_min);
	}
/* draw a line across the full screen width */
	if (drawline)
		sl = add_StringList(&sl, new_StringList("<white><hline>-"));

/* prepend header */

	if (sl != NULL)
		sl->next = usr->more_text;
	if (usr->more_text != NULL)
		usr->more_text->prev = sl;
	else
		usr->more_text = sl;
	usr->more_text = rewind_StringList(usr->more_text);
	Return;
}


void print_known_room(User *usr, Room *r) {
Joined *j;
char status[2], buf[MAX_LINE*2], buf2[MAX_LINE*3];
int read_it = 1, idx;

	if (usr == NULL || r == NULL)
		return;

	Enter(print_known_room);

	status[0] = (char)color_by_name("white");
	status[1] = ' ';

	read_it = 1;

	if ((j = in_Joined(usr->rooms, r->number)) != NULL && j->zapped) {
		status[0] = (char)color_by_name("red");
		status[1] = 'Z';
		read_it = 0;
	}
	if (read_it && in_StringList(r->kicked, usr->name) != NULL) {
		status[0] = (char)color_by_name("red");
		status[1] = 'K';
		read_it = 0;
	}
	if (read_it && (r->flags & ROOM_ANONYMOUS)) {
		status[0] = (char)color_by_name("green");
		status[1] = 'A';
	}
	if (read_it && (r->flags & ROOM_HIDDEN)) {
		status[0] = (char)color_by_name("red");
		status[1] = 'H';
		read_it = 0;
	}
	if (read_it && (r->flags & ROOM_INVITE_ONLY)) {
		status[1] = 'I';
		if (in_StringList(r->invited, usr->name) != NULL)
			status[0] = (char)color_by_name("green");
		else {
			status[0] = (char)color_by_name("red");
			read_it = 0;
		}
	}
	if (read_it && (r->flags & ROOM_CHATROOM)) {
		status[0] = (char)color_by_name("yellow");
		status[1] = 'C';
		read_it = 0;
	}
	if (read_it) {
		if (j != NULL) {
			idx = r->msg_idx-1;
			if (idx > 0 && r->msgs != NULL && r->msgs[idx] > j->last_read) {
				status[0] = (char)color_by_name("cyan");
				status[1] = '*';
			}
		} else {
			if (r->msgs != NULL && r->msg_idx > 0) {	/* there are messages in this room */
				status[0] = (char)color_by_name("cyan");
				status[1] = '*';
			}
		}
	}
	sprintf(buf, "%c%c%c %3u %c%s%c>",
		status[0], status[1],
		(char)color_by_name("white"), r->number,
		(char)color_by_name("yellow"), r->name,
		(char)color_by_name("white"));

	sprintf(buf2, "%-50s", buf);

/* add room aides to the line */
	if (r->room_aides == NULL) {
		if (r->number >= SPECIAL_ROOMS)
			sprintf(buf2+strlen(buf2), "%c(no %s)", (char)color_by_name("red"), PARAM_NAME_ROOMAIDE);
	} else {
		StringList *sl;
		int l;

		l = strlen(buf2);
		for(sl = r->room_aides; sl != NULL && l < MAX_LINE; sl = sl->next)
			l += sprintf(buf2+l, "%c%s%c, ", (char)color_by_name("cyan"), sl->str, (char)color_by_name("white"));

		l -= 3;
		buf2[l] = 0;
		if (l > MAX_LINE) {				/* display as '...' */
			buf2[MAX_LINE-1] = 0;
			buf2[MAX_LINE-2] = buf2[MAX_LINE-3] = buf2[MAX_LINE-4] = '.';
			buf2[MAX_LINE-5] = (char)color_by_name("white");
		}
	}
	buf2[MAX_LINE-1] = 0;
	usr->more_text = add_String(&usr->more_text, buf2);
	Return;
}

void known_rooms(User *usr) {
Room *r, *r_next;
Joined *j;
char *category = NULL;

	Enter(known_rooms);

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	for(r = AllRooms; r != NULL; r = r_next) {
		r_next = r->next;

/* first three rooms are special */
		if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
			if ((r = find_Roombynumber(usr, r->number)) == NULL)
				continue;

		if (in_StringList(r->room_aides, usr->name) == NULL) {
			if ((j = in_Joined(usr->rooms, r->number)) != NULL && j->zapped)
				continue;
/*
			if (r->flags & ROOM_HIDDEN)
				continue;
*/
			if (in_StringList(r->kicked, usr->name) != NULL)
				continue;

			if ((r->flags & ROOM_INVITE_ONLY) && in_StringList(r->invited, usr->name) == NULL)
				continue;
		}
		if (PARAM_HAVE_CATEGORY && ((category == NULL && r->category != NULL) || (category != NULL && r->category != NULL && strcmp(category, r->category)))) {
			usr->more_text = add_String(&usr->more_text, "");
			usr->more_text = add_String(&usr->more_text, "<white>[<cyan>%s<white>]", r->category);
			category = r->category;
		}
		print_known_room(usr, r);
	}
/*
	throw away the demand loaded room because we're not using it anyway
*/
	if ((r = find_Roombynumber(usr, 2)) != NULL)
		unload_Room(r);

	read_more(usr);
	Return;
}

void allknown_rooms(User *usr) {
Room *r, *r_next;
Joined *j;
char *category = NULL;

	Enter(allknown_rooms);

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	for(r = AllRooms; r != NULL; r = r_next) {
		r_next = r->next;

		if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
			if ((r = find_Roombynumber(usr, r->number)) == NULL)
				continue;

		if ((r->flags & ROOM_HIDDEN)
			&& (j = in_Joined(usr->rooms, r->number)) == NULL
			&& !(usr->runtime_flags & RTF_SYSOP)
			&& in_StringList(r->room_aides, usr->name) == NULL)
			continue;

		if (PARAM_HAVE_CATEGORY && ((category == NULL && r->category != NULL) || (category != NULL && r->category != NULL && strcmp(category, r->category)))) {
			usr->more_text = add_String(&usr->more_text, "");
			usr->more_text = add_String(&usr->more_text, "<white>[<cyan>%s<white>]", r->category);
			category = r->category;
		}
		print_known_room(usr, r);
	}
	if ((r = find_Roombynumber(usr, 2)) != NULL)
		unload_Room(r);

	read_more(usr);
	Return;
}


void room_info(User *usr) {
char buf[MAX_LINE*3];
Joined *j;

	if (usr == NULL)
		return;

	Enter(room_info);

	if (usr->curr_room == NULL) {
		Perror(usr, "You have no current room");
		goto_room(usr, Lobby_room);
		Return;
	}
	if ((j = in_Joined(usr->rooms, usr->curr_room->number)) != NULL)
		j->roominfo_read = usr->curr_room->roominfo_changed;		/* now we've read it */

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	usr->more_text = add_String(&usr->more_text, "<white>Room info of <yellow>%s<white>> (room #%u)",
		usr->curr_room->name, usr->curr_room->number);

	if (usr->curr_room->generation) {
		char date_buf[80];

		usr->more_text = add_String(&usr->more_text, "<green>Created on <yellow>%s", print_date(usr, usr->curr_room->generation, date_buf));
	}
	*buf = 0;
	if (usr->curr_room->flags & ROOM_READONLY)
		strcat(buf, ", read-only");
	if (usr->curr_room->flags & ROOM_INVITE_ONLY)
		strcat(buf, ", invite-only");
	if (usr->curr_room->flags & ROOM_ANONYMOUS)
		strcat(buf, ", anonymous-optional");
	if (usr->curr_room->flags & ROOM_NOZAP)
		strcat(buf, ", not zappable");
	if ((usr->curr_room->flags & ROOM_HIDDEN) && (usr->runtime_flags & RTF_SYSOP))
		strcat(buf, ", hidden");
	if (*buf) {
		usr->more_text = add_StringList(&usr->more_text, new_StringList(""));
		usr->more_text = add_String(&usr->more_text, "<green>This room is %s", buf+2);
	}
	if (usr->curr_room->room_aides != NULL) {
		if (usr->curr_room->room_aides->next != NULL) {
			StringList *sl;
			int l, dl;					/* l = strlen, dl = display length */

			sprintf(buf, "<cyan>%ss are<white>: ", PARAM_NAME_ROOMAIDE);
			l = strlen(buf);
			dl = l - 2;
			for(sl = usr->curr_room->room_aides; sl != NULL && sl->next != NULL; sl = sl->next) {
				if ((dl + strlen(sl->str)+2) < MAX_LINE)
					l += sprintf(buf+l, "<yellow>%s<green>, ", sl->str);
				else {
					usr->more_text = add_StringList(&usr->more_text, new_StringList(buf));
					l = sprintf(buf, "<yellow>%s<green>, ", sl->str);
				}
				dl = l - 2;
			}
			usr->more_text = add_String(&usr->more_text, "%s<yellow>%s<green>", buf, sl->str);
		} else
			usr->more_text = add_String(&usr->more_text, "<cyan>%s is<white>: <cyan>%s", PARAM_NAME_ROOMAIDE, usr->curr_room->room_aides->str);
	}
	if (PARAM_HAVE_CATEGORY && usr->curr_room->category && usr->curr_room->category[0]) {
		if (!in_Category(usr->curr_room->category)) {
			Free(usr->curr_room->category);
			usr->curr_room->category = NULL;
		} else
			usr->more_text = add_String(&usr->more_text, "<cyan>Category<white>:<yellow> %s", usr->curr_room->category);
	}
	usr->more_text = add_String(&usr->more_text, "<green>");

	if (usr->curr_room == NULL || usr->curr_room->info == NULL)
		usr->more_text = add_String(&usr->more_text, "<red>This room has no room info");
	else {
		if ((usr->more_text->next = copy_StringList(usr->curr_room->info)) == NULL) {
			Perror(usr, "Out of memory ; can't display room info");
		} else
			usr->more_text->next->prev = usr->more_text;
	}
	read_more(usr);
	Return;
}


void reply_x(User *usr, int all) {
BufferedMsg *m;
StringList *sl;

	if (usr == NULL)
		return;

	Enter(reply_x);

	m = unwind_BufferedMsg(usr->history);
	while(m != NULL) {
		if ((m->flags & (BUFMSG_XMSG | BUFMSG_EMOTE | BUFMSG_FEELING))
			&& strcmp(m->from, usr->name))
			break;
		m = m->prev;
	}
	if (m == NULL) {
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

		if (flags & BUFMSG_EMOTE) {
			CALL(usr, STATE_EDIT_EMOTE);
		} else {
			CALL(usr, STATE_EDIT_X);
		}
	} else {
/* replying to <many>, edit the recipient list */
		Print(usr, "<green>Replying to%s", print_many(usr, many_buf));

		if (flags & BUFMSG_EMOTE) {
			PUSH(usr, STATE_EMOTE_PROMPT);
		} else {
			PUSH(usr, STATE_X_PROMPT);
		}
		edit_recipients(usr, INIT_STATE, NULL);
	}
	Return;
}

void state_lock_password(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_lock_password);

	if (c == INIT_STATE) {
		if (usr->flags & (USR_ANSI | USR_BOLD))		/* clear screen */
			Print(usr, "%c[1;1H%c[2J", KEY_ESC, KEY_ESC);

		Print(usr, "\n<white>%s terminal locked\n"
			"<red>Enter password to unlock: ", PARAM_BBS_NAME);

		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
		usr->runtime_flags |= (RTF_BUSY | RTF_LOCKED);

		if (usr->idle_timer != NULL)
			usr->idle_timer->sleeptime = usr->idle_timer->maxtime = PARAM_LOCK_TIMEOUT * SECS_IN_MIN;

		notify_locked(usr);
		Return;
	}
	if (edit_password(usr, c) == EDIT_RETURN) {
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
			Put(usr, "<yellow>Unlocked\n");

			usr->runtime_flags &= ~(RTF_BUSY | RTF_LOCKED);

			if (usr->idle_timer != NULL)
				usr->idle_timer->sleeptime = usr->idle_timer->maxtime = PARAM_IDLE_TIMEOUT * SECS_IN_MIN;

			notify_unlocked(usr);
			RET(usr);
		} else
			Put(usr, "Wrong password\n\nEnter password to unlock: ");

		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
	}
	Return;
}

void state_chatroom(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_chatroom);

	switch(c) {
		case INIT_STATE:
			break;

		case KEY_RETURN:
			Put(usr, "\n");
			break;

		case 'q':
		case 'Q':
			RET(usr);
			Return;
	}
	usr->runtime_flags &= ~RTF_BUSY;

	spew_BufferedMsg(usr);

	Put(usr, "\n<white>> ");
	Return;
}

/*
	state_boss() is for when the boss walks in
*/
void state_boss(User *usr, char c) {
int r;

	Enter(state_boss);

	if (c == INIT_STATE) {
		StringList *boss;

		edit_line(usr, EDIT_INIT);

		if (PARAM_HAVE_HOLD) {
			if (usr->runtime_flags & RTF_HOLD)
				usr->runtime_flags |= RTF_WAS_HOLDING;
			else
				usr->runtime_flags |= RTF_HOLD;
		}
		if (usr->flags & USR_HELPING_HAND) {		/* this is inconvenient right now */
			usr->flags &= ~USR_HELPING_HAND;
			usr->runtime_flags |= RTF_WAS_HH;
		}
		if (usr->flags & (USR_ANSI | USR_BOLD))		/* clear screen */
			Print(usr, "%c[1;1H%c[2J%c[0m", KEY_ESC, KEY_ESC, KEY_ESC);
		else
			Put(usr, "\n");

		if ((boss = load_screen(PARAM_BOSS_SCREEN)) != NULL) {
			StringList *sl;

			for(sl = boss; sl != NULL; sl = sl->next)
				Print(usr, "%s\n", sl->str);

			listdestroy_StringList(boss);
			boss = NULL;
		}
		Put(usr, "$ ");
	}
	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		strcpy(usr->edit_buf, "exit");
		r = EDIT_RETURN;
	}
	if (r == EDIT_RETURN) {
		char buf[MAX_LINE];

		if (!usr->edit_buf[0]) {
			Put(usr, "$ ");
			edit_line(usr, EDIT_INIT);
			Return;
		}
		if (!strcmp(usr->edit_buf, "date")) {
			Print(usr, "%s %s\n\n$ ", print_date(usr, (time_t)0UL, buf), name_Timezone(usr->tz));
			edit_line(usr, EDIT_INIT);
			Return;
		}
		if (!strcmp(usr->edit_buf, "uname")) {
			Print(usr, "%s %s\n$ ", PARAM_BBS_NAME, print_copyright((usr->runtime_flags & RTF_SYSOP) ? FULL : SHORT, NULL, buf));
			edit_line(usr, EDIT_INIT);
			Return;
		}
		if (!strcmp(usr->edit_buf, "uptime")) {
			Print(usr, "last booted on %s\n", print_date(usr, stats.uptime, buf));
			Print(usr, "uptime is %s\n\n$ ", print_total_time(usr, rtc - stats.uptime, buf));
			edit_line(usr, EDIT_INIT);
			Return;
		}
		if (!strcmp(usr->edit_buf, "exit")) {
			if (usr->runtime_flags & RTF_WAS_HOLDING) {
/* was on hold, so still on hold now, but not busy anymore */
				usr->runtime_flags &= ~(RTF_WAS_HOLDING|RTF_BUSY);
				RET(usr);
				Return;
			}
/* no longer on hold */
			if (usr->held_msgs != NULL) {
				JMP(usr, STATE_HELD_HISTORY_PROMPT);
				Return;
			}
			usr->runtime_flags &= ~(RTF_BUSY|RTF_HOLD);
			if (usr->runtime_flags & RTF_WAS_HH) {
				usr->runtime_flags &= ~RTF_WAS_HH;
				usr->flags |= USR_HELPING_HAND;
			}
			RET(usr);
			Return;
		}
		if (usr->cmd_chain == NULL)
			usr->cmd_chain = new_PList(default_cmds);

		if (exec_cmd(usr, usr->edit_buf) < 0)
			Put(usr, "type 'exit' to return\n\n$ ");
		else
			Put(usr, "$ ");

		edit_line(usr, EDIT_INIT);
		Return;
	}
	Return;
}

void online_friends_list(User *usr) {
StringList *sl;
User *u;
struct tm *tm;
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

	total = short_who_list(usr, pl);

	listdestroy_PList(pl);

/* construct header */
	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	sl = new_StringList("");
	sl = add_String(&sl, "<magenta>There %s <yellow>%d<magenta> friend%s online at <yellow>%02d<white>:<yellow>%02d",
		(total == 1) ? "is" : "are", total, (total == 1) ? "" : "s",
		tm->tm_hour, tm->tm_min);

/* prepend header */

	sl->next = usr->more_text;
	if (usr->more_text != NULL)
		usr->more_text->prev = sl;
	else
		usr->more_text = sl;

	read_more(usr);
	Return;
}

/*
	talked-to list, donated by Richard of MatrixBBS
	basically the same as online_friends_list(), but now
	with usr->talked_to
*/
void talked_list(User *usr) {
StringList *sl;
User *u;
struct tm *tm;
PList *pl = NULL;
int total;

	if (usr == NULL)
		return;

	Enter(talked_list);

	if (usr->talked_to == NULL) {
		Put(usr, "<red>You haven't talked to anyone yet\n");
		CURRENT_STATE(usr);
		Return;
	}
	for(sl = usr->talked_to; sl != NULL; sl = sl->next) {
		if ((u = is_online(sl->str)) == NULL)
			continue;
/*
		if (in_StringList(usr->enemies, u->name) != NULL)
			continue;
*/
		pl = add_PList(&pl, new_PList(u));
	}
	if (pl == NULL) {
		Put(usr, "<red>Nobody you talked to is online anymore\n");
		CURRENT_STATE(usr);
		Return;
	}
	pl = rewind_PList(pl);
	pl = sort_PList(pl, (usr->flags & USR_SORT_DESCENDING) ? sort_who_desc_byname : sort_who_asc_byname);

	total = short_who_list(usr, pl);

	listdestroy_PList(pl);

/* construct header */
	tm = user_time(usr, (time_t)0UL);
	if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
		tm->tm_hour -= 12;

	sl = new_StringList("");
	sl = add_String(&sl, "<magenta>There %s <yellow>%d<magenta> %s you talked to online at <yellow>%02d<white>:<yellow>%02d",
		(total == 1) ? "is" : "are", total, (total == 1) ? "person" : "people",
		tm->tm_hour, tm->tm_min);

/* prepend header */

	sl->next = usr->more_text;
	if (usr->more_text != NULL)
		usr->more_text->prev = sl;
	else
		usr->more_text = sl;

	read_more(usr);
	Return;
}

void show_namelist(User *usr, StringList *names) {
StringList *sl;
int i = 0;

	if (usr == NULL || names == NULL)
		return;

	Enter(show_namelist);

	Put(usr, "\n<yellow>");
	for(sl = names; sl != NULL; sl = sl->next) {
		Print(usr, "%-18s ", sl->str);
		if (++i >= 4 && sl->next != NULL) {
			i = 0;
			Put(usr, "\n");
		}
	}
	Put(usr, "\n");
	Return;
}

void print_quicklist(User *usr) {
int i;

	Enter(print_quicklist);

	for(i = 1; i < 5; i++) {
		if (usr->quick[i-1] == NULL)
			Print(usr, "<hotkey>%d <red>%-26s ", i, "<empty>");
		else
			Print(usr, "<hotkey>%d <white>%-26s ", i, usr->quick[i-1]);

		i += 5;
		if (usr->quick[i-1] == NULL)
			Print(usr, "<hotkey>%d <red>%s\n", i, "<empty>");
		else
			Print(usr, "<hotkey>%d <white>%s\n", i, usr->quick[i-1]);
		i -= 5;
	}
	if (usr->quick[i-1] == NULL)
		Print(usr, "<hotkey>%d <red>%-26s ", i, "<empty>");
	else
		Print(usr, "<hotkey>%d <white>%-26s ", i, usr->quick[i-1]);

	if (usr->quick[9] == NULL)
		Print(usr, "<hotkey>%d <red>%s\n", 0, "<empty>");
	else
		Print(usr, "<hotkey>%d <white>%s\n", 0, usr->quick[9]);

	Return;
}

/*
	Stop reading: pretend last message has been read
*/
void stop_reading(User *usr) {
Joined *j;

	if (usr == NULL)
		return;

	Enter(stop_reading);

	if ((j = in_Joined(usr->rooms, usr->curr_room->number)) != NULL) {
		if (usr->curr_room->msgs != NULL && usr->curr_room->msg_idx > 0)
			j->last_read = usr->curr_room->msgs[usr->curr_room->msg_idx - 1];
		else
			j->last_read = 0UL;
	}
	usr->curr_msg = -1;
	Return;
}

/*
	the Jump function: goto another room
*/
void goto_room(User *usr, Room *r) {
Joined *j;

	Enter(goto_room);

	leave_room(usr);

	if (r == NULL) {							/* possible if connection is being closed */
		Return;
	}
	usr->curr_msg = -1;

	listdestroy_StringList(usr->more_text);
	usr->more_text = NULL;

	destroy_Message(usr->message);
	usr->message = NULL;
	destroy_Message(usr->new_message);
	usr->new_message = NULL;

	enter_room(usr, r);

	if ((j = in_Joined(usr->rooms, r->number)) != NULL) {
		j->zapped = 0;
		if (r->generation != j->generation) {	/* room was modified? */
			j->generation = r->generation;
			j->last_read = 0UL;
		}
	} else {
		if ((j = new_Joined()) == NULL) {		/* join room when jumped to */
			Perror(usr, "Out of memory");
		} else {
			j->number = r->number;
			j->generation = r->generation;
			add_Joined(&usr->rooms, j);
		}
	}
	if (j != NULL && r->info != NULL) {
		if (!j->roominfo_read) {
			Put(usr, "\n");
			room_info(usr);			/* first time here ; read room info */
			Return;
		}
		if (r->roominfo_changed != j->roominfo_read)
			Put(usr, "\n<red>The room info has been updated. Hit <hotkey>i to read it\n");
	}
	CURRENT_STATE(usr);
	Return;
}

void enter_room(User *usr, Room *r) {
	if (r == NULL)
		return;

	usr->curr_room = r;
/* if not RA here, reset the flag */
	if ((usr->runtime_flags & RTF_ROOMAIDE)
		&& in_StringList(usr->curr_room->room_aides, usr->name) == NULL)
		usr->runtime_flags &= ~RTF_ROOMAIDE;

	if (r->flags & ROOM_CHATROOM)
		enter_chatroom(usr);

/*
	add user to the who-is-inside list
	the reason we do it this late is that you now won't get to see people
	walking in and out of the room in the history, if there is no one else there
*/
	add_PList(&usr->curr_room->inside, new_PList(usr));
}

void leave_room(User *usr) {
PList *p;

	if (usr->curr_room == NULL)
		return;

	if ((p = in_PList(usr->curr_room->inside, usr)) != NULL) {
		remove_PList(&usr->curr_room->inside, p);
		destroy_PList(p);
	}
	if (usr->curr_room->flags & ROOM_CHATROOM)
		leave_chatroom(usr);

	if (usr->curr_room->number == HOME_ROOM && usr->curr_room->inside == NULL) {
		remove_Room(&HomeRooms, usr->curr_room);
		save_Room(usr->curr_room);
		destroy_Room(usr->curr_room);
	}
	usr->curr_room = NULL;
}

void enter_chatroom(User *usr) {
char buf[3 * MAX_LINE], name_buf[MAX_NAME+3], *str;
StringList *sl;

	if (usr == NULL)
		return;

	Enter(enter_chatroom);

	if (usr->curr_room->number == HOME_ROOM) {
		sprintf(buf, "%s Home", name_with_s(usr, usr->name, name_buf));

		if (!strcmp(buf, usr->curr_room->name))
			Print(usr, "\n<magenta>Welcome home, <yellow>%s\n", usr->name);
		else
			Print(usr, "\n<magenta>Welcome to <yellow>%s<white>>\n", usr->curr_room->name);
	} else
		Print(usr, "\n<yellow>%s<white>>\n", usr->curr_room->name);

	if (STRING_CHANCE)
		str = PARAM_NOTIFY_ENTER_CHAT;
	else
		str = RND_STR(Str_enter_chatroom);

	if (usr->runtime_flags & RTF_SYSOP)
		sprintf(buf, "<yellow>%s<white>: <yellow>%s <magenta>%s<white>", PARAM_NAME_SYSOP, usr->name, str);
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			sprintf(buf, "<yellow>%s<white>: <yellow>%s <magenta>%s<white>", PARAM_NAME_ROOMAIDE, usr->name, str);
		else
			sprintf(buf, "<yellow>%s <magenta>%s<white>", usr->name, str);

	if (usr->curr_room->chat_history != NULL) {
		Put(usr, "\n");
		for(sl = usr->curr_room->chat_history; sl != NULL; sl = sl->next)
			Print(usr, "%s\n", sl->str);
		Put(usr, "<white>--\n");
	}
	chatroom_msg(usr->curr_room, buf);
	Return;
}

void leave_chatroom(User *usr) {
char buf[3 * MAX_LINE], *str;

	if (usr == NULL || !usr->name[0])
		return;

	Enter(leave_chatroom);

	if (STRING_CHANCE)
		str = PARAM_NOTIFY_LEAVE_CHAT;
	else
		str = RND_STR(Str_leave_chatroom);

	if (usr->runtime_flags & RTF_SYSOP)
		sprintf(buf, "<yellow>%s<white>: <yellow>%s <magenta>%s<white>", PARAM_NAME_SYSOP, usr->name, str);
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			sprintf(buf, "<yellow>%s<white>: <yellow>%s <magenta>%s<white>", PARAM_NAME_ROOMAIDE, usr->name, str);
		else
			sprintf(buf, "<yellow>%s <magenta>%s<white>", usr->name, str);

	chatroom_msg(usr->curr_room, buf);
	Return;
}

/*
	user says something in a chat room
*/
void chatroom_say(User *usr, char *str) {
char buf[3 * MAX_LINE], from[MAX_LINE];
int i;

	if (usr == NULL || str == NULL || !*str)
		return;

	Enter(chatroom_say);

	i = strlen(str)-1;
	while(i >= 0 && (str[i] == ' ' || str[i] == '\t'))
		str[i--] = 0;
	if (!*str) {
		Return;
	}
	if (usr->runtime_flags & RTF_SYSOP)
		sprintf(from, "<yellow>%s<white>: <cyan>%s", PARAM_NAME_SYSOP, usr->name);
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			sprintf(from, "<yellow>%s<white>: <cyan>%s", PARAM_NAME_ROOMAIDE, usr->name);
		else
			strcpy(from, usr->name);

	if (*str == ' ')
		sprintf(buf, "<cyan>%s<yellow>%s", from, str);
	else
		sprintf(buf, "<white>[<cyan>%s<white>]: <yellow>%s", from, str);

	chatroom_tell(usr->curr_room, buf);
	Return;
}

/*
	send a message to everyone in the room, and put it in the room history
*/
void chatroom_tell(Room *r, char *str) {
	if (r == NULL || str == NULL || !*str)
		return;

	Enter(chatroom_tell);

	add_StringList(&r->chat_history, new_StringList(str));
	if (list_Count(r->chat_history) > PARAM_MAX_CHAT_HISTORY) {
		StringList *sl;

		sl = r->chat_history;
		remove_StringList(&r->chat_history, sl);
		destroy_StringList(sl);
	}
	chatroom_msg(r, str);
	Return;
}

/*
	send a message to everyone in the room, but keep it out of the history
	Used for enter/leave messages
*/
void chatroom_msg(Room *r, char *str) {
PList *p;
User *u;

	if (r == NULL || str == NULL || !*str)
		return;

	Enter(chatroom_msg);

	if (!(r->flags & ROOM_CHATROOM)) {
		Return;
	}
	for(p = r->inside; p != NULL; p = p->next) {
		u = (User *)p->p;
		if (u == NULL)
			continue;

		if (u->runtime_flags & RTF_BUSY)
			add_StringList(&u->chat_history, new_StringList(str));
		else
			Print(u, "%s\n", str);
	}
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
		return sprintf(buf, "<cyan>%-15s <%s>%02d<white>:<%s>%02d %cM",
			(worldclock[item].name == NULL) ? "" : worldclock[item].name,
			zone_color2, t->tm_hour, zone_color2, t->tm_min, am_pm);
	}
	return sprintf(buf, "<cyan>%-15s <%s>%02d<white>:<%s>%02d",
		(worldclock[item].name == NULL) ? "" : worldclock[item].name,
		zone_color2, t->tm_hour, zone_color2, t->tm_min);
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

/* EOB */
