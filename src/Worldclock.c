/*
    bbs100 1.2.2 WJ103
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
	Worldclock.c	WJ103
*/

#include "config.h"
#include "Worldclock.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>


Worldclock worldclock[] = {
	{ "America/Anchorage",		NULL },
	{ "Europe/Amsterdam",		NULL },
	{ "America/Los_Angeles",	NULL },
	{ "America/Chicago",		NULL },
	{ "America/New_York",		NULL },
	{ "America/Rio_de_Janeiro", NULL },
	{ "Europe/London",			NULL },
	{ "Africa/Cairo",			NULL },
	{ "Europe/Moscow",			NULL },
	{ "Asia/Bangkok",			NULL },
	{ "Asia/Tokyo",				NULL },
	{ "Australia/Sydney",		NULL },
};


int init_Worldclock(void) {
int i, num;
Timezone *tz;

	printf("loading worldclock ...\n");

	num = sizeof(worldclock)/sizeof(Worldclock);
	for(i = 0; i < num; i++) {
		printf("  %-26s", worldclock[i].filename);

		worldclock[i].tz = tz = load_Timezone(worldclock[i].filename);

		if (tz == NULL)
			printf("failed\n");
		else {
			struct tm *tm;

			tm = tz_time(tz, (time_t)0UL);

/* this is the format that the 'date' command uses by default */

			printf("%c%c%c %c%c%c %2d %02d:%02d:%02d %-4s %04d\n",
				Days[tm->tm_wday][0], Days[tm->tm_wday][1], Days[tm->tm_wday][2],
				Months[tm->tm_mon][0], Months[tm->tm_mon][1], Months[tm->tm_mon][2],
				tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
				name_Timezone(tz), tm->tm_year + 1900);
		}
	}
	return 0;
}

/* EOB */
