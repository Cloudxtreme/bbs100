/*
    bbs100 3.0 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

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
	locale_system.c	WJ105

	The 'system' locale defines the built-in default, the hardcoded English
	of bbs100

	You can copy and use this file as template to define other locales
*/

#include "locale_system.h"
#include "defines.h"
#include "sys_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
	Note: date_str should be large enough (80 bytes will do)
*/
char *lc_print_date(Locale *lc, struct tm *t, int twelve_hour_clock, char *date_str) {
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
		sprintf(date_str, "%sday, %s %d%c%c %d %02d:%02d:%02d %cM",
			lc->days[t->tm_wday], lc->months[t->tm_mon], t->tm_mday, add[0], add[1], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec, am_pm);
	} else {
		sprintf(date_str, "%sday, %s %d%c%c %d %02d:%02d:%02d",
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

	Note: buf must be large enough (MAX_LINE bytes should do)
*/
char *lc_print_total_time(Locale *lc, unsigned long total, char *buf) {
int div[5] = { SECS_IN_WEEK, SECS_IN_DAY, SECS_IN_HOUR, SECS_IN_MIN, 1 };
int v[5];
int i, l, elems;
char *one[5] = { "week", "day", "hour", "minute", "second" };
char *more[5] = { "weeks", "days", "hours", "minutes", "seconds" };

	elems = 0;
	for(i = 0; i < 5; i++) {
		v[i] = total / div[i];
		total %= div[i];
		if (v[i] > 0)
			elems++;
	}
	if (!elems) {
		strcpy(buf, "0 seconds");
		return buf;
	}
	buf[0] = 0;
	l = 0;
	for(i = 0; i < 5; i++) {
		if (v[i] > 0) {
			l += sprintf(buf+l, "%d %s", v[i], (v[i] == 1) ? one[i] : more[i]);
			elems--;
			if (!elems)
				break;

			l += sprintf(buf+l, (elems == 1) ? ", and " : ", ");
		}
	}
	return buf;
}

/*
	Print a large number with dots or comma's
	We use a silly trick to do this ; walk the string in a reverse order and
	insert comma's into the string representation

	Note: buf must be large enough (at least 21 bytes)
*/
char *lc_print_number(unsigned long ul, int sep, char *buf) {
char buf2[21];
int i, j = 0, n = 0;

	if (buf == NULL)
		return NULL;

	buf[0] = 0;
	sprintf(buf2, "%lu", ul);
	i = strlen(buf2)-1;
	if (i < 0)
		return buf;

	while(i >= 0) {
		buf[j++] = buf2[i--];

		n++;
		if (i >= 0 && n >= 3) {
			n = 0;
			buf[j++] = sep;			/* usually dot or comma */
		}
	}
	buf[j] = 0;

	strcpy(buf2, buf);
	i = strlen(buf2)-1;
	j = 0;
	while(i >= 0)
		buf[j++] = buf2[i--];
	return buf;
}

/*
	standard template for having large numbers displayed with comma's
	Note: buf must be large enough (at least 21 bytes)
*/
char *lc_print_number_commas(Locale *lc, unsigned long ul, char *buf) {
	if (buf == NULL)
		return NULL;

	return lc_print_number(ul, ',', buf);
}

/*
	standard template for having large numbers displayed with dots
	Note: buf must be large enough (at least 21 bytes)
*/
char *lc_print_number_dots(Locale *lc, unsigned long ul, char *buf) {
	if (buf == NULL)
		return NULL;

	return lc_print_number(ul, '.', buf);
}

/*
	print_number() with '1st', '2nd', '3rd', '4th', ... extension
	Note: buf must be large enough (MAX_LINE should do)
*/
char *lc_print_numberth(Locale *lc, unsigned long ul, char *buf) {
char add[3];

	if (buf == NULL)
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
	lc->print_number(lc, ul, buf);
	strcat(buf, add);
	return buf;
}

/*
	Note: buf must be large enough (MAX_LINE bytes in size)
*/
char *lc_possession(Locale *lc, char *name, char *obj, char *buf) {
int i, j;

	if (buf == NULL)
		return NULL;

	if (name == NULL || !*name) {
		strcpy(buf, "(NULL)");
		return buf;
	}
	strcpy(buf, name);
	i = j = strlen(buf)-1;
	buf[++i] = '\'';
	if (buf[j] != 'z' && buf[j] != 'Z' &&
		buf[j] != 's' && buf[j] != 'S')
		buf[++i] = 's';
	buf[++i] = ' ';
	buf[++i] = 0;
	strcat(buf, obj);
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
