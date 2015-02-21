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
	locale_system.c	WJ105

	The 'system' locale defines the built-in default, the hardcoded English
	of bbs100

	You can copy and use this file as template to define other locales
*/

#include "locale_system.h"
#include "defines.h"
#include "sys_time.h"
#include "bufprintf.h"
#include "cstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
	date_str must be large enough, MAX_LINE will do
*/
char *lc_print_date(Locale *lc, struct tm *t, int twelve_hour_clock, char *date_str, int buflen) {
char add[2];

	if (lc == NULL)
		lc = lc_system;

	if (t == NULL || date_str == NULL)
		return NULL;

	if (t->tm_mday >= 10 && t->tm_mday <= 20) {
		add[0] = 't';
		add[1] = 'h';
	} else {
		switch(t->tm_mday % 10) {
			case 1:
				add[0] = 's';
				add[1] = 't';
				break;

			case 2:
				add[0] = 'n';
				add[1] = 'd';
				break;

			case 3:
				add[0] = 'r';
				add[1] = 'd';
				break;

			default:
				add[0] = 't';
				add[1] = 'h';
		}
	}
	if (twelve_hour_clock) {
		char am_pm = 'A';

		if (t->tm_hour >= 12) {
			am_pm = 'P';
			if (t->tm_hour > 12)
				t->tm_hour -= 12;
		}
		bufprintf(date_str, buflen, "%sday, %s %d%c%c %d %02d:%02d:%02d %cM",
			lc->days[t->tm_wday], lc->months[t->tm_mon], t->tm_mday, add[0], add[1], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec, am_pm);
	} else {
		bufprintf(date_str, buflen, "%sday, %s %d%c%c %d %02d:%02d:%02d",
			lc->days[t->tm_wday], lc->months[t->tm_mon], t->tm_mday, add[0], add[1], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec);
	}
	return date_str;
}


/*
	this is not really a locale, but hey ... maybe there is some freaky language
	out there that wants to output something like this in a different way

	I wrote the former function out and turned into this math-like looking
	formula stuff. This one actually produces better output.

	Note: buf must be large enough, MAX_LINE should do
*/
char *lc_print_total_time(Locale *lc, unsigned long total, char *buf, int buflen) {
int div[5] = { SECS_IN_WEEK, SECS_IN_DAY, SECS_IN_HOUR, SECS_IN_MIN, 1 };
int v[5];
int i, l, elems;
char *one[5] = { "week", "day", "hour", "minute", "second" };
char *more[5] = { "weeks", "days", "hours", "minutes", "seconds" };

	if (buf == NULL || buflen <= 0)
		return NULL;

	elems = 0;
	for(i = 0; i < 5; i++) {
		v[i] = total / div[i];
		total %= div[i];
		if (v[i] > 0)
			elems++;
	}
	if (!elems) {
		cstrcpy(buf, "0 seconds", buflen);
		return buf;
	}
	buf[0] = 0;
	l = 0;
	for(i = 0; i < 5; i++) {
		if (v[i] > 0) {
			l += bufprintf(buf+l, buflen - l, "%d %s", v[i], (v[i] == 1) ? one[i] : more[i]);
			elems--;
			if (!elems)
				break;

			l += bufprintf(buf+l, buflen - l, (elems == 1) ? ", and " : ", ");
		}
	}
	return buf;
}

/*
	Print a large number with dots or comma's
	We use a silly trick to do this ; walk the string in a reverse order and
	insert comma's into the string representation

	Note: buf must be large enough, MAX_NUMBER will do
*/
char *lc_print_number(unsigned long ul, int sep, char *buf, int buflen) {
char buf2[MAX_NUMBER];
int i, j = 0, n = 0;

	if (buf == NULL || buflen <= 0)
		return NULL;

	buf[0] = 0;
	bufprintf(buf2, sizeof(buf2), "%lu", ul);
	i = strlen(buf2)-1;
	if (i < 0)
		return buf;

	while(i >= 0 && j < buflen-1) {
		buf[j++] = buf2[i--];
		if (j >= buflen-1)
			break;

		n++;
		if (i >= 0 && n >= 3) {
			n = 0;
			buf[j++] = sep;			/* usually dot or comma */
		}
	}
	buf[j] = 0;

	cstrcpy(buf2, buf, MAX_NUMBER);
	i = strlen(buf2)-1;
	j = 0;
	while(i >= 0 && j <= buflen-1)
		buf[j++] = buf2[i--];
	buf[j] = 0;
	return buf;
}

/*
	standard template for having large numbers displayed with comma's
	Note: buf must be large enough, MAX_NUMBER should do
*/
char *lc_print_number_commas(Locale *lc, unsigned long ul, char *buf, int buflen) {
	if (buf == NULL || buflen <= 0)
		return NULL;

	return lc_print_number(ul, ',', buf, buflen);
}

/*
	standard template for having large numbers displayed with dots
	Note: buf must be large enough, MAX_NUMBER should do
*/
char *lc_print_number_dots(Locale *lc, unsigned long ul, char *buf, int buflen) {
	if (buf == NULL || buflen <= 0)
		return NULL;

	return lc_print_number(ul, '.', buf, buflen);
}

/*
	print_number() with '1st', '2nd', '3rd', '4th', ... extension
	Note: buf must be large enough, MAX_NUMBER should do
*/
char *lc_print_numberth(Locale *lc, unsigned long ul, char *buf, int buflen) {
char add[3];
int l;

	if (buf == NULL || buflen <= 0)
		return NULL;

	if (((ul % 100UL) >= 10UL) && ((ul % 100UL) <= 20UL)) {
		add[0] = 't';
		add[1] = 'h';
	} else {
		switch(ul % 10UL) {
			case 1:
				add[0] = 's';
				add[1] = 't';
				break;

			case 2:
				add[0] = 'n';
				add[1] = 'd';
				break;

			case 3:
				add[0] = 'r';
				add[1] = 'd';
				break;

			default:
				add[0] = 't';
				add[1] = 'h';
		}
	}
	add[2] = 0;

	if (lc == NULL)
		lc = lc_system;
	lc->print_number(lc, ul, buf, buflen);

	l = strlen(buf);
	if (l < buflen-2) {
		buf[l++] = add[0];
		buf[l++] = add[1];
		buf[l] = 0;
	}
	return buf;
}

/*
	Note: buf must be large enough (MAX_LINE bytes in size)
*/
char *lc_possession(Locale *lc, char *name, char *obj, char *buf, int buflen) {
int l, j;

	if (buf == NULL || buflen <= 0)
		return NULL;

	if (name == NULL || !*name) {
		cstrcpy(buf, "(NULL)", buflen);
		return buf;
	}
	cstrcpy(buf, name, buflen);
	l = j = strlen(buf)-1;
	if (l < buflen-3) {
		buf[++l] = '\'';
		if (buf[j] != 'z' && buf[j] != 'Z' &&
			buf[j] != 's' && buf[j] != 'S')
			buf[++l] = 's';
		buf[++l] = ' ';
		buf[++l] = 0;

		if (obj != NULL && buflen - l > strlen(obj))
			cstrcat(buf, obj, buflen);
	}
	return buf;
}


/* module definition */

Locale system_locale = {
	"bbs100",							/* name of the language */
	{ "Sun", "Mon", "Tues", "Wednes", "Thurs", "Fri", "Satur" },
	{ "January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December" },

	lc_print_date,
	lc_print_total_time,
	lc_print_number_commas,
	lc_print_numberth,
	lc_possession
};

Locale *lc_system = &system_locale;

/* EOB */
