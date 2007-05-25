/*
    bbs100 3.2 WJ107
    Copyright (C) 2007  Walter de Jong <walter@heiho.net>

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
	access.h	WJ99
*/

#ifndef ACCESS_H_WJ99
#define ACCESS_H_WJ99 1

#include "User.h"
#include "Room.h"

#define ACCESS_HIDDEN		-3
#define ACCESS_INVITE_ONLY	-2
#define ACCESS_KICKED		-1
#define ACCESS_OK			0
#define ACCESS_INVITED		1

int multi_x_access(User *);
int multi_mail_access(User *);
int multi_ping_access(User *);
int room_access(Room *, char *);
int room_visible(User *, Room *);
int joined_visible(User *, Room *, Joined *);
int room_visible_username(Room *, char *, unsigned long);
int mail_access(User *, char *);

void check_recipients(User *);
int is_guest(char *);
int is_sysop(char *);

#endif	/* ACCESS_H_WJ99 */

/* EOB */
