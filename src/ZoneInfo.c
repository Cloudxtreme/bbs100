/*
    bbs100 1.2.0 WJ102
    Copyright (C) 2002  Walter de Jong <walter@heiho.net>

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
	ZoneInfo.c	WJ103
*/

#include "config.h"
#include "ZoneInfo.h"
#include "defines.h"
#include "util.h"
#include "cstring.h"
#include "Memory.h"
#include "Param.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


TimeZone timezones[NUM_TIMEZONES] = {
	{ "Anchorage",		NULL },
	{ "Amsterdam",		NULL },
	{ "Los Angeles",	NULL },
	{ "Chicago",		NULL },
	{ "New York",		NULL },
	{ "Rio de Janeiro", NULL },
	{ "London",			NULL },
	{ "Cairo",			NULL },
	{ "Moscow",			NULL },
	{ "Bangkok",		NULL },
	{ "Tokyo",			NULL },
	{ "Sydney",			NULL },
};


int init_ZoneInfo(void) {
int i, gmtoff, hh, mm;
char filename[MAX_PATHLEN], buf[256], *p;

	printf("loading zoneinfo ...\n");
	for(i = 0; i < NUM_TIMEZONES; i++) {
		strcpy(buf, timezones[i].city);
		while((p = cstrchr(buf, ' ')) != NULL)
			*p = '_';

		sprintf(filename, "%s/%s", PARAM_ZONEINFODIR, buf);

		if ((timezones[i].zoneinfo = load_ZoneInfo(filename)) == NULL)
			printf("  %-16s failed\n", timezones[i].city);
		else {
			gmtoff = timezones[i].zoneinfo->gmtoff;
			hh = gmtoff / 3600;
			mm = gmtoff % 3600;
			if (mm < 0)
				mm = -mm;

			if (hh < 0) {
				hh = -hh;
				printf("  %-16s GMT-%02d%02d\n", timezones[i].city, hh, mm);
			} else
				if (!hh && !mm)
					printf("  %-16s GMT\n", timezones[i].city);
				else
					printf("  %-16s GMT+%02d%02d\n", timezones[i].city, hh, mm);
		}
	}
	return 0;
}

void deinit_ZoneInfo(void) {
int i;

	for(i = 0; i < NUM_TIMEZONES; i++) {
		listdestroy_ZoneInfo(timezones[i].zoneinfo);
		timezones[i].zoneinfo = NULL;
	}
}


ZoneInfo *new_ZoneInfo(void) {
ZoneInfo *z;

	if ((z = (ZoneInfo *)Malloc(sizeof(ZoneInfo), TYPE_ZONEINFO)) == NULL)
		return NULL;

	return z;
}

void destroy_ZoneInfo(ZoneInfo *z) {
	Free(z);
}


/*
	load a text dump of a zoneinfo file

	these lines look like:
	CET  Sun Oct 26 00:59:59 2003 UTC = Sun Oct 26 02:59:59 2003 CEST isdst=1 gmtoff=7200
*/
ZoneInfo *load_ZoneInfo(char *filename) {
FILE *f;
char buf[256], **arr;
int i, this_year, lineno;
time_t now, t;
struct tm *tm, mk_tm;
ZoneInfo *zoneinfo, *z;

	zoneinfo = NULL;

	if ((f = fopen(filename, "r")) == NULL) {
		logerr("load_ZoneInfo(): failed to open file %s", filename);
		return NULL;
	}
	now = time(NULL);
	tm = gmtime(&now);
	putenv("TZ=GMT");
	tzset();

	this_year = tm->tm_year+1900;

	lineno = 0;
	while(fgets(buf, 256, f) != NULL) {
		lineno++;

		cstrip_line(buf);
		cstrip_spaces(buf);

		if (!*buf || *buf == '#')
			continue;

		if ((arr = cstrsplit(buf, ' ')) == NULL) {
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);
			logerr("load_ZoneInfo(%s): syntax error in line %d", filename, lineno);
			return NULL;
		}
/* stupid check to see if we've got all the fields */
		for(i = 0; i < 16; i++) {
			if (arr[i] == NULL) {
				Free(arr);
				fclose(f);
				listdestroy_ZoneInfo(zoneinfo);

				logerr("load_ZoneInfo(%s): syntax error in line %d", filename, lineno);
				return NULL;
			}
		}
		if (strncmp(arr[15], "gmtoff=", 7) || strncmp(arr[14], "isdst=", 6)) {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): syntax error in line %d", filename, lineno);
			return NULL;
		}
