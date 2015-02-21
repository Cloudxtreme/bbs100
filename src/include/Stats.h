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
	Stats.h	WJ99
*/

#ifndef STATS_H_WJ99
#define STATS_H_WJ99 1

#include "config.h"
#include "defines.h"
#include "User.h"
#include "CachedFile.h"
#include "sys_time.h"

/*
	loaded = 'L' : this protects us from saving stats that were not even
	properly loaded yet
*/
#define STATS_LOADED	0x4c

typedef struct {
	int loaded;

	char oldest[MAX_NAME], youngest[MAX_NAME], most_logins[MAX_NAME];
	char most_xsent[MAX_NAME], most_xrecv[MAX_NAME];
	char most_esent[MAX_NAME], most_erecv[MAX_NAME];
	char most_fsent[MAX_NAME], most_frecv[MAX_NAME];
	char most_qsent[MAX_NAME], most_qansw[MAX_NAME];
	char most_posted[MAX_NAME], most_read[MAX_NAME];

	time_t oldest_birth, youngest_birth;
	unsigned long logins, xsent, xrecv, esent, erecv, fsent, frecv, qsent, qansw, posted, read;
	unsigned long oldest_age;

	unsigned long num_logins, cache_hit, cache_miss;
	unsigned long xsent_boot, xrecv_boot, esent_boot, erecv_boot, fsent_boot, frecv_boot;
	unsigned long qsent_boot, qansw_boot, posted_boot, read_boot;
	time_t uptime;
} Stats;

extern Stats stats;

int load_Stats(Stats *, char *);
int load_Stats_version0(File *, Stats *);
int load_Stats_version1(File *, Stats *);

int save_Stats(Stats *, char *);
int save_Stats_version1(File *, Stats *);

void update_stats(User *);
void print_stats(User *);

#endif	/* STATS_H_WJ99 */

/* EOB */
