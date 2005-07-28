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
	state_msg.c	WJ99
*/

#include "config.h"
#include "defines.h"
#include "state_msg.h"
#include "state.h"
#include "edit.h"
#include "access.h"
#include "util.h"
#include "log.h"
#include "debug.h"
#include "cstring.h"
#include "Stats.h"
#include "screens.h"
#include "CachedFile.h"
#include "strtoul.h"
#include "Param.h"
#include "OnlineUser.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


void enter_message(User *usr) {
	Enter(enter_message);

	if (usr->curr_room == usr->mail)
		Print(usr, "<white>%sail message\n<green>",
			(usr->runtime_flags & RTF_UPLOAD) ? "Upload m" : "M");
	else
		Print(usr, "<white>%s message\n<green>",
			(usr->runtime_flags & RTF_UPLOAD) ? "Upload" : "Enter");

	if (usr->curr_room->flags & ROOM_CHATROOM) {
		Put(usr, "<red>You cannot enter a message in a <white>chat<red> room\n");
		CURRENT_STATE(usr);
		Return;
	}
	if (is_guest(usr->name)) {
		Print(usr, "<red>Sorry, but the <yellow>%s<red> user cannot enter messages\n", PARAM_NAME_GUEST);
		CURRENT_STATE(usr);
		Return;
	}
	if ((usr->curr_room->flags & ROOM_READONLY) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE))) {
		Put(usr, "<red>You are not allowed to post in this room\n");
		CURRENT_STATE(usr);
		Return;
	}
	destroy_Message(usr->new_message);
	if ((usr->new_message = new_Message()) == NULL) {
		Perror(usr, "Out of memory");
		CURRENT_STATE(usr);
		Return;
	}
	strcpy(usr->new_message->from, usr->name);

	if (usr->runtime_flags & RTF_SYSOP)
		usr->new_message->flags |= MSG_FROM_SYSOP;
	else
		if (usr->runtime_flags & RTF_ROOMAIDE)
			usr->new_message->flags |= MSG_FROM_ROOMAIDE;

	if (usr->curr_room == usr->mail)
		enter_recipients(usr, STATE_ENTER_MAIL_RECIPIENTS);
	else {
		if (usr->curr_room->flags & ROOM_ANONYMOUS)
			CALL(usr, STATE_POST_AS_ANON);
		else {
			if (usr->curr_room->flags & ROOM_SUBJECTS)
				CALL(usr, STATE_ENTER_SUBJECT);
			else
				enter_the_message(usr);
		}
	}
	Return;
}

void state_post_as_anon(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_post_as_anon);

	switch(c) {
		case INIT_STATE:
			usr->runtime_flags |= RTF_BUSY;
			break;

		case KEY_ESC:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			RET(usr);
			Return;

		case 'n':
		case 'N':
		case KEY_RETURN:
		case ' ':
			Put(usr, "<white>Normal\n");
			usr->runtime_flags &= ~(RTF_ANON | RTF_DEFAULT_ANON);

			if (usr->curr_room->flags & ROOM_SUBJECTS)
				JMP(usr, STATE_ENTER_SUBJECT);
			else {
				POP(usr);
				enter_the_message(usr);
			}
			Return;

		case 'a':
		case 'A':
			Put(usr, "<white>Anonymous\n");
			usr->runtime_flags |= RTF_ANON;
			JMP(usr, STATE_ENTER_ANONYMOUS);
			Return;

		case 'd':
		case 'D':
			Put(usr, "<white>Default-anonymous\n");
			usr->runtime_flags |= RTF_DEFAULT_ANON;

			if (usr->default_anon != NULL && usr->default_anon[0]) {
				cstrlwr(usr->default_anon);
				strcpy(usr->new_message->anon, usr->default_anon);
			} else
				strcpy(usr->new_message->anon, "anonymous");

			if (usr->curr_room->flags & ROOM_SUBJECTS)
				JMP(usr, STATE_ENTER_SUBJECT);
			else {
				POP(usr);
				enter_the_message(usr);
			}
			Return;
	}
	Put(usr, "\n<green>You can post as: <hotkey>Normal, <hotkey>Anonymous, <hotkey>Default-anonymous: ");
	Return;
}

void state_enter_anonymous(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_anonymous);

	if (usr->new_message == NULL) {
		Perror(usr, "The message you were going to enter has disappeared");
		RET(usr);
		Return;
	}
	if (c == INIT_STATE)
		Put(usr, "<green>Enter alias<yellow>: <cyan>");

	r = edit_name(usr, c);
	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0])
			strcpy(usr->new_message->anon, "anonymous");
		else {
			strcpy(usr->new_message->anon, usr->edit_buf);
			cstrlwr(usr->new_message->anon);
		}
		if (usr->curr_room->flags & ROOM_SUBJECTS)
			JMP(usr, STATE_ENTER_SUBJECT);
		else {
			POP(usr);
			enter_the_message(usr);
		}
	}
	Return;
}


void state_enter_mail_recipients(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_enter_mail_recipients);

	switch(edit_recipients(usr, c, multi_mail_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			if (usr->recipients == NULL) {
				RET(usr);
				break;
			}
			if (usr->new_message == NULL) {
				Perror(usr, "Your letter has caught fire or something");
				RET(usr);
				break;
			}
			if ((usr->new_message->to = copy_StringList(usr->recipients)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				break;
			}
			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			JMP(usr, STATE_ENTER_SUBJECT);
	}
	Return;
}

void state_enter_subject(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_subject);

	if (c == INIT_STATE)
		Put(usr, "<green>Subject: <yellow>");

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		if (usr->edit_buf[0])
			strcpy(usr->new_message->subject, usr->edit_buf);

		POP(usr);				/* pop this 'enter subject' call */
		enter_the_message(usr);
	}
	Return;
}

void enter_the_message(User *usr) {
	if (usr == NULL)
		return;

	Enter(enter_the_message);

	usr->new_message->mtime = rtc;
	if (usr->new_message->anon[0])
		usr->new_message->flags &= ~(MSG_FROM_SYSOP | MSG_FROM_ROOMAIDE);

	if (usr->runtime_flags & RTF_UPLOAD)
		Print(usr, "<green>Upload %smessage, press <white><<yellow>Ctrl-C<white>><green> to end\n\n",
			(usr->curr_room == usr->mail) ? "mail " : "");
	else
		Print(usr, "<green>Enter %smessage, press <white><<yellow>return<white>><green> twice or press <white><<yellow>Ctrl-C<white>><green> to end\n\n",
			(usr->curr_room == usr->mail) ? "mail " : "");

	msg_header(usr, usr->new_message);
	display_text(usr, usr->text);			/* show the message header */
	edit_text(usr, save_message, abort_message);
	Return;
}

void save_message(User *usr, char c) {
char filename[MAX_PATHLEN], *p;
int err = 0;
Joined *j;
StringIO *tmp;

	if (usr == NULL)
		return;

	Enter(save_message);

	if (usr->new_message == NULL) {
		Perror(usr, "The message has disappeared!");
		RET(usr);
		Return;
	}
/* swapping pointers around is faster than copying all the time */
	tmp = usr->new_message->msg;
	usr->new_message->msg = usr->text;
	usr->text = tmp;
	free_StringIO(usr->text);

	if (usr->curr_room == NULL) {
		Print(usr, "<red>In the meantime, the current room has vanished\n"
			"You are dropped off in the <yellow>%s<white>>\n", Lobby_room->name);

		destroy_Message(usr->new_message);
		usr->new_message = NULL;

		POP(usr);
		goto_room(usr, Lobby_room);
		Return;
	}
/*
	crude check for empty messages (completely empty or only containing spaces)
	The check is crude because a single color code already counts as 'not empty'
*/
	for(p = usr->new_message->msg->buf; p != NULL && *p; p++) {
		if (*p != ' ' && *p != '\n')
			break;
	}
	if (p == NULL || !*p) {
		Print(usr, "<red>Message is empty, message not saved\n");

		destroy_Message(usr->new_message);
		usr->new_message = NULL;
		RET(usr);
		Return;
	}
	if (usr->curr_room == usr->mail) {
		StringList *sl, *sl_next;
		User *u;

		for(sl = usr->new_message->to; sl != NULL; sl = sl_next) {
			sl_next = sl->next;

			if (!strcmp(usr->name, sl->str))
				continue;

			if ((u = is_online(sl->str)) == NULL) {
				if (!(usr->runtime_flags & RTF_SYSOP)) {	/* if not sysop, check permissions */
					StringList *sl2;

					if ((u = new_User()) == NULL) {
						Perror(usr, "Out of memory");
						RET(usr);
						Return;
					}
					if (load_User(u, sl->str, LOAD_USER_ENEMYLIST)) {
						Perror(usr, "Failed to load user");
						destroy_User(u);
						u = NULL;
						continue;
					}
					if ((sl2 = in_StringList(u->enemies, usr->name)) != NULL) {
						Print(usr, "<yellow>%s<red> does not wish to receive <yellow>Mail<white>><red> from you\n", sl->str);

						remove_StringList(&usr->new_message->to, sl2);
						destroy_StringList(sl2);
						destroy_User(u);
						u = NULL;
						continue;
					}
					destroy_User(u);
					u = NULL;
				}
				usr->new_message->number = get_mail_top(sl->str)+1;
			} else
				usr->new_message->number = room_top(u->mail)+1;

			sprintf(filename, "%s/%c/%s/%lu", PARAM_USERDIR, sl->str[0], sl->str, usr->new_message->number);
			path_strip(filename);

			if (!save_Message(usr->new_message, filename)) {
				Print(usr, "<green>Sending mail to <yellow>%s\n", sl->str);

				if (u != NULL) {
					newMsg(u->mail, usr->new_message->number, u);
					if (PARAM_HAVE_MAILROOM)
						Tell(u, "<beep><cyan>You have new mail\n");
				}
			} else {
				Print(usr, "<red>Error delivering mail to <yellow>%s\n", sl->str);
				err++;
			}
		}
/* save a copy for the sender */

		if (usr->new_message->to != NULL) {
			if (!err) {
				usr->new_message->number = room_top(usr->mail)+1;
				sprintf(filename, "%s/%c/%s/%lu", PARAM_USERDIR, usr->name[0], usr->name, usr->new_message->number);
				path_strip(filename);

				if (save_Message(usr->new_message, filename))
					err++;
				else
					newMsg(usr->mail, usr->new_message->number, usr);
			}
			if (err) {
				Perror(usr, "Error sending mail");
			}
		}
	} else {
		if ((usr->curr_room->flags & ROOM_READONLY) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE))) {
			Put(usr, "<red>You are suddenly not allowed to post in this room\n");
			destroy_Message(usr->new_message);
			usr->new_message = NULL;
			RET(usr);
			Return;
		}
		if (!(usr->runtime_flags & RTF_SYSOP) && in_StringList(usr->curr_room->kicked, usr->name) != NULL) {
			Put(usr, "<red>In the meantime, you have been kicked from this room\n");
			if ((j = in_Joined(usr->rooms, usr->curr_room->number)) != NULL)
				j->zapped = 1;

			destroy_Message(usr->new_message);
			usr->new_message = NULL;
			POP(usr);
			goto_room(usr, Lobby_room);
			Return;
		}
		usr->new_message->number = room_top(usr->curr_room)+1;
		sprintf(filename, "%s/%u/%lu", PARAM_ROOMDIR, usr->curr_room->number, usr->new_message->number);
		path_strip(filename);

		if (!save_Message(usr->new_message, filename)) {
			newMsg(usr->curr_room, usr->new_message->number, usr);
			room_beep(usr, usr->curr_room);
		} else {
			err++;
			Perror(usr, "Error saving message");
		}
	}
