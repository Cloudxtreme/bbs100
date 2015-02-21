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

#ifndef TELNET_H_WJ105
#define TELNET_H_WJ105	1

#include "Conn.h"

/*
	the value of 8 is ridiculously small, but OK for the BBS
	it only cares for NAWS bytes and the "USER" environment variable
*/
#define MAX_SUB_BUF			8

#define MAX_TERM			500

/* telnet states */
#define TS_DATA				0
#define TS_IAC				1
#define TS_ARG				2
#define TS_WILL				3
#define TS_DO				4
#define TS_NAWS				5
#define TS_NEW_ENVIRON		6
#define TS_NEW_ENVIRON_IS	7
#define TS_NEW_ENVIRON_VAR	8
#define TS_NEW_ENVIRON_VAL	9

typedef struct {
	int state, in_sub;
	int term_width, term_height;
	char in_sub_buf[MAX_SUB_BUF];
} Telnet;

Telnet *new_Telnet(void);
void destroy_Telnet(Telnet *);
int telnet_negotiations(Telnet *, Conn *, unsigned char, void (*)(Conn *, Telnet *));

#endif	/* TELNET_H_WJ105 */

/* EOB */
