/*
    bbs100 1.2.1 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	OnlineUser.c	WJ100
*/

#include "config.h"
#include "OnlineUser.h"
#include "cstring.h"
#include "Memory.h"
#include "Types.h"
#include "Param.h"

#include <stdio.h>
#include <stdlib.h>

OnlineUser **online_users = NULL;
int online_users_size = 0;

int init_OnlineUser(void) {
	if (!PARAM_USERHASH_SIZE)
		PARAM_USERHASH_SIZE = DEFAULT_USERHASH_SIZE;

	if ((online_users = (OnlineUser **)Malloc(PARAM_USERHASH_SIZE * sizeof(OnlineUser *), TYPE_POINTER)) == NULL)
		return -1;

	online_users_size = PARAM_USERHASH_SIZE;
	return 0;
}

void deinit_OnlineUser() {
	Free(online_users);
	online_users = NULL;
	online_users_size = 0;
}

/*
	resizes hash to new PARAM_USERHASH_SIZE
*/
int resize_OnlineUser(void) {
int i, addr, old_size;
OnlineUser *ou, *ou_next, **new_online_users;

	if ((new_online_users = (OnlineUser **)Malloc(PARAM_USERHASH_SIZE * sizeof(OnlineUser *), TYPE_POINTER)) == NULL)
		return -1;

	old_size = online_users_size;
	online_users_size = PARAM_USERHASH_SIZE;

	for(i = 0; i < old_size; i++) {
		for(ou = online_users[i]; ou != NULL; ou = ou_next) {
			ou_next = ou->next;

			ou->prev = ou->next = NULL;
			if (ou->u == NULL || ou->u->name == NULL || ou->u->socket < 0
				|| (addr = hashaddr_OnlineUser(ou->u->name)) == -1)
				continue;

			add_List(&new_online_users[addr], ou);
		}
	}
	deinit_OnlineUser();
	online_users = new_online_users;
	online_users_size = PARAM_USERHASH_SIZE;
	return 0;
}


/*
	practically the same as the hashaddr function of the file cache
*/
int hashaddr_OnlineUser(char *username) {
char *p;
int addr, c;

	p = username;
	addr = *p;
	p++;
	while(*p) {
		c = *p - ' ';

		addr <<= 4;
		addr ^= c;

		p++;
	}
	addr %= online_users_size;
	if (addr < 0)
		addr = -addr;
	return addr;
}

OnlineUser *new_OnlineUser(void) {
	return (OnlineUser *)Malloc(sizeof(OnlineUser), TYPE_ONLINEUSER);
}

void destroy_OnlineUser(OnlineUser *u) {
	Free(u);
}

OnlineUser *add_OnlineUser(User *u) {
OnlineUser *ou;
int addr;

	if (u == NULL || !u->name[0] || u->socket <= 0
		|| (addr = hashaddr_OnlineUser(u->name)) == -1
		|| (ou = new_OnlineUser()) == NULL)
		return NULL;

	ou->u = u;
	return (OnlineUser *)add_List(&online_users[addr], ou);
}

void remove_OnlineUser(User *u) {
OnlineUser *ou;
int addr;

	if (u == NULL || !u->name[0] || u->socket <= 0
		|| (addr = hashaddr_OnlineUser(u->name)) == -1)
		return;

	for(ou = online_users[addr]; ou != NULL; ou = ou->next) {
		if (ou->u == NULL)
			continue;

		if (!strcmp(u->name, ou->u->name)) {
			remove_List(&online_users[addr], ou);
			destroy_OnlineUser(ou);
			return;
		}
	}
}


User *is_online(char *name) {
OnlineUser *ou;
int addr;

	if (name == NULL || !name[0]
		|| (addr = hashaddr_OnlineUser(name)) == -1)
		return NULL;

	for(ou = online_users[addr]; ou != NULL; ou = ou->next) {
		if (ou->u == NULL)
			continue;

		if (ou->u->socket > 0 && !strcmp(ou->u->name, name))
			return ou->u;
	}
	return NULL;
}

/* EOB */
