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
	MsgIndex.c	WJ99
*/

#include <config.h>

#include "MsgIndex.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

MsgIndex *new_MsgIndex(unsigned long n) {
MsgIndex *m;

	if ((m = (MsgIndex *)Malloc(sizeof(MsgIndex), TYPE_MSGINDEX)) == NULL)
		return NULL;

	m->number = n;
	return m;
}

void destroy_MsgIndex(MsgIndex *m) {
	if (m == NULL)
		return;

	Free(m);
}

/* EOB */
