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
	calloc.h	WJ105
*/

#ifndef CALLOC_H_WJ105
#define CALLOC_H_WJ105	1

#include <config.h>

#ifndef HAVE_CALLOC

#include "memset.h"

#include <stdlib.h>

void *calloc(size_t nmemb, size_t size) {
void *ptr;

	size *= nmemb;
	if ((ptr = malloc(size)) != NULL)
		memset(ptr, 0, size);
	return ptr;
}

#endif	/* HAVE_CALLOC */

#endif	/* CALLOC_H_WJ105 */

/* EOB */
