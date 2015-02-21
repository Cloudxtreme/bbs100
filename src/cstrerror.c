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
	cstrerror.c	WJ105
*/

#include "config.h"
#include "cstrerror.h"
#include "cstring.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *cstrerror(int err, char *buf, int maxlen) {
	if (buf == NULL || maxlen < 0)
		return NULL;

#ifdef HAVE_STRERROR
	cstrcpy(buf, strerror(err), maxlen);
#else
	bufprintf(buf, maxlen, "error %d", err);
#endif

	return buf;
}

/* EOB */
