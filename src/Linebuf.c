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
	Linebuf.c	WJ105
*/

#include "Linebuf.h"
#include "Types.h"
#include "Memory.h"
#include "defines.h"
#include "edit.h"

#include <stdio.h>
#include <stdlib.h>


Linebuf *new_Linebuf(void) {
Linebuf *lb;

	if ((lb = (Linebuf *)Malloc(sizeof(Linebuf), TYPE_LINEBUF)) == NULL)
		return NULL;

	if ((lb->buf = (char *)Malloc(MAX_INPUTBUF, TYPE_CHAR)) == NULL) {
		destroy_Linebuf(lb);
		return NULL;
	}
	lb->size = lb->max = MAX_INPUTBUF;
	lb->idx = 0;
	return lb;
}

void destroy_Linebuf(Linebuf *lb) {
	if (lb == NULL)
		return;

	Free(lb->buf);
	Free(lb);
}

void reset_Linebuf(Linebuf *lb) {
	if (lb == NULL || lb->buf == NULL)
		return;

	lb->idx = 0;
	lb->buf[0] = 0;
}

int input_Linebuf(Linebuf *lb, char c) {
	if (lb == NULL || lb->buf == NULL)
		return -1;

	switch(c) {
		case KEY_RETURN:
			return EDIT_RETURN;

		default:
			if (lb->idx < lb->size-1) {
				lb->buf[lb->idx++] = c;
				lb->buf[lb->idx] = 0;
			} else {
				if (lb->size < lb->max) {
					char *p;
					int size;

					size = lb->size + MAX_INPUTBUF;
					if (size > lb->max)
						size = lb->max;

					if ((p = (char *)Malloc(size, TYPE_CHAR)) == NULL)
						return -1;

					strcpy(p, lb->buf);
					Free(lb->buf);
					lb->buf = p;
					lb->size = size;

					if (lb->idx < lb->size-1) {
						lb->buf[lb->idx++] = c;
						lb->buf[lb->idx] = 0;
					}
				}
			}
	}
	return 0;
}

/* EOB */
