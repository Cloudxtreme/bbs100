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
	inet.h
*/

#ifndef _INET_H_WJ97
#define _INET_H_WJ97 1

#include "Conn.h"

/*
	bind() may retry for as long as 30*5 seconds == 2,5 minutes
*/
#define BIND_RETRIES	30
#define BIND_WAIT		5

char *inet_error(int, char *, int);
char *inet_printaddr(char *, char *, char *, int);

int inet_listen(char *, char *, ConnType *);
int unix_sock(char *);
int test_fd(int);
void mainloop(void);

#endif	/* _INET_H_WJ97 */

/* EOB */
