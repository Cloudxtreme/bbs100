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

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef TELCMDS
#define TELCMDS
#endif
#ifndef TELOPTS
#define TELOPTS
#endif

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

#ifndef TELOPT_NEW_ENVIRON
#define TELOPT_NEW_ENVIRON	39
#endif

#ifndef SUN_LEN
#define SUN_LEN(x)	(sizeof(*(x)) - sizeof((x)->sun_path) + strlen((x)->sun_path))
#endif


Wrapper *wrappers = NULL;
int main_socket = -1, dns_main_socket = -1, dns_socket = -1;


int inet_sock(unsigned int port) {
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

void new_connection(int fd) {
User *new_conn;
struct sockaddr_in client;
int client_len = sizeof(struct sockaddr_in);
char buf[256];
StringList *sl;
int s;
char optval;

	Enter(new_connection);

	if ((s = accept(fd, (struct sockaddr *)&client, (int *)&client_len)) < 0) {
		log_err("inet socket accept()");
		Return;
	}
	if ((new_conn = new_User()) == NULL) {
		log_err("Out of memory allocating new User");

		shutdown(s, 2);
		close(s);
		Return;
	}
	new_conn->socket = s;

	optval = 1;
	ioctl(new_conn->socket, FIONBIO, &optval);		/* set non-blocking */

	new_conn->ipnum = ntohl(client.sin_addr.s_addr);
	strncpy(new_conn->from_ip, inet_ntoa(client.sin_addr), MAX_LINE-1);
	new_conn->from_ip[MAX_LINE-1] = 0;

	sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c", IAC, WILL, TELOPT_SGA, IAC, WILL, TELOPT_ECHO,
		IAC, DO, TELOPT_NAWS, IAC, DO, TELOPT_NEW_ENVIRON);

	if (write(new_conn->socket, buf, strlen(buf)) < 0) {
		close_connection(new_conn, "bad new connection");
		Return;
	}
	log_auth("CONN (%s)", new_conn->from_ip);
	add_User(&AllUsers, new_conn);
	dns_gethostname(new_conn->from_ip);		/* send out request for hostname */

/*
	display the login screen
	it's a pity that we still do not know the user's terminal height+width,
	we don't have that input yet, and we're surely not going to wait for it
*/
	for(sl = login_screen; sl != NULL; sl = sl->next)
		Print(new_conn, "%s\n", sl->str);
	Print(new_conn, "        %s\n", print_copyright(SHORT, NULL, buf));

/*
	This code is commented out, but if you want to lock out sites
	permanently (rather than for new users only), I suggest you
	enable this code
	(inspired by Richard of MatrixBBS)

	if (!allow_Wrapper(wrappers, usr->ipnum)) {
		Put(usr, "\nSorry, but you're connecting from a site that has been locked out of the BBS.\n\n");
		close_connection(usr, "connection closed by wrapper");
		Return;
	}
*/
	CALL(new_conn, STATE_LOGIN_PROMPT);
	Return;
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
			sprintf(buf, "CLOSE %s (%s): ", usr->name, usr->from_ip);
		else
			sprintf(buf, "CLOSE (%s): ", usr->from_ip);

		va_start(ap, reason);
		vsprintf(buf+strlen(buf), reason, ap);
		va_end(ap);

		log_auth(buf);
	}
	if (usr->socket > 0) {
		Put(usr, "<default>\n");
		Flush(usr);
		shutdown(usr->socket, 2);
		close(usr->socket);
		usr->socket = -1;
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


void dns_gethostname(char *ipnum) {
	if (ipnum == NULL || !*ipnum || dns_socket < 0)
		return;

	if (write(dns_socket, ipnum, strlen(ipnum)) < 0) {
		log_err("dns write()");
	}
}

void dnsserver_io(void) {
int n;
char buf[MAX_LINE], *p;

	if ((n = read(dns_socket, buf, MAX_LINE-1)) <= 0) {
		log_err("dns read(), closing socket");

		shutdown(dns_socket, 2);
		close(dns_socket);
		dns_socket = -1;
		return;
	}
	if (n >= MAX_LINE)
		n = MAX_LINE-1;
	buf[n] = 0;

	if ((p = cstrchr(buf, ' ')) != NULL) {
		*p = 0;
		p++;
		if (!*p)
			return;
	}
	if (p != NULL && strcmp(buf, p)) {
		User *u;

		for(u = AllUsers; u != NULL; u = u->next)
			if (!strcmp(u->from_ip, buf))
				strcpy(u->from_ip, p);			/* fill in IP name */
	}
}


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
					if (usr->socket > 0)
						write(usr->socket, buf, 6);
					break;

				default:
					sprintf(buf, "%c%c%c", IAC, DONT, c);
					if (usr->socket > 0)
						write(usr->socket, buf, 3);
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
					if (usr->socket > 0)
						write(usr->socket, buf, 3);
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
						usr->callstack->ip(usr, usr->in_sub_buf[i]);	/* enter it as login name */
					usr->callstack->ip(usr,  KEY_RETURN);
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


/*
	The Main Loop
*/
void mainloop(void) {
struct timeval timeout;
fd_set fds;
User *c, *c_next;
int err, highest_fd = -1, nap;
char k;

	Enter(mainloop);

	this_user = NULL;

	setjmp(jumper);			/* trampoline for crashed users */
	jump_set = 1;
	crash_recovery();		/* recover crashed users */

	nap = 1;
	while(main_socket > 0) {
		FD_ZERO(&fds);
		FD_SET(main_socket, &fds);
		highest_fd = main_socket+1;

		if (dns_main_socket > 0) {
			FD_SET(dns_main_socket, &fds);
			if (dns_main_socket >= highest_fd)
				highest_fd = dns_main_socket+1;
		}
		if (dns_socket > 0) {
			FD_SET(dns_socket, &fds);
			if (dns_socket >= highest_fd)
				highest_fd = dns_socket+1;
		}
		timeout.tv_sec = nap;
		timeout.tv_usec = 0;

		for(c = AllUsers; c != NULL; c = c_next) {
			c_next = c->next;
/*
	dead connections
*/
			if (c->socket <= 0) {
				remove_OnlineUser(c);			/* probably already removed by close_connection() :P */
				remove_User(&AllUsers, c);
				destroy_User(c);
				continue;
			}
/*
	looping users
*/
			if (c->runtime_flags & RTF_LOOPING) {
				if (c->loop_counter)
					c->loop_counter--;

				process(c, LOOP_STATE);

				if ((c->runtime_flags & RTF_LOOPING) && !c->loop_counter) {
					c->runtime_flags &= ~RTF_LOOPING;
					RET(c);
				}
				timeout.tv_sec = 0;
				continue;
			}
/*
	regular users that have input ready
*/
			if (c->input_idx) {
				k = c->inputbuf[0];
				c->input_idx--;
				if (c->input_idx) {
					timeout.tv_sec = 0;
					memmove(c->inputbuf, &c->inputbuf[1], c->input_idx);	/* this is inefficient; better use a head and tail pointer */
				}
				process(c, k);

				if (c->input_idx)
					continue;
			}
			if (!c->input_idx && !(c->runtime_flags & RTF_SELECT)) {
/*
	input buffer empty, try to read()
*/
				if ((err = read(c->socket, c->inputbuf, MAX_INPUTBUF)) > 0) {
					c->input_idx = err;
					timeout.tv_sec = 0;
					continue;
				}
#ifdef EWOULDBLOCK
				if (errno == EWOULDBLOCK) {
#else
				if (errno == EAGAIN) {
#endif
					FD_SET(c->socket, &fds);
					if (c->socket >= highest_fd)
						highest_fd = c->socket+1;

					c->runtime_flags |= RTF_SELECT;		/* go wait for select() */
					continue;
				}
				if (errno == EINTR)			/* interrupted, try again next time */
					continue;
/*
	something bad happened, probably lost connection
*/
				if (c->name[0]) {
					notify_linkdead(c);
					log_auth("LINKDEAD %s (%s)", c->name, c->from_ip);
					close_connection(c, "%s went linkdead", c->name);
				} else
					close_connection(c, NULL);
				continue;
			}
/*
	regular users waiting for select()
*/
			if (c->runtime_flags & RTF_SELECT) {
				FD_SET(c->socket, &fds);
				if (c->socket >= highest_fd)
					highest_fd = c->socket+1;
			}
		}
/*
	call select() to see if any more data is ready to arrive

	Note that the name resolver has no buffers like the users do
	To make things perfect, the name resolver should be changed to
	use buffers as well

	select() is only called to see if input is ready
	To make things perfect, we should check if write() causes EWOULDBLOCK,
	setup an output buffer, and call select() with a write-set as well
*/
		errno = 0;
		err = select(highest_fd, &fds, NULL, NULL, &timeout);

/*
	first update timers ... if we do it later, new connections can immediately
	time out
*/
		handle_pending_signals();
		nap = update_timers();

		if (err > 0) {
/*
	handle new connections
*/
			if (main_socket > 0 && FD_ISSET(main_socket, &fds))
				new_connection(main_socket);
/*
	handle name resolver I/O
*/
			if (dns_socket > 0 && FD_ISSET(dns_socket, &fds))
				dnsserver_io();

			if (dns_main_socket > 0 && FD_ISSET(dns_main_socket, &fds)) {
				int un_len;
				struct sockaddr_un un;

				if (dns_socket > 0) {
					FD_CLR(dns_socket, &fds);
					shutdown(dns_socket, 2);
					close(dns_socket);
				}
				un_len = sizeof(struct sockaddr_un);
				dns_socket = accept(dns_main_socket, (struct sockaddr *)&un, (int *)&un_len);
				if (dns_socket < 0) {
					log_err("dns accept()");
				} else {
					FD_SET(dns_socket, &fds);
					if (dns_socket >= highest_fd)
						highest_fd = dns_socket+1;
				}
			}
/*
	handle user I/O
	instead of calling read() immediately, we will do it the next loop
*/
			for(c = AllUsers; c != NULL; c = c_next) {
				c_next = c->next;

				if ((c->runtime_flags & RTF_SELECT) && c->socket > 0 && FD_ISSET(c->socket, &fds)) {
					c->runtime_flags &= ~RTF_SELECT;
					c->input_idx = 0;
					FD_CLR(c->socket, &fds);
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
/*
	there is a bad connection amongst us..!
	we rebuild the fdset
*/
		if (errno == EBADF) {
			FD_ZERO(&fds);
			FD_SET(main_socket, &fds);
			highest_fd = main_socket+1;

			if (dns_main_socket > 0) {
				FD_SET(dns_main_socket, &fds);
				if (dns_main_socket >= highest_fd)
					highest_fd = dns_main_socket+1;
			}
			if (dns_socket > 0) {
				FD_SET(dns_socket, &fds);
				if (dns_socket >= highest_fd)
					highest_fd = dns_socket+1;
			}
			for(c = AllUsers; c != NULL; c = c->next) {
				if (c->runtime_flags & RTF_SELECT) {
					if (c->socket > 0) {
						FD_SET(c->socket, &fds);
						if (c->socket >= highest_fd)
							highest_fd = c->socket+1;
					} else
						c->runtime_flags &= ~RTF_SELECT;
				}
			}
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
