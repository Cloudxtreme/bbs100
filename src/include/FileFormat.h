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
	FileFormat.h	WJ103
*/

#ifndef FILEFORMAT_H_WJ103
#define FILEFORMAT_H_WJ103	1

#include "CachedFile.h"

/*
	macros that help when writing file save/load functions

	- assumes that f is the open file
	- buf is the keyword and also used for saving StringIO buffers
	- p is the value
	- ff1_continue controls continuing from within a 'do { } while(0)' loop

	FF1 is for FileFormat version 1
*/

#define FF1_ERROR			\
	do {					\
		Fclose(f);			\
		return -1;			\
	} while(0)

#define FF1_PARSE								\
	do {										\
		ff1_continue = 0;						\
		if ((p = cstrchr(buf, '=')) == NULL)	\
			FF1_ERROR;							\
		*p = 0;									\
		p++;									\
		if (!*p)								\
			ff1_continue = 1;					\
	} while(0)

#define FF1_LOAD_LEN(x,y,z)					\
	do {									\
		if (ff1_continue)					\
			break;							\
		if (!strcmp(buf, (x))) {			\
			if (!*p)						\
				(y)[0] = 0;					\
			else {							\
				if (strlen(p) >= (z))		\
					FF1_ERROR;				\
				strncpy((y), p, (z)-1);		\
				(y)[(z)-1] = 0;				\
			}								\
			ff1_continue = 1;				\
		}									\
	} while(0)

#define FF1_LOAD_DUP(x,y)							\
	do {											\
		if (ff1_continue)							\
			break;									\
		if (!strcmp(buf, (x))) {					\
			if (!*p) {								\
				Free((y));							\
				(y) = NULL;							\
			} else {								\
				if (strlen(p) > MAX_LINE)			\
					p[MAX_LINE] = 0;				\
				Free((y));							\
				if (((y) = cstrdup(p)) == NULL)		\
					FF1_ERROR;						\
			}										\
			ff1_continue = 1;						\
		}											\
	} while(0)

#define FF1_LOAD_ULONG(x,y)						\
	do {										\
		if (ff1_continue)						\
			break;								\
		if (!strcmp(buf, (x))) {				\
			if (!*p)							\
				(y) = 0UL;						\
			else								\
				(y) = strtoul(p, NULL, 10);		\
			ff1_continue = 1;					\
		}										\
	} while(0)

#define FF1_LOAD_HEX(x,y)									\
	do {													\
		if (ff1_continue)									\
			break;											\
		if (!strcmp(buf, (x))) {							\
			if (!*p)										\
				(y) = 0;									\
			else											\
				(y) = (unsigned int)strtoul(p, NULL, 16);	\
			ff1_continue = 1;								\
		}													\
	} while(0)

#define FF1_LOAD_INT(x,y)			\
	do {							\
		if (ff1_continue)			\
			break;					\
		if (!strcmp(buf, (x))) {	\
			if (!*p)				\
				(y) = 0;			\
			else					\
				(y) = atoi(p);		\
			ff1_continue = 1;		\
		}							\
	} while(0)

#define FF1_LOAD_UINT(x,y)									\
	do {													\
		if (ff1_continue)									\
			break;											\
		if (!strcmp(buf, (x))) {							\
			if (!*p)										\
				(y) = 0;									\
			else											\
				(y) = (unsigned int)strtoul(p, NULL, 10);	\
			ff1_continue = 1;								\
		}													\
	} while(0)

#define FF1_LOAD_STRINGLIST(x,y)								\
	do {														\
		if (ff1_continue)										\
			break;												\
		if (!strcmp(buf, (x))) {								\
			if (*p) {											\
				(y) = add_StringList(&(y), new_StringList(p));	\
				(y) = rewind_StringList((y));					\
			}													\
			ff1_continue = 1;									\
		}														\
	} while(0)

