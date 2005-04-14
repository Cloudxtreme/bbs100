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
	ConnResolv.c	WJ105
*/

#include "ConnResolv.h"
#include "cstring.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


ConnType ConnResolv = {
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnResolv_process,
	ConnResolv_accept,
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_handler,
};


int init_ConnResolv(void) {

	return 0;
}

/*
	got answer back from asynchronous resolver process
*/
void ConnResolv_process(Conn *conn, char k) {
char *p;

	if (conn == NULL || conn->input_head <= conn->input_tail)
		return;

	if (conn->input_tail >= MAX_INPUTBUF)
		conn->input_tail = MAX_INPUTBUF-1;
	conn->inputbuf[conn->input_tail] = 0;

	if ((p = cstrchr(conn->inputbuf, ' ')) != NULL) {
		*p = 0;
		p++;
		if (!*p)
			return;
	}
	if (p != NULL && strcmp(conn->inputbuf, p)) {
		Conn *c;

		for(c = AllConns; c != NULL; c = c->next)
			if (!strcmp(c->from_ip, conn->inputbuf))
				strcpy(c->from_ip, p);			/* fill in IP name */
	}
	conn->input_head = conn->input_tail = 0;
}

void ConnResolv_accept(Conn *conn) {
int un_len, sock;
struct sockaddr_un un;
Conn *new_conn;

	if (conn == NULL)
		return;

	un_len = sizeof(struct sockaddr_un);
	if ((sock = accept(conn->sock, (struct sockaddr *)&un, (int *)&un_len)) < 0) {
		log_err("ConnResolv_accept(): failed to accept()");
		return;
	}
	if ((new_conn = new_Conn()) == NULL) {
		shutdown(sock, 2);
		close(sock);
		return;
	}
	new_conn->sock = sock;
	new_conn->conn_type = &ConnResolv;
	new_conn->state |= CONN_ESTABLISHED;
	AllConns = add_Conn(&AllConns, new_conn);
}

/* EOB */
