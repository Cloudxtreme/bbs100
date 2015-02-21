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
	state_history.c	WJ105
*/

#ifndef STATE_HISTORY_H_WJ105
#define STATE_HISTORY_H_WJ105	1

#include "User.h"

#define STATE_HISTORY_PROMPT			state_history_prompt
#define STATE_HIST_MSG_NUMBER			state_hist_msg_number
#define STATE_HIST_MINUS				state_hist_minus
#define STATE_HIST_PLUS					state_hist_plus
#define STATE_HELD_HISTORY_PROMPT		state_held_history_prompt
#define STATE_HELD_MSG_NUMBER			state_held_msg_number
#define STATE_HELD_MINUS				state_held_minus
#define STATE_HELD_PLUS					state_held_plus

int history_prompt(User *);
void state_history_prompt(User *, char);
void state_hist_msg_number(User *, char);
void state_hist_minus(User *, char);
void state_hist_plus(User *, char);

int held_history_prompt(User *);
void state_held_history_prompt(User *, char);
void state_held_msg_number(User *, char);
void state_held_minus(User *, char);
void state_held_plus(User *, char);

void expire_history(User *);

#endif	/* STATE_HISTORY_H_WJ105 */

/* EOB */
