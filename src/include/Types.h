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
	Types.h	WJ100
*/

#ifndef TYPES_H_WJ100
#define TYPES_H_WJ100		1

#define TYPE_CHAR			0
#define TYPE_POINTER		1
#define TYPE_STRINGLIST		2
#define TYPE_PLIST			3
#define TYPE_CALLSTACK		4
#define TYPE_SYMBOLTABLE	5
#define TYPE_TIMER			6
#define TYPE_SIGNALVECTOR	7
#define TYPE_USER			8
#define TYPE_ROOM			9
#define TYPE_JOINED			10
#define TYPE_MESSAGE		11
#define TYPE_MSGINDEX		12
#define TYPE_BUFFEREDMSG	13
#define TYPE_FILE			14
#define TYPE_WRAPPER		15
#define TYPE_CACHEDFILE		16
#define TYPE_ATOMICFILE		17
#define TYPE_SU_PASSWD		18
#define TYPE_TIMEZONE		19
#define TYPE_DST_TRANS		20
#define TYPE_TIMETYPE		21
#define TYPE_HASH			22
#define TYPE_LANG			23
#define TYPE_CONN			24
#define TYPE_TELNET			25
#define TYPE_LINEBUF		26
#define TYPE_KVPAIR			27
#define NUM_TYPES			28

typedef struct Typedef_tag Typedef;

struct Typedef_tag {
	char *type;
	int size;
};

extern Typedef Types_table[NUM_TYPES+1];

#endif	/* TYPES_H_WJ100 */

/* EOB */
