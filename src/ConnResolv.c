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

#include "config.h"
#include "ConnResolv.h"
#include "Process.h"
#include "Param.h"
#include "cstring.h"
#include "log.h"
#include "inet.h"
#include "util.h"
#include "Linebuf.h"
#include "edit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif


ConnType ConnResolv = {
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnResolv_process,
	ConnResolv_accept,
	dummy_Conn_handler,
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnResolv_destroy,
};

/*
	The external resolver process connects to a listening UNIX socket,
	creating a new connection named conn_resolver
	The BBS sends requests for resolving addresses to the resolver,
	which answers over the same connection
*/
static Conn *conn_resolver = NULL;

static char *argv_resolver[3] =	{ "(bbs100 resolver)", NULL, NULL };
static Process process_resolver = {
	"resolver", NULL, argv_resolver, (pid_t)-1L, PROC_RESTART, 0, (time_t)0UL
};


/*
	start the resolver process
*/
int init_ConnResolv(void) {
Conn *listener;
char buf[MAX_LINE];

	process_resolver.path = PARAM_PROGRAM_RESOLVER;

	if ((listener = new_ConnResolv()) == NULL) {
		log_warn("name resolving disabled");
		return -1;
	}
/* make pathname for the unix domain socket */

	sprintf(buf, "%s/resolver.%lu", PARAM_CONFDIR, (unsigned long)getpid());
	path_strip(buf);
	if ((listener->sock = unix_sock(buf)) < 0) {
		log_err("failed to create unix domain socket");
		log_warn("name resolving disabled");

		destroy_Conn(listener);
		listener = NULL;
		return -1;
	}
	listener->state = CONN_LISTEN;

	log_msg("starting resolver with socket %s", buf);
	process_resolver.argv[1] = buf;

	if (fork_process(&process_resolver) == -1) {
		log_err("init_ConnResolv(): failed to start name resolver");

		destroy_Conn(listener);
		listener = NULL;
		return -1;
	}
	add_Conn(&AllConns, listener);
	return 0;
}

Conn *new_ConnResolv(void) {
Conn *conn;

	if ((conn = new_Conn()) != NULL)
		conn->conn_type = &ConnResolv;

	return conn;
}

/*
	got answer back from asynchronous resolver process
*/
void ConnResolv_process(Conn *conn, char k) {
Linebuf *lb;
char *p;

	if (conn == NULL)
		return;

	lb = (Linebuf *)conn->data;
	if (input_Linebuf(lb, k) != EDIT_RETURN)
		return;

	if ((p = cstrchr(lb->buf, ' ')) != NULL) {
		*p = 0;
		p++;
		if (!*p)
			return;
	}
	if (p != NULL && strcmp(lb->buf, p)) {
		Conn *c;

		for(c = AllConns; c != NULL; c = c->next)
			if (!strcmp(c->from_ip, lb->buf))
				strcpy(c->from_ip, p);			/* fill in IP name */
	}
	conn->input_head = conn->input_tail = 0;
	reset_Linebuf((Linebuf *)lb);
}

void ConnResolv_accept(Conn *conn) {
int un_len, sock;
struct sockaddr_un un;
char optval;

	if (conn == NULL)
		return;

	un_len = sizeof(struct sockaddr_un);
	if ((sock = accept(conn->sock, (struct sockaddr *)&un, (int *)&un_len)) < 0) {
		log_err("ConnResolv_accept(): failed to accept()");
		return;
	}
	if (conn_resolver != NULL) {
		remove_Conn(&AllConns, conn_resolver);
		destroy_Conn(conn_resolver);
		conn_resolver = NULL;
	}
	if ((conn_resolver = new_ConnResolv()) == NULL) {
		shutdown(sock, 2);
		close(sock);
		return;
	}
	conn_resolver->sock = sock;
	conn_resolver->state |= CONN_ESTABLISHED;

	optval = 1;
	ioctl(conn_resolver->sock, FIONBIO, &optval);		/* set non-blocking */

	conn_resolver->data = new_Linebuf();

	add_Conn(&AllConns, conn_resolver);
}

void ConnResolv_destroy(Conn *c) {
	if (c == NULL)
		return;

	destroy_Linebuf((Linebuf *)c->data);
	c->data = NULL;
}

/*
	send request for name resolving
*/
void dns_gethostname(char *ipnum) {
	if (ipnum == NULL || !*ipnum || conn_resolver == NULL)
		return;

	write_Conn(conn_resolver, ipnum);
	flush_Conn(conn_resolver);
}

/* EOB */
