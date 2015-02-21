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
	Joined.c	WJ99

	the joined room
*/

#include "config.h"
#include "Joined.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

Joined *new_Joined(void) {
Joined *j;

	if ((j = (Joined *)Malloc(sizeof(Joined), TYPE_JOINED)) == NULL)
		return NULL;

	j->roominfo_read = (unsigned int)-1;		/* never read it */
	return j;
}

void destroy_Joined(Joined *j) {
	Free(j);
}

Joined *in_Joined(Joined *j, unsigned int n) {
	while(j != NULL) {
		if (j->number == n)
			break;

		j = j->next;
	}
	return j;
}

/* EOB */
