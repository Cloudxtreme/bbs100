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

void state_friendlist_prompt(User *, char);
void state_add_friend(User *, char);
void state_remove_friend(User *, char);
void state_enemylist_prompt(User *, char);
void state_add_enemy(User *, char);
void state_remove_enemy(User *, char);

#endif	/* STATE_FRIENDLIST_H_WJ99 */

/* EOB */
