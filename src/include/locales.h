/*
    bbs100 3.2 WJ107
    Copyright (C) 2007  Walter de Jong <walter@heiho.net>

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
	locales.h	WJ105
*/

#ifndef LOCALES_H_WJ105
#define LOCALES_H_WJ105	1

#include <time.h>

typedef struct Locale_tag Locale;

struct Locale_tag {
	char *name, *days[7], *months[12];

	char *(*print_date)(Locale *, struct tm *, int, char *, int);
	char *(*print_total_time)(Locale *, unsigned long, char *, int);
	char *(*print_number)(Locale *, unsigned long, char *, int);
	char *(*print_numberth)(Locale *, unsigned long, char *, int);
	char *(*possession)(Locale *, char *, char *, char *, int);
};

extern Locale *all_locales[];

#endif	/* LOCALES_H_WJ105 */

/* EOB */
