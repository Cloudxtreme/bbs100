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
	Chatter18	WJ97
	inet.c
*/

#include "config.h"

#ifndef HAVE_SELECT
#error This package relies on select(), which is not available on this platform
#endif

#include "copyright.h"
#include "defines.h"
#include "debug.h"
#include "inet.h"
#include "util.h"
#include "log.h"
#include "state_login.h"
#include "CallStack.h"
#include "Stats.h"
#include "Wrapper.h"
#include "screens.h"
#include "Timer.h"
#include "Signal.h"
#include "Param.h"
#include "edit.h"
#include "sys_time.h"
#include "main.h"
#include "state.h"
#include "OnlineUser.h"
#include "state_data.h"
#include "DataCmd.h"
#include "Conn.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON	39
#endif

#ifndef SUN_LEN
#define SUN_LEN(x)	(sizeof(*(x)) - sizeof((x)->sun_path) + strlen((x)->sun_path))
#endif


int inet_listen(unsigned int port) {
struct sockaddr_in sin;
int sock, optval, sleepcount = 0;

	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		log_err("inet socket()");
		return -1;
	}
	optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval,
			sizeof(optval)) == -1) {
		log_err("inet socket setsockopt()");
		close(sock);
		return -1;
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	while(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		if (errno == EACCES) {
			printf("bind: Permission denied\n");
			close(sock);
			return -1;
		}
		if (errno == EADDRNOTAVAIL) {
			printf("bind: Address not available\n");
			close(sock);
			return -1;
		}
		if (errno == EADDRINUSE) {
			printf("bind: Address already is in use\n");
			close(sock);
			return -1;
		}
		if (++sleepcount > 10) {
			printf("bind took too long, timed out\n");
			close(sock);
			return -1;
		}
		printf("waiting on bind ...\n");
		sleep(1);
	}
/*
	This is not needed...

	sin_len = sizeof(sin);
	if (getsockname(sock, (struct sockaddr *)&sin, (int *)&sin_len) == -1) {
		log_err("inet socket getsockname()");
		close(sock);
		return -1;
	}
*/
	optval = 1;
	if (ioctl(sock, FIONBIO, &optval) == -1) {
		log_err("inet socket ioctl()");
		close(sock);
		return -1;
	}
	if (listen(sock, MAX_NEWCONNS) == -1) {
		log_err("inet socket listen()");
		close(sock);
		return -1;
	}
	return sock;
}

void close_connection(User *usr, char *reason, ...) {
	if (usr == NULL)
		return;

	Enter(close_connection);

	if (usr->name[0]) {
		if (usr->runtime_flags & RTF_WAS_HH)	/* restore HH status before saving the User */
			usr->flags |= USR_HELPING_HAND;

		usr->last_logout = (unsigned long)rtc;
		update_stats(usr);

		if (save_User(usr))
			log_err("failed to save user %s", usr->name);

		remove_OnlineUser(usr);
	}
	if (reason != NULL) {			/* log why we're being disconnected */
		va_list ap;
		char buf[PRINT_BUF];

		if (usr->name[0])
			sprintf(buf, "CLOSE %s (%s): ", usr->name, usr->conn->from_ip);
		else
			sprintf(buf, "CLOSE (%s): ", usr->conn->from_ip);

		va_start(ap, reason);
		vsprintf(buf+strlen(buf), reason, ap);
		va_end(ap);

		log_auth(buf);
	}
	if (usr->conn->sock > 0) {
		Put(usr, "<default>\n");
		Flush(usr);
		shutdown(usr->conn->sock, 2);
		close(usr->conn->sock);
		usr->conn->sock = -1;
	}
	leave_room(usr);

	usr->name[0] = 0;
	Return;
}

