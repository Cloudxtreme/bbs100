/*
    bbs100 1.2.2 WJ103
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
	strerror.h	WJ99
*/

#ifndef STRERROR_H_WJ99
#define STRERROR_H_WJ99 1

#include <config.h>

#ifndef HAVE_STRERROR
#define HAVE_STRERROR 1

#include <stdio.h>
#include <errno.h>

/* Note: returns a static buffer */
char *strerror(int err) {
char buf[64];

	sprintf(buf, "errno == %d\n", errno);
	return buf;
}

#endif

#endif	/* STRERROR_H_WJ99 */

/* EOB */

