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
	ConnUser.c	WJ105
*/

#include "ConnUser.h"
#include "User.h"
#include "OnlineUser.h"
#include "debug.h"
#include "inet.h"
#include "log.h"
#include "timeout.h"
#include "copyright.h"
#include "screens.h"
#include "state_login.h"
#include "Param.h"
#include "ConnResolv.h"
#include "Wrapper.h"
#include "bufprintf.h"
#include "cstring.h"
#include "cstrerror.h"
#include "sys_wait.h"
#include "my_fcntl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#ifndef TELOPT_NAWS
#define TELOPT_NAWS 31			/* negotiate about window size */
#endif

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON 39	/* set new environment variable */
#endif

#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif


ConnType ConnUser = {
	ConnUser_process,
	ConnUser_accept,
	dummy_Conn_handler,
	ConnUser_wait_close,
	close_Conn,
	ConnUser_linkdead,
	ConnUser_destroy,
};


int init_ConnUser(void) {
	return inet_listen(PARAM_BIND_ADDRESS, PARAM_PORT_NUMBER, &ConnUser);
}

Conn *new_ConnUser(void) {
Conn *conn;

	if ((conn = new_Conn()) != NULL)
		conn->conn_type = &ConnUser;

	return conn;
}

void ConnUser_accept(Conn *conn) {
User *new_user;
Conn *new_conn;
char buf[MAX_LONGLINE], errbuf[MAX_LINE];
int s, err, optval;
struct sockaddr_storage client;
socklen_t client_len = sizeof(struct sockaddr_storage);

	Enter(ConnUser_accept);

	if ((s = accept(conn->sock, (struct sockaddr *)&client, &client_len)) < 0) {
		err = errno;
		log_err("ConnUser_accept(): failed to accept(): %s", cstrerror(err, errbuf, MAX_LINE));

		if (err == ENOTSOCK || err == EOPNOTSUPP) {
			log_err("This is a serious error, aborting");
			abort();
		}
		Return;
	}
	if ((new_user = new_User()) == NULL) {
		shutdown(s, SHUT_RDWR);
		close(s);
		Return;
	}
	if ((new_user->telnet = new_Telnet()) == NULL) {
		destroy_User(new_user);
		shutdown(s, SHUT_RDWR);
		close(s);
		Return;
	}
	if ((new_conn = new_ConnUser()) == NULL) {
		destroy_User(new_user);
		shutdown(s, SHUT_RDWR);
		close(s);
		Return;
	}
	new_user->conn = new_conn;
	new_conn->data = new_user;
	new_conn->sock = s;
	new_conn->state = CONN_ESTABLISHED;

	optval = 1;
	if (ioctl(new_conn->sock, FIONBIO, &optval) == -1)		/* set non-blocking */
		log_warn("ConnUser_accept(): failed to set socket nonblocking");

	optval = 1;
	if (setsockopt(new_conn->sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) == -1)
		log_warn("ConnUser_accept(): setsockopt(SO_KEEPALIVE) failed: %s", cstrerror(errno, errbuf, MAX_LINE));

	optval = 0;
	if (setsockopt(new_conn->sock, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(int)) == -1)
		log_warn("ConnUser_accept(): setsockopt(SO_OOBINLINE) failed: %s", cstrerror(errno, errbuf, MAX_LINE));

	if ((err = getnameinfo((struct sockaddr *)&client, client_len, buf, MAX_LONGLINE-1, NULL, 0, NI_NUMERICHOST)) != 0) {
		log_warn("ConnUser_accept(): getnameinfo(): %s", inet_error(err, errbuf, MAX_LINE));
		new_conn->ipnum = cstrdup("0.0.0.0");
	} else
		new_conn->ipnum = cstrdup(buf);
	new_conn->hostname = cstrdup(new_conn->ipnum);

	if (PARAM_HAVE_WRAPPER_ALL && !allow_Wrapper(new_conn->ipnum, WRAPPER_ALL_USERS)) {
		put_Conn(new_conn, "\nSorry, but you're connecting from a site that has been locked out of the BBS.\n\n");
		log_auth("connection from %s closed by wrapper", new_conn->ipnum);
		destroy_Conn(new_conn);
		Return;
	}
	if (reboot_timer != NULL && time_to_dd(reboot_timer) < 10) {
		put_Conn(new_conn, "\nSorry, but the system is rebooting. Come back soon.\n\n");
		log_auth("connection from %s closed, reboot in progress", new_conn->ipnum);
		destroy_Conn(new_conn);
		Return;
	}
	if (shutdown_timer != NULL && time_to_dd(shutdown_timer) < 10) {
		put_Conn(new_conn, "\nSorry, but the system is going down for maintenance. Come back soon.\n\n");
		log_auth("connection from %s closed, shutdown in progress", new_conn->ipnum);
		destroy_Conn(new_conn);
		Return;
	}
	bufprintf(buf, sizeof(buf), "%c%c%c%c%c%c%c%c%c%c%c%c", IAC, WILL, TELOPT_SGA, IAC, WILL, TELOPT_ECHO,
		IAC, DO, TELOPT_NAWS, IAC, DO, TELOPT_NEW_ENVIRON);

	if (write(new_conn->sock, buf, strlen(buf)) < 0) {
		destroy_Conn(new_conn);
		Return;
	}
	log_auth("CONN (%s)", new_conn->ipnum);
	(void)add_Conn(&AllConns, new_conn);
	(void)add_User(&AllUsers, new_user);
	dns_gethostname(new_conn->ipnum);		/* send out request for hostname */

/*
	display the login screen
	it's a pity that we still do not know the user's terminal height+width,
	we don't have that input yet, and we're surely not going to wait for it
*/
	display_screen(new_user, PARAM_LOGIN_SCREEN);
	Print(new_user, "<center>%s\n", print_copyright(SHORT, NULL, buf, MAX_LONGLINE));

	CALL(new_user, STATE_LOGIN_PROMPT);
	Return;
}

