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
	Worldclock.c	WJ103
*/

#include "config.h"
#include "Worldclock.h"
#include "Memory.h"
#include "util.h"
#include "cstring.h"
#include "locale_system.h"

#include <stdio.h>
#include <stdlib.h>


Worldclock worldclock[] = {
	{ "America/Anchorage",		NULL,	NULL, },
	{ "Europe/Amsterdam",		NULL,	NULL, },
	{ "America/Los_Angeles",	NULL,	NULL, },
	{ "America/Chicago",		NULL,	NULL, },
	{ "America/New_York",		NULL,	NULL, },
	{ "America/Sao_Paulo",		NULL,	NULL, },
	{ "Europe/London",			NULL,	NULL, },
	{ "Africa/Cairo",			NULL,	NULL, },
	{ "Europe/Moscow",			NULL,	NULL, },
	{ "Asia/Bangkok",			NULL,	NULL, },
	{ "Asia/Tokyo",				NULL,	NULL, },
	{ "Australia/Sydney",		NULL,	NULL, },
};


static char *zonefile_name(char *filename, char *buf, int maxlen) {
char *p;

	if (buf == NULL || maxlen <= 0)
		return NULL;

	buf[0] = 0;
	if (filename == NULL || !*filename)
		return buf;

	if ((p = cstrrchr(filename, '/')) != NULL)
		p++;
	else
		p = filename;

	cstrncpy(buf, p, maxlen);

	while((p = cstrchr(buf, '_')) != NULL)
		*p = ' ';

	if (buf[0] >= 'a' && buf[0] <= 'z')
		buf[0] -= ' ';

	return buf;
}


int init_Worldclock(void) {
int i, num;
Timezone *tz;
char zfn_buf[MAX_LINE];

	printf("loading worldclock ...\n");

	num = sizeof(worldclock)/sizeof(Worldclock);
	for(i = 0; i < num; i++) {
		worldclock[i].tz = tz = load_Timezone(worldclock[i].filename);
		worldclock[i].name = cstrdup(zonefile_name(worldclock[i].filename, zfn_buf, MAX_LINE));

		printf("  %-16s", (worldclock[i].name == NULL) ? worldclock[i].filename : worldclock[i].name);

		if (tz == NULL)
			printf("failed\n");
		else {
			struct tm *tm;

			tm = tz_time(tz, (time_t)0UL);

/* this is the format that the 'date' command uses by default */

			printf("%c%c%c %c%c%c %2d %02d:%02d:%02d %-4s %04d\n",
				lc_system->days[tm->tm_wday][0], lc_system->days[tm->tm_wday][1], lc_system->days[tm->tm_wday][2],
				lc_system->months[tm->tm_mon][0], lc_system->months[tm->tm_mon][1], lc_system->months[tm->tm_mon][2],
				tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
				name_Timezone(tz), tm->tm_year + 1900);
		}
	}
	return 0;
}

void deinit_Worldclock(void) {
int i, num;

	num = sizeof(worldclock)/sizeof(Worldclock);
	for(i = 0; i < num; i++) {
		unload_Timezone(worldclock[i].filename);
		worldclock[i].tz = NULL;

		Free(worldclock[i].name);
		worldclock[i].name = NULL;
	}
}

/* EOB */
