/*
    bbs100 2.0 WJ104
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
	log.c	WJ103

	- complete rewrite of logging code
	- more 'syslog'-like to the outside world
	- automatic rotation
	- automatic archiving
*/

#include "config.h"
#include "log.h"
#include "defines.h"
#include "StringList.h"
#include "Param.h"
#include "util.h"
#include "Timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

StringList *internal_log = NULL;
int internal_log_len = 0;

Timer *logrotate_timer = NULL;


static int logrotate_reset_timer(void);

static void logrotate_timerfunc(void *dummy) {
time_t t;
struct tm *tm;

	dummy = NULL;

	t = rtc;
	tm = gmtime(&t);

	if (!cstricmp(PARAM_LOGROTATE, "none"))
		return;

	if (!cstricmp(PARAM_LOGROTATE, "daily")
		|| (!cstricmp(PARAM_LOGROTATE, "weekly") && tm->tm_wday == 1)
		|| (!cstricmp(PARAM_LOGROTATE, "monthly") && tm->tm_mday == 1)
		|| (!cstricmp(PARAM_LOGROTATE, "yearly") && tm->tm_yday == 1))
		log_rotate();
	else
		log_err("unknown value '%s' for param logrotate", PARAM_LOGROTATE);

	logrotate_reset_timer();
}

/*
	initialize rotation timer
*/
static int logrotate_reset_timer(void) {
time_t t;
struct tm *tm;

	if (logrotate_timer == NULL) {
		if ((logrotate_timer = new_Timer(SECS_IN_DAY, logrotate_timerfunc, TIMER_RESTART)) == NULL) {
			log_err("logrotate_reset_timer(): failed to allocate a new Timer");
			return -1;
		}
	} else
		remove_Timer(&timerq, logrotate_timer);

	t = rtc;
	tm = localtime(&t);
/*
	sleep exactly till midnight
	the timer must be re-inserted into the timerq because the queue is sorted
*/
	logrotate_timer->sleeptime = SECS_IN_DAY - tm->tm_hour * SECS_IN_HOUR - tm->tm_min * SECS_IN_MIN - tm->tm_sec;
	add_Timer(&timerq, logrotate_timer);
	return 0;
}

int init_log(void) {
int fd;

	if ((fd = open(PARAM_SYSLOG, O_WRONLY | O_CREAT | O_APPEND, (mode_t)0640)) == -1)
		log_err("failed to open logfile %s", PARAM_SYSLOG);
	else {
#ifndef HAVE_DUP2
#error This platform has no dup2() function
#endif
		dup2(fd, fileno(stdout));
		close(fd);
	}
	if ((fd = open(PARAM_AUTHLOG, O_WRONLY | O_CREAT | O_APPEND, (mode_t)0640)) == -1)
		log_err("failed to open logfile %s\n", PARAM_AUTHLOG);
	else {
#ifndef HAVE_DUP2
#error This platform has no dup2() function
#endif
		dup2(fd, fileno(stderr));
		close(fd);
	}
	if (!cstricmp(PARAM_LOGROTATE, "none"))
		return 0;

	return logrotate_reset_timer();
}


void log_entry(FILE *f, char *msg, char level, va_list ap) {
time_t t;
struct tm *tm;
char buf[4096];

	t = rtc;
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

void log_warn(char *msg, ...) {
va_list ap;

	va_start(ap, msg);
	log_entry(stdout, msg, 'W', ap);
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

/*
	move_log() archives the logfile in the PARAM_ARCHIVEDIR directory
*/
static void move_log(char *logfile) {
char filename[MAX_PATHLEN], *p;
time_t t;
struct tm *tm;

	t = rtc;
	t -= SECS_IN_DAY/2;			/* take week/month number of yesterday */
	tm = localtime(&t);
	sprintf(filename, "%s/%04d/%02d", PARAM_ARCHIVEDIR, tm->tm_year+1900, tm->tm_mon+1);
	path_strip(filename);
	if (mkdir_p(filename) < 0)
		return;

/* 'basename' logfile */
	if ((p = cstrrchr(logfile, '/')) == NULL)
		p = logfile;
	else {
		p++;
		if (!*p)
			p = logfile;
	}
	sprintf(filename, "%s/%04d/%02d/%s.%04d%02d%02d", PARAM_ARCHIVEDIR, tm->tm_year+1900, tm->tm_mon+1,
		p, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	path_strip(filename);

	if (rename(logfile, filename) == -1)
		log_err("failed to rename %s to %s", logfile, filename);
}

/*
	rotate: close the logfile and start a new one
*/
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