#define FF1_LOAD_STRINGQUEUE(x,y)								\
	do {														\
		if (ff1_continue)										\
			break;												\
		if (!strcmp(buf, (x))) {								\
			if (*p)												\
				(void)add_StringQueue((y), new_StringList(p));	\
			ff1_continue = 1;									\
		}														\
	} while(0)

#define FF1_LOAD_MAILTO(x,y)										\
	do {															\
		if (ff1_continue)											\
			break;													\
		if (!strcmp(buf, (x))) {									\
			if (*p) {												\
				if ((y) == NULL)									\
					(y) = new_MailToQueue();						\
				(void)add_MailToQueue((y), new_MailTo_from_str(p));	\
			}														\
			ff1_continue = 1;										\
		}															\
	} while(0)

#define FF1_LOAD_STRINGIO(x,y)										\
	do {															\
		if (ff1_continue)											\
			break;													\
		if (!strcmp(buf, (x))) {									\
			if (*p) {												\
				if ((y) == NULL && ((y) = new_StringIO()) == NULL)	\
					continue;										\
				put_StringIO((y), p);								\
				write_StringIO((y), "\n", 1);						\
			}														\
			ff1_continue = 1;										\
		}															\
	} while(0)

#define FF1_LOAD_USERLIST(x,y)												\
	do {																	\
		if (ff1_continue)													\
			break;															\
		if (!strcmp(buf, (x))) {											\
			if (*p && user_exists(p) && in_StringList((y), p) == NULL)		\
				(void)prepend_StringList(&(y), new_StringList(p));			\
			ff1_continue = 1;												\
		}																	\
	} while(0)

#define FF1_SKIP(x)					\
	do {							\
		if (ff1_continue)			\
			break;					\
		if (!strcmp(buf, (x)))		\
			ff1_continue = 1;		\
	} while(0)

#ifdef __FUNCTION__
#define FF1_LOAD_UNKNOWN			\
	if (ff1_continue)				\
		continue;					\
	else							\
		log_warn("%s(): unknown keyword '%s', ignored", __FUNCTION__, buf)
#else
#define FF1_LOAD_UNKNOWN			\
	if (ff1_continue)				\
		continue;					\
	else							\
		log_warn("unknown keyword '%s', ignored", buf)
#endif

/*
	save macros
*/

#define FF1_SAVE_VERSION	Fputs(f, "version=1")

/* this macro only saves non-null values */
#define FF1_SAVE_STR(x,y)							\
	do {											\
		if ((y) != NULL && (y)[0])					\
			Fprintf(f, "%s=%s", (x), (y));			\
	} while(0)

#define FF1_SAVE_STRINGLIST(x,y)					\
	do {											\
		(y) = rewind_StringList((y));				\
		for(sl = (y); sl != NULL; sl = sl->next)	\
			FF1_SAVE_STR((x), sl->str);				\
	} while(0)

#define FF1_SAVE_STRINGQUEUE(x,y)											\
	do {																	\
		if ((y) != NULL)													\
			for(sl = (StringList *)(y)->tail; sl != NULL; sl = sl->next)	\
				FF1_SAVE_STR((x), sl->str);									\
	} while(0)

#define FF1_SAVE_MAILTO(x,y)												\
	do {																	\
		if ((y) != NULL)													\
			for(to = (MailTo *)(y)->tail; to != NULL; to = to->next) {		\
				if (flags == SAVE_MAILTO) {									\
					if (to->number)											\
						Fprintf(f, "%s=%s|%lu", (x), to->name, to->number);	\
				} else														\
					Fprintf(f, "%s=%s", (x), to->name);						\
			}																\
	} while(0)

#define FF1_SAVE_STRINGIO(x,y)									\
	do {														\
		if ((y) != NULL) {										\
			seek_StringIO((y), 0, STRINGIO_SET);				\
			while(gets_StringIO((y), buf, PRINT_BUF) != NULL)	\
				FF1_SAVE_STR((x), buf);							\
		}														\
	} while(0)

int fileformat_version(File *);

#endif	/* FILEFORMAT_H_WJ103 */

/* EOB */
