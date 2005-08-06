/*
    bbs100 3.0 WJ105
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
	cstrcpy.c	WJ105
*/

#include "config.h"
#include "cstrcpy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
	strcpy() with bounds check
*/
char *cstrcpy(char *dest, char *src, int buflen) {
	if (dest == NULL || src == NULL || buflen <= 0)
		return NULL;

	if (buflen < strlen(src)+1) {
		*dest = 0;
		return NULL;				/* don't copy half strings */
	}
	strcpy(dest, src);
	return dest;
}

/*
	strncpy() with bounds check
	also always terminates dest with a zero byte
*/
char *cstrncpy(char *dest, char *src, int n, int buflen) {
	if (dest == NULL || src == NULL || buflen <= 0)
		return NULL;

	if (n > buflen-1)
		n = buflen - 1;

	if (n <= 0)
		return dest;

	strncpy(dest, src, n);
	dest[n] = 0;
	return dest;
}

/*
	strcat() with bounds check

	buflen is the size of the entire buffer, starting from 'dest'
*/
char *cstrcat(char *dest, char *src, int buflen) {
int len;

	if (dest == NULL || src == NULL || buflen <= 0)
		return NULL;

	len = strlen(dest);
	return cstrcpy(dest+len, src, buflen - len);
}

/*
	strncat() with bounds check
	always terminates dest with a zero byte
*/
char *cstrncat(char *dest, char *src, int n, int buflen) {
int len;

	if (dest == NULL || src == NULL || buflen <= 0)
		return NULL;

	len = strlen(dest);
	return cstrncpy(dest+len, src, n, buflen - len);
}

/* EOB */
