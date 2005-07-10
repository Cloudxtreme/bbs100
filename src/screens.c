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
	screens.c	WJ99
*/

#include "config.h"
#include "screens.h"
#include "CachedFile.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

StringList *login_screen = NULL, *logout_screen = NULL, *nologin_screen = NULL,
	*motd_screen = NULL, *crash_screen = NULL;


/*
	this function seems so simple, but note that it reads the file
	from the cache
*/
int load_screen(StringIO *s, char *filename) {
int err;
File *f;

	if (s == NULL || filename == NULL || !*filename || (f = Fopen(filename)) == NULL)
		return -1;

	err = copy_StringIO(s, f->data);

	Fclose(f);
	return err;
}

/* EOB */
