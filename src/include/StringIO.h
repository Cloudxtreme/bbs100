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
	StringIO.h	WJ105
*/

#ifndef STRINGIO_H_WJ105
#define STRINGIO_H_WJ105	1

#include <stdarg.h>

#define STRINGIO_MINSIZE	256
#define STRINGIO_BLKSIZE	1024

#define STRINGIO_SET		0
#define STRINGIO_CUR		1
#define STRINGIO_END		2

typedef struct {
	int len, size, pos;
	char *buf;
} StringIO;


StringIO *new_StringIO(void);
void destroy_StringIO(StringIO *);

int init_StringIO(StringIO *, int);
int grow_StringIO(StringIO *);
int trunc_StringIO(StringIO *, int);
int shift_StringIO(StringIO *, int);
int read_StringIO(StringIO *, char *, int);
int write_StringIO(StringIO *, char *, int);
int tell_StringIO(StringIO *);
int rewind_StringIO(StringIO *);
int seek_StringIO(StringIO *, int, int);
int copy_StringIO(StringIO *, StringIO *);
int concat_StringIO(StringIO *, StringIO *);

int load_StringIO(StringIO *, char *);
int save_StringIO(StringIO *, char *);

char *gets_StringIO(StringIO *, char *, int);
int put_StringIO(StringIO *, char *);
int vprint_StringIO(StringIO *, char *, va_list);
int print_StringIO(StringIO *, char *, ...);

void free_StringIO(StringIO *);

#endif	/* STRINGIO_H_WJ105 */

/* EOB */