/* update stats */
	if (!err && !(usr->new_message->to != NULL && usr->new_message->to->next == NULL
		&& !strcmp(usr->name, usr->new_message->to->str))) {
		usr->posted++;
		if (usr->posted > stats.posted) {
			stats.posted = usr->posted;
			strcpy(stats.most_posted, usr->name);
		}
	}
	RET(usr);
	Return;
}

void abort_message(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(abort_message);

	free_StringIO(usr->text);

	RET(usr);
	Return;
}


void edit_text(User *usr, void (*save)(User *, char), void (*abort)(User *, char)) {
	Enter(edit_text);

	PUSH(usr, save);
	PUSH(usr, abort);
	CALL(usr, STATE_EDIT_TEXT);

	Return;
}

void state_edit_text(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_edit_text);

	if (c == INIT_STATE) {
		free_StringIO(usr->text);
		usr->total_lines = 0;

		if (usr->curr_room == usr->mail)
			usr->runtime_flags |= RTF_BUSY_MAILING;
	}
	r = edit_msg(usr, c);

	if (r == EDIT_RETURN) {
		if (!usr->edit_buf[0]) {
			if (usr->runtime_flags & RTF_UPLOAD) {
				usr->edit_buf[0] = ' ';
				usr->edit_buf[1] = 0;
			} else
				r = EDIT_BREAK;
		}
	}
	if (r == EDIT_RETURN || r == EDIT_BREAK) {
		if (usr->edit_buf[0]) {
			put_StringIO(usr->text, usr->edit_buf);
			write_StringIO(usr->text, "\n", 1);
			usr->total_lines++;
		}
		usr->edit_pos = 0;
		usr->edit_buf[0] = 0;
	}
	if (r == EDIT_BREAK) {
		JMP(usr, STATE_SAVE_TEXT);
	}
	Return;
}

void state_save_text(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_save_text);

	switch(c) {
		case INIT_STATE:
			Put(usr, "<hotkey>A<green>bort, <hotkey>Save, <hotkey>Continue<white>: ");
			usr->runtime_flags |= RTF_BUSY;
			break;

		case 'a':
		case 'A':
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			Put(usr, "<white>Abort -- ");
			JMP(usr, STATE_ABORT_TEXT);
			break;

		case 'c':
		case 'C':
			Put(usr, "<white>Continue<green>\n");

			if (usr->total_lines >= PARAM_MAX_MSG_LINES) {
				Put(usr, "<red>You already have entered too many lines\n\n");
				CURRENT_STATE(usr);
			} else
				MOV(usr, STATE_EDIT_TEXT);
			break;

		case 's':
		case 'S':
			Put(usr, "<white>Save\n");
			usr->runtime_flags &= ~RTF_BUSY_MAILING;
			POP(usr);				/* pop the abort_text() user exit */
			RET(usr);
			break;

		default:
			Put(usr, "\n");
			CURRENT_STATE(usr);
	}
	Return;
}

void state_abort_text(User *usr, char c) {
void (*abort_func)(void *, char);

	if (usr == NULL)
		return;

	Enter(state_abort_text);

	if (c == INIT_STATE) {
		Put(usr, "<cyan>Are you sure? <white>(<cyan>y<white>/<cyan>N<white>): ");
		Return;
	}
	switch(yesno(usr, c, 'N')) {
		case YESNO_YES:
			free_StringIO(usr->text);

			usr->runtime_flags &= ~RTF_BUSY_MAILING;
/*
	fiddle with stack ; remove the abort() and save() handlers, and jump to
	the abort() user exit function
*/
			POP(usr);							/* pop current */
			abort_func = usr->conn->callstack->ip;	/* abort func */
			POP(usr);							/* pop abort func */
			JMP(usr, abort_func);				/* overwrite save() address */
			break;

		case YESNO_NO:
			JMP(usr, STATE_SAVE_TEXT);
			break;

		case YESNO_UNDEF:
			CURRENT_STATE(usr);
			break;
	}
	Return;
}


void state_enter_msg_number(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_msg_number);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter message number<yellow>: ");

	r = edit_number(usr, c);

	if (r == EDIT_BREAK) {
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		unsigned long num;
		int idx;

		num = strtoul(usr->edit_buf, NULL, 10);
		if (num <= 0) {
			Put(usr, "<red>No such message\n");
			RET(usr);
			Return;
		}
		if (usr->curr_room->msgs == NULL)
			idx = usr->curr_room->msg_idx;
		else {
			for(idx = 0; idx < usr->curr_room->msg_idx; idx++) {
				if (usr->curr_room->msgs[idx] == num)
					break;

				if (usr->curr_room->msgs[idx] > num) {	/* we're not getting there anymore */
					idx = usr->curr_room->msg_idx;
					break;
				}
			}
		}
		if (idx >= usr->curr_room->msg_idx) {
			Put(usr, "<red>No such message\n");
			RET(usr);
			Return;
		}
		usr->curr_msg = idx;
		readMsg(usr);
	}
	Return;
}

void state_enter_minus_msg(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_enter_minus_msg);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter number of messages to read back<yellow>: ");

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
		Put(usr, "<green>Enter number of messages to skip<yellow>: ");

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