/*
	keep the logout screen visible for a couple of seconds
	(X-)Windows programs have a habit of closing the window when the connection
	is terminated
*/
void ConnUser_wait_close(Conn *conn) {
User *usr;
Timer *t;

	if (conn == NULL || conn->sock < 0)
		return;

	usr = (User *)conn->data;
	if (usr == NULL)
		return;

	if ((t = new_Timer(LOGOUT_TIMEOUT, close_logout, TIMER_ONESHOT)) != NULL) {
		listdestroy_Timer(usr->timerq);
		usr->timerq = usr->idle_timer = NULL;
		add_Timer(&usr->timerq, t);
		conn->state = CONN_WAIT_CLOSE2;
	} else
		conn->state = CONN_CLOSED;
}

void ConnUser_process(Conn *conn, char c) {
User *usr;

	if (conn == NULL)
		return;

	usr = (User *)conn->data;
	if (usr == NULL || usr->conn == NULL || usr->conn->callstack == NULL || usr->conn->callstack->ip == NULL)
		return;

	this_user = usr;
	c = telnet_negotiations(usr->telnet, usr->conn, (unsigned char)c, ConnUser_window_event);
	this_user = NULL;
	if (c == (char)-1)
		return;

/* user is doing something, reset idle timer */
	usr->idle_time = rtc;

/*
	reset timeout timer, unless locked
*/
	if (!(usr->runtime_flags & RTF_LOCKED) && usr->idle_timer != NULL) {
		usr->idle_timer->restart = TIMEOUT_USER;
		set_Timer(&usr->timerq, usr->idle_timer, usr->idle_timer->maxtime);
	}
/* call routine on top of the callstack */
	this_user = usr;
	usr->conn->callstack->ip(usr, c);				/* process input */
	this_user = NULL;
}

void ConnUser_linkdead(Conn *conn) {
User *usr;

	usr = (User *)conn->data;
	if (usr == NULL)
		return;

	if (usr->name[0]) {
		notify_linkdead(usr);
		log_auth("LINKDEAD %s (%s)", usr->name, usr->conn->ipnum);
		close_connection(usr, "%s went linkdead", usr->name);
	}
}

void ConnUser_destroy(Conn *conn) {
User *usr;

	usr = (User *)conn->data;
	if (usr == NULL)
		return;

	remove_OnlineUser(usr);
	(void)remove_User(&AllUsers, usr);
	destroy_User(usr);

	conn->data = NULL;
}

/*
	window size changed
	(I'm not talking about TCP window size here)
*/
void ConnUser_window_event(Conn *conn, Telnet *t) {
User *usr;

	if (conn == NULL || t == NULL || conn->data == NULL || ((User *)conn->data)->telnet != t)
		return;

	usr = (User *)conn->data;
	if (usr->display != NULL && !(usr->flags & USR_FORCE_TERM)) {
		usr->display->term_width = t->term_width;
		usr->display->term_height = t->term_height;
	}
}

/* EOB */
