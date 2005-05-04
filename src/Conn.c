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

/*
	write to a connection using an output buffer
*/
int write_Conn(Conn *c, char *str, int len) {
int bytes_written, n, old_idx;

	if (c == NULL || c->sock < 0 || str == NULL || !*str || len < 0)
		return -1;

	if (!len)
		return 0;

	bytes_written = 0;
	while(bytes_written < len) {

/* n is how many bytes still fit in the buffer */

		n = MAX_OUTPUTBUF-1 - c->output_idx;
		if (n < 0) {
			log_err("write_Conn(): BUG ! n < 0");
			return bytes_written;
		}
		if (!n) {
			old_idx = c->output_idx;

			if (flush_Conn(c) < 0)
				return -1;

			if (old_idx == c->output_idx) {		/* nothing was flushed */
				log_warn("write_Conn(): write() blocked, buffer overrun; %d bytes discarded", len - bytes_written);
				return bytes_written;
			}
			continue;							/* flush succeeded */
		}
		if (len - bytes_written < n)			/* everything fits in the buffer */
			n = len - bytes_written;

		memcpy(c->outputbuf + c->output_idx, str, n);		/* add to output buffer */
		c->output_idx += n;
		c->outputbuf[c->output_idx] = 0;
		bytes_written += n;
		str = str + n;
	}
	return bytes_written;
}

/*
	optimized form of write_Conn() for a single character
*/
int putc_Conn(Conn *conn, char c) {
	if (conn == NULL || conn->sock < 0)
		return -1;

	if (conn->output_idx >= MAX_OUTPUTBUF - 1) {
		if (flush_Conn(conn) < 0)
			return -1;

		if (conn->output_idx >= MAX_OUTPUTBUF - 1) {
			log_warn("putc_Conn(): write() blocked, buffer overrun; 1 byte discarded");
			return 0;
		}
	}
	conn->outputbuf[conn->output_idx++] = c;
	conn->outputbuf[conn->output_idx] = 0;
	return 1;
}

int put_Conn(Conn *conn, char *str) {
	if (str == NULL)
		return -1;

	return write_Conn(conn, str, strlen(str));
}

int flush_Conn(Conn *conn) {
int err;

	if (conn == NULL || conn->sock < 0)
		return -1;

	while(conn->output_idx > 0) {
		err = write(conn->sock, conn->outputbuf, conn->output_idx);

		if (!err) {
			log_err("flush_Conn(): write() of %d bytes returned zero", conn->output_idx);
			return 0;
		}
		if (err == -1) {
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
		if (err == conn->output_idx) {			/* all data written */
			conn->outputbuf[0] = 0;
			conn->output_idx = 0;
		} else {								/* partially written */
			memmove(conn->outputbuf, conn->outputbuf + err, conn->output_idx - err);
			conn->output_idx -= err;
			conn->outputbuf[conn->output_idx] = 0;
		}
	}
	return 0;
}

/*
	inverse of flush(); buffer input
*/
int input_Conn(Conn *conn) {
int err;

	if (conn == NULL || conn->sock < 0)
		return -1;

	if (conn->input_head < conn->input_tail)		/* already got input ready */
		return conn->input_tail - conn->input_head;

	conn->input_head = conn->input_tail = 0;

	err = read(conn->sock, conn->inputbuf, MAX_INPUTBUF);
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
	conn->input_tail = err;
	return conn->input_tail;
}

/* EOB */
