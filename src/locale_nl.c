/*
    bbs100 2.2 WJ105
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
	locale_nl.c	WJ105

	The 'system' locale defines the built-in default, the hardcoded English
	of bbs100

	You can copy and use this file as template to define other locales
*/

#include "locale_system.h"
#include "locale_nl.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*
	Note: date_str should be large enough (80 bytes will do)
*/
char *nl_print_date(Locale *lc, struct tm *t, int twelve_hour_clock, char *date_str) {
	if (lc == NULL)
		lc = lc_system;

	if (t == NULL || date_str == NULL)
		return NULL;

	if (twelve_hour_clock) {
		char am_pm = 'A';

		if (t->tm_hour >= 12) {
			am_pm = 'P';
			if (t->tm_hour > 12)
				t->tm_hour -= 12;
		}
		sprintf(date_str, "%sdag %d %s %d %02d:%02d:%02d %cM",
			lc->days[t->tm_wday], t->tm_mday, lc->months[t->tm_mon], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec, am_pm);
	} else {
		sprintf(date_str, "%sdag %d %s %d %02d:%02d:%02d",
			lc->days[t->tm_wday], t->tm_mday, lc->months[t->tm_mon], t->tm_year + 1900,
			t->tm_hour, t->tm_min, t->tm_sec);
	}
	return date_str;
}


/*
	this is not really a locale, but hey ... maybe there is some freaky language
	out there that wants to output something like this in a different way

	Note: buf must be large enough (MAX_LINE bytes should do)
*/
char *nl_print_total_time(Locale *lc, unsigned long total, char *buf) {
int div[5] = { SECS_IN_WEEK, SECS_IN_DAY, SECS_IN_HOUR, SECS_IN_MIN, 1 };
int v[5];
int i, l, elems;
char *one[5] = { "week", "dag", "uur", "minuut", "seconde" };
char *more[5] = { "weken", "dagen", "uur", "minuten", "seconden" };

	elems = 0;
	for(i = 0; i < 5; i++) {
		v[i] = total / div[i];
		total %= div[i];
		if (v[i] > 0)
			elems++;
	}
	if (!elems) {
		strcpy(buf, "0 seconden");
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

			l += sprintf(buf+l, (elems == 1) ? " en " : ", ");
		}
	}
	return buf;
}

/*
	print_number() with '1st', '2nd', '3rd', '4th', ... extension
	Note: buf must be large enough (MAX_LINE should do)
*/
char *nl_print_numberth(Locale *lc, unsigned long ul, char *buf) {
int l;

	if (buf == NULL)
		return NULL;

	if (lc == NULL)
		lc = lc_system;
	lc->print_number(lc, ul, buf);

	l = strlen(buf);
	if (l > 0) {
		buf[l++] = 'e';
		buf[l] = 0;
	}
	return buf;
}


/* module definition */

Locale nl_locale = {
	"nederlands",							/* name of the language */
	{ "Zon", "Maan", "Dins", "Woens", "Donder", "Vrij", "Zater" },
	{ "Januari", "Februari", "Maart", "April", "Mei", "Juni",
	"Juli", "Augustus", "September", "Oktober", "November", "December" },

	nl_print_date,
	nl_print_total_time,
	lc_print_number_dots,
	nl_print_numberth,
	lc_name_with_s
};

/* EOB */
