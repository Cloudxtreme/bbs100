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
	ConnResolv.c	WJ105
*/

#include "config.h"
#include "ConnResolv.h"
#include "StringIO.h"
#include "Process.h"
#include "Param.h"
#include "cstring.h"
#include "log.h"
#include "inet.h"
#include "util.h"
#include "edit.h"
#include "bufprintf.h"
#include "Memory.h"
#include "cstrerror.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif


ConnType ConnResolv = {
	ConnResolv_process,
	ConnResolv_accept,
	dummy_Conn_handler,
	close_Conn,
	close_Conn,
	ConnResolv_linkdead,
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
char buf[MAX_PATHLEN];

	process_resolver.path = PARAM_PROGRAM_RESOLVER;

	if ((listener = new_ConnResolv()) == NULL) {
		log_warn("name resolving disabled");
		return -1;
	}
/* make pathname for the unix domain socket */

	bufprintf(buf, sizeof(buf), "%s/resolver.%lu", PARAM_CONFDIR, (unsigned long)getpid());
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
	(void)add_Conn(&AllConns, listener);
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
StringIO *s;
char *p;

	if (conn == NULL)
		return;

	s = (StringIO *)conn->data;

	if (k != KEY_RETURN) {
		if (write_StringIO(s, &k, 1) != 1)
			log_err("ConnResolv_process(): input lost");
		return;
	} else {
		k = 0;
		if (write_StringIO(s, &k, 1) != 1) {
			log_err("ConnResolv_process(): input lost");
			return;
		}
	}
	if ((p = cstrchr(s->buf, ' ')) != NULL) {
		*p = 0;
		p++;
		if (!*p)
			return;
	}
	if (p != NULL && strcmp(s->buf, p)) {
		Conn *c;

		for(c = AllConns; c != NULL; c = c->next) {
			if (c->ipnum != NULL && !strcmp(c->ipnum, s->buf)) {
				Free(c->hostname);
				c->hostname = cstrdup(p);		/* fill in IP name */
			}
		}
	}
	rewind_StringIO(s);
	shift_StringIO(s, MAX_LINE);
}

void ConnResolv_accept(Conn *conn) {
int sock, err;
struct sockaddr_un un;
socklen_t un_len;
char optval, errbuf[MAX_LINE];

	if (conn == NULL)
		return;

	un_len = sizeof(struct sockaddr_un);
	if ((sock = accept(conn->sock, (struct sockaddr *)&un, &un_len)) < 0) {
		err = errno;
		log_err("ConnResolv_accept(): failed to accept(): %s", cstrerror(err, errbuf, MAX_LINE));

		if (err == ENOTSOCK || err == EOPNOTSUPP) {
			log_err("This is a serious error, aborting");
			abort();
		}
		return;
	}
	if (conn_resolver != NULL) {
		(void)remove_Conn(&AllConns, conn_resolver);
		destroy_Conn(conn_resolver);
		conn_resolver = NULL;
	}
	if ((conn_resolver = new_ConnResolv()) == NULL) {
		shutdown(sock, 2);
		close(sock);
		return;
	}
	conn_resolver->sock = sock;
	conn_resolver->state = CONN_ESTABLISHED;

	optval = 1;
	ioctl(conn_resolver->sock, FIONBIO, &optval);		/* set non-blocking */

	if ((conn_resolver->data = new_StringIO()) == NULL) {
		log_err("ConnResolv_accept(): name resolving disabled");
		destroy_Conn(conn_resolver);
	} else
		(void)add_Conn(&AllConns, conn_resolver);
}

void ConnResolv_linkdead(Conn *c) {
	if (c == NULL)
		return;

	log_warn("lost connection with resolver, continuing without");

	close(c->sock);
	c->sock = -1;
}

void ConnResolv_destroy(Conn *c) {
	if (c == NULL)
		return;

	destroy_StringIO((StringIO *)c->data);
	c->data = NULL;
}

/*
	send request for name resolving
*/
void dns_gethostname(char *ipnum) {
	if (ipnum == NULL || !*ipnum || conn_resolver == NULL)
		return;

	put_Conn(conn_resolver, ipnum);
	flush_Conn(conn_resolver);
}

/* EOB */