void readMsg(User *usr) {
char filename[MAX_PATHLEN], date_buf[MAX_LINE];
Joined *j;
unsigned long msg_number;

	if (usr == NULL)
		return;

	Enter(readMsg);

	if (usr->curr_msg < 0 || usr->curr_msg >= usr->curr_room->msg_idx || usr->curr_room->msgs == NULL) {
		Perror(usr, "The message you were attempting to read has all of a sudden vaporized");
		RET(usr);
		Return;
	}
	if ((j = in_Joined(usr->rooms, usr->curr_room->number)) == NULL) {
		Perror(usr, "Suddenly you haven't joined this room ?");
		RET(usr);
		Return;
	}
	msg_number = usr->curr_room->msgs[usr->curr_msg];
	if (msg_number > j->last_read)
		j->last_read = msg_number;

/* construct filename */
	if (usr->curr_room == usr->mail)
		sprintf(filename, "%s/%c/%s/%lu", PARAM_USERDIR, usr->name[0], usr->name, msg_number);
	else
		sprintf(filename, "%s/%u/%lu", PARAM_ROOMDIR, usr->curr_room->number, msg_number);
	path_strip(filename);

/* load the message */
	destroy_Message(usr->message);
	if ((usr->message = load_Message(filename, msg_number)) == NULL) {
		Perror(usr, "The message vaporizes as you attempt to read it");
		RET(usr);
		Return;
	}
	free_StringIO(usr->text);
	msg_header(usr, usr->message);

	if (usr->message->deleted != (time_t)0UL) {
		put_StringIO(usr->text, "\n");

		if (usr->message->flags & MSG_DELETED_BY_ANON)
			print_StringIO(usr->text, "<yellow>[<red>Deleted on <yellow>%s<red> by <cyan>- %s -<yellow>]\n",
				print_date(usr, usr->message->deleted, date_buf), usr->message->anon);
		else {
			char deleted_by[MAX_LINE];

			deleted_by[0] = 0;
			if (usr->message->flags & MSG_DELETED_BY_SYSOP)
				strcpy(deleted_by, PARAM_NAME_SYSOP);
			else
				if (usr->message->flags & MSG_DELETED_BY_ROOMAIDE)
					strcpy(deleted_by, PARAM_NAME_ROOMAIDE);

			if (*deleted_by)
				strcat(deleted_by, ": ");
			strcat(deleted_by, usr->message->deleted_by);

			print_StringIO(usr->text, "<yellow>[<red>Deleted on <yellow>%s<red> by <white>%s<yellow>]\n",
				print_date(usr, usr->message->deleted, date_buf), deleted_by);
		}
	} else {
		if (usr->curr_room->flags & ROOM_SUBJECTS) {		/* room has subject lines */
			if (usr->message->subject[0]) {
				if (usr->message->flags & MSG_FORWARDED)
					print_StringIO(usr->text, "<cyan>Subject: <white>Fwd:<yellow> %s\n", usr->message->subject);
				else
					if (usr->message->flags & MSG_REPLY)
						print_StringIO(usr->text, "<cyan>Subject: <white>Re:<yellow> %s\n", usr->message->subject);
					else
						print_StringIO(usr->text, "<cyan>Subject:<yellow> %s\n", usr->message->subject);
			} else {
/*
	Note: in Mail>, the reply_number is always 0 so this message is never displayed there
	which is actually correct, because the reply_number would be different to each
	recipient of the Mail> message
*/
				if (usr->message->flags & MSG_REPLY && usr->message->reply_number)
					print_StringIO(usr->text, "<cyan>Subject:<white> Re:<yellow> <message #%lu>\n", usr->message->reply_number);
			}
		} else {
			if ((usr->message->flags & MSG_FORWARDED) && usr->message->subject[0])
				print_StringIO(usr->text, "<white>Fwd:<yellow> %s\n", usr->message->subject);
			else {
				if (usr->message->flags & MSG_REPLY) {			/* room without subject lines */
					if (usr->message->subject[0])
						print_StringIO(usr->text, "<white>Re:<yellow> %s\n", usr->message->subject);
					else
						if (usr->message->reply_number)
							print_StringIO(usr->text, "<white>Re:<yellow> <message #%lu>\n", usr->message->reply_number);
				}
			}
		}
		put_StringIO(usr->text, "<green>\n");
		concat_StringIO(usr->text, usr->message->msg);

		usr->read++;						/* update stats */
		if (usr->read > stats.read) {
			stats.read = usr->read;
			strcpy(stats.most_read, usr->name);
		}
	}
	POP(usr);
	read_text(usr);
	Return;
}

void state_del_msg_prompt(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_del_msg_prompt);

	if (c == INIT_STATE) {
		Print(usr, "<cyan>Are you sure? <white>(<cyan>y<white>/<cyan>N<white>): ");
		usr->runtime_flags |= RTF_BUSY;
		Return;
	}
	r = yesno(usr, c, 'Y');
	if (r == YESNO_YES) {
		char buf[MAX_PATHLEN];

/* mark message as deleted */
		usr->message->deleted = rtc;
		strcpy(usr->message->deleted_by, usr->name);

		if (usr->runtime_flags & RTF_SYSOP)
			usr->message->flags |= MSG_DELETED_BY_SYSOP;
		else
			if (usr->runtime_flags & RTF_ROOMAIDE)
				usr->message->flags |= MSG_DELETED_BY_ROOMAIDE;
			else
				if (usr->message->anon[0])
					usr->message->flags |= MSG_DELETED_BY_ANON;

		if (usr->curr_room == usr->mail)
			sprintf(buf, "%s/%c/%s/%lu", PARAM_USERDIR, usr->name[0], usr->name, usr->message->number);
		else
			sprintf(buf, "%s/%u/%lu", PARAM_ROOMDIR, usr->curr_room->number, usr->message->number);
		path_strip(buf);

		if (save_Message(usr->message, buf)) {
			Perror(usr, "Failed to delete message");
		} else
			Put(usr, "<green>Message deleted\n\n");
	}
	if (r == YESNO_UNDEF) {
		CURRENT_STATE(usr);
		Return;
	}
	usr->runtime_flags &= ~RTF_BUSY;
	RET(usr);
	Return;
}

void undelete_msg(User *usr) {
char filename[MAX_PATHLEN];

	if (usr == NULL)
		return;

	Enter(undelete_msg);

	if (usr->message == NULL) {
		Put(usr, "<red>No message to undelete!\n");
		Return;
	}
	if (usr->message->deleted == (time_t)0UL) {
		Put(usr, "<red>Message has not been deleted, so you can't undelete it..!\n\n");
		Return;
	}
	if ((usr->curr_room != usr->mail
		&& ((usr->message->flags & MSG_DELETED_BY_SYSOP) && !(usr->runtime_flags & RTF_SYSOP)))
		|| ((usr->message->flags & MSG_DELETED_BY_ROOMAIDE) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))
		|| (strcmp(usr->message->deleted_by, usr->name) && !(usr->runtime_flags & (RTF_SYSOP | RTF_ROOMAIDE)))) {
		Put(usr, "<red>You are not allowed to undelete this message\n\n");
		Return;
	}
	usr->message->deleted_by[0] = 0;
	usr->message->deleted = (time_t)0UL;
	usr->message->flags &= ~(MSG_DELETED_BY_SYSOP | MSG_DELETED_BY_ROOMAIDE | MSG_DELETED_BY_ANON);

	if (usr->curr_room == usr->mail)
		sprintf(filename, "%s/%c/%s/%lu", PARAM_USERDIR, usr->name[0], usr->name, usr->message->number);
	else
		sprintf(filename, "%s/%u/%lu", PARAM_ROOMDIR, usr->curr_room->number, usr->message->number);
	path_strip(filename);

	if (save_Message(usr->message, filename)) {
		Perror(usr, "Failed to undelete message");
	} else
		Put(usr, "<green>Message undeleted\n");
	Return;
}

