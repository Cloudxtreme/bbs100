/*
    bbs100 1.2.2 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	FileFormat.h	WJ103
*/

#ifndef FILEFORMAT_H_WJ103
#define FILEFORMAT_H_WJ103	1

#include "CachedFile.h"
#include "log.h"

/*
	macros that help when writing file save/load functions

	assumptions:
	- f is the open file
	- buf is the keyword
	- p is the value

	FF1 is for FileFormat version 1
*/
#define FF1_ERROR	do {	\
		Fclose(f);			\
		return -1;			\
	} while(0)

#define FF1_PARSE	do {						\
		if ((p = cstrchr(buf, '=')) == NULL)	\
			FF1_ERROR;							\
		*p = 0;									\
		p++;									\
		if (!*p)								\
			continue;							\
	} while(0)

#define FF1_LOAD_LEN(x,y,z)		if (!strcmp(buf, (x))) {	\
		if (!*p)											\
			(y)[0] = 0;										\
		else {												\
			if (strlen(p) >= (z))							\
				FF1_ERROR;									\
			strncpy((y), p, (z)-1);							\
			(y)[(z)-1] = 0;									\
		}													\
		continue;											\
	}

#define FF1_LOAD_DUP(x,y)		if (!strcmp(buf, (x))) {	\
		if (!*p) {											\
			Free((y));										\
			(y) = NULL;										\
		} else {											\
			if (strlen(p) > MAX_LINE)						\
				p[MAX_LINE] = 0;							\
			Free((y));										\
			if (((y) = cstrdup(p)) == NULL)					\
				FF1_ERROR;									\
		}													\
		continue;											\
	}

#define FF1_LOAD_ULONG(x,y)		if (!strcmp(buf, (x))) {	\
		if (!*p)											\
			(unsigned long)(y) = 0UL;						\
		else												\
			(unsigned long)(y) = strtoul(p, NULL, 10);		\
		continue;											\
	}

#define FF1_LOAD_HEX(x,y)		if (!strcmp(buf, (x))) {	\
		if (!*p)											\
			(y) = 0;										\
		else												\
			(y) = (unsigned int)strtoul(p, NULL, 16);		\
		continue;											\
	}

#define FF1_LOAD_INT(x,y)		if (!strcmp(buf, (x))) {	\
		if (!*p)											\
			(y) = 0;										\
		else												\
			(y) = atoi(p);									\
		continue;											\
	}

#define FF1_LOAD_UINT(x,y)		if (!strcmp(buf, (x))) {	\
		if (!*p)											\
			(y) = 0;										\
		else												\
			(y) = (unsigned int)strtoul(p, NULL, 10);		\
		continue;											\
	}

#define FF1_LOAD_STRINGLIST(x,y)	if (!strcmp(buf, (x))) {	\
		if (*p) {												\
			(y) = add_StringList(&(y), new_StringList(p));		\
			(y) = rewind_StringList((y));						\
		}														\
		continue;												\
	}

#define FF1_LOAD_USERLIST(x,y)		if (!strcmp(buf, (x))) {			\
		if (*p && user_exists(p) && in_StringList((y), p) == NULL) {	\
			(y) = add_StringList(&(y), new_StringList(p));				\
			(y) = rewind_StringList((y));								\
		}																\
		continue;														\
	}

#define FF1_LOAD_UNKNOWN		log_err("%s(): unknown keyword '%s'", __FUNCTION__, buf);	\
								FF1_ERROR


/*
	save macros
*/

#define FF1_SAVE_VERSION	Fputs(f, "version=1")

/* this macro only saves non-null values */
#define FF1_SAVE_STR(x,y)	do {			\
		if ((y) != NULL && (y)[0])			\
			Fprintf(f, "%s=%s", (x), (y));	\
	} while(0)

#define FF1_SAVE_STRINGLIST(x,y)	do {			\
		(y) = rewind_StringList((y));				\
		for(sl = (y); sl != NULL; sl = sl->next)	\
			FF1_SAVE_STR((x), sl->str);				\
	} while(0)


int fileformat_version(File *);

#endif	/* FILEFORMAT_H_WJ103 */

/* EOB */
