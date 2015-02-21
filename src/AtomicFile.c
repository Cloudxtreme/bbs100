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
	AtomicFile.c	WJ102

	Note: In the memory stats in the bbs you will usually see there is 1
	atomic file allocated. This happens because the atomic file is put on
	the freelist of allocated objects. The number does not represent the
	number of open files. If you turn off object caching, the number of
	atomic files in the memory stats should always be zero.
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
#include <sys/stat.h>
#include <sys/types.h>


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

	cstrcpy(tmpfile, filename, MAX_PATHLEN);

	if (cstrchr(mode, 'w') != NULL || cstrchr(mode, 'a') != NULL) {
		if ((f->filename = cstrdup(filename)) == NULL) {
			destroy_AtomicFile(f);
			return NULL;
		}
		cstrcat(tmpfile, ".tmp", MAX_PATHLEN);
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
int closefile(AtomicFile *f) {
	if (f == NULL)
		return -1;

	fclose(f->f);
	f->f = NULL;

	if (f->filename != NULL) {
		char tmpfile[MAX_PATHLEN];
		struct stat statbuf;

		cstrcpy(tmpfile, f->filename, MAX_PATHLEN);
		cstrcat(tmpfile, ".tmp", MAX_PATHLEN);

		if (lstat(tmpfile, &statbuf)) {
			log_err("closefile(): failed to stat file %s", tmpfile);
			destroy_AtomicFile(f);
			return -1;
		}
		if (statbuf.st_size <= (off_t)0L) {
			log_err("closefile(): file %s has zero size", tmpfile);
			unlink(tmpfile);
			destroy_AtomicFile(f);
			return -1;
		}
		if (rename(tmpfile, f->filename) == -1) {
			log_err("closefile(): failed to rename %s to %s", tmpfile, f->filename);
			unlink(tmpfile);
			destroy_AtomicFile(f);
			return -1;
		}
	}
	destroy_AtomicFile(f);
	return 0;
}

int cancelfile(AtomicFile *f) {
	if (f == NULL)
		return -1;

	fclose(f->f);
	f->f = NULL;

	if (f->filename != NULL) {
		char tmpfile[MAX_PATHLEN];

		cstrcpy(tmpfile, f->filename, MAX_PATHLEN);
		cstrcat(tmpfile, ".tmp", MAX_PATHLEN);

		unlink(tmpfile);
	}
	destroy_AtomicFile(f);
	return 0;
}

/* EOB */
