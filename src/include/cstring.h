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
	Chatter18	WJ97
	cstring.h
*/

#ifndef _CSTRING_H_WJ97
#define _CSTRING_H_WJ97 1

#include "config.h"
#include "cstrcpy.h"
#include "defines.h"
#include "memset.h"

#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_STRCHR
#define cstrchr(x,y)		strchr(x,y)
#else
#define cstrchr(x,y)		index((x), (y))
#endif

#ifdef HAVE_STRRCHR
#define cstrrchr(x,y)		strrchr(x,y)
#else
#define cstrrchr(x,y)		rindex((x), (y))
#endif

#define cstricmp(x,y)		strcasecmp((x), (y))
#define cstrnicmp(x,y,z)	strncasecmp((x), (y), (z))

#define ctoupper(x)			((((x) >= 'a') && ((x) <= 'z')) ? ((x) - ' ') : (x))
#define ctolower(x)			((((x) >= 'A') && ((x) <= 'Z')) ? ((x) + ' ') : (x))

char *cstrdup(char *);
void cstrfree(char *);
char *cstrlwr(char *);
char *cstrupr(char *);
char *cstristr(char *, char *);
char *cstrstr(char *, char *);
char *cstrichr(char *, char);

void chop(char *);
void cstrip_line(char *);
void ctrim_line(char *);
void cstrip_spaces(char *);
char **cstrsplit(char *, char);
char *cstrjoin(char **);

int is_numeric(char *);
int is_hexadecimal(char *);
int is_octal(char *);

unsigned long cstrtoul(char *, int);

int cstrmatch_char(char, char);
int cstrmatch(char *, char *);

#endif /* _CSTRING_H_WJ97 */

/* EOB */
