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
	screens.c	WJ99
*/

#include "config.h"
#include "debug.h"
#include "screens.h"
#include "CachedFile.h"
#include "Param.h"
#include "util.h"
#include "log.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>

/*
	the crash screen is the only screen that is kept in-core and does
	not go through the file cache
*/
StringIO *crash_screen = NULL;


int init_screens(void) {
File *f;

	Enter(init_screens);
/*
	This is only to see if they load, and to cache them if the cache
	is enabled
*/
	printf("loading login_screen %s ... ", PARAM_LOGIN_SCREEN);
	if ((f = Fopen(PARAM_LOGIN_SCREEN)) == NULL) {
		printf("failed\n");
		Return -1;
	}
	Fclose(f);
	printf("Ok\n");

	printf("loading logout_screen %s ... ", PARAM_LOGOUT_SCREEN);
	if ((f = Fopen(PARAM_LOGOUT_SCREEN)) == NULL) {
		printf("failed\n");
		Return -1;
	}
	Fclose(f);
	printf("Ok\n");

	printf("loading motd_screen %s ... ", PARAM_MOTD_SCREEN);
	if ((f = Fopen(PARAM_MOTD_SCREEN)) == NULL) {
		printf("failed\n");
		Return -1;
	}
	Fclose(f);
	printf("Ok\n");

	printf("loading crash_screen %s ... ", PARAM_CRASH_SCREEN);
	if ((crash_screen = new_StringIO()) == NULL) {
		printf("failed\n");
		Return -1;
	}
	if (load_StringIO(crash_screen, PARAM_CRASH_SCREEN) < 0) {
		printf("failed\n");
		Return -1;
	}
	printf("Ok\n");
	Return 0;
}

/*
	this function seems so simple, but note that it reads the file
	from the cache
*/
int load_screen(StringIO *s, char *filename) {
int err;
File *f;

	if (s == NULL || filename == NULL || !*filename || (f = Fopen(filename)) == NULL)
		return -1;

	err = Fget_StringIO(f, s);

	Fclose(f);
	return err;
}

int display_screen(User *usr, char *filename) {
File *f;
char buf[PRINT_BUF];

	if (usr == NULL || filename == NULL || !*filename)
		return -1;

	Enter(display_screen);

	if ((f = Fopen(filename)) == NULL) {
		log_err("display_screen(): failed to open file %s", filename);
		Return -1;
	}
	while(Fgets(f, buf, PRINT_BUF) != NULL) {
		Put(usr, buf);
		Put(usr, "\n");
	}
	Fclose(f);
	Return 0;
}

void display_text(User *usr, StringIO *s) {
char buf[PRINT_BUF];

	if (usr == NULL || s == NULL)
		return;

	Enter(display_text);

	seek_StringIO(s, 0, STRINGIO_SET);
	while(gets_StringIO(s, buf, PRINT_BUF)) {
		Put(usr, buf);
		Put(usr, "\n");
	}
	Return;
}

/* EOB */
