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
	bufprintf.c	WJ105

	place-holder functions for snprintf() and vsnprintf(), which may not be
	present everywhere yet

	Do not call these function directly
	Use the defines bufprintf() and bufvprintf() instead
*/

#include "config.h"
#include "bufprintf.h"

#include <stdio.h>
#include <string.h>

#ifndef HAVE_VPRINTF
#error This package uses vprintf(), which you do not have
#endif


int bufprintf(char *buf, int buflen, char *fmt, ...) {
va_list args;

	va_start(args, fmt);
	return bufvprintf(buf, buflen, fmt, args);
}

int bufvprintf(char *buf, int buflen, char *fmt, va_list args) {
int ret;

	if (buflen <= 0)
		return 0;

#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, buflen, fmt, args);
	ret = strlen(buf);						/* <-- important! */
#else
	ret = vsprintf(buf, fmt, args);
#endif
	va_end(args);
	return ret;
}

/* EOB */
