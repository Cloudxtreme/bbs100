/*
    bbs100 2.2 WJ105
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
	StringIO.h	WJ105
*/

#ifndef STRINGIO_H_WJ105
#define STRINGIO_H_WJ105	1

#define STRINGIO_MINSIZE	128
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

int read_StringIO(StringIO *, char *, int);
int write_StringIO(StringIO *, char *, int);
int tell_StringIO(StringIO *);
int rewind_StringIO(StringIO *);
int seek_StringIO(StringIO *, int, int);

int load_StringIO(StringIO *, char *);
int save_StringIO(StringIO *, char *);

int put_StringIO(StringIO *, char *);
int printf_StringIO(StringIO *, char *, ...);

void free_StringIO(StringIO *);

#endif	/* STRINGIO_H_WJ105 */

/* EOB */
