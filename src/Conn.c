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
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>


Conn *AllConns = NULL;


ConnType ConnType_default = {
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

	if ((c->input = new_StringIO()) == NULL || (c->output = new_StringIO()) == NULL) {
		destroy_Conn(c);
		return NULL;
	}
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
	destroy_StringIO(c->input);
	destroy_StringIO(c->output);
	listdestroy_CallStack(c->callstack);
	Free(c);
}

void dummy_Conn_handler(Conn *c) {
	;
}

void dummy_Conn_process(Conn *c, char k) {
	;
}

int write_Conn(Conn *c, char *str, int len) {
	if (c == NULL || c->sock < 0 || str == NULL || !*str || len < 0)
		return -1;

	if (!len)
		return 0;

	return write_StringIO(c->output, str, len);
}

int putc_Conn(Conn *conn, char c) {
	return write_Conn(conn, &c, 1);
}

int put_Conn(Conn *conn, char *str) {
	if (str == NULL)
		return -1;

	return write_Conn(conn, str, strlen(str));
}

int flush_Conn(Conn *conn) {
int n, err;
char buf[MAX_PATHLEN];

	if (conn == NULL || conn->sock < 0)
		return -1;

	rewind_StringIO(conn->output);

	while((n = read_StringIO(conn->output, buf, MAX_PATHLEN)) > 0) {
		err = write(conn->sock, buf, n);

		if (!err) {
			seek_StringIO(conn->output, -n, STRINGIO_CUR);

			log_err("flush_Conn(): write() of %d bytes returned zero", n);
			return 0;
		}
		if (err == -1) {
			seek_StringIO(conn->output, -n, STRINGIO_CUR);

			if (errno == EINTR)					/* better luck next time */
				return 0;

#ifdef EWOULDBLOCK
			if (errno == EWOULDBLOCK)
				return 0;
#else
			if (errno == EAGAIN)
				return 0;
#endif
			close(conn->sock);
			conn->sock = -1;
			conn->conn_type->linkdead(conn);	/* some other error */
			return -1;
		}
		if (err != n)							/* partially written */
			seek_StringIO(conn->output, err - n, STRINGIO_CUR);

		shift_StringIO(conn->output);
	}
	return 0;
}

/*
	inverse of flush(); buffer input
*/
int input_Conn(Conn *conn) {
int err;
char buf[MAX_PATHLEN];

	if (conn == NULL || conn->sock < 0)
		return -1;

	if (conn->input->len > 0)			/* already got input ready */
		return conn->input->len;

	err = read(conn->sock, buf, MAX_PATHLEN);
	if (!err) {							/* EOF, connection closed */
		close(conn->sock);
		conn->sock = -1;
		conn->conn_type->linkdead(conn);
		return -1;
	}
	if (err < 0) {
		if (errno == EINTR)				/* better luck next time */
			return 0;

#ifdef EWOULDBLOCK
		if (errno == EWOULDBLOCK)
			return 0;
#else
		if (errno == EAGAIN)
			return 0;
#endif
		if (errno != EBADF)
			log_warn("input_Conn(): read(): %s, closing connection", strerror(errno));

		if (conn->sock >= 0) {
			close(conn->sock);
			conn->sock = -1;
			conn->conn_type->linkdead(conn);
		}
		return -1;
	}
	if ((err = write_StringIO(conn->input, buf, err)) < 0)
		log_warn("input_Conn(): failed to write to input buffer, input lost");

	rewind_StringIO(conn->input);
	return err;
}

/* EOB */
