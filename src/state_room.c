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
	state_room.c	WJ105
*/

#include "config.h"
#include "debug.h"
#include "state_room.h"
#include "state.h"
#include "state_msg.h"
#include "state_login.h"
#include "state_config.h"
#include "state_history.h"
#include "state_friendlist.h"
#include "state_roomconfig.h"
#include "state_sysop.h"
#include "edit.h"
#include "util.h"
#include "screens.h"
#include "access.h"
#include "cstring.h"
#include "Stats.h"
#include "Memory.h"
#include "Param.h"
#include "SU_Passwd.h"
#include "bufprintf.h"

#include <stdlib.h>
#include <stdio.h>

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


void state_room_prompt(User *usr, char c) {
int i, idx;
char num_buf[MAX_NUMBER];

	if (usr == NULL)
		return;

	Enter(state_room_prompt);

	if (usr->curr_room == NULL)
		usr->curr_room = Lobby_room;

	if ((usr->curr_room->flags & ROOM_CHATROOM) && !PARAM_HAVE_CHATROOMS && usr->curr_room->number != HOME_ROOM) {
		usr->curr_room->flags &= ~ROOM_CHATROOM;
		usr->curr_room->flags |= ROOM_DIRTY;
	}
/*
	First check for chat rooms (they're a rather special case)
*/
	if ((usr->curr_room->flags & ROOM_CHATROOM) && !(usr->runtime_flags & RTF_CHAT_ESCAPE)) {
		char buf[MAX_LINE];

		switch(c) {
			case INIT_STATE:
				Put(usr, "\n");
			case INIT_PROMPT:
				edit_chatline(usr, EDIT_INIT);
/* note how you are not busy when editing a chat line */
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
				i = edit_chatline(usr, c);

				if (i == EDIT_BREAK) {
					CURRENT_STATE_X(usr, INIT_PROMPT);
					Return;
				}
				if (i == EDIT_RETURN) {
					if (!usr->edit_buf[0]) {
						CURRENT_STATE(usr);
						Return;
					}
					wipe_line(usr);
					cstrcpy(buf, usr->edit_buf, MAX_LINE);
					edit_chatline(usr, EDIT_INIT);
					usr->runtime_flags &= ~(RTF_BUSY | RTF_CHAT_ESCAPE);
/* Note: chatroom_say() will reprint the prompt for us */
					chatroom_say(usr, buf);
					Return;
				}
		}
		Return;
	}
/*
	Now handle the more 'normal' DOC-style rooms
*/
	switch(c) {
		case INIT_STATE:
		case INIT_PROMPT:
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
			if (load_screen(usr->text, PARAM_HELP_STD) < 0) {
				Put(usr, "<red>No help available\n");
				break;
			}
			read_text(usr);
			Return;

		case 'q':
		case 'Q':
			if (PARAM_HAVE_QUESTIONS) {
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>Question%s\n", num_buf);

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
					if (get_su_passwd(usr->name) == NULL) {
/*
	first see if the person is old enough to use to enable Helper status
	Sysop-capable users get a free ride
	PARAM_HELPER_AGE is measured in 24-hour days and may be 0
*/
						if (!usr->online_timer)
							usr->online_timer = rtc;
						if (usr->online_timer < rtc)
							usr->total_time += (rtc - usr->online_timer);
						usr->online_timer = rtc;

						if (usr->total_time / SECS_IN_DAY < PARAM_HELPER_AGE) {
							Put(usr, "<red>You are still wet behind the ears. True wisdom comes with age\n");
							break;
						}
					}
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
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>eXpress Message%s\n", num_buf);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send eXpress Messages\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>You have put messages on hold\n");
					break;
				}
				Put(usr, "<green>");
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
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>Quick X%s\n", num_buf);

				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send Quick eXpress Messages\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>You have put messages on hold\n");
					break;
				}
				if (usr->quick[c - '1'] != NULL) {
					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;

					cstrcpy(usr->edit_buf, usr->quick[c - '1'], MAX_LINE);
					usr->edit_pos = strlen(usr->edit_buf);
					usr->runtime_flags |= RTF_BUSY;

					Print(usr, "<green>Enter recipient: <yellow>%s", usr->edit_buf);
					PUSH(usr, STATE_X_PROMPT);
					Return;
				} else
					Put(usr, "<red>That quicklist entry is empty. Press<yellow> <Ctrl-C><red> to enter the <yellow>Config menu<red>\n"
						"so you can configure your quicklist\n");
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Quick X<red> is not enabled on this server\n");
			break;

		case KEY_CTRL('Q'):
			if (PARAM_HAVE_QUICK_X) {
				Put(usr, "<white>Quicklist\n");
				buffer_text(usr);
				print_quicklist(usr);
				read_menu(usr);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Quick X<red> is not enabled on this server\n");
			break;

		case ':':
		case ';':
			if (PARAM_HAVE_EMOTES) {
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>Emote%s\n", num_buf);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send emotes\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>You have put messages on hold\n");
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
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>Feelings%s\n", num_buf);
				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send feelings\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>You have put messages on hold\n");
					break;
				}
				Put(usr, "<green>");
				enter_recipients(usr, STATE_FEELINGS_PROMPT);
				Return;			
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>Feelings<red> are not enabled on this server\n");
			break;

		case 'v':
			if (PARAM_HAVE_X_REPLY) {
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>Reply%s\n", num_buf);

				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send replies\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>You have put messages on hold\n");
					break;
				}
				Put(usr, "<green>");
				reply_x(usr, REPLY_X_ONE);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but the <yellow>X Reply<red> feature is not enabled on this server\n");
			break;

		case 'V':
			if (PARAM_HAVE_X_REPLY) {
				if (usr->flags & USR_XMSG_NUM)
					bufprintf(num_buf, MAX_NUMBER, " (#%d)", usr->msg_seq_sent+1);
				else
					num_buf[0] = 0;

				Print(usr, "<white>Reply to All%s\n", num_buf);

				if (is_guest(usr->name)) {
					Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot send replies\n", PARAM_NAME_GUEST);
					break;
				}
				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>You have put messages on hold\n");
					break;
				}
				Put(usr, "<green>");
				reply_x(usr, REPLY_X_ALL);
				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but the <yellow>X Reply<red> feature is not enabled on this server\n");
			break;

		case 'l':
		case 'L':
			if ((usr->curr_room->flags & ROOM_CHATROOM) && usr->curr_room->number != LOBBY_ROOM) {
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

		case KEY_CTRL('L'):
			Put(usr, "<white>Lock\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot lock the terminal\n", PARAM_NAME_GUEST);
				break;
			}
			if (!(usr->flags & USR_DONT_ASK_REASON)) {
				PUSH(usr, STATE_LOCK_PASSWORD);
				Put(usr, "<green>");
				CALL(usr, STATE_ASK_AWAY_REASON);
			} else
				CALL(usr, STATE_LOCK_PASSWORD);
			Return;

		case '`':
			CALL(usr, STATE_BOSS);
			Return;

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
			Return;

		case KEY_CTRL('X'):
			if (PARAM_HAVE_XMSGS) {
				Put(usr, "<white>Message history\n");
				if (!history_prompt(usr))
					break;

				Return;
			} else
				if (PARAM_HAVE_DISABLED_MSG)
					Put(usr, "<red>Sorry, but <yellow>eXpress Messages<red> are not enabled on this server\n");
			break;

		case KEY_CTRL('B'):
			if (PARAM_HAVE_HOLD) {
				Put(usr, "<white>Hold messages\n");

				usr->runtime_flags ^= RTF_HOLD;

				if (usr->runtime_flags & RTF_HOLD) {
					Put(usr, "<magenta>Messages will be held until you press<yellow> <Ctrl-B><magenta> again\n");

					if (usr->flags & USR_HELPING_HAND) {		/* this is inconvenient right now */
						usr->flags &= ~USR_HELPING_HAND;
						usr->runtime_flags |= RTF_WAS_HH;
					}
					notify_hold(usr);

					if (!(usr->flags & USR_DONT_ASK_REASON)) {
						CALL(usr, STATE_ASK_AWAY_REASON);
						Return;
					}
				} else {
					Put(usr, "<magenta>Messages will <yellow>no longer<magenta> be held\n");

					if (usr->held_msgs != NULL) {
						held_history_prompt(usr);
						Return;
					} else {
						Free(usr->away);
						usr->away = NULL;

						notify_unhold(usr);
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
			Put(usr, "<white>Message number\n");
			CALL(usr, STATE_ENTER_MSG_NUMBER);
			Return;

		case '-':
		case '_':
			Put(usr, "<white>Read last\n");
			CALL(usr, STATE_ENTER_MINUS_MSG);
			Return;

		case '+':
		case '=':
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
					Put(usr, "<red>Sorry, but this server has no <yellow>Mail> <red>room\n");
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
			if (!(usr->curr_room->flags & ROOM_CHATROOM)) {
				Room *r;

				Put(usr, "<white>Skip\n");

				r = next_unread_room(usr);
				if (r != usr->curr_room) {
					Print(usr, "<white>Goto <yellow>%s<white>\n", r->name);
					goto_room(usr, r);
					Return;
				}
				Put(usr, "<red>No new messages\n");
			}
			break;

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
					Print(usr, "<white>Read New (%d new messages)\n", usr->curr_room->msg_idx - usr->curr_msg);
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
					Put(usr, "<red>Message already has been deleted<yellow>..!\n");
					break;
				}
				if (((usr->message->flags & MSG_FROM_SYSOP) && !(usr->runtime_flags & RTF_SYSOP))
					|| ((usr->message->flags & MSG_FROM_ROOMAIDE) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))
					|| (usr->curr_room != usr->mail && strcmp(usr->name, usr->message->from) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))) {
					Put(usr, "<red>You are not allowed to delete this message\n");
					break;
				}
				CALL(usr, STATE_DELETE_MSG);
				Return;
			} else
				Put(usr, "<red>No message to delete\n");
			break;

		case 'u':
		case 'U':
			Put(usr, "<white>Undelete\n");
			if (is_guest(usr->name)) {
				Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot undelete messages\n", PARAM_NAME_GUEST);
				break;
			}
			if (usr->message == NULL) {
				Put(usr, "<red>No message to undelete!\n");
				break;
			}
			if (usr->message->deleted == (time_t)0UL) {
				Put(usr, "<red>Message has not been deleted, so you can't undelete it..!\n");
				break;
			}
			if ((usr->curr_room != usr->mail
				&& ((usr->message->flags & MSG_DELETED_BY_SYSOP) && !(usr->runtime_flags & RTF_SYSOP)))
				|| ((usr->message->flags & MSG_DELETED_BY_ROOMAIDE) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))
				|| (usr->message->deleted_by != NULL && strcmp(usr->message->deleted_by, usr->name) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))) {
				Put(usr, "<red>You are not allowed to undelete this message\n");
				break;
			}
			PUSH(usr, STATE_UNDELETE_MSG);
			read_message(usr, usr->message, READ_DELETED);
			Return;

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
				cstrcpy(m->from, usr->name, MAX_NAME);

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

				if (usr->message->subject && usr->message->subject[0])
					m->subject = cstrdup(usr->message->subject);

				if (usr->curr_room == usr->mail)
					m->reply_number = 0UL;
				else
					m->reply_number = usr->message->number;

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
					if (usr->new_message->subject == NULL || !usr->new_message->subject[0]) {
						char subject[MAX_LINE];

						bufprintf(subject, MAX_LINE, "<mail message from %s>", usr->new_message->from);
						Free(usr->new_message->subject);
						usr->new_message->subject = cstrdup(subject);
					}
					cstrcpy(usr->new_message->from, usr->name, MAX_NAME);

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

		case 'k':
			Put(usr, "<white>Known rooms\n");
			known_rooms(usr);
			Return;

		case 'K':
		case KEY_CTRL('K'):
			Put(usr, "<white>All known rooms\n");
			allknown_rooms(usr);
			Return;

		case 'i':
		case 'I':
			Put(usr, "<white>Room info\n");
			room_info(usr);
			Return;

		default:
			if (fun_common(usr, c)) {
				Return;
			}
	}
	if (usr->curr_room->flags & ROOM_CHATROOM) {
		Put(usr, "\n");
		edit_line(usr, EDIT_INIT);
		usr->runtime_flags &= ~(RTF_BUSY | RTF_BUSY_SENDING | RTF_BUSY_MAILING | RTF_CHAT_ESCAPE);
	} else {
		usr->runtime_flags &= ~(RTF_BUSY | RTF_BUSY_SENDING | RTF_BUSY_MAILING);

		if (!(usr->runtime_flags & RTF_HOLD) && usr->held_msgs != NULL)
			Put(usr, "\n<green>The following messages were held while you were busy:\n");
	}
	PrintPrompt(usr);
	Return;
}

