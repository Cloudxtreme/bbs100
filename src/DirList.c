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
int list_DirList(DirList *dl, char *dirname, int flags) {
	if (dl == NULL)
		return -1;

	if (dirname != NULL) {
		Free(dl->name);
		dl->name = cstrdup(dirname);
	}
	if (dl->name == NULL)
		return -1;

	if ((dl->list = listdir(dl->name, flags)) == NULL)
		return -1;

	return 0;
}

/*
	create a directory listing in a StringQueue
	flags can be IGNORE_SYMLINKS|IGNORE_HIDDEN
*/
StringQueue *listdir(char *dirname, int flags) {
StringQueue *l;
StringList *sl;
DIR *dir;
struct dirent *entry;
char fullpath[MAX_PATHLEN], *path;
struct stat statbuf;
int max, n;

	if (dirname == NULL || !*dirname)
		return NULL;

	if ((dir = opendir(dirname)) == NULL) {
		log_err("listdir(): opendir(%s) failed %s", dirname, cstrerror(errno));
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
			log_err("listdir(): lstat(%s) failed: %s", fullpath, cstrerror(errno));
			continue;
		}
		switch(statbuf.st_mode & S_IFMT) {
			case S_IFREG:
				break;

			case S_IFDIR:				/* for directories, append a slash */
				n = strlen(path);
				path[n++] = '/';
				path[n] = 0;
				break;

			case S_IFLNK:
				if (!(flags & IGNORE_SYMLINKS))
					break;
/* fall through */

			default:
				*path = 0;
		}
		if (!*path)
			continue;

		if ((sl = new_StringList(path)) == NULL) {
			log_err("listdir(): out of memory for directory %s", dirname);
			break;
		}
		add_StringQueue(l, sl);
	}
	closedir(dir);
	sort_StringQueue(l, alphasort_StringList);
	return l;
}


/* EOB */
