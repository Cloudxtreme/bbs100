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
	log.c	WJ103

	- complete rewrite of logging code
	- more 'syslog'-like to the outside world
*/

#include "config.h"
#include "log.h"
#include "defines.h"
#include "StringList.h"
#include "Param.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

StringList *internal_log = NULL;
int internal_log_len = 0;


int init_log(void) {
int fd;

	if ((fd = open(PARAM_SYSLOG, O_WRONLY | O_CREAT | O_APPEND, (mode_t)0640)) == -1)
		log_err("failed to open logfile %s", PARAM_SYSLOG);
	else {
		dup2(fd, fileno(stdout));
		close(fd);
	}
	if ((fd = open(PARAM_AUTHLOG, O_WRONLY | O_CREAT | O_APPEND, (mode_t)0640)) == -1)
		log_err("failed to open logfile %s\n", PARAM_AUTHLOG);
	else {
		dup2(fd, fileno(stderr));
		close(fd);
	}
	return 0;
}


void log_entry(FILE *f, char *msg, char level, va_list ap) {
time_t t;
struct tm *tm;
char buf[4096];

	t = time(NULL);
	tm = localtime(&t);		/* logging goes in localtime */

	sprintf(buf, "%c%c%c %2d %02d:%02d:%02d %c ", Months[tm->tm_mon][0], Months[tm->tm_mon][1], Months[tm->tm_mon][2],
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, level);
	vsprintf(buf+strlen(buf), msg, ap);

	if (internal_log_len > MAX_INTERNAL_LOG) {
		StringList *sl;

		sl = internal_log;
		remove_StringList(&internal_log, sl);
		destroy_StringList(sl);
		internal_log_len--;
	}
	add_StringList(&internal_log, new_StringList(buf));
	internal_log_len++;

	fprintf(f, "%s\n", buf);
}

void log_msg(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, ' ', ap);
	va_end(ap);
}

void log_info(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'I', ap);
	va_end(ap);
}

void log_err(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'E', ap);
	va_end(ap);
}

void log_debug(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'D', ap);
	va_end(ap);
}

void log_auth(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stderr, msg, 'A', ap);
	va_end(ap);
}

static void move_log(char *logfile) {
int n;
char filename[MAX_PATHLEN];
struct stat statbuf;

	for(n = 1; n < 9999; n++) {
		sprintf(filename, "%s.%d", PARAM_SYSLOG, n);
		if (stat(filename, &statbuf) == -1)
			break;
	}
	if (rename(logfile, filename) == -1)
		log_err("failed to rename %s to %s", logfile, filename);
}

void log_rotate(void) {
	log_entry(stdout, "switching to new log", 'I', NULL);
	log_entry(stderr, "switching to new log", 'I', NULL);

	move_log(PARAM_SYSLOG);
	move_log(PARAM_AUTHLOG);
	init_log();

	log_entry(stdout, "start of new log", 'I', NULL);
	log_entry(stderr, "start of new log", 'I', NULL);
}

/* EOB */
