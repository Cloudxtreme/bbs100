/*
    bbs100 2.1 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	state.h	WJ99
*/

#ifndef STATE_H_WJ99
#define STATE_H_WJ99 1

#include "User.h"

#define REPLY_X_ONE		0
#define REPLY_X_ALL		1

#define WHO_LIST_SHORT	0
#define WHO_LIST_LONG	1
#define WHO_LIST_ROOM	2

#define STATE_DUMMY						state_dummy
#define STATE_ROOM_PROMPT				state_room_prompt
#define STATE_X_PROMPT					state_x_prompt
#define STATE_RECIPIENTS_ERR			state_recipients_err
#define STATE_EMOTE_PROMPT				state_emote_prompt
#define STATE_FEELINGS_PROMPT			state_feelings_prompt
#define STATE_PING_PROMPT				state_ping_prompt
#define STATE_PROFILE_USER				state_profile_user
#define STATE_EDIT_EMOTE				state_edit_emote
#define STATE_EDIT_X					state_edit_x
#define STATE_EDIT_QUESTION				state_edit_question
#define STATE_CHOOSE_FEELING			state_choose_feeling
#define STATE_MAIL_SEND_MSG				state_mail_send_msg
#define STATE_JUMP_ROOM					state_jump_room
#define STATE_ZAP_PROMPT				state_zap_prompt
#define STATE_ZAPALL_PROMPT				state_zapall_prompt
#define STATE_SU_PROMPT					state_su_prompt
#define STATE_LOCK_PASSWORD				state_lock_password
#define STATE_ASSIGN_ROOMAIDE			state_assign_roomaide
#define STATE_CHANGE_ROOMNAME			state_change_roomname

#define LOOP_PING						loop_ping
#define LOOP_SEND_MSG					loop_send_msg

void state_dummy(User *, char);
void state_x_prompt(User *, char);
void state_recipients_err(User *, char);
void state_edit_x(User *, char);
void state_edit_question(User *, char);
void state_edit_emote(User *, char);
void state_choose_feeling(User *, char);
void state_mail_send_msg(User *, char);
void state_room_prompt(User *, char);
void state_ansi_prompt(User *, char);
void state_emote_prompt(User *, char);
void state_feelings_prompt(User *, char);
void state_ping_prompt(User *, char);
void state_jump_room(User *, char);
void state_zap_prompt(User *, char);
void state_zapall_prompt(User *, char);
void state_profile_user(User *, char);
void state_su_prompt(User *, char);
void state_lock_password(User *, char);
void state_assign_roomaide(User *, char);
void state_change_roomname(User *, char);

void loop_ping(User *, char);
void loop_send_msg(User *, char);

void PrintPrompt(User *);
void print_version_info(User *);
void enter_recipients(User *, void (*)(User *, char));
void enter_name(User *, void (*)(User *, char));

int sort_who_asc_byname(void *, void *);
int sort_who_asc_bytime(void *, void *);
int sort_who_desc_byname(void *, void *);
int sort_who_desc_bytime(void *, void *);

void who_list(User *, int);
int long_who_list(User *, PList *);
int short_who_list(User *, PList *);
void who_list_header(User *, int, int);

void print_known_room(User *, Room *);
void known_rooms(User *);
void allknown_rooms(User *);
void room_info(User *);
void reply_x(User *, int);
void do_reply_x(User *, int);
void online_friends_list(User *);
void talked_list(User *);
void show_namelist(User *, StringList *);
void print_quicklist(User *);
void stop_reading(User *);

void goto_room(User *, Room *);
void enter_room(User *, Room *);
void leave_room(User *);
void enter_chatroom(User *);
void leave_chatroom(User *);

void chatroom_say(User *, char *);
void chatroom_tell(Room *, char *);
void chatroom_msg(Room *, char *);

void print_calendar(User *);

#endif	/* STATE_H_WJ99 */

/* EOB */