int unix_sock(char *path) {
struct sockaddr_un un;
int sock;
char optval;

	unlink(path);

	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
		log_err("unix socket()");
		return -1;
	}
	memset(&un, 0, sizeof(un));

	un.sun_family = AF_UNIX;
	strncpy(un.sun_path, path, sizeof(un.sun_path) - 1);
	un.sun_path[sizeof(un.sun_path) - 1] = 0;

	if (bind(sock, (struct sockaddr *)&un, sizeof(un)) == -1) {
		log_err("unix socket bind()");
		return -1;
	}
/*
	This is not needed...

	un_len = sizeof(un);
	if (getsockname(sock, (struct sockaddr *)&un, (int *)&un_len) == -1) {
		log_err("unix socket getsockname()");
		return -1;
	}
*/
/* set non-blocking */
	optval = 1;
	if (ioctl(sock, FIONBIO, &optval) == -1) {
		log_err("unix socket ioctl()");
		return -1;
	}

	if (listen(sock, 3) == -1) {
		log_err("unix socket listen()");
		return -1;
	}
	return sock;
}

#ifdef OLD_CODE

#ifndef TELOPT_NAWS
#define TELOPT_NAWS 31			/* negotiate about window size */
#endif

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON 39	/* set new environment variable */
#endif


