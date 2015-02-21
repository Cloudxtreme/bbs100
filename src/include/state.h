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
	state.h	WJ99
*/

#ifndef STATE_H_WJ99
#define STATE_H_WJ99 1

#include "User.h"

#define REPLY_X_ONE		0
#define REPLY_X_ALL		1

#define WHO_LIST_SHORT	0
#define WHO_LIST_LONG	1

#define PROFILE_LONG	0
#define PROFILE_SHORT	1

#define STATE_DUMMY						state_dummy
#define STATE_X_PROMPT					state_x_prompt
#define STATE_RECIPIENTS_ERR			state_recipients_err
#define STATE_EMOTE_PROMPT				state_emote_prompt
#define STATE_FEELINGS_PROMPT			state_feelings_prompt
#define STATE_PING_PROMPT				state_ping_prompt
#define STATE_SHORT_PROFILE				state_short_profile
#define STATE_LONG_PROFILE				state_long_profile
#define STATE_EDIT_EMOTE				state_edit_emote
#define STATE_EDIT_X					state_edit_x
#define STATE_EDIT_QUESTION				state_edit_question
#define STATE_EDIT_ANSWER				state_edit_answer
#define STATE_CHOOSE_FEELING			state_choose_feeling
#define STATE_MAIL_SEND_MSG				state_mail_send_msg
#define STATE_SU_PROMPT					state_su_prompt
#define STATE_LOCK_PASSWORD				state_lock_password
#define STATE_BOSS						state_boss
#define STATE_ASK_AWAY_REASON			state_ask_away_reason
#define STATE_DOWNLOAD_TEXT				state_download_text

#define LOOP_PING						loop_ping
#define LOOP_SEND_MSG					loop_send_msg
#define STATE_RETURN_MAIL_MSG			state_return_mail_msg

int fun_common(User *, char);
void state_dummy(User *, char);
void state_x_prompt(User *, char);
void state_recipients_err(User *, char);
void state_edit_x(User *, char);
void state_edit_question(User *, char);
void state_edit_answer(User *, char);
void state_edit_emote(User *, char);
void state_choose_feeling(User *, char);
void state_mail_send_msg(User *, char);
void state_ansi_prompt(User *, char);
void state_emote_prompt(User *, char);
void state_feelings_prompt(User *, char);
void state_ping_prompt(User *, char);
void print_profile(User *, int);
void state_short_profile(User *, char);
void state_long_profile(User *, char);
void state_su_prompt(User *, char);
void state_lock_password(User *, char);
void state_boss(User *, char);
int cmd_line(User *, char *);
void state_ask_away_reason(User *, char);

void loop_ping(User *, char);
void loop_send_msg(User *, char);
void mail_msg(User *, BufferedMsg *, char *);
void state_return_mail_msg(User *, char);

void print_version_info(User *);
void enter_recipients(User *, void (*)(User *, char));
void enter_name(User *, void (*)(User *, char));

int sort_who_asc_byname(void *, void *);
int sort_who_asc_bytime(void *, void *);
int sort_who_desc_byname(void *, void *);
int sort_who_desc_bytime(void *, void *);

void who_list(User *, int);
void who_list_status(User *, User *, char *, char *);
int long_who_list(User *, PQueue *);
int short_who_list(User *, PQueue *);
void who_list_header(User *, int, int);
void reply_x(User *, int);
void do_reply_x(User *, int);
void online_friends_list(User *);
void talked_list(User *);
void print_quicklist(User *);
void print_calendar(User *);
void drop_sysop_privs(User *);

void state_download_text(User *, char);
void print_reboot_status(User *);

#endif	/* STATE_H_WJ99 */

/* EOB */
