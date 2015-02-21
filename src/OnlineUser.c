/*
    bbs100 2.1 WJ104
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
	OnlineUser.c	WJ103

	- rewritten to work with a Hash type
*/

#include "config.h"
#include "OnlineUser.h"
#include "Hash.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

static Hash *online_users = NULL;

int init_OnlineUser(void) {
	if ((online_users = new_Hash()) == NULL)
		return -1;

	online_users->hashaddr = hashaddr_ascii;
	return 0;
}

void deinit_OnlineUser(void) {
	destroy_Hash(online_users);
	online_users = NULL;
}


int add_OnlineUser(User *u) {
	if (u == NULL || !u->name[0])
		return -1;

	return add_Hash(online_users, u->name, u, NULL);
}

void remove_OnlineUser(User *u) {
	if (u == NULL || !u->name[0])
		return;

	remove_Hash(online_users, u->name);
}

User *is_online(char *name) {
User *u;

	if (name == NULL || !*name)
		return NULL;

	if ((u = (User *)in_Hash(online_users, name)) != NULL)
		return u;

	return NULL;
}

/* EOB */
