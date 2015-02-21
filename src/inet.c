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
	inet.c
*/

#include "config.h"

#ifndef HAVE_SOCKET
#error This package relies on socket(), which is not available on this platform
#endif

#ifndef HAVE_SELECT
#error This package relies on select(), which is not available on this platform
#endif

#include "copyright.h"
#include "defines.h"
#include "debug.h"
#include "inet.h"
#include "cstring.h"
#include "log.h"
#include "User.h"
#include "Signals.h"
#include "memset.h"
#include "bufprintf.h"
#include "cstrerror.h"
#include "sys_wait.h"
#include "my_fcntl.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#ifndef SUN_LEN
#define SUN_LEN(x)	(sizeof(*(x)) - sizeof((x)->sun_path) + strlen((x)->sun_path))
#endif

#define MAX_NEWCONNS	5


char *inet_error(int err, char *buf, int maxlen) {
	if (buf == NULL || maxlen < 0)
		return NULL;

	if (err == EAI_SYSTEM)
		return cstrerror(errno, buf, maxlen);

	bufprintf(buf, maxlen, "%s", (char *)gai_strerror(err));
	return buf;
}

/*
	buf should be large enough, MAX_LINE should do
*/
char *inet_printaddr(char *host, char *service, char *buf, int buflen) {
	if (host == NULL || service == NULL || buf == NULL || buflen <= 0)
		return NULL;

	if (cstrchr(host, ':') != NULL)
		bufprintf(buf, buflen, "[%s]:%s", host, service);
	else
		bufprintf(buf, buflen, "%s:%s", host, service);

	return buf;
}

