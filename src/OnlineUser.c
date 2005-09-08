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
	OnlineUser.c	WJ103
*/

#include "config.h"
#include "OnlineUser.h"
#include "defines.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static OnlineUser *online_users = NULL;

static int cmp_online_online(void *, void *);
static int cmp_online_user(void *, void *);
static int cmp_online_name(void *, void *);

static OnlineUser *new_OnlineUser(User *usr) {
OnlineUser *o;

	if (usr == NULL || (o = (OnlineUser *)Malloc(sizeof(OnlineUser), TYPE_ONLINEUSER)) == NULL)
		return NULL;

	o->usr = usr;
	return o;
}

static void destroy_OnlineUser(OnlineUser *o) {
	Free(o);
}


int add_OnlineUser(User *u) {
OnlineUser *o;

	if (u == NULL || !u->name[0] || (o = new_OnlineUser(u)) == NULL)
		return -1;

	if (add_Tree(&online_users, o, cmp_online_online) == NULL) {
		destroy_OnlineUser(o);
		return -1;
	}
	return 0;
}

void remove_OnlineUser(User *u) {
OnlineUser *o;

	if (u == NULL || !u->name[0])
		return;

	if ((o = (OnlineUser *)remove_Tree(&online_users, u, cmp_online_user)) == NULL)
		return;

	destroy_OnlineUser(o);
}

User *is_online(char *name) {
OnlineUser *o;

	if (name == NULL || !*name)
		return NULL;

	if ((o = (OnlineUser *)in_Tree(online_users, name, cmp_online_name)) == NULL)
		return NULL;

	return o->usr;
}


static int cmp_online_online(void *v1, void *v2) {
OnlineUser *o1, *o2;

	o1 = (OnlineUser *)v1;
	o2 = (OnlineUser *)v2;

	return strcmp(o1->usr->name, o2->usr->name);
}

static int cmp_online_user(void *v1, void *v2) {
OnlineUser *o;
User *u;

	o = (OnlineUser *)v1;
	u = (User *)v2;

	return strcmp(o->usr->name, u->name);
}

static int cmp_online_name(void *v1, void *v2) {
OnlineUser *o;
char *name;

	o = (OnlineUser *)v1;
	name = (char *)v2;

	return strcmp(o->usr->name, name);
}

/* EOB */
