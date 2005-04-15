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
	ConnUser.c	WJ105
*/

#include "ConnUser.h"
#include "User.h"
#include "OnlineUser.h"
#include "Timer.h"
#include "debug.h"
#include "inet.h"
#include "log.h"
#include "timeout.h"
#include "copyright.h"
#include "screens.h"
#include "state_login.h"
#include "Param.h"
#include "ConnResolv.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
#ifndef TELCMDS
#define TELCMDS
#endif
#ifndef TELOPTS
#define TELOPTS
#endif
*/

#include <arpa/telnet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif


#ifndef TELOPT_NAWS
#define TELOPT_NAWS 31			/* negotiate about window size */
#endif

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON 39	/* set new environment variable */
#endif


ConnType ConnUser = {
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnUser_process,
	ConnUser_accept,
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnUser_linkdead,
	ConnUser_destroy,
};


int init_ConnUser(void) {
Conn *conn;

	if ((conn = new_Conn()) == NULL)
		return -1;

	conn->conn_type = &ConnUser;

	if ((conn->sock = inet_listen(PARAM_PORT_NUMBER)) < 0) {
		destroy_Conn(conn);
		return -1;
	}
	conn->state |= CONN_LISTEN;

	add_Conn(&AllConns, conn);
	return 0;
}

void ConnUser_accept(Conn *conn) {
User *new_user;
Conn *new_conn;
struct sockaddr_in client;
int client_len = sizeof(struct sockaddr_in);
char buf[256];
StringList *sl;
int s;
char optval;

	Enter(ConnUser_accept);

	if ((s = accept(conn->sock, (struct sockaddr *)&client, (int *)&client_len)) < 0) {
		log_err("ConnUser_accept(): failed to accept()");
		Return;
	}
	if ((new_user = new_User()) == NULL) {
		shutdown(s, 2);
		close(s);
		Return;
	}
	if ((new_conn = new_Conn()) == NULL) {
		destroy_User(new_user);
		shutdown(s, 2);
		close(s);
		Return;
	}
	new_user->conn = new_conn;
	new_conn->data = new_user;
	new_conn->sock = s;
	new_conn->conn_type = &ConnUser;
	new_conn->state |= CONN_ESTABLISHED;

	optval = 1;
	ioctl(new_conn->sock, FIONBIO, &optval);		/* set non-blocking */

	new_conn->ipnum = ntohl(client.sin_addr.s_addr);
	strncpy(new_conn->from_ip, inet_ntoa(client.sin_addr), MAX_LINE-1);
	new_conn->from_ip[MAX_LINE-1] = 0;

	sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c", IAC, WILL, TELOPT_SGA, IAC, WILL, TELOPT_ECHO,
		IAC, DO, TELOPT_NAWS, IAC, DO, TELOPT_NEW_ENVIRON);

	if (write(new_conn->sock, buf, strlen(buf)) < 0) {
		close_connection(new_user, "bad new connection");
		Return;
	}
	log_auth("CONN (%s)", new_conn->from_ip);
	add_Conn(&AllConns, new_conn);
	add_User(&AllUsers, new_user);
	dns_gethostname(new_conn->from_ip);		/* send out request for hostname */

/*
	display the login screen
	it's a pity that we still do not know the user's terminal height+width,
	we don't have that input yet, and we're surely not going to wait for it
*/
	for(sl = login_screen; sl != NULL; sl = sl->next)
		Print(new_user, "%s\n", sl->str);
	Print(new_user, "        %s\n", print_copyright(SHORT, NULL, buf));

/*
	This code is commented out, but if you want to lock out sites
	permanently (rather than for new users only), I suggest you
	enable this code
	(inspired by Richard of MatrixBBS)

	if (!allow_Wrapper(wrappers, new_conn->ipnum)) {
		Put(new_user, "\nSorry, but you're connecting from a site that has been locked out of the BBS.\n\n");
		close_connection(new_user, "connection closed by wrapper");
		Return;
	}
*/
	CALL(new_user, STATE_LOGIN_PROMPT);
	Return;
}

void ConnUser_process(Conn *conn, char c) {
User *usr;

	usr = (User *)conn->data;

	if (usr == NULL || usr->conn == NULL || usr->conn->callstack == NULL || usr->conn->callstack->ip == NULL
		|| (c = telnet_negotiations(usr, (unsigned char)c)) == (char)-1)
		return;

/* user is doing something, reset idle timer */
	usr->idle_time = rtc;

/* reset timeout timer, unless locked */
	if (!(usr->runtime_flags & RTF_LOCKED) && usr->idle_timer != NULL) {
		usr->idle_timer->sleeptime = usr->idle_timer->maxtime;
		usr->idle_timer->restart = TIMEOUT_USER;
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
		log_auth("LINKDEAD %s (%s)", usr->name, usr->conn->from_ip);
		close_connection(usr, "%s went linkdead", usr->name);
	}
}

void ConnUser_destroy(Conn *conn) {
User *usr;

	usr = (User *)conn->data;
	if (usr == NULL)
		return;

	remove_OnlineUser(usr);
	remove_User(&AllUsers, usr);
	destroy_User(usr);

	conn->data = NULL;
}

/* EOB */
