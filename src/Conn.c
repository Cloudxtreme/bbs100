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
	Conn.c	WJ105
*/

#include "config.h"
#include "Conn.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


Conn *AllConns = NULL;


ConnType ConnType_default = {
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_process,
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_handler,
};


Conn *new_Conn(void) {
Conn *c;

	if ((c = (Conn *)Malloc(sizeof(Conn), TYPE_CONN)) == NULL)
		return NULL;

	c->sock = -1;
	c->conn_type = &ConnType_default;
	return c;
}

void destroy_Conn(Conn *c) {
	if (c == NULL)
		return;

	c->conn_type->destroy(c);

	if (c->sock > 0) {
		flush_Conn(c);
		shutdown(c->sock, 2);
		close(c->sock);
		c->sock = -1;
		c->state = 0;
	}
	listdestroy_CallStack(c->callstack);
	Free(c);
}

void dummy_Conn_handler(Conn *c) {
	;
}

void dummy_Conn_process(Conn *c, char k) {
	;
}


void write_Conn(Conn *c, char *str) {
int l;

	if (c == NULL || str == NULL)
		return;

	l = strlen(str);

	if ((c->output_idx+l) >= (MAX_OUTPUTBUF-1))
		flush_Conn(c);

	strncpy(c->outputbuf + c->output_idx, str, l);
	c->output_idx += l;
	c->outputbuf[c->output_idx] = 0;
}

void putc_Conn(Conn *conn, char c) {
	if (conn == NULL)
		return;

	if (conn->output_idx >= (MAX_OUTPUTBUF-1))
		flush_Conn(conn);

	conn->outputbuf[conn->output_idx++] = c;
	conn->outputbuf[conn->output_idx] = 0;
}

void flush_Conn(Conn *conn) {
	if (conn == NULL)
		return;

	if (conn->sock > 0 && conn->output_idx > 0) {
		write(conn->sock, conn->outputbuf, conn->output_idx);
		conn->outputbuf[0] = 0;
		conn->output_idx = 0;
	}
}

/* EOB */
