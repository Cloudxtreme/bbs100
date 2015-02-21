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
	AtomicFile.h	WJ102
*/

#ifndef ATOMICFILE_H_WJ102
#define ATOMICFILE_H_WJ102	1

#include <stdio.h>

typedef struct {
	FILE *f;
	char *filename;
} AtomicFile;

AtomicFile *new_AtomicFile(void);
void destroy_AtomicFile(AtomicFile *);

AtomicFile *openfile(char *filename, char *mode);
int closefile(AtomicFile *);
int cancelfile(AtomicFile *);

#endif	/* ATOMICFILE_H_WJ102 */

/* EOB */
