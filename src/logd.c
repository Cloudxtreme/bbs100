/*
    bbs100 1.2.1 WJ103
    Copyright (C) 2003  Walter de Jong <walter@heiho.net>

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
	logd.c	WJ99

	the logger process is forked by the main program and it logs everything
	into the appropriate log files
	The main program's stderr is piped to the logger's stdin
*/

#include <config.h>

#include "copyright.h"
#include "sys_time.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif


typedef struct {
	char *id, *filename;
	FILE *f;
} LogFile;

LogFile logfiles[4] = {
	{ "MSG ",	"messages",	NULL },
	{ "ERR ",	"errors" ,	NULL },
	{ "AUTH ",	"auth",		NULL },
	{ "TD ",	"debug",	NULL },
};

int sigpiped = 0;


char *get_basename(char *path) {
char *p;

	if ((p = strrchr(path, '/')) == NULL)
		return path;
	p++;
	if (!*p)
		return path;
	return p;
}

/* WARNING: this function returns a static buffer */
char *logtime(void) {
static char buf[64];
time_t t;
struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);

	sprintf(buf, "%02d/%02d/%d %2d:%02d:%02d", tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	return buf;
}

int open_logfiles(int day) {
int i, num;
char filename[64];

	num = sizeof(logfiles)/sizeof(LogFile);
	for(i = 0; i < num; i++) {
		sprintf(filename, "log/%s/%s.%02d", logfiles[i].filename, logfiles[i].filename, day);
		if ((logfiles[i].f = fopen(filename, "a")) == NULL) {
			fprintf(stderr, "logd: failed to open logfile '%s'\n", filename);
			return -1;
		}
#ifdef SETVBUF_REVERSED
		setvbuf(logfiles[i].f, _IOLBF, NULL, 256);
#else
		setvbuf(logfiles[i].f, NULL, _IOLBF, 256);
#endif
		fprintf(logfiles[i].f, "\n%s logd: opened log, logd pid %lu\n", logtime(), (unsigned long)getpid());
	}
	return 0;
}

void close_logfiles(void) {
int i, num;

	num = sizeof(logfiles)/sizeof(LogFile);
	for(i = 0; i < num; i++) {
		if (logfiles[i].f != NULL) {
			fprintf(logfiles[i].f, "%s logd: closing log, bye\n", logtime());
			fclose(logfiles[i].f);
			logfiles[i].f = NULL;
		}
	}
}

int remove_logfiles(int day) {
int i, num;
char filename[64];

	num = sizeof(logfiles)/sizeof(LogFile);
	for(i = 0; i < num; i++) {
		sprintf(filename, "%s/%s.%02d", logfiles[i].filename, logfiles[i].filename, day);
		unlink(filename);
	}
	return 0;
}

/*
	Log() writes the messages to the appropriate log file
	If it doesn't know where a message belongs, it dumps it to stderr
*/
void Log(char *msg) {
int i, num;
char *msgp;

	if (msg == NULL || !*msg || (*msg == '\n' && !msg[1]))
		return;

	if ((msgp = strchr(msg, ' ')) == NULL) {
		fprintf(stderr, "%s %s", logtime(), msg);
		return;
	}
	msgp++;
	if (!*msgp) {
		fprintf(stderr, "%s %s\n", logtime(), msg);
		return;
	}
	num = sizeof(logfiles)/sizeof(LogFile);
	if (!strncmp(msg, "ALL ", 4)) {
		for(i = 0; i < num; i++)
			fprintf(logfiles[i].f, "%s %s", logtime(), msgp);
		return;
	} else {
		for(i = 0; i < num; i++) {
			if (!strncmp(msg, logfiles[i].id, strlen(logfiles[i].id))) {
				fprintf(logfiles[i].f, "%s %s", logtime(), msgp);
				return;
			}
		}
	}
	fprintf(stderr, "%s %s", logtime(), msg);
}


RETSIGTYPE sigterm_handler(int sig) {
	Log("ALL logd: going down on SIGTERM\n");
	fprintf(stderr, "%s logd: going down on SIGTERM\n", logtime());
	close_logfiles();
	exit(128+sig);
}


void init_signals(void) {
struct sigaction sa;
struct sigaction old_sa;

	sigfillset(&sa.sa_mask);
#ifdef SA_RESTART
	sa.sa_flags = SA_RESTART;
#endif
	sa.sa_handler = sigterm_handler;
	sigaction(SIGTERM, &sa, &old_sa);
}


int main(int argc, char **argv) {
time_t t;
struct tm *tm;
int day_now;
char buf[1024];

	printf("%s", print_copyright(SHORT, "logd"));

	if (strcmp(get_basename(argv[0]), "(bbs100 logd)")) {
		printf("You must not run this program by hand. It is supposed to be started by\n"
			"the bbs100 main program.\n");
		exit(1);
	}
	t = time(NULL);
	tm = localtime(&t);
	day_now = tm->tm_mday;
	if (open_logfiles(day_now))
		exit(-1);

	if (argv[0][0] == '(') {	/* started as daemon */
		close(1);
		close(2);
	}
	init_signals();

	sprintf(buf, "ALL logd: up and running, pid %lu\n", (unsigned long)getpid());
	Log(buf);

/*
	now, give the parent time to connect the pipes
	when done, the parent will kill() the sleep() with SIGCONT
*/
	Log("MSG logd: waiting for parent to connect pipes\n");
	sleep(15);
	Log("MSG logd: continuing\n");

	while(fgets(buf, 1024, stdin) != NULL) {
/*
	check the time (is done for logfile cycling)
	this used to be in a SIGALRM handler, but still caused problems :P
*/
		t = time(NULL);
		tm = localtime(&t);
		if (tm->tm_mday != day_now) {
			Log("ALL logd: switching to new log\n");
			close_logfiles();

			day_now = tm->tm_mday;
			remove_logfiles(day_now);		/* delete old logfiles */
			open_logfiles(day_now);
		}
		if (*buf && !(*buf == '\n' && !buf[1]))
			Log(buf);
	}
	Log("ALL logd: going down on closed pipe\n");

	close_logfiles();

	fprintf(stderr, "%s logd: terminated\n", logtime());
	exit(0);
	return 0;
}

/* EOB */