/*
	receive a message that was sent
	If busy, the message will be put in a buffer
*/
void recvMsg(User *usr, User *from, BufferedMsg *msg) {
BufferedMsg *new_msg;
char msg_type[MAX_LINE];
StringList *sl;

	if (usr == NULL || from == NULL || msg == NULL || msg->msg == NULL || msg->msg->buf == NULL)
		return;

	Enter(recvMsg);

	if (usr == from) {			/* talking to yourself is a no-no, because of kludgy code :P */
		Return;
	}
	if (usr->runtime_flags & RTF_LOCKED) {
		Print(from, "<red>Sorry, but <yellow>%s<red> suddenly locked the terminal\n", usr->name);
		goto Rcv_Remove_Recipient;
	}
	if (!(from->runtime_flags & RTF_SYSOP)) {
		if ((sl = in_StringList(usr->enemies, from->name)) != NULL) {
			Print(from, "<red>Sorry, but <yellow>%s<red> does not wish to receive any messages from you any longer\n", usr->name);
			goto Rcv_Remove_Recipient;
		}
		if ((usr->flags & USR_DENY_MULTI) && msg->to != NULL && msg->to->next != NULL) {
			Print(from, "<yellow>%s<green> doesn't wish to receive multi messages\n", usr->name);
			goto Rcv_Remove_Recipient;
		}
		if ((usr->flags & USR_X_DISABLED)
			&& (in_StringList(usr->friends, from->name) == NULL)) {
			Print(from, "<red>Sorry, but <yellow>%s<red> suddenly disabled message reception\n", usr->name);

Rcv_Remove_Recipient:
			if ((sl = in_StringList(from->recipients, usr->name)) != NULL) {
				remove_StringList(&from->recipients, sl);
				destroy_StringList(sl);
			}
			Return;
		}
	}
	if (msg->flags & BUFMSG_XMSG) {
		strcpy(msg_type, (!PARAM_HAVE_XMSG_HDR || msg->xmsg_header == NULL || !msg->xmsg_header[0]) ? "eXpress message" : msg->xmsg_header);

		if (from != usr) {
			usr->xrecv++;							/* update stats */
			if (usr->xrecv > stats.xrecv) {
				stats.xrecv = usr->xrecv;
				strcpy(stats.most_xrecv, usr->name);
			}
		}
	} else {
		if (msg->flags & BUFMSG_EMOTE) {
			strcpy(msg_type, "Emote");

			if (from != usr) {
				usr->erecv++;						/* update stats */
				if (usr->erecv > stats.erecv) {
					stats.erecv = usr->erecv;
					strcpy(stats.most_erecv, usr->name);
				}
			}
		} else {
			if (msg->flags & BUFMSG_FEELING) {
				strcpy(msg_type, "Feeling");

				if (from != usr) {
					usr->frecv++;					/* update stats */
					if (usr->frecv > stats.frecv) {
						stats.frecv = usr->frecv;
						strcpy(stats.most_frecv, usr->name);
					}
				}
			} else
				if (msg->flags & BUFMSG_QUESTION)
					strcpy(msg_type, "Question");
				else
					strcpy(msg_type, "Message");
		}
	}
	if ((usr->runtime_flags & RTF_BUSY) || (PARAM_HAVE_HOLD && (usr->runtime_flags & RTF_HOLD)))
		msg->flags &= ~BUFMSG_SEEN;
	else
		msg->flags |= BUFMSG_SEEN;

	if ((new_msg = copy_BufferedMsg(msg)) == NULL) {
		Perror(from, "Out of memory");
		Print(from, "<red>%s<red> was not received by <yellow>%s\n", msg_type, usr->name);
		Return;
	}
/*
	put the new copy of the message on the correct list
*/
	if (PARAM_HAVE_HOLD && (usr->runtime_flags & RTF_HOLD)) {
		if (!(msg->flags & ~BUFMSG_SEEN)) {		/* one-shot message */
			if (usr->runtime_flags & RTF_BUSY)
				add_BufferedMsg(&usr->held_msgs, new_msg);
			else
				add_BufferedMsg(&usr->history, new_msg);
		} else
			add_BufferedMsg(&usr->held_msgs, new_msg);
	} else
		if (usr->runtime_flags & RTF_BUSY)
			add_BufferedMsg(&usr->held_msgs, new_msg);
		else
			add_BufferedMsg(&usr->history, new_msg);

	if (usr->history_p != usr->history && list_Count(usr->history) > PARAM_MAX_HISTORY) {
		BufferedMsg *m;

		m = usr->history;						/* remove the head */
		usr->history = usr->history->next;
		usr->history->prev = NULL;
		destroy_BufferedMsg(m);
	}
	if ((usr->runtime_flags & RTF_BUSY) || (PARAM_HAVE_HOLD && (usr->runtime_flags & RTF_HOLD))) {
		if ((usr->runtime_flags & RTF_BUSY_SENDING)
			&& in_StringList(usr->recipients, from->name) != NULL)
/*
	warn follow-up mode by Richard of MatrixBBS
*/
			Print(from, "<yellow>%s<green> is busy sending you a message%s\n", usr->name,
				(PARAM_HAVE_FOLLOWUP && (usr->flags & USR_FOLLOWUP)) ? " in follow-up mode" : "");
		else
			if ((usr->runtime_flags & RTF_BUSY_MAILING)
				&& in_StringList(usr->recipients, from->name) != NULL)
				Print(from, "<yellow>%s<green> is busy mailing you a message\n", usr->name);
			else
				if (PARAM_HAVE_HOLD && (usr->runtime_flags & RTF_HOLD)) {
					if (usr->away != NULL && usr->away[0])
						Print(from, "<yellow>%s<green> has put messages on hold; %s\n", usr->name, usr->away);
					else
						Print(from, "<yellow>%s<green> has put messages on hold for a while\n", usr->name);
				} else
					Print(from, "<yellow>%s<green> is busy and will receive your %s<green> when done\n", usr->name, msg_type);
		Return;
	}
	Put(usr, "<beep>");									/* alarm beep */
	print_buffered_msg(usr, new_msg);
	Put(usr, "\n");

	Print(from, "<green>%s<green> received by <yellow>%s\n", msg_type, usr->name);

	if (in_StringList(from->talked_to, usr->name) == NULL)
		add_StringList(&from->talked_to, new_StringList(usr->name));

/* auto-reply if FOLLOWUP or if it was a Question */

	if ((PARAM_HAVE_FOLLOWUP && (usr->flags & USR_FOLLOWUP))
		|| ((msg->flags & BUFMSG_QUESTION) && (usr->flags & USR_HELPING_HAND))) {
		listdestroy_StringList(usr->recipients);
		if ((usr->recipients = new_StringList(msg->from)) == NULL) {
			Perror(usr, "Out of memory");
			Return;
		}
		do_reply_x(usr, msg->flags);
	}
	Return;
}

void spew_BufferedMsg(User *usr) {
BufferedMsg *m, *m_next;
int printed;

	if (usr == NULL)
		return;

	Enter(spew_BufferedMsg);

/*
	one-shot messages are received 'out of order'
	This is because they are system messages that you do not want to wait
	for until after you've gone through all the auto-reply Xs
*/
	printed = 0;
	for(m = usr->held_msgs; m != NULL; m = m_next) {
		m_next = m->next;

		if (!m->flags) {						/* one shot message */
			display_text(usr, m->msg);
			printed++;
			remove_BufferedMsg(&usr->held_msgs, m);
			destroy_BufferedMsg(m);				/* one-shots are not remembered */
		}
	}
	if (printed)
		Put(usr, "<white>");					/* do color correction */

	while(usr->held_msgs != NULL) {
		m = usr->held_msgs;
		remove_BufferedMsg(&usr->held_msgs, m);
		add_BufferedMsg(&usr->history, m);		/* remember this message */

		Put(usr, "<beep>");
		print_buffered_msg(usr, m);
		Put(usr, "\n");

/* auto-reply is follow up or question */
		if (strcmp(m->from, usr->name)
			&& ((PARAM_HAVE_FOLLOWUP && (usr->flags & USR_FOLLOWUP))
			|| ((m->flags & BUFMSG_QUESTION) && (usr->flags & USR_HELPING_HAND)))) {
			if (is_online(m->from) != NULL) {
				listdestroy_StringList(usr->recipients);
				usr->recipients = NULL;

				if ((usr->recipients = new_StringList(m->from)) == NULL) {
					Perror(usr, "Out of memory");
					Return;
				}
				do_reply_x(usr, m->flags);
				Return;
			} else
				Print(usr, "<red>Sorry, but <yellow>%s<red> left before you could reply!\n", m->from);
		}
	}
	Return;
}

/*
	Produce an X message header
	Note: buf must be large enough (PRINT_BUF in size)
*/
char *buffered_msg_header(User *usr, BufferedMsg *msg, char *buf) {
struct tm *tm;
char frombuf[256] = "", namebuf[256] = "", multi[8] = "", msgtype[MAX_LINE] = "";
int from_me = 0;

	if (usr == NULL || msg == NULL || buf == NULL)
		return NULL;

	Enter(buffered_msg_header);

	if (!msg->flags) {			/* one-shot message */
		*buf = 0;
		Return buf;
	}
	tm = user_time(usr, msg->mtime);
	if ((usr->flags & USR_12HRCLOCK) && tm->tm_hour > 12)
		tm->tm_hour -= 12;		/* use 12 hour clock, no 'military' time */

	if (!strcmp(msg->from, usr->name)) {
		from_me = 1;
		if (msg->to != NULL) {
			if (msg->to->next != NULL)
				strcpy(namebuf, "<many>");
			else {
				if (!strcmp(msg->to->str, usr->name))
					strcpy(namebuf, "yourself");
				else
					strcpy(namebuf, msg->to->str);
			}
		} else
			strcpy(namebuf, "<bug!>");
	}
	if (msg->flags & BUFMSG_SYSOP) {
		if (from_me)
			sprintf(frombuf, "as %s ", PARAM_NAME_SYSOP);
		else
			sprintf(frombuf, "<%s>%s<white>:<%s> %s", (msg->flags & BUFMSG_EMOTE) ? "cyan" : "yellow",
				PARAM_NAME_SYSOP, (msg->flags & BUFMSG_EMOTE) ? "cyan" : "yellow", msg->from);
	} else {
		if (!from_me)
			sprintf(frombuf, "<%s>%s", (msg->flags & BUFMSG_EMOTE) ? "cyan" : "yellow", msg->from);
	}
	if (msg->to != NULL && msg->to->next != NULL)
		strcpy(multi, "Multi ");

/* the message type */

	if (msg->flags & BUFMSG_XMSG)
		strcpy(msgtype, (!PARAM_HAVE_XMSG_HDR || msg->xmsg_header == NULL || !msg->xmsg_header[0]) ? "eXpress message" : msg->xmsg_header);
	else
		if (msg->flags & BUFMSG_EMOTE)
			strcpy(msgtype, "Emote");
		else
			if (msg->flags & BUFMSG_FEELING)
				strcpy(msgtype, "Feeling");
			else
				if (msg->flags & BUFMSG_QUESTION)
					strcpy(msgtype, "Question");

	if ((msg->flags & BUFMSG_EMOTE) && !from_me) {
		sprintf(buf, "<yellow>%c<white>%d<yellow>:<white>%02d<yellow>%c %s <yellow>", (multi[0] == 0) ? '(' : '[',
			tm->tm_hour, tm->tm_min, (multi[0] == 0) ? ')' : ']', frombuf);
	} else {
		if (msg->flags & (BUFMSG_XMSG | BUFMSG_EMOTE | BUFMSG_FEELING | BUFMSG_QUESTION)) {
			if (from_me)
				sprintf(buf, "<blue>*** <cyan>You sent this %s%s<cyan> to <yellow>%s<cyan> %sat <white>%02d<yellow>:<white>%02d <blue>***<yellow>\n", multi, msgtype, namebuf, frombuf, tm->tm_hour, tm->tm_min);
			else
				sprintf(buf, "<blue>*** <cyan>%s%s<cyan> received from %s<cyan> at <white>%02d<yellow>:<white>%02d <blue>***<yellow>\n", multi, msgtype, frombuf, tm->tm_hour, tm->tm_min);
		}
	}
	Return buf;
}

