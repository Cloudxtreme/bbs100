/*
    bbs100 1.2.3 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	PList.c	WJ100

	List of pointers
*/

#include "config.h"
#include "PList.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PList *new_PList(void *v) {
PList *p;

	if ((p = (PList *)Malloc(sizeof(PList), TYPE_PLIST)) == NULL)
		return NULL;

	p->p = v;
	return p;
}

void destroy_PList(PList *p) {
	Free(p);
}

PList *in_PList(PList *p, void *v) {
	while(p != NULL) {
		if (p->p == v)
			return p;
		p = p->next;
	}
	return NULL;
}

/* EOB */
