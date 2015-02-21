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
	Feelings.c	WJ100

	(this is all that is left of the once so large Feelings code)
*/

#include "config.h"
#include "Feeling.h"
#include "Param.h"
#include "mydirentry.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int feelings_generation = 0;	/* increased whenever Sysop changes something */

int init_Feelings(void) {
DIR *dirp;

	if ((dirp = opendir(PARAM_FEELINGSDIR)) == NULL)
		return -1;

	closedir(dirp);
	return 0;
}

/* EOB */
