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
	helper.c	WJ105
*/

#include "config.h"
#include "helper.h"
#include "cstring.h"
#include "User.h"
#include "Param.h"
#include "Memory.h"
#include "memset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PQueue helpers;		/* FIFO of helpers, with LRU behaviour */

int init_helper(void) {
	memset(&helpers, 0, sizeof(PQueue));
	deinit_PQueue(&helpers);
	return 0;
}

void add_helper(User *usr) {
	if (usr == NULL || !(usr->flags & USR_HELPING_HAND) || !PARAM_HAVE_QUESTIONS)
		return;

	if (usr->total_time / SECS_IN_DAY < PARAM_HELPER_AGE) {
		usr->flags &= ~USR_HELPING_HAND;
		return;
	}
	(void)add_PQueue(&helpers, new_PList(usr));
}

void remove_helper(User *usr) {
PList *pl;

	if (usr == NULL)
		return;

	if ((pl = in_PList((PList *)helpers.tail, usr)) != NULL) {
		(void)remove_PQueue(&helpers, pl);
		destroy_PList(pl);
	}
}

/*
	select the next helper to ask a question
	if we previously called for help, go back to the same helper (if available)

	helpers are selected in an LRU manner

	silent can be GH_SILENT, which means to be silent
*/
User *get_helper(User *usr, int silent) {
User *u;
PList *pl;
char instead[24] = "";

	if (usr == NULL || !PARAM_HAVE_QUESTIONS)
		return NULL;

	if (usr->question_asked != NULL) {
		for(pl = (PList *)helpers.tail; pl != NULL; pl = pl->next) {
			u = (User *)pl->p;
			if (!strcmp(u->name, usr->name))
				continue;

			if (!strcmp(usr->question_asked, u->name)) {
				(void)remove_PQueue(&helpers, pl);

				if (!(u->flags & USR_HELPING_HAND) || u->total_time / SECS_IN_DAY < PARAM_HELPER_AGE) {
					u->flags &= ~USR_HELPING_HAND;
					destroy_PList(pl);
					break;
				}
				(void)add_PQueue(&helpers, pl);

				if (!silent)
					Print(usr, "<green>The question goes to <yellow>%s\n", usr->question_asked);
				return u;
			}
		}
		Print(usr, "<yellow>%s<red> is no longer available to help you\n", usr->question_asked);
		Free(usr->question_asked);
		usr->question_asked = NULL;

		strcpy(instead, " <green>instead");
	}
	while(count_Queue(&helpers) > 0) {
		if ((pl = pop_PQueue(&helpers)) == NULL) {
			Put(usr, "<red>Sorry, but currently there is no one available to help you\n");
			return NULL;
		}
		u = (User *)pl->p;
		if (!u->name[0] || !(u->flags & USR_HELPING_HAND) || u->total_time / SECS_IN_DAY < PARAM_HELPER_AGE) {
			u->flags &= ~USR_HELPING_HAND;
			destroy_PList(pl);
			continue;
		}
		(void)add_PQueue(&helpers, pl);

		if (!strcmp(u->name, usr->name)) {
			if (count_Queue(&helpers) > 1)
				continue;

			Put(usr, "<red>Sorry, but currently there is no one available to help you\n");
			return NULL;
		}
		Free(usr->question_asked);
		usr->question_asked = cstrdup(u->name);

		if (!silent || instead[0])
			Print(usr, "<green>The question goes to <yellow>%s%s\n", usr->question_asked, instead);
		return u;
	}
	Put(usr, "<red>Sorry, but currently there is no one available to help you\n");
	return NULL;
}

/* EOB */
