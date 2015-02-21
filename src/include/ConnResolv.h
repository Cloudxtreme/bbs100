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
	ConnResolv.h	WJ105
*/

#ifndef CONN_RESOLV_H_WJ105
#define CONN_RESOLV_H_WJ105	1

#include "Conn.h"

int init_ConnResolv(void);

Conn *new_ConnResolv(void);

void ConnResolv_process(Conn *, char);
void ConnResolv_accept(Conn *);
void ConnResolv_linkdead(Conn *);
void ConnResolv_destroy(Conn *);
void dns_gethostname(char *);

#endif	/* CONN_RESOLV_H_WJ105 */

/* EOB */
