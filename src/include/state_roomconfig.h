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
	state_roomconfig.h	WJ99
*/

#ifndef STATE_ROOMCONFIG_H_WJ99
#define STATE_ROOMCONFIG_H_WJ99 1

#include "User.h"

#define STATE_ROOM_CONFIG_MENU	state_room_config_menu
#define STATE_CHOOSE_CATEGORY	state_choose_category
#define STATE_CHANGE_ROOMINFO	state_change_roominfo
#define STATE_INVITE_PROMPT		state_invite_prompt
#define STATE_KICKOUT_PROMPT	state_kickout_prompt
#define STATE_REMOVE_ALL_POSTS	state_remove_all_posts
#define STATE_DELETE_ROOM		state_delete_room

void state_room_config_menu(User *, char);
void state_choose_category(User *, char);
void state_change_roominfo(User *, char);
void state_edit_roominfo(User *, char);
void state_save_roominfo(User *, char);
void state_abort_roominfo(User *, char);
void state_invite_prompt(User *, char);
void state_kickout_prompt(User *, char);
void state_remove_all_posts(User *, char);
void state_delete_room(User *, char);

void save_roominfo(User *, char);
void abort_roominfo(User *, char);
void remove_all_posts(Room *);
void delete_room(User *, Room *);

#endif	/* STATE_ROOMAIDE_H_WJ99 */

/* EOB */