/*
	get the year
*/
		i = atoi(arr[5]);
		if (i < this_year) {	/* that was last year, don't bother anymore */
			Free(arr);
			continue;
		}
		mk_tm.tm_year = i - 1900;

/*
	get the day of the week
*/
		for(i = 0; i < 7; i++) {
			if (!strncmp(arr[1], Days[i], 3))
				break;
		}
		if (i >= 7) {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): syntax error in line %d; dow == %d (?)", filename, lineno, i);
			return NULL;
		}
		mk_tm.tm_wday = i;

/*
	get the month number
*/
		for (i = 0; i < 12; i++) {
			if (!strncmp(arr[2], Months[i], 3))
				break;
		}
		if (i >= 12) {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): syntax error in line %d; month == %d (?)", filename, lineno, i);
			return NULL;
		}
		mk_tm.tm_mon = i;

/*
	get day of the month
*/
		i = atoi(arr[3]);
		if (i <= 0 || i > 31) {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): syntax error in line %d; mday == %d (?)", filename, lineno, i);
			return NULL;
		}
		mk_tm.tm_mday = i;

/*
	get the time of day; hh:mm:ss format
*/
		if (strlen(arr[4]) != 8 || arr[4][2] != ':' || arr[4][5] != ':') {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): syntax error in line %d; invalid time format", filename, lineno);
			return NULL;
		}
		arr[4][2] = 0;
		arr[4][5] = 0;
		mk_tm.tm_hour = atoi(arr[4]);
		mk_tm.tm_min = atoi(arr[4]+3);
		mk_tm.tm_sec = atoi(arr[4]+6);

/* mind leap seconds ... they usually don't happen at DST transitions, but hey ... */
		if (mk_tm.tm_hour < 0 || mk_tm.tm_hour > 23
			|| mk_tm.tm_min < 0 || mk_tm.tm_min > 60
			|| mk_tm.tm_sec < 0 || mk_tm.tm_sec > 62) {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): syntax error in line %d; time == %02d:%02d:%02d (?)", filename, lineno, mk_tm.tm_hour, mk_tm.tm_min, mk_tm.tm_sec);
			return NULL;
		}
		mk_tm.tm_yday = 0;		/* I have no clue what day in the year it is, and I don't care */
		mk_tm.tm_isdst = 0;		/* UTC does not have DST */

		if ((t = mktime(&mk_tm)) == (time_t)-1L) {
			logerr("load_ZoneInfo(%s): syntax error in line %d; mktime() failed", filename, lineno);
/*
			printf("TD: %d %04d%02d%02d %02d:%02d:%02d\n", mk_tm.tm_wday,
				mk_tm.tm_year+1900, mk_tm.tm_mon, mk_tm.tm_mday,
				mk_tm.tm_hour, mk_tm.tm_min, mk_tm.tm_sec);
*/
		}
		if (t < now) {
			Free(arr);
			continue;
		}
/*
		if (zoneinfo == NULL) printf("TD: next transition is at: %s", ctime(&t));
*/
		if ((z = new_ZoneInfo()) == NULL) {
			Free(arr);
			fclose(f);
			listdestroy_ZoneInfo(zoneinfo);

			logerr("load_ZoneInfo(%s): out of memory allocating new ZoneInfo structure", filename);
			return NULL;
		}
		z->t = t;
		z->gmtoff = atoi(arr[15]+7);
		add_ZoneInfo(&zoneinfo, z);

		Free(arr);
	}
	fclose(f);
	return zoneinfo;
}

/* EOB */
