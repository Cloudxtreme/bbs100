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
	Types.h	WJ100
*/

#ifndef TYPES_H_WJ100
#define TYPES_H_WJ100		1

/*
	for strings, work with multiples of 16 bytes
	it is very important for the memory allocator code that
	sizeof(char) == 16 and not 1
*/
#define SIZE_CHAR			16

#define TYPE_CHAR			0
#define TYPE_INT			1
#define TYPE_LONG			2
#define TYPE_POINTER		3
#define TYPE_STRINGLIST		4
#define TYPE_PLIST			5
#define TYPE_CALLSTACK		6
#define TYPE_SYMBOLTABLE	7
#define TYPE_TIMER			8
#define TYPE_SIGNALVECTOR	9
#define TYPE_USER			10
#define TYPE_ROOM			11
#define TYPE_JOINED			12
#define TYPE_MESSAGE		13
#define TYPE_BUFFEREDMSG	14
#define TYPE_FILE			15
#define TYPE_WRAPPER		16
#define TYPE_CACHEDFILE		17
#define TYPE_ATOMICFILE		18
#define TYPE_TIMEZONE		19
#define TYPE_DST_TRANS		20
#define TYPE_TIMETYPE		21
#define TYPE_HASH			22
#define TYPE_CONN			23
#define TYPE_TELNET			24
#define TYPE_KVPAIR			25
#define TYPE_STRINGIO		26
#define TYPE_DISPLAY		27
#define TYPE_QUEUE			28
#define TYPE_MAILTO			29
#define TYPE_NEWUSERLOG		30
#define TYPE_DIRLIST		31
#define NUM_TYPES			32

typedef struct Typedef_tag Typedef;

struct Typedef_tag {
	char *type;
	int size;
};

extern Typedef Types_table[NUM_TYPES];

#endif	/* TYPES_H_WJ100 */

/* EOB */
