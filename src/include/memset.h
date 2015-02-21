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
	memset.h	WJ100
*/

#ifndef MEMSET_H_WJ100
#define MEMSET_H_WJ100

#include <config.h>

#ifndef HAVE_MEMSET
#define HAVE_MEMSET 1

void *memset(void *s, int c, size_t n) {
	while(n > 0) {
		*((char *)s) = (char)c;
		n--;
	}
	return s;
}

#else	/* HAVE_MEMSET */

#include <string.h>

#endif	/* HAVE_MEMSET */

#endif	/* MEMSET_H_WJ100 */

/* EOB */
