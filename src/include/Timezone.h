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
	Timezone.h	WJ103
*/

#ifndef TIMEZONE_H_WJ103
#define TIMEZONE_H_WJ103	1

#include "Hash.h"

#include <time.h>

#define TZ_INDEX_FILE		"/.tz_index"

typedef struct {
	time_t when;			/* when DST transition occurs */
	int type_idx;			/* 'local time type'; index to TimeType structs */
} DST_Transition;

typedef struct {
	int gmtoff;				/* offset to GMT (e.g. 3600 for GMT+0100) */
	int isdst;				/* is Daylight Savings in effect? (summer time) */
	int tzname_idx;			/* pointer to tznames; name of the timezone */
} TimeType;

typedef struct {
	time_t when;			/* when leap second is included */
	int num_secs;			/* how many leaps this time (usually just 1) */
} LeapSecond;

typedef struct {
	int refcount;
	int curr_idx;			/* currently at this transition */

	int num_trans, num_types;

	DST_Transition *transitions;
	TimeType *types;

	char *tznames;			/* space for names; like 'AMT NET CET CEST' with zeroes included (!) */
} Timezone;

extern Hash *tz_hash;

int init_Timezone(void);

Timezone *load_Timezone(char *);
void unload_Timezone(char *);
char *name_Timezone(Timezone *);

#endif	/* TIMEZONE_H_WJ103 */

/* EOB */
