/*
    bbs100 3.0 WJ105
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
	ConnUser.h	WJ105
*/

#include "Conn.h"
#include "Telnet.h"

extern ConnType ConnUser;

int init_ConnUser(void);

Conn *new_ConnUser(void);

void ConnUser_accept(Conn *);
void ConnUser_readable(Conn *);
void ConnUser_wait_close(Conn *);
void ConnUser_process(Conn *, char);
void ConnUser_linkdead(Conn *);
void ConnUser_destroy(Conn *);

void ConnUser_window_event(Conn *, Telnet *);

/* EOB */
