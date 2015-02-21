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
	state_login.h	WJ99
*/

#ifndef STATE_LOGIN_H_WJ99
#define STATE_LOGIN_H_WJ99 1

#include "User.h"

#define MAX_LOGIN_ATTEMPTS	3

#define STATE_LOGIN_PROMPT				state_login_prompt
#define STATE_PASSWORD_PROMPT			state_password_prompt
#define STATE_LOGOUT_PROMPT				state_logout_prompt
#define STATE_NEW_ACCOUNT_YESNO			state_new_account_yesno
#define STATE_NEW_LOGIN_PROMPT			state_new_login_prompt
#define STATE_NEW_PASSWORD_PROMPT		state_new_password_prompt
#define STATE_NEW_PASSWORD_AGAIN		state_new_password_again
#define STATE_ANSI_PROMPT				state_ansi_prompt
#define STATE_DISPLAY_MOTD				state_display_motd
#define STATE_GO_ONLINE					state_go_online

void state_login_prompt(User *, char);
void state_password_prompt(User *, char);
void state_logout_prompt(User *, char);
void state_new_account_yesno(User *, char);
void state_new_login_prompt(User *, char);
void state_new_password_prompt(User *, char);
void state_new_password_again(User *, char);
void state_ansi_terminal(User *, char);
void state_display_motd(User *, char);
void state_go_online(User *, char);

void mail_lost_msg(User *, BufferedMsg *, User *);
int print_user_status(User *);

extern StringList *banished;

#endif	/* STATE_LOGIN_H_WJ99 */

/* EOB */
