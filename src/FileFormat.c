/*
    bbs100 1.2.3 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	FileFormat.c	WJ103
*/

#include "config.h"
#include "FileFormat.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fileformat_version(File *f) {
char buf[MAX_PATHLEN];

	if (f == NULL || Fgets(f, buf, MAX_PATHLEN) == NULL)
		return -1;

	cstrip_line(buf);
	if (strncmp(buf, "version=", 8))		/* no versioning in this file */
		return 0;

	return atoi(buf+8);						/* version number must be an integer */
}

/* EOB */
