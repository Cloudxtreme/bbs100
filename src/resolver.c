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
	resolver.c	WJ99

	IP name resolver (IPv6 enabled)
	It is forked by the main program, and communicates with its parent
	through a Unix domain socket 
*/

#include "config.h"

#ifndef HAVE_SOCKET
#error This package relies on socket(), which is not available on this platform
#endif

#include "copyright.h"
#include "cstring.h"
#include "cstrerror.h"
#include "memset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>


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

/*
	resolve IP number in 'ip', return hostname in 'host'
	this function is IPv6 enabled
*/
int resolv(char *ip, char *host) {
int err;
struct addrinfo hints, *res, *ai_p;

#ifndef PF_UNSPEC
#error inet.c: PF_UNSPEC is undefined on this system
#endif
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((err = getaddrinfo(ip, NULL, &hints, &res)) != 0) {
		fprintf(stderr, "resolver: getaddrinfo(): %s\n", gai_strerror(err));
		return -1;
	}
	for(ai_p = res; ai_p != NULL; ai_p = ai_p->ai_next) {
		if ((err = getnameinfo((struct sockaddr *)ai_p->ai_addr, ai_p->ai_addrlen,
			host, NI_MAXHOST, NULL, 0, 0)) == 0)
			break;			/* use first name you get */
		else {
			fprintf(stderr, "resolver: getnameinfo(): %s\n", gai_strerror(err));
			freeaddrinfo(res);
			return -1;
		}
	}
	freeaddrinfo(res);

	fprintf(stderr, "D %s => %s", ip, host);
	return 0;
}

int main(int argc, char **argv) {
char request[128], result[NI_MAXHOST], errbuf[MAX_LINE];
struct sockaddr_un un;
int s, n, un_len;

	printf("%s", print_copyright(SHORT, "resolver", result, NI_MAXHOST));

	if (strcmp(basename(argv[0]), "(bbs100 resolver)")) {
		printf("You must not run this program by hand. It is supposed to be started by\n"
			"the bbs100 main program.\n");
		exit(1);
	}
/*
	connect to unix named socket
*/
	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket()");
		exit(-1);
	}
	un.sun_family = AF_UNIX;
	cstrncpy(un.sun_path, argv[1], sizeof(un.sun_path));
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
		if ((n = read(s, request, 127)) <= 0) {
			fprintf(stderr, "resolver: read(): %s\n", cstrerror(errno, errbuf, MAX_LINE));
			break;
		}
		request[n] = 0;
		result[0] = 0;
		if (resolv(request, result) < 0)
			continue;

		fprintf(stderr, "resolver: %s\n", result);
/*
	write the answer back to the unix socket connection
*/
		if (write(s, request, strlen(request)) < 0) {
			fprintf(stderr, "resolver: write(): %s\n", cstrerror(errno, errbuf, MAX_LINE));
			break;
		}
		if (write(s, " ", 1) < 0) {
			fprintf(stderr, "resolver: write(): %s\n", cstrerror(errno, errbuf, MAX_LINE));
			break;
		}
		if (write(s, result, strlen(result)) < 0) {
			fprintf(stderr, "resolver: write(): %s\n", cstrerror(errno, errbuf, MAX_LINE));
			break;
		}
		if (write(s, "\r", 1) < 0) {
			fprintf(stderr, "resolver: write(): %s\n", cstrerror(errno, errbuf, MAX_LINE));
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
