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
	AtomicFile.c	WJ102

	Note: In the memory stats in the bbs you will usually see there is 1 atomic
	file allocated. This happens because the atomic file is put on the freelist
	of allocated objects. The number does not represent the number of open files.
*/

#include "config.h"
#include "AtomicFile.h"
#include "Memory.h"
#include "cstring.h"
#include "defines.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


AtomicFile *new_AtomicFile(void) {
AtomicFile *f;

	if ((f = (AtomicFile *)Malloc(sizeof(AtomicFile), TYPE_ATOMICFILE)) == NULL)
		return NULL;

	f->f = NULL;
	f->filename = NULL;
	return f;
}

void destroy_AtomicFile(AtomicFile *f) {
	if (f == NULL)
		return;

	Free(f->filename);
	Free(f);
}

/*
	if the file is opened for writing, first write to a temp file
*/
AtomicFile *openfile(char *filename, char *mode) {
AtomicFile *f;
char tmpfile[MAX_PATHLEN];

	if ((f = new_AtomicFile()) == NULL)
		return NULL;

	strcpy(tmpfile, filename);

	if (cstrchr(mode, 'w') != NULL || cstrchr(mode, 'a') != NULL) {
		if ((f->filename = cstrdup(filename)) == NULL) {
			destroy_AtomicFile(f);
			return NULL;
		}
		strcat(tmpfile, ".tmp");
	}
	if ((f->f = fopen(tmpfile, mode)) == NULL) {
		destroy_AtomicFile(f);
		return NULL;
	}
	return f;
}

/*
	if the file was open for writing (f->filename is set), move the temp file
	over the original file
*/
void closefile(AtomicFile *f) {
char tmpfile[MAX_PATHLEN];

	if (f == NULL)
		return;

	fclose(f->f);
	f->f = NULL;

	if (f->filename != NULL) {
		strcpy(tmpfile, f->filename);
		strcat(tmpfile, ".tmp");
		if (rename(tmpfile, f->filename) == -1)
			log_err("failed to rename %s to %s", tmpfile, f->filename);
	}
	destroy_AtomicFile(f);
}

/* EOB */