/*
	listen on a service port
	(actually, this function does a lot more than just listen()ing)

	This function is protocol-family independent, so it supports
	both IPv4 and IPv6 (and possibly even more ...)

	Mind that this function may take a long time to complete, as bind()
	may need some time to wait for a port to come available again
*/
int inet_listen(char *node, char *service, ConnType *conn_type) {
int sock, err, optval, retval, retry, success;
struct addrinfo hints, *res, *ai_p;
char host[NI_MAXHOST], serv[NI_MAXSERV], buf[NI_MAXHOST+NI_MAXSERV+MAX_LINE], errbuf[MAX_LINE];
Conn *conn;

	retval = -1;

#ifndef PF_UNSPEC
#error inet.c: PF_UNSPEC is undefined on this system
#endif
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
/*	hints.ai_protocol = 0;		already 0 */
/*	hints.ai_flags |= AI_PASSIVE; */			/* accept clients on any network */

	if ((err = getaddrinfo(node, service, &hints, &res)) != 0) {
		log_err("inet_listen(%s): %s", service, inet_error(err, errbuf, MAX_LINE));
		return -1;
	}
	for(ai_p = res; ai_p != NULL; ai_p = ai_p->ai_next) {
		if (ai_p->ai_family == PF_LOCAL)		/* skip local sockets */
			continue;

		if ((sock = socket(ai_p->ai_family, ai_p->ai_socktype, ai_p->ai_protocol)) == -1) {
/* be cool about errors about IPv6 ... not many people have it yet */
			if (!(errno == EAFNOSUPPORT && ai_p->ai_family == PF_INET6))
				log_warn("inet_listen(%s): socket(family = %d, socktype = %d, protocol = %d) failed: %s",
					service, ai_p->ai_family, ai_p->ai_socktype, ai_p->ai_protocol, cstrerror(errno, errbuf, MAX_LINE));
			continue;
		}
/*
	This is commented out because you don't really want to restrict
	BBS use to IPv6-only users

#ifdef IPV6_V6ONLY
		optval = 1;
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(int))) == -1)
			log_warn("inet_listen(%s): failed to set IPV6_V6ONLY: %s", service, cstrerror(errno, errbuf, MAX_LINE));
#endif
*/
/*
	SO_REUSEADDR allows us to do quick restarts of the BBS
	I have sometimes seen problems with this (the port going into TIME_WAIT
	and new connections fail, without the socket reporting any error at all)
*/
		optval = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
			log_warn("inet_listen(%s): setsockopt(SO_REUSEADDR) failed: %s", service, cstrerror(errno, errbuf, MAX_LINE));

		optval = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) == -1)
			log_warn("inet_listen(%s): setsockopt(SO_KEEPALIVE) failed: %s", service, cstrerror(errno, errbuf, MAX_LINE));

		optval = 0;
		if (setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(int)) == -1)
			log_warn("inet_listen(%s): setsockopt(SO_OOBINLINE) failed: %s", service, cstrerror(errno, errbuf, MAX_LINE));

		if (bind(sock, (struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen) == -1) {
			err = errno;
			success = 0;
			if (getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
				host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0) {
/*
	retry the bind a number of times if the port is in TIME_WAIT state
*/
				if (err == EADDRINUSE) {
					for(retry = 0; retry < BIND_RETRIES; retry++) {
						log_warn("inet_listen(): waiting on bind() on %s", inet_printaddr(host, serv, buf, sizeof(buf)));
						sleep(BIND_WAIT);
						if (bind(sock, (struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen) == -1) {
							err = errno;
							if (err == EADDRINUSE)
								continue;
							else
								break;				/* some other error */
						} else {
							success = 1;
							break;
						}
					}
				}
				if (!success)
					log_warn("inet_listen(): bind() failed on %s: %s", inet_printaddr(host, serv, buf, sizeof(buf)), cstrerror(err, errbuf, sizeof(errbuf)));
			} else
				log_warn("inet_listen(%s): bind failed on an interface, but I don't know which one(!)", service);

			if (!success) {
				close(sock);
				continue;
			}
		}
/*
	I don't think FIONBIO is needed on a listening socket, but just
	to make sure ...
*/
		if (ioctl(sock, FIONBIO, &optval) == -1) {
			log_err("inet_listen(%s): failed to set socket non-blocking: %s", service, cstrerror(errno, errbuf, MAX_LINE));
			close(sock);
			continue;
		}
		if (listen(sock, MAX_NEWCONNS) == -1) {
			if (getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
				host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0)
				log_err("inet_listen(): listen() failed on %s", inet_printaddr(host, serv, buf, sizeof(buf)));
			else
				log_err("inet_listen(%s): listen() failed", service);
			close(sock);
			continue;
		}

/* add to global list of all connections */

		if ((conn = new_Conn()) == NULL) {
			log_err("inet_listen(): out of memory allocating Conn");
			close(sock);
			freeaddrinfo(res);
			return -1;
		}
		conn->conn_type = conn_type;
		conn->state = CONN_LISTEN;
		conn->sock = sock;
		(void)add_Conn(&AllConns, conn);

		if (getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
			host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0)
			log_msg("listening on %s", inet_printaddr(host, serv, buf, sizeof(buf)));
		else
			log_msg("listening on port %s", service);

		retval = 0;			/* success */
		break;
	}
	freeaddrinfo(res);
/*
	it's actually possible to get here without a single error message being logged yet
	(which happens if you specify an IPv6 address, but do not have it)
	so just print a message if all is not well
*/
	if (retval)
		log_err("failed to start network");
	return retval;
}

int unix_sock(char *path) {
struct sockaddr_un un;
int sock, optval;
char errbuf[MAX_LINE];

	unlink(path);

	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
		log_err("unix socket()");
		return -1;
	}
	memset(&un, 0, sizeof(un));

	un.sun_family = AF_UNIX;
	cstrncpy(un.sun_path, path, sizeof(un.sun_path));

	if (bind(sock, (struct sockaddr *)&un, sizeof(un)) == -1) {
		log_err("unix socket bind()");
		return -1;
	}
/* set non-blocking */
	optval = 1;
	if (ioctl(sock, FIONBIO, &optval) == -1) {
		log_err("unix_sock(): failed to set socket non-blocking");
		close(sock);
		return -1;
	}
	if (listen(sock, MAX_NEWCONNS) == -1) {
		log_err("unix_sock(): listen() failed: %s", cstrerror(errno, errbuf, MAX_LINE));
		close(sock);
		return -1;
	}
	return sock;
}