void PrintPrompt(User *usr) {
	if (usr == NULL)
		return;

	Enter(PrintPrompt);

	if (usr->curr_room == NULL)
		usr->curr_room = Lobby_room;

/*
	do housekeeping: free up memory that we're not going to use anyway
*/
	free_StringIO(usr->text);
	listdestroy_PList(usr->scroll);
	usr->scroll = usr->scrollp = NULL;

/*
	these messages were held while you were busy ...
*/
	if (!(usr->runtime_flags & (RTF_BUSY|RTF_HOLD)) && usr->held_msgs != NULL) {
		if (usr->flags & USR_HOLD_BUSY) {
			held_history_prompt(usr);
			Return;
		} else
			spew_BufferedMsg(usr);
	}
	if (!(usr->runtime_flags & RTF_BUSY)) {

/* print a short prompt for chatrooms */

		if (usr->curr_room->flags & ROOM_CHATROOM) {

/* spool the chat messages we didn't get while we were busy */

			if (usr->chat_history != NULL) {
				StringList *sl;

				Put(usr, "\n");
				for(sl = usr->chat_history; sl != NULL; sl = sl->next)
					Print(usr, "%s\n", sl->str);

				listdestroy_StringList(usr->chat_history);
				usr->chat_history = NULL;
			}
			Print(usr, "<white>%c ", (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
		} else {
			char roomname[MAX_LINE];

			if (usr->curr_room == usr->mail)
				possession(usr->name, "Mail", roomname, MAX_LINE);
			else
				cstrcpy(roomname, usr->curr_room->name, MAX_LINE);

/* print a long prompt with msg number when reading messages */

			if (usr->curr_msg >= 0) {
				int remaining = -1;
				char num_buf1[MAX_NUMBER], num_buf2[MAX_NUMBER];

				remaining = usr->curr_room->msg_idx - 1 - usr->curr_msg;

				if (usr->flags & USR_ROOMNUMBERS)
					Print(usr, "<yellow>\n[%u %s]<green> msg #%s (%s remaining) %c ",
						usr->curr_room->number, roomname,
						print_number(usr->curr_room->msgs[usr->curr_msg], num_buf1, MAX_NUMBER),
						print_number(remaining, num_buf2, MAX_NUMBER),
						(usr->runtime_flags & RTF_SYSOP) ? '#' : '>'
					);
				else
					Print(usr, "<yellow>\n[%s]<green> msg #%s (%s remaining) %c ",
						roomname,
						print_number(usr->curr_room->msgs[usr->curr_msg], num_buf1, MAX_NUMBER),
						print_number(remaining, num_buf2, MAX_NUMBER),
						(usr->runtime_flags & RTF_SYSOP) ? '#' : '>'
					);
			} else {
				destroy_Message(usr->message);
				usr->message = NULL;

/* print a prompt with roomname */

				if (usr->flags & USR_ROOMNUMBERS)
					Print(usr, "<yellow>\n%u %s%c ", usr->curr_room->number, roomname, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
				else
					Print(usr, "<yellow>\n%s%c ", roomname, (usr->runtime_flags & RTF_SYSOP) ? '#' : '>');
			}
		}
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
		Put(usr, "<green>Enter room name: <yellow>");
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
			Put(usr, "<red>You can't Zap your own <yellow>Mail><red> room\n");
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
		Put(usr, "<cyan>Are you sure? (Y/n): ");
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
		Put(usr, "<cyan>Are you sure? (y/N): ");
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

void print_known_room(User *usr, Room *r) {
Joined *j;
char status[2], buf[MAX_LONGLINE], buf2[MAX_LONGLINE];
int read_it = 1, idx, l;

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
	bufprintf(buf, MAX_LONGLINE, "%c%c%c %3u %c%s>",
		status[0], status[1],
		(char)color_by_name("white"), r->number,
		(char)color_by_name("yellow"), r->name);

	bufprintf(buf2, MAX_LONGLINE, "%-50s", buf);

/* add room aides to the line */
	if (r->room_aides == NULL) {
		if (r->number >= SPECIAL_ROOMS) {
			l = strlen(buf2);
			bufprintf(buf2+l, MAX_LONGLINE - l, "%c (no %s)", (char)color_by_name("red"), PARAM_NAME_ROOMAIDE);
		}
	} else {
		StringList *sl;

		l = strlen(buf2);
		for(sl = r->room_aides; sl != NULL && l < MAX_LINE; sl = sl->next)
			l += bufprintf(buf2+l, MAX_LONGLINE - l, " %c%s,", (char)color_by_name("cyan"), sl->str);

		l--;							/* strip the final comma */
		buf2[l] = 0;
		if (l > MAX_LINE) {				/* display as '...' */
			buf2[MAX_LINE-1] = 0;
			buf2[MAX_LINE-2] = buf2[MAX_LINE-3] = buf2[MAX_LINE-4] = '.';
		}
	}
	buf2[MAX_LINE-1] = 0;
	Put(usr, buf2);
	Put(usr, "\n");
	Return;
}

void known_rooms(User *usr) {
Room *r, *r_next;
Joined *j;
char *category = NULL;

	Enter(known_rooms);

	buffer_text(usr);

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
			Print(usr, "<cyan>\n[%s]\n", r->category);
			category = r->category;
		}
		print_known_room(usr, r);
	}
/*
	throw away the demand loaded room because we're not using it anyway
*/
	if ((r = find_Roombynumber(usr, 2)) != NULL)
		unload_Room(r);

	read_text(usr);
	Return;
}

void allknown_rooms(User *usr) {
Room *r, *r_next;
char *category = NULL;

	Enter(allknown_rooms);

	buffer_text(usr);

	for(r = AllRooms; r != NULL; r = r_next) {
		r_next = r->next;

		if (r->number == LOBBY_ROOM || r->number == MAIL_ROOM || r->number == HOME_ROOM)
			if ((r = find_Roombynumber(usr, r->number)) == NULL)
				continue;

		if ((r->flags & ROOM_HIDDEN)
			&& in_Joined(usr->rooms, r->number) == NULL
			&& !(usr->runtime_flags & RTF_SYSOP)
			&& in_StringList(r->room_aides, usr->name) == NULL)
			continue;

		if (PARAM_HAVE_CATEGORY && ((category == NULL && r->category != NULL) || (category != NULL && r->category != NULL && strcmp(category, r->category)))) {
			Print(usr, "\n<white>[<cyan>%s<white>]\n", r->category);
			category = r->category;
		}
		print_known_room(usr, r);
	}
	if ((r = find_Roombynumber(usr, 2)) != NULL)
		unload_Room(r);

	read_text(usr);
	Return;
}

void room_info(User *usr) {
char buf[MAX_LONGLINE];
Joined *j;
StringList *sl;

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

	buffer_text(usr);

	Print(usr, "<white>Room info of <yellow>%s><white> (room #%u)\n",
		usr->curr_room->name, usr->curr_room->number);

	if (usr->curr_room->generation) {
		char date_buf[MAX_LINE];

		Print(usr, "<green>Created on <yellow>%s\n", print_date(usr, usr->curr_room->generation, date_buf, MAX_LINE));
	}
	*buf = 0;
	if (usr->curr_room->flags & ROOM_READONLY)
		cstrcat(buf, ", read-only", MAX_LONGLINE);

	if (usr->curr_room->flags & ROOM_INVITE_ONLY)
		cstrcat(buf, ", invite-only", MAX_LONGLINE);

	if (usr->curr_room->flags & ROOM_ANONYMOUS)
		cstrcat(buf, ", anonymous-optional", MAX_LONGLINE);

	if (usr->curr_room->flags & ROOM_NOZAP)
		cstrcat(buf, ", not zappable", MAX_LONGLINE);

	if ((usr->curr_room->flags & ROOM_HIDDEN) && (usr->runtime_flags & RTF_SYSOP))
		cstrcat(buf, ", hidden", MAX_LONGLINE);

	if (*buf)
		Print(usr, "\n<green>This room is %s\n", buf+2);

	if (usr->curr_room->room_aides != NULL) {
		if (usr->curr_room->room_aides->next != NULL) {
			Print(usr, "<cyan>%ss are: <yellow>", PARAM_NAME_ROOMAIDE);
			for(sl = usr->curr_room->room_aides; sl != NULL && sl->next != NULL; sl = sl->next)
				Print(usr, "%s, ", sl->str);

			Print(usr, "%s\n", sl->str);
		} else
			Print(usr, "<cyan>%s is: %s\n", PARAM_NAME_ROOMAIDE, usr->curr_room->room_aides->str);
	}
	if (PARAM_HAVE_CATEGORY && usr->curr_room->category && usr->curr_room->category[0])
		Print(usr, "<cyan>Category:<yellow> %s\n", usr->curr_room->category);

	Put(usr, "<green>\n");

	load_roominfo(usr->curr_room, usr->name);

	if (usr->curr_room == NULL || usr->curr_room->info == NULL || usr->curr_room->info->buf == NULL) {
		if (usr->curr_room != NULL && usr->curr_room->number == MAIL_ROOM)
			Put(usr, "Here you can leave messages to users that are not online.\n");
		else
			Put(usr, "<red>This room has no room info\n");
	} else
		concat_StringIO(usr->text, usr->curr_room->info);

	if (!PARAM_HAVE_RESIDENT_INFO) {
		destroy_StringIO(usr->curr_room->info);
		usr->curr_room->info = NULL;
	}
	read_text(usr);
	Return;
}

void state_enter_msg_number(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_msg_number);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter message number: <yellow>");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		unsigned long num;
		int idx;

		num = cstrtoul(usr->edit_buf, 10);
		if (num <= 0UL) {
			Put(usr, "<red>No such message\n");
			RET(usr);
			Return;
		}
		if (usr->curr_room->msgs == NULL)
			idx = usr->curr_room->msg_idx;
		else {
			for(idx = 0; idx < usr->curr_room->msg_idx; idx++) {
				if (usr->curr_room->msgs[idx] == num) {
					usr->curr_msg = idx;
					readMsg(usr);
					Return;
				}
				if (usr->curr_room->msgs[idx] > num) {	/* we're not getting there anymore */
					idx = usr->curr_room->msg_idx;
					break;
				}
			}
		}
		if (idx >= usr->curr_room->msg_idx) {
			if (num <= 1 && usr->curr_room->msg_idx >= 1) {		/* goto first */
				usr->curr_msg = 0;
				readMsg(usr);
				Return;
			}
		}
		Put(usr, "<red>No such message\n");
		RET(usr);
	}
	Return;
}

void state_enter_minus_msg(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_minus_msg);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of messages to read back: <yellow>");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		int i;

		i = atoi(usr->edit_buf);
		if (i <= 0) {
			RET(usr);
			Return;
		}
		if (usr->curr_msg < 0) {
			usr->curr_msg = usr->curr_room->msg_idx - 1;
			if (usr->curr_msg < 0) {
				Put(usr, "<red>No messages\n");
				RET(usr);
				Return;
			}
		}
		usr->curr_msg -= i;
		if (usr->curr_msg < 0)
			usr->curr_msg = 0;

		readMsg(usr);			/* readMsg() RET()s for us :P */
	}
	Return;
}

