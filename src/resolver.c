/*
    bbs100 2.1 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	resolver.c	WJ99

	IP name resolver
	It is forked by the main program, and communicates with its parent
	through a Unix domain socket 
*/

#include "config.h"
#include "copyright.h"
#include "sys_time.h"
#include "cstring.h"
#include "strerror.h"
#include "ipv4_aton.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif


char *get_basename(char *path) {
char *p;

	if ((p = strrchr(path, '/')) == NULL)
		return path;
	p++;
	if (!*p)
		return path;
	return p;
}

RETSIGTYPE sigpipe_handler(int sig) {
	fprintf(stderr, "resolver: SIGPIPE caught, terminating\n");
	exit(128+sig);
}

void init_signals(void) {
struct sigaction sa;
struct sigaction old_sa;

	sigfillset(&sa.sa_mask);
#ifdef SA_RESTART
	sa.sa_flags = SA_RESTART;
#endif
	sa.sa_handler = sigpipe_handler;
	sigaction(SIGPIPE, &sa, &old_sa);
}


int main(int argc, char **argv) {
char buf[256], buf2[256];
struct sockaddr_un un;
int s, n, un_len;
struct hostent *host;
struct in_addr ipnum;

	printf("%s", print_copyright(SHORT, "resolver", buf));

	if (strcmp(get_basename(argv[0]), "(bbs100 resolver)")) {
		printf("You must not run this program by hand. It is supposed to be started by\n"
			"the bbs100 main program.\n");
		exit(1);
	}
	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket()");
		exit(-1);
	}
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, argv[1]);
	un_len = sizeof(struct sockaddr_un);

	if (connect(s, (struct sockaddr *)&un, un_len) < 0) {
		perror("client: connect()");
		shutdown(s, 2);
		close(s);
		exit(-1);
	}
	if (argv[0][0] == '(') {
		close(0);
		close(1);
		close(2);
	}
	fprintf(stderr, "resolver: connected to %s\n", argv[1]);
	unlink(argv[1]);

	init_signals();

	for(;;) {
		if ((n = read(s, buf, 256)) <= 0) {
			fprintf(stderr, "resolver: read(): %s\n", strerror(errno));
			break;
		}
		if (n >= 256)
			n = 255;
		buf[n] = 0;

		if (sscanf(buf, "%d.%d.%d.%d", &n, &n, &n, &n) != 4) {
			fprintf(stderr, "resolver: got malformed ip address '%s'\n", buf);
			continue;
		}
		if (ipv4_aton(buf, &ipnum) == 0) {
			sprintf(buf2, "%s %s", buf, buf);
			goto reply;
		}
		if ((host = gethostbyaddr((char *)&ipnum, 4, AF_INET)) == NULL) {
			sleep(1);
			if ((host = gethostbyaddr((char *)&ipnum, 4, AF_INET)) == NULL) {
				sprintf(buf2, "%s %s", buf, buf);
				goto reply;
			}
		}
		sprintf(buf2, "%s %s", buf, host->h_name);
reply:
		fprintf(stderr, "resolver: %s\n", buf2);
		if (write(s, buf2, strlen(buf2)) < 0) {
			fprintf(stderr, "resolver: write(): %s\n", strerror(errno));
			break;
		}
	}
	shutdown(s, 2);
	close(s);
	fprintf(stderr, "resolver: terminated\n");
	exit(0);
	return 0;
}

/* EOB */