/*
	test for valid/invalid file descriptors
*/
int test_fd(int fd) {
fd_set fds;
struct timeval timeout;
int err;

retry_test_fd:
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	timeout.tv_sec = timeout.tv_usec = 0L;

	err = select(fd+1, &fds, NULL, NULL, &timeout);
	if (err < 0) {
		if (errno == EINTR)			/* signal received (handle it later) */
			goto retry_test_fd;

		return -1;					/* got some error, most likely EBADF */
	}
	return 0;						/* no error */
}

/*
	The Main Loop
	this is the connection engine
*/
void mainloop(void) {
struct timeval timeout;
fd_set rfds, wfds, efds;
Conn *c, *c_next;
int err, highest_fd = -1, wait_for_input, isset;
char input_char[2];

	Enter(mainloop);

	this_user = NULL;

	setjmp(jumper);			/* trampoline for crashed connections */
	jump_set = 1;
	crash_recovery();		/* recover crashed connections */

	update_timers();
	while(1) {
		rtc = time(NULL);

		for(c = AllConns; c != NULL; c = c_next) {
			c_next = c->next;

/* remove dead connections */
			if (c->sock <= 0) {
				(void)remove_Conn(&AllConns, c);
				destroy_Conn(c);
				continue;
			}
			switch(c->state) {
				case CONN_LOOPING:
					if (c->loop_counter)
						c->loop_counter--;

					c->conn_type->process(c, LOOP_STATE);

					if ((c->state == CONN_LOOPING) && !c->loop_counter) {
						c->state = CONN_ESTABLISHED;
						Retx(c, INIT_STATE);
					}
					break;

				case CONN_LISTEN:		/* non-blocking listen, handled below */
					break;

				case CONN_CONNECTING:	/* non-blocking connect, handled below */
					break;

				case CONN_ESTABLISHED:
					if (c->input->pos < c->input->len) {
						if ((err = read_StringIO(c->input, input_char, 1)) == 1)
							c->conn_type->process(c, input_char[0]);
						else
							log_err("mainloop(): failed to read from input buffer");
					}
					break;

				case CONN_WAIT_CLOSE:
				case CONN_WAIT_CLOSE2:
				case CONN_CLOSED:
					free_StringIO(c->input);		/* discard any input */
					break;
			}
		}
/*
	now we need to check _again_ if a status has changed without doing
	further processing
	this is needed because it is possible that one user changes the status
	of another, that already has been checked in the loop -- if you don't
	use two loops, it possible to that everything freezes until select()
	times out, when a user changes the status of a 'previous' user
	Everything is very dynamic and things like process() and Ret()
	actually may change a lot...
*/
		wait_for_input = 1;
		highest_fd = 0;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		for(c = AllConns; c != NULL; c = c_next) {
			c_next = c->next;

			if (c->sock > 0) {
				switch(c->state) {
					case CONN_LOOPING:
						wait_for_input = 0;
						break;

					case CONN_LISTEN:
						FD_SET(c->sock, &rfds);
						if (highest_fd <= c->sock)
							highest_fd = c->sock + 1;
						break;

					case CONN_CONNECTING:
						FD_SET(c->sock, &wfds);
						if (highest_fd <= c->sock)
							highest_fd = c->sock + 1;
						break;

					case CONN_ESTABLISHED:
						if (c->output->len > 0) {				/* got data to write */
							FD_SET(c->sock, &wfds);
							if (highest_fd <= c->sock)
								highest_fd = c->sock + 1;
							wait_for_input = 0;
						}
						if (c->input->pos < c->input->len) {	/* got input ready */
							wait_for_input = 0;
							break;
						}
/*
	no input ready, mark for request to read()
*/
						if (c->sock > 0) {
							shift_StringIO(c->input, MIN_INPUTBUF);

							FD_SET(c->sock, &rfds);
							if (highest_fd <= c->sock)
								highest_fd = c->sock + 1;
							break;
						}
						break;

					case CONN_WAIT_CLOSE2:						/* really wait for close */
						if (c->output->len > 0) {				/* flush any pending output */
							FD_SET(c->sock, &wfds);
							if (highest_fd <= c->sock)
								highest_fd = c->sock + 1;
							wait_for_input = 0;
						}
						break;

					case CONN_WAIT_CLOSE:
						c->conn_type->wait_close(c);
						if (c->state == CONN_WAIT_CLOSE) {		/* can't stay in this state */
							c->state = CONN_WAIT_CLOSE2;
							break;
						}
						if (c->state != CONN_CLOSED)
							break;
/*
	a connection may be closed, but still have data to flush
*/
					case CONN_CLOSED:
						if (c->output->len > 0) {				/* got data to write */
							FD_SET(c->sock, &wfds);
							if (highest_fd <= c->sock)
								highest_fd = c->sock + 1;
							wait_for_input = 0;
						} else {								/* no more data, close */
							shutdown(c->sock, SHUT_RDWR);
							close(c->sock);
							c->sock = -1;
						}
						break;

					default:
						log_err("mainloop(): BUG ! got invalid connection state %d", c->state);
				}
			}
			if (c->sock <= 0) {					/* remove dead connections */
				(void)remove_Conn(&AllConns, c);
				destroy_Conn(c);
				continue;
			}
			FD_SET(c->sock, &efds);
		}
/*
	call select() to see if any more data is ready to arrive or to flush output
*/
		timeout.tv_sec = (wait_for_input == 0) ? 0 : shortest_timer();
		timeout.tv_usec = 0;
		errno = 0;
		err = select(highest_fd, &rfds, &wfds, &efds, &timeout);

		if (err < 0 && errno == EINTR)				/* signal received within select() */
			handle_pending_signals();

		update_timers();							/* run timers and any other signals */

		if (err > 0) {								/* number of ready fds */
			for(c = AllConns; c != NULL; c = c_next) {
				c_next = c->next;

				isset = 0;
/*
	check for exceptions: out-of-band data bytes
	This is mainly in here for debugging purposes, I don't think telnet ever uses OOB data
*/
				if (c->sock > 0 && FD_ISSET(c->sock, &efds)) {
					log_debug("mainloop(): exception on socket");
					if (recv(c->sock, input_char, 1, MSG_OOB) <= 0) {
						if (errno != EAGAIN && errno != EINTR) {
							log_debug("mainloop(): error reading OOB byte");
							shutdown(c->sock, SHUT_RDWR);
							close(c->sock);
							c->sock = -1;
							c->conn_type->linkdead(c);
						}
					} else {
/* ... now that we have the OOB data, what do we do with it??? */
						log_debug("mainloop(): got OOB byte 0x%02f", input_char[0]);
					}
					isset++;
				}
/*
	readable: new connection or just input data
*/
				if (c->sock > 0 && FD_ISSET(c->sock, &rfds)) {
					if (c->state == CONN_LISTEN)
						c->conn_type->accept(c);
					else
						input_Conn(c);
					isset++;
				}
/*
	writeable: complete non-blocking connect or ready to flush output
*/
				if (c->sock > 0 && FD_ISSET(c->sock, &wfds)) {
					if (c->state == CONN_CONNECTING)
						c->conn_type->complete_connect(c);
					else
						flush_Conn(c);
					isset++;
				}
				if (isset)
					err--;

				if (err <= 0) {
					err = 0;
					break;
				}
			}
		}
		if (err >= 0)				/* no error return from select() */
			continue;
/*
	handle select() errors
*/
		if (errno == EINTR) {		/* interrupted, better luck next time */
			log_debug("mainloop(): select() got EINTR, continuing");
			continue;
		}
		if (errno == EBADF) {
			log_debug("mainloop(): select() got EBADF, finding bad connection");

			for(c = AllConns; c != NULL; c = c->next) {
				if (c->sock > 0 && test_fd(c->sock)) {
					c->sock = -1;					/* mark as invalid */
					c->conn_type->linkdead(c);
				}
			}
			continue;
		}
		if (errno == EINVAL) {
			log_debug("mainloop(): select() got EINVAL, continuing");
			continue;
		}
/*
	strange, select() got some other error
*/
		log_err("select() got errno %d", errno);
/*
		log_err("exiting and restarting ; killing myself with SIGINT");
		kill(0, SIGINT);
*/
	}
}

/* EOB */
