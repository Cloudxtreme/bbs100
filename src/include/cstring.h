/*
    bbs100 2.1 WJ104
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
	Chatter18	WJ97
	cstring.h

	Linux doesn't have stricmp() and strnicmp()... *real deep sigh*
	So we make our own cstricmp() and cstrnicmp()
*/

#ifndef _CSTRING_H_WJ97
#define _CSTRING_H_WJ97 1

#include <config.h>

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
char *cstrlwr(char *);
char *cstrupr(char *);
char *cstristr(char *, char *);
char *cstrstr(char *, char *);
char *cstrichr(char *, char);

void chop(char *);
void cstrip_line(char *);
void cstrip_spaces(char *);
void cstrip_filename(char *);
char **cstrsplit(char *, char);
char *cstrjoin(char **);

#endif /* _CSTRING_H_WJ97 */

/* EOB */