void print_buffered_msg(User *usr, BufferedMsg *msg) {
char buf[PRINT_BUF];

	if (usr == NULL || msg == NULL)
		return;

	Enter(print_buffered_msg);
/*
	update talked-to list
	we do it this late because you haven't really talked to anyone until you
	see the message they sent you
	(think about users that are in hold message mode)
*/
	if (msg->from[0] && strcmp(usr->name, msg->from) && in_StringList(usr->talked_to, msg->from) == NULL)
		add_StringList(&usr->talked_to, new_StringList(msg->from));

	Put(usr, "\n");
	Put(usr, buffered_msg_header(usr, msg, buf));
	display_text(usr, msg->msg);
	Return;
}

void state_history_prompt(User *usr, char c) {
StringList *sl;
char prompt[PRINT_BUF];

	if (usr == NULL)
		return;

	Enter(state_history_prompt);

	switch(c) {
		case INIT_STATE:
			usr->history_p = unwind_BufferedMsg(usr->history);

			while(usr->history_p != NULL) {
				if (usr->history_p->flags & (BUFMSG_XMSG | BUFMSG_EMOTE | BUFMSG_FEELING | BUFMSG_QUESTION))
					break;
				usr->history_p = usr->history_p->prev;
			}
			if (usr->history_p == NULL) {
				Put(usr, "<red>No messages have been sent yet\n");
				RET(usr);
				Return;
			}
			print_buffered_msg(usr, usr->history_p);

			usr->runtime_flags |= RTF_BUSY;
			break;

		case 'b':
		case 'B':
		case 'p':
		case 'P':
		case KEY_BS:
		case '-':
		case '_':
			if (c == 'p' || c == 'P')
				Put(usr, "Previous\n");
			else
				Put(usr, "Back\n");

			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			if (usr->history_p->prev == NULL)
				Put(usr, "<red>Can't go further back\n");
			else
				usr->history_p = usr->history_p->prev;
			print_buffered_msg(usr, usr->history_p);
			break;

		case 'f':
		case 'F':
		case 'n':
		case 'N':
		case ' ':
		case '+':
		case '=':
			if (c == 'f' || c == 'F')
				Put(usr, "Forward\n");
			else
				Put(usr, "Next\n");

			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			usr->history_p = usr->history_p->next;
			if (usr->history_p == NULL) {
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			print_buffered_msg(usr, usr->history_p);
			break;

		case 'r':
		case 'v':
			Put(usr, "Reply\n");
			goto History_Reply_Code;

		case 'R':
		case 'V':
		case 'A':
		case 'a':
			Put(usr, "Reply to All\n");

History_Reply_Code:
			if (usr->history_p == NULL) {
				Put(usr, "<red>No message to reply to\n");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			if (c == 'R' || c == 'V' || c == 'A' || c == 'a') {
				if ((usr->recipients = copy_StringList(usr->history_p->to)) == NULL) {
					Perror(usr, "Out of memory");

					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;
				}
			}
			if ((sl = in_StringList(usr->recipients, usr->history_p->from)) == NULL) {
				if ((sl = new_StringList(usr->history_p->from)) == NULL) {
					Perror(usr, "Out of memory");
				} else
					concat_StringList(&usr->recipients, sl);
			}
			check_recipients(usr);
			if (usr->recipients == NULL)
				break;

			usr->edit_pos = 0;
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_buf[0] = 0;

			if (usr->recipients->next == NULL && usr->recipients->prev == NULL)
				usr->runtime_flags &= ~RTF_MULTI;

			Print(usr, "\n<green>Replying to%s\n", print_many(usr, prompt));

			if (usr->history_p->flags & BUFMSG_EMOTE) {
				CALL(usr, STATE_EDIT_EMOTE);
			} else {
				CALL(usr, STATE_EDIT_X);
			}
			Return;

		case 'l':
		case 'L':
		case KEY_CTRL('L'):
			Put(usr, "List recipients\n");

			if (usr->history_p == NULL) {
				Perror(usr, "Your history buffer is gone");
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			if (usr->history_p->to != NULL && usr->history_p->to->next != NULL)
				Put(usr, "<magenta>Recipients are <yellow>");
			else
				Put(usr, "<magenta>The recipient is <yellow>");

			for(sl = usr->history_p->to; sl != NULL; sl = sl->next) {
				if (sl->next == NULL)
					Print(usr, "%s\n", sl->str);
				else
					Print(usr, "%s, ", sl->str);
			}
			break;

		case 's':
		case 'S':
		case 'q':
		case 'Q':
		case KEY_ESC:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			if (c == 'q' || c == 'Q')
				Put(usr, "Quit\n");
			else
				Put(usr, "Stop\n");
			usr->runtime_flags &= ~RTF_BUSY;
			RET(usr);
			Return;
	}
	if (usr->history_p == NULL) {
		usr->runtime_flags &= ~RTF_BUSY;
		RET(usr);
		Return;
	}
	prompt[0] = 0;
	if (usr->history_p->prev != NULL)
		strcat(prompt, "<hotkey>Back, ");
	if (usr->history_p->next != NULL)
		strcat(prompt, "<hotkey>Next, ");
	if (strcmp(usr->history_p->from, usr->name))
		strcat(prompt, "<hotkey>Reply, ");
	if (usr->history_p->to != NULL && usr->history_p->to->next != NULL)
		strcat(prompt, "reply <hotkey>All, <hotkey>List recipients, ");

	Print(usr, "\n<green>%s<hotkey>Stop<white>: ", prompt);
	Return;
}

/*
	for messages that were held
	pretty much the same function as above, but now for held_msgp
	instead of history_p
*/
void state_held_history_prompt(User *usr, char c) {
StringList *sl;
BufferedMsg *m, *m_next;
char prompt[PRINT_BUF];
int printed;

	if (usr == NULL)
		return;

	Enter(state_held_history_prompt);

/*
	spit out any received one-shot messages
	it is not as elegant, but you have to deal with them some time
*/
	printed = 0;
	for(m = usr->held_msgs; m != NULL; m = m_next) {
		m_next = m->next;
		if (!m->flags) {					/* one shot message */
			display_text(usr, m->msg);
			printed++;
			remove_BufferedMsg(&usr->held_msgs, m);
			destroy_BufferedMsg(m);			/* one-shots are not remembered */
		}
	}
	if (printed)
		Put(usr, "<white>");				/* do color correction */

	switch(c) {
		case INIT_STATE:
			if (usr->held_msgp == NULL)
				usr->held_msgp = usr->held_msgs;

			while(usr->held_msgp != NULL) {
				if (usr->held_msgp->flags & (BUFMSG_XMSG|BUFMSG_EMOTE|BUFMSG_FEELING|BUFMSG_QUESTION))
					break;
				else
					log_err("state_held_history_prompt(): BUG ! Got message with unchecked flags 0x%x", usr->held_msgp->flags);

				usr->held_msgp = usr->held_msgp->prev;
			}
			if (usr->held_msgp == NULL) {
				usr->runtime_flags &= ~RTF_BUSY;
				RET(usr);
				Return;
			}
			print_buffered_msg(usr, usr->held_msgp);

			usr->runtime_flags |= RTF_BUSY;
			break;

		case 'b':
		case 'B':
		case 'p':
		case 'P':
		case KEY_BS:
		case '-':
		case '_':
			if (c == 'p' || c == 'P')
				Put(usr, "Previous\n");
			else
				Put(usr, "Back\n");

			if (usr->held_msgp == NULL) {
				Perror(usr, "Your held messages buffer is gone");
				goto Exit_Held_History;
			}
			if (usr->held_msgp->prev == NULL)
				Put(usr, "<red>Can't go further back\n");
			else
				usr->held_msgp = usr->held_msgp->prev;
			print_buffered_msg(usr, usr->held_msgp);
			break;

		case 'f':
		case 'F':
		case 'n':
		case 'N':
		case ' ':
		case '+':
		case '=':
			if (c == 'f' || c == 'F')
				Put(usr, "Forward\n");
			else
				Put(usr, "Next\n");

			if (usr->held_msgp == NULL) {
				Perror(usr, "Your held messages buffer is gone");
				goto Exit_Held_History;
			}
			usr->held_msgp = usr->held_msgp->next;
			if (usr->held_msgp == NULL)
				goto Exit_Held_History;

			print_buffered_msg(usr, usr->held_msgp);
			break;

		case 'r':
		case 'v':
			Put(usr, "Reply\n");
			goto Held_History_Reply;

		case 'R':
		case 'V':
		case 'A':
		case 'a':
			Put(usr, "Reply to All\n");

Held_History_Reply:
			if (usr->held_msgp == NULL) {
				Put(usr, "<red>No message to reply to\n");
				goto Exit_Held_History;
			}
			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			if (c == 'R' || c == 'V' || c == 'A' || c == 'a') {
				if ((usr->recipients = copy_StringList(usr->held_msgp->to)) == NULL) {
					Perror(usr, "Out of memory");

					listdestroy_StringList(usr->recipients);
					usr->recipients = NULL;
				}
			}
			if ((sl = in_StringList(usr->recipients, usr->held_msgp->from)) == NULL) {
				if ((sl = new_StringList(usr->held_msgp->from)) == NULL) {
					Perror(usr, "Out of memory");
				} else
					concat_StringList(&usr->recipients, sl);
			}
			check_recipients(usr);
			if (usr->recipients == NULL)
				break;

			usr->edit_pos = 0;
			usr->runtime_flags |= RTF_BUSY;
			usr->edit_buf[0] = 0;

			if (usr->recipients->next == NULL && usr->recipients->prev == NULL)
				usr->runtime_flags &= ~RTF_MULTI;

			Print(usr, "\n<green>Replying to%s\n", print_many(usr, prompt));

			if (usr->held_msgp->flags & BUFMSG_EMOTE) {
				CALL(usr, STATE_EDIT_EMOTE);
			} else {
				CALL(usr, STATE_EDIT_X);
			}
			Return;

		case 'l':
		case 'L':
		case KEY_CTRL('L'):
			Put(usr, "List recipients\n");

			if (usr->held_msgp == NULL) {
				Perror(usr, "Your held messages buffer is gone");
				goto Exit_Held_History;
			}
			if (usr->held_msgp->to != NULL && usr->held_msgp->to->next != NULL)
				Put(usr, "<magenta>Recipients are <yellow>");
			else
				Put(usr, "<magenta>The recipient is <yellow>");

			for(sl = usr->held_msgp->to; sl != NULL; sl = sl->next) {
				if (sl->next == NULL)
					Print(usr, "%s\n", sl->str);
				else
					Print(usr, "%s, ", sl->str);
			}
			break;

		case 's':
		case 'S':
		case 'q':
		case 'Q':
		case KEY_ESC:
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
			if (c == 'q' || c == 'Q')
				Put(usr, "Quit\n");
			else
				Put(usr, "Stop\n");

Exit_Held_History:
			concat_BufferedMsg(&usr->history, usr->held_msgs);
			usr->held_msgs = usr->held_msgp = NULL;

			usr->runtime_flags &= ~(RTF_BUSY|RTF_HOLD);
			Free(usr->away);
			usr->away = NULL;

			if (usr->runtime_flags & RTF_WAS_HH) {
				usr->runtime_flags &= ~RTF_WAS_HH;
				usr->flags |= USR_HELPING_HAND;
			}
			RET(usr);
			Return;
	}
	if (usr->held_msgp == NULL) {
		usr->runtime_flags &= ~RTF_BUSY;
		RET(usr);
		Return;
	}
	prompt[0] = 0;
	if (usr->held_msgp->prev != NULL)
		strcat(prompt, "<hotkey>Back, ");
	if (usr->held_msgp->next != NULL)
		strcat(prompt, "<hotkey>Next, ");
	if (strcmp(usr->held_msgp->from, usr->name))
		strcat(prompt, "<hotkey>Reply, ");
	if (usr->held_msgp->to != NULL && usr->held_msgp->to->next != NULL)
		strcat(prompt, "reply <hotkey>All, <hotkey>List recipients, ");

	Print(usr, "\n<green>%s<hotkey>Stop<white>: ", prompt);
	Return;
}

void state_enter_forward_recipients(User *usr, char c) {
StringIO *tmp;

	if (usr == NULL)
		return;

	Enter(state_enter_forward_recipients);

	switch(edit_recipients(usr, c, multi_mail_access)) {
		case EDIT_BREAK:
			RET(usr);
			break;

		case EDIT_RETURN:
			if (usr->recipients == NULL) {
				RET(usr);
				break;
			}
			if (usr->new_message == NULL) {
				Perror(usr, "The message to forward has disappeared (?)");
				RET(usr);
				break;
			}
			usr->new_message->flags |= MSG_FORWARDED;
			usr->new_message->flags &= ~MSG_REPLY;

			listdestroy_StringList(usr->new_message->to);
			if ((usr->new_message->to = copy_StringList(usr->recipients)) == NULL) {
				Perror(usr, "Out of memory");
				RET(usr);
				break;
			}
			listdestroy_StringList(usr->recipients);
			usr->recipients = NULL;

			usr->new_message->mtime = rtc;

/* swap these pointers for save_message() */
			free_StringIO(usr->text);
			tmp = usr->text;
			usr->text = usr->new_message->msg;
			usr->new_message->msg = tmp;

			Put(usr, "<green>Message forwarded\n");

			save_message(usr, INIT_STATE);		/* send the forwarded Mail> */
/*			RET(usr);	save_message() already RET()s for us */
	}
	Return;
}

void state_forward_room(User *usr, char c) {
int r;

	if (usr == NULL)
		return;

	Enter(state_forward_room);

	if (c == INIT_STATE)
		Put(usr, "<green>Enter room name<yellow>: ");

	r = edit_roomname(usr, c);

	if (r == EDIT_BREAK) {
		Put(usr, "<red>Message not forwarded\n");
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		Room *r, *curr_room;
		StringIO *tmp;

		if (!usr->edit_buf[0]) {
			RET(usr);
			Return;
		}
		if ((r = find_abbrevRoom(usr, usr->edit_buf)) == NULL) {
			Put(usr, "<red>No such room\n");
			RET(usr);
			Return;
		}
		if (r->number == MAIL_ROOM) {
			unload_Room(r);
			r = usr->mail;
		} else {
			if (r == usr->curr_room) {
				Put(usr, "<red>The message already is in this room\n");
				RET(usr);
				Return;
			}
			if (r->flags & ROOM_CHATROOM) {
				Put(usr, "<red>You cannot forward a message to a <white>chat<red> room\n");
				unload_Room(r);
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
					case ACCESS_OK:
						break;
				}
			}
			if ((r->flags & ROOM_READONLY)
				&& !((usr->runtime_flags & RTF_SYSOP)
				|| (in_StringList(r->room_aides, usr->name) != NULL))) {
				Put(usr, "<red>You are not allowed to post in that room\n");
				unload_Room(r);
				RET(usr);
				Return;
			}
		}
		if (!usr->new_message->subject[0]) {
			char buf[MAX_LINE*3];

			sprintf(buf, "<yellow><forwarded from %s>", usr->curr_room->name);
			if (strlen(buf) >= MAX_LINE)
				strcpy(buf + MAX_LINE - 5, "...>");

			strcpy(usr->new_message->subject, buf);
		}
		usr->new_message->flags |= MSG_FORWARDED;
		usr->new_message->mtime = rtc;
		listdestroy_StringList(usr->new_message->to);
		usr->new_message->to = NULL;

		if (r == usr->mail) {
			char recipient[MAX_LINE+1], *p;
/*
	extract the username from the entered room name
*/
			strcpy(recipient, usr->edit_buf);
			if ((p = cstrrchr(recipient, ' ')) != NULL) {
				*p = 0;
				p--;
				if (*p == 's') {
					*p = 0;
					p--;
				}
				if (*p == '\'')
					*p = 0;

				usr->edit_buf[0] = 0;
			} else
				strcpy(recipient, usr->name);

			listdestroy_StringList(usr->message->to);
			usr->message->to = NULL;

			if (mail_access(usr, recipient)) {
				RET(usr);
				Return;
			}
			if ((usr->new_message->to = new_StringList(recipient)) == NULL) {
				Perror(usr, "Out of memory");
				unload_Room(r);
				RET(usr);
				Return;
			}
		}
/* swap these pointers for save_message() */
		free_StringIO(usr->text);
		tmp = usr->text;
		usr->text = usr->new_message->msg;
		usr->new_message->msg = tmp;

		PUSH(usr, STATE_DUMMY);			/* push dummy ret (prevent reprinting prompt in wrong room) */
		PUSH(usr, STATE_DUMMY);			/* one is not enough (!) */
		curr_room = usr->curr_room;
		usr->curr_room = r;
		save_message(usr, INIT_STATE);	/* save forwarded message in other room */
		usr->curr_room = curr_room;		/* restore current room */

		Put(usr, "<green>Message forwarded\n");

		unload_Room(r);
		RET(usr);
	}
	Return;
}

/*
	generally used after state_more_prompt()
*/
void state_press_any_key(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_press_any_key);

	if (c == INIT_STATE) {
		usr->runtime_flags |= RTF_BUSY;
		Put(usr, "<red>-- Press any key to continue --");
	} else {
		wipe_line(usr);
		RET(usr);
	}
	Return;
}

/*
	find the next unread room
	this is an unseemingly costly operation
*/
Room *next_unread_room(User *usr) {
Room *r, *r2, *next_joined = NULL;
Joined *j;

	if (usr == NULL)
		return Lobby_room;

	if (usr->curr_room == NULL) {
		Perror(usr, "Suddenly your environment goes up in flames. Please re-login (?)");
		return Lobby_room;
	}
	if (PARAM_HAVE_MAILROOM && usr->mail == NULL) {
		Perror(usr, "Your mailbox seems to have vaporized. Please re-login (?)");
		return Lobby_room;
	}
/*
	anything new in the Lobby?
*/
	if (unread_room(usr, Lobby_room) != NULL)
		return Lobby_room;

/* then check the mail room */

	if (PARAM_HAVE_MAILROOM) {
		if ((j = in_Joined(usr->rooms, MAIL_ROOM)) == NULL) {
			if ((j = new_Joined()) == NULL) {			/* this should never happen, but hey... */
				Perror(usr, "Out of memory");
				return Lobby_room;
			}
			j->number = MAIL_ROOM;
			if (usr->mail != NULL)
				j->generation = usr->mail->generation;
			add_Joined(&usr->rooms, j);
		}
		if (usr->mail->msgs != NULL && usr->mail->msg_idx > 0 && usr->mail->msgs[usr->mail->msg_idx-1] > j->last_read)
			return usr->mail;
	}
/*
	scan for next unread rooms
	we search the current room, and we proceed our search from there
*/
	for(r = AllRooms; r != NULL; r = r->next) {
		if (r->number == usr->curr_room->number)
			break;
	}
	if (r == NULL) {								/* current room does not exist anymore */
		Perror(usr, "Your environment has vaporized!");
		return Lobby_room;
	}
/* we've seen the current room, skip to the next and search for an unread room */

	for(r = r->next; r != NULL; r = r->next) {
		if (PARAM_HAVE_CYCLE_ROOMS && next_joined == NULL && joined_room(usr, r) != NULL)
			next_joined = r;

		if ((r2 = unread_room(usr, r)) != NULL)
			return r2;
	}
/* couldn't find a room, now search from the beginning up to curr_room */

	for(r = AllRooms; r != NULL && r->number != usr->curr_room->number; r = r->next) {
		if (PARAM_HAVE_CYCLE_ROOMS && next_joined == NULL && joined_room(usr, r) != NULL)
			next_joined = r;

		if ((r2 = unread_room(usr, r)) != NULL)
			return r2;
	}
/* couldn't find an unread room; goto next joined room */
	if (PARAM_HAVE_CYCLE_ROOMS && next_joined != NULL)
		return next_joined;

/* couldn't find an unread room; goto the Lobby> */
	return Lobby_room;
}

/*
	Checks if room r may be read and if it is unread
	This is a helper routine for next_unread_room()
*/
Room *unread_room(User *usr, Room *r) {
Joined *j;

	Enter(unread_room);

	if ((j = joined_room(usr, r)) == NULL) {
		Return NULL;
	}
	if (r->msgs != NULL && r->msg_idx > 0 && r->msgs[r->msg_idx-1] > j->last_read) {
		Return r;
	}
	Return NULL;
}

/*
	return if a room has been joined
*/
Joined *joined_room(User *usr, Room *r) {
Joined *j;

	if (usr == NULL || r == NULL || (r->flags & ROOM_CHATROOM))
		return NULL;

	Enter(joined_room);

/*
	NOTE: we skip room #1 and #2 (because they are 'dynamic' in the User
	      and Mail> is checked seperately
	      (remove this check and strange things happen... :P)
*/
	if (r->number == MAIL_ROOM || r->number == HOME_ROOM) {
		Return NULL;
	}
	j = in_Joined(usr->rooms, r->number);

/* if not welcome anymore, unjoin and proceed */

	if (in_StringList(r->kicked, usr->name) != NULL
		|| ((r->flags & ROOM_INVITE_ONLY) && in_StringList(r->invited, usr->name) == NULL)
		|| ((r->flags & ROOM_HIDDEN) && j == NULL)
		|| ((r->flags & ROOM_HIDDEN) && j != NULL && r->generation != j->generation)
		) {
		if (j != NULL) {
			remove_Joined(&usr->rooms, j);
			destroy_Joined(j);
		}
		Return NULL;
	}
	if (j != NULL) {
		if (j->zapped) {				/* Zapped this public room, forget it */
			if (j->generation == r->generation) {
				Return NULL;
			}
			j->zapped = 0;				/* auto-unzap changed room */
		}
	} else {
		if (!(r->flags & ROOM_HIDDEN)) {
			if ((j = new_Joined()) == NULL) {			/* auto-join public room */
				Perror(usr, "Out of memory, room not joined");
				Return NULL;
			}
			j->number = r->number;
			add_Joined(&usr->rooms, j);
		}
	}
	if (j->generation != r->generation) {
		j->generation = r->generation;
		j->last_read = 0UL;				/* re-read changed room */
	}
	Return j;
}

/*
	Send a message as Mail>
*/
void mail_msg(User *usr, BufferedMsg *msg) {
StringList *sl;
Room *room;
char buf[PRINT_BUF], c;

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
	strcpy(usr->new_message->from, usr->name);

	sl = usr->recipients;
	if (sl != NULL) {
		unsigned long i;

		for(i = 0UL; i < usr->conn->loop_counter; i++) {
			sl = sl->next;
			if (sl == NULL)
				break;
		}
	}
	if (sl == NULL) {
		Perror(usr, "The recipient has vanished!");
		Return;
	}
	if ((usr->new_message->to = new_StringList(sl->str)) == NULL) {
		Perror(usr, "Out of memory");
		Return;
	}
	strcpy(usr->new_message->subject, "<lost message>");

	free_StringIO(usr->text);
	put_StringIO(usr->text, "<cyan>Delivery of this message was impossible. You do get it this way.\n \n");
/*
	This is the most ugly hack ever; temporarely reset name to get a correct
	msg header out of buffered_msg_header();
	I really must rewrite this some day
*/
	c = usr->name[0];
	usr->name[0] = 0;
	buffered_msg_header(usr, usr->send_msg, buf);
	usr->name[0] = c;

	put_StringIO(usr->text, buf);
	concat_StringIO(usr->text, usr->send_msg->msg);

	room = usr->curr_room;
	usr->curr_room = usr->mail;

/* save the Mail> message */
	PUSH(usr, STATE_DUMMY);
	PUSH(usr, STATE_DUMMY);
	save_message(usr, INIT_STATE);

	destroy_Message(usr->new_message);
	usr->new_message = NULL;

	usr->curr_room = room;
	Return;
}

void room_beep(User *usr, Room *r) {
PList *p;
User *u;

	if (usr == NULL || r == NULL)
		return;

	Enter(room_beep);

	for(p = r->inside; p != NULL; p = p->next) {
		u = (User *)p->p;
		if (u == NULL || u == usr)
			continue;

		if (u->runtime_flags & RTF_BUSY)
			continue;

		if (u->flags & USR_ROOMBEEP)
			Put(u, "<beep>");
	}
	Return;
}

void msg_header(User *usr, Message *msg) {
char from[MAX_LINE], buf[MAX_LINE*3], date_buf[MAX_LINE];

	if (usr == NULL)
		return;

	Enter(msg_header);

	free_StringIO(usr->text);

	if (msg == NULL) {
		Perror(usr, "I have a problem with this");
		Return;
	}
/* print message header */

	if (msg->anon[0])
		sprintf(from, "<cyan>- %s <cyan>-", msg->anon);
	else
		if (msg->flags & MSG_FROM_SYSOP)
			sprintf(from, "<yellow>%s<white>:<yellow> %s", PARAM_NAME_SYSOP, msg->from);
		else
			if (msg->flags & MSG_FROM_ROOMAIDE)
				sprintf(from, "<yellow>%s<white>:<yellow> %s", PARAM_NAME_ROOMAIDE, msg->from);
			else
				sprintf(from, "<yellow>%s", msg->from);

	if (msg->to != NULL) {			/* in the Mail> room */
		StringList *sl;
		int l, dl, max_dl;			/* l = strlen, dl = display length */

		max_dl = usr->display->term_width-1;
		if (max_dl >= (MAX_LINE*3-1))	/* MAX_LINE*3 is used buffer size */
			max_dl = MAX_LINE*3-1;

		if (!strcmp(msg->from, usr->name) && !(msg->flags & (MSG_FROM_SYSOP|MSG_FROM_ROOMAIDE)) && !msg->anon[0]) {
			l = sprintf(buf, "<cyan>%s<green>, to ", print_date(usr, msg->mtime, date_buf));
			dl = color_strlen(buf);

			for(sl = msg->to; sl != NULL && sl->next != NULL; sl = sl->next) {
				if ((dl + strlen(sl->str)+2) < max_dl)
					l += sprintf(buf+l, "<yellow>%s<green>, ", sl->str);
				else {
					put_StringIO(usr->text, buf);
					write_StringIO(usr->text, "\n", 1);
					l = sprintf(buf, "<yellow>%s<green>, ", sl->str);
				}
				dl = color_strlen(buf);
			}
			print_StringIO(usr->text, "%s<yellow>%s<green>\n", buf, sl->str);
		} else {
			if (msg->to != NULL && msg->to->next == NULL && !strcmp(msg->to->str, usr->name))
				print_StringIO(usr->text, "<cyan>%s<green>, from %s<green>\n", print_date(usr, msg->mtime, date_buf), from);
			else {
				l = sprintf(buf, "<cyan>%s<green>, from %s<green> to ", print_date(usr, msg->mtime, date_buf), from);
				dl = color_strlen(buf);

				for(sl = msg->to; sl != NULL && sl->next != NULL; sl = sl->next) {
					if ((dl + strlen(sl->str)+2) < MAX_LINE)
						l += sprintf(buf+strlen(buf), "<yellow>%s<green>, ", sl->str);
					else {
						put_StringIO(usr->text, buf);
						put_StringIO(usr->text, "\n");
						l = sprintf(buf, "<yellow>%s<green>, ", sl->str);
					}
					dl = color_strlen(buf);
				}
				print_StringIO(usr->text, "%s<yellow>%s<green>\n", buf, sl->str);
			}
		}
	} else
		print_StringIO(usr->text, "<cyan>%s<green>, from %s<green>\n", print_date(usr, msg->mtime, date_buf), from);
	Return;
}

/*
	helper function for reading usr->text
	start is 0 for a full page, 1 for a page minus 1 line, etc.
*/
static void display_page(User *usr, int start) {
int l;

	for(l = start; l < usr->display->term_height && usr->scrollp != NULL; usr->scrollp = usr->scrollp->next) {
		usr->display->cpos = usr->display->line = 0;
		Out_text(usr->conn->output, usr, (char *)usr->scrollp->p, &usr->display->cpos, &usr->display->line, 1, 0);
		usr->read_lines++;
		l++;
	}
}

/*
	helper function for reading text
*/
static void goto_page_start(User *usr) {
int l;

	for(l = 0; l < usr->display->term_height && usr->scrollp != NULL; usr->scrollp = usr->scrollp->prev) {
		l++;
		usr->read_lines--;
	}
	if (usr->scrollp == NULL)
		usr->scrollp = usr->scroll;
}

void read_text(User *usr) {
int pos, len;

	Enter(read_text);

	pos = 0;
	seek_StringIO(usr->text, 0, STRINGIO_END);
	len = tell_StringIO(usr->text);

	listdestroy_PList(usr->scroll);
	usr->scroll = usr->scrollp = new_PList(usr->text->buf);
	if (usr->scroll == NULL) {
		Perror(usr, "Out of memory");
		RET(usr);
		Return;
	}
	while(pos < len) {
		usr->display->cpos = 0;
		usr->display->line = 0;
		pos += Out_text(NULL, usr, usr->text->buf+pos, &usr->display->cpos, &usr->display->line, 1, 0);
		usr->scroll = add_PList(&usr->scroll, new_PList(usr->text->buf+pos));
	}
	usr->scrollp = usr->scroll = rewind_PList(usr->scroll);
	usr->read_lines = 0;
	usr->total_lines = list_Count(usr->scroll) - 1;

	Put(usr, "\n");
	display_page(usr, 2);

	CALL(usr, STATE_SCROLL_TEXT);
	Return;
}

void state_scroll_text(User *usr, char c) {
int l, color;

	if (usr == NULL)
		return;

	Enter(state_scroll_text);

	wipe_line(usr);

	switch(c) {
		case INIT_STATE:
			if (usr->scroll == NULL) {
				RET(usr);
				Return;
			}
			usr->runtime_flags |= RTF_BUSY;
			break;

		case 'b':
		case 'B':
		case 'u':
		case 'U':
			for(l = 1; l < usr->display->term_height * 2; l++) {
				if (usr->scrollp->prev != NULL) {
					usr->scrollp = usr->scrollp->prev;
					if (usr->read_lines)
						usr->read_lines--;
				} else {
					if (l <= usr->display->term_height)
						usr->scrollp = NULL;	/* user that's keeping 'b' pressed */
					break;
				}
			}
			display_page(usr, 0);
			break;

		case ' ':
		case 'n':
		case 'N':
		case 'v':
		case 'V':
			display_page(usr, 1);
			break;

		case KEY_RETURN:
		case '+':
		case '=':
			usr->display->cpos = usr->display->line = 0;
			Out_text(usr->conn->output, usr, usr->scrollp->p, &usr->display->cpos, &usr->display->line, 1, 0);
			usr->scrollp = usr->scrollp->next;
			usr->read_lines++;
			break;

		case KEY_BS:
		case '-':
		case '_':
			for(l = 0; l < (usr->display->term_height+1); l++) {
				if (usr->scrollp->prev != NULL) {
					usr->scrollp = usr->scrollp->prev;
					if (usr->read_lines > 0)
						usr->read_lines--;
				} else
					break;
			}
			display_page(usr, 0);
			break;

		case 'g':						/* goto beginning */
			usr->scrollp = usr->scroll;
			usr->read_lines = 0;
			display_page(usr, 0);
			break;

		case 'G':						/* goto end ; display last page */
			if (usr->scrollp == NULL)
				break;

/* goto the end */
			for(; usr->scrollp != NULL && usr->scrollp->next != NULL; usr->scrollp = usr->scrollp->next)
				usr->read_lines++;

/* go one screen back */
			l = 0;
			while(usr->scrollp != NULL && usr->scrollp->prev != NULL && l < usr->display->term_height) {
				usr->scrollp = usr->scrollp->prev;
				l++;
			}
			usr->read_lines -= l;

			display_page(usr, 0);
			break;

		case '/':						/* find */
			if (usr->scrollp == NULL)
				break;

			CALL(usr, STATE_SCROLL_FIND_PROMPT);
			Return;

		case '?':						/* find backwards */
			if (usr->scrollp == NULL)
				break;

			CALL(usr, STATE_SCROLL_FINDBACK_PROMPT);
			Return;

		case 'q':
		case 'Q':
		case 's':
		case 'S':
		case KEY_CTRL('C'):
		case KEY_CTRL('D'):
		case KEY_ESC:
			usr->scrollp = NULL;
			break;

		default:
			color = usr->color;
			Put(usr, "<cyan>Press <white><<yellow>space<white>><cyan> for next page, <white><<yellow>b<white>><cyan> for previous page, <white><<yellow>enter<white>><cyan> for next line");
			restore_color(usr, color);
			Return;
	}
	if (usr->scrollp != NULL) {
		color = usr->color;
		Print(usr, "<white>--<yellow>More<white>-- (<cyan>line %d<white>/<cyan>%d %d<white>%%)", usr->read_lines, usr->total_lines,
			100 * usr->read_lines / usr->total_lines);
		restore_color(usr, color);
	} else {
/*
	Don't destroy in order to be able to reply to a message

		destroy_Message(usr->message);
		usr->message = NULL;
*/
		listdestroy_PList(usr->scroll);
		usr->scroll = usr->scrollp = NULL;
		free_StringIO(usr->text);
		usr->read_lines = usr->total_lines = 0;
		RET(usr);
	}
	Return;
}

/*
	helper function for state_scroll_find_prompt()
	We want to do a cstrstr(), but end on a newline character

	Like strcmp(), this function returns 0 on match, and 1 on no match
*/
static int line_search(char *line, char *search) {
int len;

	if (line == NULL || search == NULL || !*search)
		return 0;

	len = strlen(search);
	while(*line) {
		if (*line == '\n')
			break;

		if (*line == *search && !strncmp(line, search, len))
			return 0;

		line++;
	}
	return 1;
}

void state_scroll_find_prompt(User *usr, char c) {
PList *pl;
int r, l;

	if (usr == NULL)
		return;

	Enter(state_scroll_find_prompt);

	if (c == INIT_STATE)
		Put(usr, "<white>Find: ");

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		wipe_line(usr);
		goto_page_start(usr);

		Put(usr, "<green>");
		display_page(usr, 0);
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		wipe_line(usr);
		goto_page_start(usr);

		if (!usr->edit_buf[0]) {
			Put(usr, "<green>");
			display_page(usr, 0);
			RET(usr);
			Return;
		}
		l = usr->read_lines;
		for(pl = usr->scrollp; pl != NULL; pl = pl->next) {
			if (!line_search((char *)pl->p, usr->edit_buf)) {
				usr->scrollp = pl;
				usr->read_lines = l;
/*
	if on the last page, keep the --More-- prompt active
	this means we have to go back to the beginning of the last page
*/
				if (usr->read_lines > usr->total_lines - usr->display->term_height) {
					for(pl = usr->scrollp; pl != NULL && pl->next != NULL; pl = pl->next);
					usr->scrollp = pl;
					usr->read_lines = usr->total_lines;

					goto_page_start(usr);
				}
				Put(usr, "<green>");
				display_page(usr, 0);
				RET(usr);
				Return;
			}
			l++;
		}
		MOV(usr, STATE_SCROLL_TEXT);
		CALL(usr, STATE_SCROLL_TEXT_NOTFOUND);
	}
	Return;
}