int telnet_negotiations(User *usr, unsigned char c) {
char buf[20];

	if (usr == NULL)
		return -1;

	Enter(telnet_negotiations);

	switch(usr->telnet_state) {
		case TS_DATA:
			switch(c) {
				case 0:
				case '\n':
					Return -1;

				case 0x7f:			/* DEL/BS conversion */
					c = KEY_BS;
					break;

				case IAC:
					usr->telnet_state = TS_IAC;
					Return -1;

				default:
					if (c > 0x7f) {
						Return -1;
					}
			}
			Return c;

		case TS_IAC:
			switch(c) {
				case IAC:				/* IAC IAC received */
					usr->telnet_state = TS_DATA;
					Return 0;			/* return 0, although the BBS probably won't like it much ... */

				case AYT:
					Print(usr, "\n[YeS]\n");
					usr->telnet_state = TS_DATA;
					Return -1;

				case NOP:
					usr->telnet_state = TS_DATA;
					Return -1;

				case SB:
					usr->in_sub++;
					Return -1;

				case SE:
					usr->in_sub--;
					if (usr->in_sub < 0)
						usr->in_sub = 0;
					usr->telnet_state = TS_DATA;
					Return -1;

				case WILL:
					usr->telnet_state = TS_WILL;
					Return -1;

				case DO:
					usr->telnet_state = TS_DO;
					Return -1;

/* after a SB we can have... */
				case TELOPT_NAWS:
					usr->telnet_state = TS_NAWS;
					Return -1;

				case TELOPT_NEW_ENVIRON:
					usr->telnet_state = TS_NEW_ENVIRON;
					Return -1;
			}
			usr->telnet_state = TS_ARG;
			Return -1;


		case TS_WILL:
			switch(c) {
				case TELOPT_NAWS:
					break;

				case TELOPT_NEW_ENVIRON:		/* NEW-ENVIRON SEND */
					sprintf(buf, "%c%c%c%c%c%c", IAC, SB, TELOPT_NEW_ENVIRON, 1, IAC, SE);
					if (usr->conn->sock > 0)
						write(usr->conn->sock, buf, 6);
					break;

				default:
					sprintf(buf, "%c%c%c", IAC, DONT, c);
					if (usr->conn->sock > 0)
						write(usr->conn->sock, buf, 3);
			}
			usr->telnet_state = TS_DATA;
			Return -1;

		case TS_DO:
			switch(c) {
				case TELOPT_SGA:
					break;

				case TELOPT_ECHO:
					break;

				default:
					sprintf(buf, "%c%c%c", IAC, WONT, c);
					if (usr->conn->sock > 0)
						write(usr->conn->sock, buf, 3);
			}
			usr->telnet_state = TS_DATA;
			Return -1;


		case TS_NAWS:
			if (usr->in_sub <= 4)				/* expect next NAWS byte */
				usr->in_sub_buf[usr->in_sub++] = c;
			else {
				int width, height;

				width = (unsigned int)usr->in_sub_buf[1] & 0xff;
				width <<= 8;
				width |= ((unsigned int)usr->in_sub_buf[2] & 0xff);

				height = (unsigned int)usr->in_sub_buf[3] & 0xff;
				height <<= 8;
				height |= ((unsigned int)usr->in_sub_buf[4] & 0xff);

				usr->term_width = width;
				if (usr->term_width < 1)
					usr->term_width = 80;

				usr->term_height = height-1;
				if (usr->term_height < 1)
					usr->term_height = 23;

				usr->in_sub = 0;
				usr->telnet_state = TS_IAC;			/* expect SE */
			}
			Return -1;

/*
	Environment variables
*/
		case TS_NEW_ENVIRON:
			if (c == 0)								/* IS */
				usr->telnet_state = TS_NEW_ENVIRON_IS;

			usr->in_sub = 1;
			usr->in_sub_buf[usr->in_sub] = 0;
			Return -1;

		case TS_NEW_ENVIRON_IS:
			switch(c) {
				case 0:									/* expect variable */
				case 2:
				case 3:
					usr->in_sub = 1;
					usr->in_sub_buf[usr->in_sub] = 0;
					usr->telnet_state = TS_NEW_ENVIRON_VAR;
					break;

				case IAC:
					usr->telnet_state = TS_IAC;			/* expect SE */
					break;

				default:
					usr->telnet_state = TS_DATA;		/* must be wrong */
			}
			Return -1;

		case TS_NEW_ENVIRON_VAR:
			if (c == 1) {
				usr->in_sub_buf[usr->in_sub++] = 0;
				usr->telnet_state = TS_NEW_ENVIRON_VAL;	/* expect value */
				Return -1;
			}
			if (c == IAC) {
				usr->telnet_state = TS_IAC;				/* expect SE */
				Return -1;
			}
			if (usr->in_sub < MAX_SUB_BUF - 2)
				usr->in_sub_buf[usr->in_sub++] = c;
			Return -1;

		case TS_NEW_ENVIRON_VAL:
			if (c <= 3 || c == IAC) {					/* next variable or end of list */
				usr->in_sub_buf[usr->in_sub] = 0;
				if (!strcmp(usr->in_sub_buf+1, "USER")) {
					int i;

					for(i = 6; i < usr->in_sub; i++)
						CURRENT_INPUT(usr, usr->in_sub_buf[i]);	/* enter it as login name */
					CURRENT_INPUT(usr, KEY_RETURN);
				}

/* variable has been processed ; get next one or end on IAC */

				usr->in_sub = 1;
				usr->in_sub_buf[usr->in_sub] = 0;

				if (c == IAC)
					usr->telnet_state = TS_IAC;
				else
					usr->telnet_state = TS_NEW_ENVIRON_VAR;
			} else {
				if (usr->in_sub < MAX_SUB_BUF - 2)
					usr->in_sub_buf[usr->in_sub++] = c;
			}
			Return -1;

		case TS_ARG:
			usr->telnet_state = usr->in_sub ? TS_IAC : TS_DATA;
			Return -1;

	}
	Return -1;
}

#endif

