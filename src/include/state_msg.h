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
	state_msg.h	WJ99
*/

#ifndef STATE_MSG_H_WJ99
#define STATE_MSG_H_WJ99 1

#include "User.h"
#include "BufferedMsg.h"

#define STATE_POST_AS_ANON				state_post_as_anon
#define STATE_ENTER_ANONYMOUS			state_enter_anonymous
#define STATE_ENTER_MAIL_RECIPIENTS		state_enter_mail_recipients
#define STATE_ENTER_SUBJECT				state_enter_subject
#define STATE_EDIT_TEXT					state_edit_text
#define STATE_SAVE_TEXT					state_save_text
#define STATE_ABORT_TEXT				state_abort_text
#define STATE_ENTER_MSG_NUMBER			state_enter_msg_number
#define STATE_ENTER_MINUS_MSG			state_enter_minus_msg
#define STATE_ENTER_PLUS_MSG			state_enter_plus_msg
#define STATE_DEL_MSG_PROMPT			state_del_msg_prompt
#define STATE_HISTORY_PROMPT			state_history_prompt
#define STATE_HELD_HISTORY_PROMPT		state_held_history_prompt
#define STATE_ENTER_FORWARD_RECIPIENTS	state_enter_forward_recipients
#define STATE_FORWARD_ROOM				state_forward_room
#define STATE_PRESS_ANY_KEY				state_press_any_key
#define STATE_SCROLL_TEXT				state_scroll_text
#define STATE_SCROLL_FIND_PROMPT		state_scroll_find_prompt
#define STATE_SCROLL_FINDBACK_PROMPT	state_scroll_findback_prompt
#define STATE_SCROLL_TEXT_NOTFOUND		state_scroll_text_notfound
#define STATE_RETURN_MENU				state_return_menu

void state_post_as_anon(User *, char);
void state_enter_anonymous(User *, char);
void state_enter_mail_recipients(User *, char);
void state_enter_subject(User *, char);
void state_edit_text(User *, char);
void state_save_text(User *, char);
void state_abort_text(User *, char);
void state_enter_msg_number(User *, char);
void state_enter_minus_msg(User *, char);
void state_enter_plus_msg(User *, char);
void state_del_msg_prompt(User *, char);
void state_history_prompt(User *, char);
void state_held_history_prompt(User *, char);
void state_enter_forward_recipients(User *, char);
void state_forward_room(User *, char);
void state_press_any_key(User *, char);

void enter_message(User *);
void enter_the_message(User *);
void edit_text(User *, void (*)(User *, char), void (*)(User *, char));
void save_message(User *, char);
void abort_message(User *, char);
void read_more(User *);
void readMsg(User *);
void recvMsg(User *, User *, BufferedMsg *);
char *buffered_msg_header(User *, BufferedMsg *, char *);
void print_buffered_msg(User *, BufferedMsg *);
void spew_BufferedMsg(User *);
void expire_msg(Room *);
void expire_mail(User *);
void undelete_msg(User *);
Room *next_unread_room(User *);
Room *unread_room(User *, Room *);
Joined *joined_room(User *, Room *);
void mail_msg(User *, BufferedMsg *);
void room_beep(User *, Room *);
void msg_header(User *, Message *);
int setup_read_text(User *);
void read_text(User *);
void state_scroll_text(User *, char);
void state_scroll_find_prompt(User *, char);
void state_scroll_findback_prompt(User *, char);
void state_scroll_text_notfound(User *, char);
void read_menu(User *);
void state_return_menu(User *, char);

#endif	/* STATE_MSG_H_WJ99 */

/* EOB */
