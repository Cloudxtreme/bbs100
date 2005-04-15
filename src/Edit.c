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
	Edit.c	WJ105
*/

#include "Edit.h"
#include "Types.h"
#include "Memory.h"
#include "defines.h"
#include "edit.h"

#include <stdio.h>
#include <stdlib.h>


Edit *new_Edit(void) {
Edit *e;

	if ((e = (Edit *)Malloc(sizeof(Edit), TYPE_EDIT)) == NULL)
		return NULL;

	if ((e->buf = (char *)Malloc(MAX_LINE, TYPE_CHAR)) == NULL) {
		destroy_Edit(e);
		return NULL;
	}
	e->size = e->max = MAX_LINE;
	e->idx = 0;
	return e;
}

void destroy_Edit(Edit *e) {
	if (e == NULL)
		return;

	Free(e->buf);
	Free(e);
}

void reset_edit(Edit *e) {
	if (e == NULL || e->buf == NULL)
		return;

	e->idx = 0;
	e->buf[0] = 0;
}

int edit_input(Edit *e, char c) {
	if (e == NULL || e->buf == NULL)
		return -1;

	switch(c) {
		case KEY_RETURN:
			return EDIT_RETURN;

		default:
			if (e->idx < e->size-1) {
				e->buf[e->idx++] = c;
				e->buf[e->idx] = 0;
			}
	}
	return 0;
}

/* EOB */
