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
	NewUserLog.c	WJ105
*/

#include "config.h"
#include "NewUserLog.h"
#include "CachedFile.h"
#include "FileFormat.h"
#include "Memory.h"
#include "Param.h"
#include "Timer.h"
#include "cstring.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>


NewUserLog *new_NewUserLog(void) {
	return (NewUserLog *)Malloc(sizeof(NewUserLog), TYPE_NEWUSERLOG);
}

void destroy_NewUserLog(NewUserLog *l) {
	Free(l);
}

NewUserQueue *load_NewUserQueue(char *filename) {
NewUserQueue *q;
File *f;
int version;
int (*load_func)(File *, NewUserQueue *);

	if ((q = new_NewUserQueue()) == NULL)
		return NULL;

	if ((f = Fopen(filename)) == NULL) {
		log_err("load_NewUserQueue(): failed to open file '%s'", filename);
		destroy_NewUserQueue(q);
		return NULL;
	}
	version = fileformat_version(f);
	load_func = NULL;
	switch(version) {
		case -1:
			log_err("load_NewUserQueue(): error trying to determine file format version of %s", filename);
			break;

		case 1:
			load_func = load_NewUserQueue_version1;
			break;

		default:
			log_err("load_User(): don't know how to load file format version %d of %s", version, filename);
	}
	if (load_func != NULL && !load_func(f, q)) {
		Fclose(f);
		return q;
	}
	Fclose(f);
	destroy_NewUserQueue(q);
	return NULL;
}

int load_NewUserQueue_version1(File *f, NewUserQueue *q) {
NewUserLog *l;
char buf[PRINT_BUF], *p;
int ff1_continue;

	if (f == NULL || q == NULL)
		return -1;

	while(Fgets(f, buf, PRINT_BUF) != NULL) {
		FF1_PARSE;

		if (!strcmp(buf, "newuser")) {
			if ((l = new_NewUserLog()) == NULL)
				FF1_ERROR;

			l->timestamp = cstrtoul(p, 10);

			if ((p = cstrchr(p, ' ')) == NULL) {
				destroy_NewUserLog(l);
				FF1_ERROR;
			}
			p++;
			if (!*p) {
				destroy_NewUserLog(l);
				FF1_ERROR;
			}
			cstrcpy(l->name, p, MAX_NAME);

			add_newuserlog(q, l);
		}
	}
	return 0;
}

int save_NewUserQueue(NewUserQueue *q, char *filename) {
File *f;
NewUserLog *l;

	if ((f = Fcreate(filename)) == NULL)
		return -1;

	FF1_SAVE_VERSION;

	for(l = (NewUserLog *)q->tail; l != NULL; l = l->next)
		Fprintf(f, "newuser=%lu %s", l->timestamp, l->name);

	return Fclose(f);
}

void add_newuserlog(NewUserQueue *newusers, NewUserLog *l) {
NewUserLog *old;

	while(count_Queue(newusers) >= PARAM_MAX_NEWUSERLOG
		&& (old = pop_NewUserQueue(newusers)) != NULL)
		destroy_NewUserLog(old);

	(void)add_NewUserQueue(newusers, l);
}

void log_newuser(char *name) {
NewUserQueue *newusers;
NewUserLog *l;

	if (name == NULL || !*name || (l = new_NewUserLog()) == NULL)
		return;

	l->timestamp = (unsigned long)rtc;
	cstrcpy(l->name, name, MAX_NAME);

	if ((newusers = load_NewUserQueue(PARAM_NEWUSERLOG)) == NULL
		&& (newusers = new_NewUserQueue()) == NULL) {
		log_err("log_newuser(%s) failed, out of memory?", name);
		return;
	}
	add_newuserlog(newusers, l);
	if (save_NewUserQueue(newusers, PARAM_NEWUSERLOG))
		log_err("log_newuser(): failed to save new user queue");

	destroy_NewUserQueue(newusers);
}

/* EOB */
