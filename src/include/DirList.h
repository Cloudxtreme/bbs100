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
	DirList.h	WJ105
*/

#ifndef DIRLIST_H_WJ105
#define DIRLIST_H_WJ105	1

#include "StringList.h"

#define IGNORE_SYMLINKS			1	/* listdir() does not list symlinks */
#define IGNORE_HIDDEN			2	/* listdir() does not list hidden files */
#define NO_SLASHES				4	/* don't append slashes to directories */
#define NO_DIRS					8	/* skip subdirectories */

typedef struct {
	char *name;
	StringQueue *list;
} DirList;

DirList *new_DirList(void);
void destroy_DirList(DirList *);

DirList *list_DirList(char *, int);
StringQueue *listdir(char *, int);

#endif	/* DIRLIST_H_WJ105 */

/* EOB */
