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
	log.c	WJ103

	- more 'syslog'-like to the outside world
	- automatic rotation
	- automatic archiving
*/

#include "config.h"
#include "log.h"
#include "defines.h"
#include "cstring.h"
#include "StringList.h"
#include "Param.h"
#include "util.h"
#include "Timer.h"
#include "Memory.h"
#include "locale_system.h"
#include "cstring.h"
#include "sys_time.h"
#include "my_fcntl.h"
#include "bufprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

static int console_log = 0;

StringList *internal_log = NULL;
int internal_log_len = 0;

Timer *logrotate_timer = NULL;


static void logrotate_reset_timer(void);

static void logrotate_timerfunc(void *dummy) {
time_t t;
struct tm *tm;

	dummy = NULL;

	t = rtc;
	tm = gmtime(&t);

	if (!cstricmp(PARAM_LOGROTATE, "never")) {
		logrotate_reset_timer();			/* keep timer running correctly anyway */
		return;
	}
	if (!cstricmp(PARAM_LOGROTATE, "daily")
		|| (!cstricmp(PARAM_LOGROTATE, "weekly") && tm->tm_wday == 1)
		|| (!cstricmp(PARAM_LOGROTATE, "monthly") && tm->tm_mday == 1)
		|| (!cstricmp(PARAM_LOGROTATE, "yearly") && tm->tm_yday == 1))
		log_rotate();						/* will do an implicit logrotate_reset_timer() */
	else {
		log_err("unknown value '%s' for param logrotate, resetting to 'daily'", PARAM_LOGROTATE);
		Free(PARAM_LOGROTATE);
		PARAM_LOGROTATE = cstrdup("daily");
	}
	logrotate_reset_timer();
}

/*
	reset the rotation timer, so that it will rotate again the next day
*/
static void logrotate_reset_timer(void) {
time_t t;
struct tm *tm;

	if (logrotate_timer == NULL)
		return;

	t = rtc;
	tm = localtime(&t);
/*
	sleep exactly till midnight
	only modifying the sleeptime is enough; the timer will be re-inserted (in sorted order)
	into the timer queue by update_timerqueue()
*/
	logrotate_timer->sleeptime = SECS_IN_DAY - tm->tm_hour * SECS_IN_HOUR - tm->tm_min * SECS_IN_MIN - tm->tm_sec;
}

int init_log(int debugger) {
int fd;

	if (debugger > 0)
		console_log = 1;

	if (logrotate_timer == NULL) {
		if ((logrotate_timer = new_Timer(SECS_IN_DAY, logrotate_timerfunc, TIMER_RESTART)) == NULL) {
			log_err("init_log(): failed to allocate a new Timer");
			return -1;
		}
		logrotate_reset_timer();
		add_Timer(&timerq, logrotate_timer);
	}
	if (!console_log) {
		if ((fd = open(PARAM_SYSLOG, O_WRONLY | O_CREAT | O_APPEND, (mode_t)0640)) == -1) {
			log_err("failed to open logfile %s", PARAM_SYSLOG);
			return -1;
		}
#ifndef HAVE_DUP2
#error This platform has no dup2() function
#endif
		close(fileno(stdout));
		dup2(fd, fileno(stdout));
		close(fd);

		if ((fd = open(PARAM_AUTHLOG, O_WRONLY | O_CREAT | O_APPEND, (mode_t)0640)) == -1) {
			log_err("failed to open logfile %s\n", PARAM_AUTHLOG);
			return -1;
		}
#ifndef HAVE_DUP2
#error This platform has no dup2() function
#endif
		close(fileno(stderr));
		dup2(fd, fileno(stderr));
		close(fd);
	}
	if (!cstricmp(PARAM_LOGROTATE, "none")) {		/* old; backwards compatibility */
		Free(PARAM_LOGROTATE);
		PARAM_LOGROTATE = cstrdup("never");
	}
	if (!cstricmp(PARAM_LOGROTATE, "never")
		|| !cstricmp(PARAM_LOGROTATE, "daily")
		|| !cstricmp(PARAM_LOGROTATE, "weekly")
		|| !cstricmp(PARAM_LOGROTATE, "monthly")
		|| !cstricmp(PARAM_LOGROTATE, "yearly"))
		return 0;

	log_warn("init_log(): unknown value '%s' for param logrotate; resetting to 'daily'");
	Free(PARAM_LOGROTATE);
	PARAM_LOGROTATE = cstrdup("daily");
	return 0;
}

void log_entry(FILE *f, char *msg, char level, va_list ap) {
time_t t;
struct tm *tm;
char buf[MAX_LOGLINE];
int l;

	t = rtc;
	tm = localtime(&t);		/* logging goes in localtime */

	l = bufprintf(buf, sizeof(buf), "%c%c%c %2d %02d:%02d:%02d %c ", lc_system->months[tm->tm_mon][0], lc_system->months[tm->tm_mon][1], lc_system->months[tm->tm_mon][2],
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, level);
	bufvprintf(buf+l, sizeof(buf) - l, msg, ap);
/*
	This is nice but it causes deadly recursion when Malloc() is trying
	to log something

	if (internal_log_len > MAX_INTERNAL_LOG) {
		StringList *sl;

		sl = internal_log;
		remove_StringList(&internal_log, sl);
		destroy_StringList(sl);
		internal_log_len--;
	}
	add_StringList(&internal_log, new_StringList(buf));
	internal_log_len++;
*/
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
char filename[MAX_PATHLEN+10], *p;
time_t t;
struct tm *tm;
int i;

	t = rtc;
	t -= SECS_IN_DAY/2;			/* take week/month number of yesterday */
	tm = localtime(&t);
	bufprintf(filename, sizeof(filename), "%s/%04d/%02d", PARAM_ARCHIVEDIR, tm->tm_year+1900, tm->tm_mon+1);
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
	bufprintf(filename, sizeof(filename), "%s/%04d/%02d/%s.%04d%02d%02d", PARAM_ARCHIVEDIR, tm->tm_year+1900, tm->tm_mon+1,
		p, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	path_strip(filename);

/*
	defensive programming so that logfiles don't get overwritten easily
*/
	p = filename + strlen(filename);
	for(i = 1; i < 10; i++) {
		if (!file_exists(filename))
			break;

		bufprintf(p, 10, "-%d", i);
	}
	if (i >= 10) {
		log_err("too many logfiles already exist in the archive for this date; not rotating today");
		return;
	}
	if (rename(logfile, filename) == -1)
		log_err("failed to rename %s to %s", logfile, filename);
}

/*
	rotate: close the logfile and start a new one
*/
void log_rotate(void) {
	log_info("switching to new log");
	log_auth("switching to new log");

	move_log(PARAM_SYSLOG);
	move_log(PARAM_AUTHLOG);
	init_log(-1);		/* create new logfiles */

	log_info("start of new log");
	log_auth("start of new log");
}

/* EOB */
