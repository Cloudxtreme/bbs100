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

#ifndef STATE_FRIENDLIST_H_WJ99
#define STATE_FRIENDLIST_H_WJ99 1

#include "User.h"

#define STATE_FRIENDLIST_PROMPT			state_friendlist_prompt
#define STATE_ADD_FRIEND				state_add_friend
#define STATE_REMOVE_FRIEND				state_remove_friend
#define STATE_ENEMYLIST_PROMPT			state_enemylist_prompt
#define STATE_ADD_ENEMY					state_add_enemy
#define STATE_REMOVE_ENEMY				state_remove_enemy
#define STATE_OVERRIDE_MENU				state_override_menu
#define STATE_ADD_OVERRIDE				state_add_override
#define STATE_REMOVE_OVERRIDE			state_remove_override

void state_friendlist_prompt(User *, char);
void state_add_friend(User *, char);
void state_remove_friend(User *, char);
void state_enemylist_prompt(User *, char);
void state_add_enemy(User *, char);
void state_remove_enemy(User *, char);
void state_override_menu(User *, char);
void state_add_override(User *, char);
void state_remove_override(User *, char);

#endif	/* STATE_FRIENDLIST_H_WJ99 */

/* EOB */
