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
	guard.c	WJ106

	The guard process launches the main program and restarts it in case
	of a crash or reboot
*/

#include "config.h"
#include "version.h"
#include "sys_wait.h"
#include "my_fcntl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif


/*
	if the program exits within MIN_RUNTIME seconds, it won't be
	restarted automatically
*/
#define MIN_RUNTIME		60

#ifndef MAX_PATHLEN
#define MAX_PATHLEN		1024
#endif

char currdir[MAX_PATHLEN];


/*
	first start runs in the foreground and launches the program
*/
void first_start(char **argv) {
int fds[2], err, status;
char buf[256];
pid_t pid;

	if (pipe(fds) == -1) {
		perror("guard: pipe() failed");
		exit(-1);
	}
	pid = fork();
	if (pid == (pid_t)-1L) {
		perror("guard: fork() failed");
		exit(-1);
	}
	if (!pid) {				/* child */
		close(fds[0]);

		close(fileno(stderr));
		if (dup2(fileno(stdout), fileno(stderr)) == -1) {
			perror("guard child: dup2() failed");
			exit(-1);
		}
		close(fileno(stdout));
		if (dup2(fds[1], fileno(stdout)) == -1) {
			perror("guard child: dup2() failed");
			exit(-1);
		}
		close(fds[1]);

		if (chdir(currdir) == -1) {
			perror("guard: failed to change directory");
			exit(-1);
		}
		execv(argv[0], argv);
		perror("guard child(): execv() failed");
		exit(-1);
	}
	close(fds[1]);

	waitpid(-1, &status, WNOHANG);

	while((err = read(fds[0], buf, 256)) > 0)
		write(fileno(stdout), buf, err);

	close(fds[0]);
	close(fds[1]);
}

void goto_background(void) {
pid_t pid;

#ifdef TIOCNOTTY
int fd;

	if ((fd = open("/dev/tty", O_WRONLY 	/* detach from tty */
#ifdef O_NOCTTY
| O_NOCTTY
#endif
	)) != -1) {
		ioctl(fd, TIOCNOTTY);
		close(fd);
	}
#endif	/* TIOCNOTTY */

	pid = fork();							/* go to background */
	if (pid == (pid_t)-1L) {
		perror("guard: fork() failed");
		exit(-1);
	}
	if (pid != (pid_t)0L)					/* parent: exit */
		exit(0);

	setsid();								/* become new process group leader */

	pid = fork();
	if (pid == (pid_t)-1L) {
		perror("guard: fork() failed");
		exit(-1);
	}
	if (pid != (pid_t)0L)					/* parent: exit */
		exit(0);
}

/*
	monitor and restart the program, if necessary
*/
void restart_program(char **argv, time_t start) {
time_t end;
int status;
pid_t pid;

	if (argv == NULL || *argv == NULL || !**argv) {
		fprintf(stderr, "guard: invalid argument\n");
		exit(-1);
	}
	for(;;) {
		for(;;) {
			pid = waitpid(-1, &status, 0);
			if (pid == -1) {
				perror("guard: waitpid() failed");
				exit(-1);
			}
			if (WIFEXITED(status) || WIFSIGNALED(status))
				break;
		}
		end = time(NULL);
		if (end - start < MIN_RUNTIME) {		/* if program ran too short, don't restart */
			fprintf(stderr, "guard: program ran too short, exiting\n");
			exit(0);
		}
		if (WIFEXITED(status) && !WEXITSTATUS(status)) {	/* normal exit (BBS shutdown) */
			exit(0);
		}
		if (WIFSIGNALED(status) && (WTERMSIG(status) == SIGKILL || WTERMSIG(status) == SIGTERM)) {
			exit(128 + WTERMSIG(status));
		}
/* BBS reboot or crash; restart the program */
		pid = fork();
		if (pid == (pid_t)-1) {
			perror("guard: fork() failed");
			exit(-1);
		}
		if (!pid) {
			if (chdir(currdir) == -1) {
				perror("guard: failed to change directory");
				exit(-1);
			}
			execv(argv[0], argv);
			perror("guard child: exec() failed");
			exit(-1);
		}
		start = time(NULL);
	}
}

int main(int argc, char **argv) {
time_t start;

	if (argv[0][0] != '(') {
		char *old_argv0, *new_argv0 = "(bbs100 guard)";

		old_argv0 = argv[0];
		argv[0] = new_argv0;
		execv(old_argv0, argv);			/* restart with new name */

		fprintf(stderr, "bbs100: startup failed\n");
		exit(-1);
	}
	printf("bbs100 guard %s by Walter de Jong <walter@heiho.net> (C) 1997-2015\n", VERSION);
	printf("bbs100 comes with ABSOLUTELY NO WARRANTY. This is free software.\n"
		"For details, see the GNU General Public License.\n\n");

	if (argc <= 1) {
		printf("usage: guard <main program>\n");
		return 1;
	}
	if (getcwd(currdir, MAX_PATHLEN) == NULL) {
		perror("guard: can't get current directory");
		exit(-1);
	}
	if (chdir("/") == -1) {
		perror("guard: failed to change directory to / ");
		exit(-1);
	}
	start = time(NULL);

/*
	Odd ... when guard closes stdin, the main program's listen()
	will go into TIME_WAIT and eventually close automatically

	close(fileno(stdin));
*/

/*
	bah ... we have to go to the background immediately, otherwise
	waitpid() won't find this child, as it will the child of another
	process ...
*/
	goto_background();

/*
	start the program
*/
	first_start(&argv[1]);

/*
	Odd (2) ...
	When guard closes stdout/stderr, accept() will fail when SO_REUSEADDR
	is used on the listen()ing socket

	close(fileno(stdout));
	close(fileno(stderr));
*/

/*
	monitor and restart the program if necessary
*/
	restart_program(&argv[1], start);
	return 0;
}

/* EOB */
