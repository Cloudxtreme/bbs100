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
	state_room.h	WJ105
*/

#ifndef STATE_ROOM_H_WJ105
#define STATE_ROOM_H_WJ105	1

#include "User.h"

#define STATE_ROOM_PROMPT				state_room_prompt
#define STATE_JUMP_ROOM					state_jump_room
#define STATE_ZAP_PROMPT				state_zap_prompt
#define STATE_ZAPALL_PROMPT				state_zapall_prompt
#define STATE_ENTER_MSG_NUMBER			state_enter_msg_number
#define STATE_ENTER_MINUS_MSG			state_enter_minus_msg
#define STATE_ENTER_PLUS_MSG			state_enter_plus_msg

void state_room_prompt(User *, char);
void PrintPrompt(User *);
Joined *join_room(User *, Room *);
void unjoin_room(User *, Room *);
void state_jump_room(User *, char);
void state_zap_prompt(User *, char);
void state_zapall_prompt(User *, char);
void print_known_room(User *, Room *);
void known_rooms(User *);
void allknown_rooms(User *);
void room_info(User *);
void state_enter_msg_number(User *, char);
void state_enter_minus_msg(User *, char);
void state_enter_plus_msg(User *, char);
void goto_room(User *, Room *);
void enter_room(User *, Room *);
void leave_room(User *);
void enter_chatroom(User *);
void leave_chatroom(User *);
void chatroom_say(User *, char *);
void chatroom_tell(Room *, char *);
void chatroom_msg(Room *, char *);
void chatroom_tell_user(User *, char *);
void chatroom_recvMsg(User *, BufferedMsg *);

#endif	/* STATE_ROOM_H_WJ105 */

/* EOB */