void state_scroll_findback_prompt(User *usr, char c) {
PList *pl;
int r, l;

	if (usr == NULL)
		return;

	Enter(state_scroll_findback_prompt);

	if (c == INIT_STATE)
		Put(usr, "<white>Find (backwards): ");

	r = edit_line(usr, c);

	if (r == EDIT_BREAK) {
		wipe_line(usr);
		goto_page_start(usr);

		Put(usr, "<green>");
		display_page(usr, 0);
		RET(usr);
		Return;
	}
	if (r == EDIT_RETURN) {
		wipe_line(usr);
		goto_page_start(usr);

		if (!usr->edit_buf[0]) {
			Put(usr, "<green>");
			display_page(usr, 0);
			RET(usr);
			Return;
		}
		l = usr->read_lines;
		for(pl = usr->scrollp->prev; pl != NULL; pl = pl->prev) {
			l--;
			if (!line_search((char *)pl->p, usr->edit_buf)) {
				usr->scrollp = pl;
				usr->read_lines = l;

				Put(usr, "<green>");
				display_page(usr, 0);
				RET(usr);
				Return;
			}
		}
		MOV(usr, STATE_SCROLL_TEXT);
		CALL(usr, STATE_SCROLL_TEXT_NOTFOUND);
	}
	Return;
}

void state_scroll_text_notfound(User *usr, char c) {
	if (usr == NULL)
		return;

	Enter(state_scroll_text_notfound);

	if (c == INIT_STATE) {
		Put(usr, "<red>Not found");
		usr->runtime_flags |= RTF_BUSY;
		Return;
	}
	if (c == KEY_RETURN || c == ' ' || c == KEY_BS || c == KEY_CTRL('H')
		|| c == KEY_CTRL('C') || c == KEY_CTRL('D') || c == KEY_ESC) {
		wipe_line(usr);
		Put(usr, "<green>");
		display_page(usr, 0);
		RET(usr);
	} else
		Put(usr, "<beep>");

	Return;
}

/* EOB */