/*
	The Main Loop
	this is the connection engine
*/
void mainloop(void) {
struct timeval timeout;
fd_set rfds, wfds;
Conn *c, *c_next;
int err, highest_fd = -1, wait_for_input, nap;

	Enter(mainloop);

	this_user = NULL;

	setjmp(jumper);			/* trampoline for crashed connections */
	jump_set = 1;
	crash_recovery();		/* recover crashed connections */

	nap = 1;
	while(1) {
		wait_for_input = 1;
		highest_fd = 0;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		for(c = AllConns; c != NULL; c = c_next) {
			c_next = c->next;

/* remove dead connections */
			if (c->sock <= 0) {
				remove_Conn(&AllConns, c);
				destroy_Conn(c);
				continue;
			}
/*
	looping connections
*/
			if (c->state & CONN_LOOPING) {
				if (c->loop_counter)
					c->loop_counter--;

				c->conn_type->process(c, LOOP_STATE);

				if ((c->state & CONN_LOOPING) && !c->loop_counter) {
					c->state &= ~CONN_LOOPING;
					Ret(c);
				}
				wait_for_input = 0;
				continue;
			}
/*
	non-blocking listen()
*/
			if (c->state & CONN_LISTEN) {
				FD_SET(c->sock, &rfds);
				if (highest_fd <= c->sock)
					highest_fd = c->sock + 1;
				continue;
			}
/*
	non-blocking connect()
*/
			if (c->state & CONN_CONNECTING) {
				FD_SET(c->sock, &wfds);
				if (highest_fd <= c->sock)
					highest_fd = c->sock + 1;
				continue;
			}
/*
	connected and input ready
*/
			if (c->state & CONN_ESTABLISHED) {
				if (c->input_head < c->input_tail) {
					c->conn_type->process(c, c->inputbuf[c->input_head++]);
					if (c->input_head < c->input_tail) {
						wait_for_input = 0;
						continue;
					}
				}
/*
	no input ready, try to read()

				if (c->input_head >= c->input_tail) {
*/
				c->input_head = 0;
				if ((err = read(c->sock, c->inputbuf, MAX_INPUTBUF)) > 0) {
					c->input_tail = err;
					c->inputbuf[c->input_tail] = 0;
					wait_for_input = 0;
					continue;
				} else
					c->input_tail = 0;
#ifdef EWOULDBLOCK
				if (errno == EWOULDBLOCK) {
#else
				if (errno == EAGAIN) {
#endif
					FD_SET(c->sock, &rfds);
					if (highest_fd <= c->sock)
						highest_fd = c->sock + 1;
					continue;
				}
				if (errno == EINTR)		/* interrupted, try again next time */
					continue;
/*
	something bad happened, probably lost connection
*/
				c->conn_type->linkdead(c);
				continue;
			}
		}

/*
	call select() to see if any more data is ready to arrive

	select() is only called to see if input is ready
	To make things perfect, we should check if write() causes EWOULDBLOCK,
	setup an output buffer, and call select() with a write-set as well
*/
		timeout.tv_sec = (wait_for_input == 0) ? 0 : nap;
		timeout.tv_usec = 0;
		errno = 0;
		err = select(highest_fd, &rfds, &wfds, NULL, &timeout);
/*
	first update timers ... if we do it later, new connections can immediately
	time out
*/
		handle_pending_signals();
		nap = update_timers();

		if (err > 0) {
			for(c = AllConns; c != NULL; c = c_next) {
				c_next = c->next;

				if (c->sock > 0) {
					if (FD_ISSET(c->sock, &rfds)) {
						if (c->state & CONN_LISTEN)
							c->conn_type->accept(c);
						else
							c->conn_type->readable(c);
					}
					if (FD_ISSET(c->sock, &wfds)) {
						if (c->state & CONN_CONNECTING)
							c->conn_type->complete_connect(c);
						else
							c->conn_type->writable(c);
					}
				}
			}
		}
		if (err >= 0)				/* no error return from select() */
			continue;
/*
	handle select() errors
*/
		if (errno == EINTR)			/* interrupted, better luck next time */
			continue;

		if (errno == EBADF) {
			log_warn("mainloop(): select() got EBADF, continuing");
			continue;
		}
		if (errno == EINVAL) {
			log_warn("mainloop(): select() got EINVAL, continuing");
			continue;
		}
/*
	strange, select() got some other error
*/
		log_err("select() got errno %d", errno);
		log_err("exiting and restarting ; killing myself with SIGINT");
		kill(0, SIGINT);
	}
}

/* EOB */
