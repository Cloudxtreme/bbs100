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
void write_Conn(Conn *c, char *str) {
int len, bytes_written, n, old_idx;

	if (c == NULL || c->sock < 0 || str == NULL || !*str)
		return;

	len = strlen(str);
	bytes_written = 0;

	while(bytes_written < len) {

/* n is how many bytes still fit in the buffer */

		n = MAX_OUTPUTBUF-1 - c->output_idx;
		if (n < 0) {
			log_err("write_Conn(): BUG ! n < 0");
			return;
		}
		if (!n) {
			old_idx = c->output_idx;

			flush_Conn(c);

			if (c->sock < 0)					/* connection lost */
				return;

			if (old_idx == c->output_idx) {		/* nothing was flushed */
				log_warn("write_Conn(): write() blocked, buffer overrun; %d bytes discarded", len - bytes_written);
				return;
			} else
				continue;						/* flush succeeded */
		}
		if (len < n)							/* everything fits in the buffer */
			n = len;

		memcpy(c->outputbuf + c->output_idx, str, n);		/* add to output buffer */
		c->output_idx += n;
		c->outputbuf[c->output_idx] = 0;
		bytes_written += n;
		str = str + n;
	}
}

/*
	optimized form of write_Conn() for a single character
*/
void putc_Conn(Conn *conn, char c) {
	if (conn == NULL || conn->sock < 0)
		return;

	if (conn->output_idx >= MAX_OUTPUTBUF - 1) {
		flush_Conn(conn);

		if (conn->sock < 0)
			return;

		if (conn->output_idx >= MAX_OUTPUTBUF - 1) {
			log_warn("putc_Conn(): write() blocked, buffer overrun; 1 byte discarded");
			return;
		}
	}
	conn->outputbuf[conn->output_idx++] = c;
	conn->outputbuf[conn->output_idx] = 0;
}

void flush_Conn(Conn *conn) {
int err;

	if (conn == NULL)
		return;

	while(conn->sock > 0 && conn->output_idx > 0) {
		err = write(conn->sock, conn->outputbuf, conn->output_idx);

		if (!err) {
			log_err("flush_Conn(): write() of %d bytes returned zero", conn->output_idx);
			break;
		}
		if (err == -1) {
			if (errno == EAGAIN)		/* better luck next time */
				break;

			close(conn->sock);
			conn->sock = -1;
			conn->conn_type->linkdead(conn);	/* some other error */
			break;
		} else {
			if (err == conn->output_idx) {		/* all data written */
				conn->outputbuf[0] = 0;
				conn->output_idx = 0;
			} else {							/* partially written */
				memmove(conn->outputbuf, conn->outputbuf + err, conn->output_idx - err);
				conn->output_idx -= err;
				conn->outputbuf[conn->output_idx] = 0;
			}
		}
	}
}

/* EOB */
