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
	ConnData.c	WJ105
*/

#include "ConnData.h"
#include "ConnUser.h"
#include "ConnResolv.h"
#include "DataCmd.h"
#include "User.h"
#include "Memory.h"
#include "Param.h"
#include "debug.h"
#include "inet.h"
#include "state_data.h"
#include "timeout.h"
#include "util.h"
#include "copyright.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif



ConnType ConnData = {
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnData_process,
	ConnData_accept,
	dummy_Conn_handler,
	dummy_Conn_handler,
	ConnUser_linkdead,
	ConnUser_destroy,
};


int init_ConnData(void) {
	return inet_listen(PARAM_DATA_PORT, &ConnData);
}

Conn *new_ConnData(void) {
Conn *conn;

	if ((conn = new_Conn()) == NULL)
		return NULL;

	conn->conn_type = &ConnData;
	return conn;
}

void ConnData_process(Conn *conn, char c) {
User *usr;

	usr = (User *)conn->data;

	if (usr == NULL || usr->conn == NULL || usr->conn->callstack == NULL || usr->conn->callstack->ip == NULL)
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

void ConnData_accept(Conn *conn) {
User *new_user;
Conn *new_conn;
struct sockaddr_in client;
int client_len = sizeof(struct sockaddr_in);
char buf[256];
int s;
char optval;

	if (conn == NULL)
		return;

	Enter(ConnData_accept);

	if ((s = accept(conn->sock, (struct sockaddr *)&client, (int *)&client_len)) < 0) {
		log_err("ConnData_accept(): accept() failed");
		Return;
	}
	if ((new_conn = new_ConnData()) == NULL) {
		shutdown(s, 2);
		close(s);
		Return;
	}
	new_conn->sock = s;
	new_conn->state = CONN_ESTABLISHED;

	if ((new_user = new_User()) == NULL) {
		destroy_Conn(new_conn);
		Return;
	}
	new_user->conn = new_conn;
	new_conn->data = new_user;

	optval = 1;
	ioctl(new_conn->sock, FIONBIO, &optval);		/* set non-blocking */

	new_conn->ipnum = ntohl(client.sin_addr.s_addr);
	strncpy(new_conn->from_ip, inet_ntoa(client.sin_addr), MAX_LINE-1);
	new_conn->from_ip[MAX_LINE-1] = 0;
/*
	This code is commented out, but if you want to lock out sites
	permanently (rather than for new users only), I suggest you
	enable this code
	(inspired by Richard of MatrixBBS)

	if (!allow_Wrapper(wrappers, new_conn->ipnum)) {
		Put(new_user, "\nSorry, but you're connecting from a site that has been locked out of the BBS.\n\n");
		log_auth("connection from %s closed by wrapper", new_conn->from_ip);
		destroy_Conn(new_conn);
		Return;
	}
*/
	log_auth("DATACONN (%s)", new_conn->from_ip);
	add_Conn(&AllConns, new_conn);
	add_User(&AllUsers, new_user);
	dns_gethostname(new_conn->from_ip);		/* send out request for hostname */

	Put(new_user, print_copyright(SHORT, NULL, buf));

	new_user->cmd_chain = new_PList(default_cmds);
	add_PList(&new_user->cmd_chain, new_PList(login_cmds));

	CALL(new_user, STATE_DATA_CONN);
	Return;
}

/* EOB */
