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
	Chatter18	WJ97
	inet.h
*/

#ifndef _INET_H_WJ97
#define _INET_H_WJ97 1

#include "User.h"

#include <stdarg.h>

#define MAX_NEWCONNS		5

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

int inet_listen(unsigned int);
void close_connection(User *, char *, ...);
int unix_sock(char *);
int telnet_negotiations(User *, unsigned char);
void mainloop(void);

#endif	/* _INET_H_WJ97 */

/* EOB */
