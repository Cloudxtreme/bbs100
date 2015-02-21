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
	state_msg.h	WJ99
*/

#ifndef STATE_MSG_H_WJ99
#define STATE_MSG_H_WJ99 1

#include "User.h"
#include "BufferedMsg.h"

#define READ_DELETED	1

#define STATE_POST_AS_ANON				state_post_as_anon
#define STATE_ENTER_ANONYMOUS			state_enter_anonymous
#define STATE_ENTER_MAIL_RECIPIENTS		state_enter_mail_recipients
#define STATE_ENTER_SUBJECT				state_enter_subject
#define STATE_EDIT_TEXT					state_edit_text
#define STATE_SAVE_TEXT					state_save_text
#define STATE_ABORT_TEXT				state_abort_text
#define STATE_DELETE_MSG				state_delete_msg
#define STATE_UNDELETE_MSG				state_undelete_msg
#define STATE_ENTER_FORWARD_RECIPIENTS	state_enter_forward_recipients
#define STATE_FORWARD_ROOM				state_forward_room
#define STATE_RETURN_FORWARD			state_return_forward
#define STATE_PRESS_ANY_KEY				state_press_any_key
#define STATE_SCROLL_TEXT				state_scroll_text
#define STATE_SCROLL_FIND_PROMPT		state_scroll_find_prompt
#define STATE_SCROLL_FINDBACK_PROMPT	state_scroll_findback_prompt
#define STATE_SCROLL_TEXT_NOTFOUND		state_scroll_text_notfound
#define STATE_RETURN_MENU				state_return_menu
#define LOOP_SEND_MAIL					loop_send_mail
#define LOOP_DELETE_MAIL				loop_delete_mail
#define LOOP_UNDELETE_MAIL				loop_undelete_mail

void state_post_as_anon(User *, char);
void state_enter_anonymous(User *, char);
void state_enter_mail_recipients(User *, char);
void state_enter_subject(User *, char);
void state_edit_text(User *, char);
void state_save_text(User *, char);
void state_abort_text(User *, char);
void state_delete_msg(User *, char);
void state_undelete_msg(User *, char);
void state_enter_forward_recipients(User *, char);
void state_forward_room(User *, char);
void state_return_forward(User *, char);
void state_press_any_key(User *, char);

int set_mailto(Message *, StringQueue *);
void enter_message(User *);
void enter_the_message(User *);
void edit_text(User *, void (*)(User *, char), void (*)(User *, char));
void abort_message(User *, char);
void save_message(User *, char);
void save_message_room(User *, Room *);
void read_more(User *);
void readMsg(User *);
void read_message(User *, Message *, int);
void recvMsg(User *, User *, BufferedMsg *);
char *buffered_msg_header(User *, BufferedMsg *, char *, int);
void print_buffered_msg(User *, BufferedMsg *);
void spew_BufferedMsg(User *);
void expire_msg(Room *);
void expire_mail(User *);
Room *next_unread_room(User *);
Room *skip_room(User *);
Room *unread_room(User *, Room *);
Joined *joined_room(User *, Room *);
void room_beep(User *, Room *);
void msg_header(User *, Message *);
void print_subject(User *, Message *);
int setup_read_text(User *);
void read_text(User *);
void state_scroll_text(User *, char);
void state_scroll_find_prompt(User *, char);
void state_scroll_findback_prompt(User *, char);
void state_scroll_text_notfound(User *, char);
void read_menu(User *);
void state_return_menu(User *, char);
void loop_send_mail(User *, char);
void loop_delete_mail(User *, char);
void loop_undelete_mail(User *, char);

#endif	/* STATE_MSG_H_WJ99 */

/* EOB */