void state_enter_plus_msg(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_plus_msg);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of messages to skip: <yellow>");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		int i;

		i = atoi(usr->edit_buf);
		if (i <= 0) {
			RET(usr);
			Return;
		}
		if (usr->curr_msg < 0) {
			usr->curr_msg = usr->curr_room->msg_idx - 1;
			if (usr->curr_msg < 0) {
				Put(usr, "<red>No messages\n");
				RET(usr);
				Return;
			}
		}
		usr->curr_msg += i;
		if (usr->curr_msg >= usr->curr_room->msg_idx)
			usr->curr_msg = usr->curr_room->msg_idx - 1;

		readMsg(usr);			/* readMsg() RET()s for us :P */
	}
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

	free_StringIO(usr->text);

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
	if (j != NULL) {
		if (j->roominfo_read == -1) {
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

/* if not RA here, reset the flag

	if ((usr->runtime_flags & RTF_ROOMAIDE)
		&& in_StringList(usr->curr_room->room_aides, usr->name) == NULL)
		usr->runtime_flags &= ~RTF_ROOMAIDE;
*/
/*
	always reset the RA flag
	if you don't like this, uncomment the code above
*/
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
char buf[MAX_LONGLINE], *str;
StringList *sl;

	if (usr == NULL)
		return;

	Enter(enter_chatroom);

	if (usr->curr_room->number == HOME_ROOM) {
		possession(usr->name, "Home", buf, MAX_LONGLINE);

		if (!strcmp(buf, usr->curr_room->name))
			Print(usr, "\n<magenta>Welcome home, <yellow>%s\n", usr->name);
		else
			Print(usr, "\n<magenta>Welcome to <yellow>%s>\n", buf);
	} else
		Print(usr, "\n<yellow>%s>\n", usr->curr_room->name);

	if (STRING_CHANCE)
		str = PARAM_NOTIFY_ENTER_CHAT;
	else
		str = RND_STR(Str_enter_chatroom);

	if (usr->runtime_flags & RTF_SYSOP)
		bufprintf(buf, MAX_LONGLINE, "<yellow>%s: %s <magenta>%s<white>", PARAM_NAME_SYSOP, usr->name, str);
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			bufprintf(buf, MAX_LONGLINE, "<yellow>%s: %s <magenta>%s<white>", PARAM_NAME_ROOMAIDE, usr->name, str);
		else
			bufprintf(buf, MAX_LONGLINE, "<yellow>%s <magenta>%s<white>", usr->name, str);

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
char buf[MAX_LONGLINE], *str;

	if (usr == NULL || !usr->name[0])
		return;

	Enter(leave_chatroom);

	if (STRING_CHANCE)
		str = PARAM_NOTIFY_LEAVE_CHAT;
	else
		str = RND_STR(Str_leave_chatroom);

	if (usr->runtime_flags & RTF_SYSOP)
		bufprintf(buf, MAX_LONGLINE, "<yellow>%s: %s <magenta>%s<white>", PARAM_NAME_SYSOP, usr->name, str);
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			bufprintf(buf, MAX_LONGLINE, "<yellow>%s: %s <magenta>%s<white>", PARAM_NAME_ROOMAIDE, usr->name, str);
		else
			bufprintf(buf, MAX_LONGLINE, "<yellow>%s <magenta>%s<white>", usr->name, str);

	chatroom_msg(usr->curr_room, buf);
	Return;
}

/*
	user says something in a chat room
*/
void chatroom_say(User *usr, char *str) {
char buf[MAX_LONGLINE], from[MAX_LINE];

	if (usr == NULL || str == NULL || !*str)
		return;

	Enter(chatroom_say);

	ctrim_line(str);
	if (!*str) {
		Return;
	}
	if (usr->runtime_flags & RTF_SYSOP)
		bufprintf(from, MAX_LINE, "<yellow>%s:<cyan> %s", PARAM_NAME_SYSOP, usr->name);
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			bufprintf(from, MAX_LINE, "<yellow>%s:<cyan> %s", PARAM_NAME_ROOMAIDE, usr->name);
		else
			cstrcpy(from, usr->name, MAX_NAME);

	if (*str == ' ')
		bufprintf(buf, MAX_LONGLINE, "<cyan>%s<yellow> %s", from, str+1);
	else
		bufprintf(buf, MAX_LONGLINE, "<cyan>%s:<yellow> %s", from, str);

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
	r->flags |= ROOM_DIRTY;
	if (list_Count(r->chat_history) > PARAM_MAX_CHAT_HISTORY)
		destroy_StringList(pop_StringList(&r->chat_history));

	chatroom_msg(r, str);
	Return;
}

/*
	send a message to everyone in the room, but keep it out of the history
	Used for enter/leave messages

	it reprints the line that users are editing on, so it also
	reprints the prompt
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

		chatroom_tell_user(u, str);
	}
	Return;
}

void chatroom_tell_user(User *u, char *str) {
	if (u == NULL || str == NULL || !*str)
		return;

	Enter(chatroom_tell_user);

	if (!(u->curr_room->flags & ROOM_CHATROOM)) {
		Return;
	}
	if (u->runtime_flags & RTF_BUSY)
		add_StringList(&u->chat_history, new_StringList(str));
	else {
/*
	when not busy, pretty print the lines
*/
		wipe_line(u);
		Put(u, str);
		Put(u, "\n");

		PrintPrompt(u);
		if (u->runtime_flags & RTF_CHAT_ESCAPE)
			Put(u, "<white>/");
		else
			Put(u, u->edit_buf);
	}
	Return;
}

/*
	you're in a chatroom, but someone is sending you eXpress Messages
	btw, if RTF_BUSY is set, then it is handled by recvMsg()
*/
void chatroom_recvMsg(User *usr, BufferedMsg *msg) {
	if (usr == NULL || msg == NULL)
		return;

	Enter(chatroom_recvMsg);

	if (!(usr->curr_room->flags & ROOM_CHATROOM)) {
		Return;
	}
	wipe_line(usr);

	Put(usr, "<beep>");						/* alarm beep */
	print_buffered_msg(usr, msg);
	Put(usr, "\n");

	PrintPrompt(usr);
	if (usr->runtime_flags & RTF_CHAT_ESCAPE)
		Put(usr, "<white>/");
	else
		Put(usr, usr->edit_buf);

	Return;
}

/* EOB */
