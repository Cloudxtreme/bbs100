/*
    bbs100 1.2.0 WJ102
    Copyright (C) 2002  Walter de Jong <walter@heiho.net>

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
	SignalVector.c	WJ99
*/

#include <config.h>

#include "SignalVector.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

SignalVector *new_SignalVector(void (*func)(int)) {
SignalVector *h;

	if ((h = (SignalVector *)Malloc(sizeof(SignalVector), TYPE_SIGNALVECTOR)) == NULL)
		return NULL;

	h->handler = func;
	return h;
}

void destroy_SignalVector(SignalVector *h) {
	Free(h);
}

/* EOB */
