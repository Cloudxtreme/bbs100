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
	OnlineUser.h	WJ100
*/

#ifndef ONLINEUSER_H_WJ100
#define ONLINEUSER_H_WJ100	1

#include "User.h"

int init_OnlineUser(void);

int add_OnlineUser(User *);
void remove_OnlineUser(User *);

User *is_online(char *);

#endif	/* ONLINEUSER_H_WJ100 */

/* EOB */
