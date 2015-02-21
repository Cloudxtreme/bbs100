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
	DirList.c	WJ105

	The DirList represents a directory listing
	It doesn't do much by itself, use listdir()
*/

#include "config.h"
#include "DirList.h"
#include "Memory.h"
#include "cstring.h"
#include "cstrerror.h"
#include "log.h"
#include "mydirentry.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

DirList *new_DirList(void) {
	return (DirList *)Malloc(sizeof(DirList), TYPE_DIRLIST);
}

void destroy_DirList(DirList *dl) {
	if (dl == NULL)
		return;

	Free(dl->name);
	destroy_StringQueue(dl->list);
	Free(dl);
}

/*
	dl->name will be an allocated copy of dirname
	dirname may be NULL, but then dl->name has to be set

	dl->list will have the directory listing if successful

	flags can be IGNORE_SYMLINKS|IGNORE_HIDDEN
*/
DirList *list_DirList(char *dirname, int flags) {
DirList *dl;

	if (dirname == NULL || !*dirname)
		return NULL;

	if ((dl = new_DirList()) == NULL)
		return NULL;

	if ((dl->name = cstrdup(dirname)) == NULL) {
		destroy_DirList(dl);
		return NULL;
	}
	if ((dl->list = listdir(dl->name, flags)) == NULL) {
		destroy_DirList(dl);
		return NULL;
	}
	return dl;
}

/*
	create a directory listing in a StringQueue
	flags can be IGNORE_SYMLINKS|IGNORE_HIDDEN|NO_SLASHES|NO_DIRS
*/
StringQueue *listdir(char *dirname, int flags) {
StringQueue *l;
StringList *sl;
DIR *dir;
struct dirent *entry;
char fullpath[MAX_PATHLEN], *path, errbuf[MAX_LINE];
struct stat statbuf;
int max, n;

	if (dirname == NULL || !*dirname)
		return NULL;

	if ((dir = opendir(dirname)) == NULL) {
		log_err("listdir(): opendir(%s) failed %s", dirname, cstrerror(errno, errbuf, MAX_LINE));
		return NULL;
	}
	if ((l = new_StringQueue()) == NULL) {
		closedir(dir);
		return NULL;
	}
	cstrcpy(fullpath, dirname, MAX_PATHLEN);
	max = strlen(fullpath);
	path = fullpath + max;
	*path = '/';
	path++;
	*path = 0;
	max = MAX_PATHLEN - max - 1;

	while((entry = readdir(dir)) != NULL) {
		if ((flags & IGNORE_HIDDEN) && entry->d_name[0] == '.')			/* skip hidden files */
			continue;

		cstrcpy(path, entry->d_name, max);
		if (lstat(fullpath, &statbuf) < 0) {
			log_err("listdir(): lstat(%s) failed: %s", fullpath, cstrerror(errno, errbuf, MAX_LINE));
			continue;
		}
		switch(statbuf.st_mode & S_IFMT) {
			case S_IFREG:
				break;

			case S_IFDIR:				/* for directories, append a slash */
				if (flags & NO_DIRS) {
					*path = 0;
					break;
				}
				if (!(flags & NO_SLASHES)) {
					n = strlen(path);
					path[n++] = '/';
					path[n] = 0;
				}
				break;

			case S_IFLNK:
				if (flags & IGNORE_SYMLINKS)
					*path = 0;
				break;

			default:					/* no fifo's, sockets, device files, etc. */
				*path = 0;
		}
		if (!*path)
			continue;

		if ((sl = new_StringList(path)) == NULL) {
			log_err("listdir(): out of memory for directory %s", dirname);
			break;
		}
		(void)add_StringQueue(l, sl);
	}
	closedir(dir);
	sort_StringQueue(l, alphasort_StringList);
	return l;
}


/* EOB */
