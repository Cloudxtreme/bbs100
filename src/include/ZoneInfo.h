/*
    bbs100 1.2.1 WJ103
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
	ZoneInfo.h	WJ103
*/

#ifndef ZONEINFO_H_WJ103
#define ZONEINFO_H_WJ103	1

#include "List.h"

#include <time.h>


#define add_ZoneInfo(x,y)		(ZoneInfo *)add_List((x), (y))
#define concat_ZoneInfo(x,y)	(ZoneInfo *)concat_List((x), (y))
#define remove_ZoneInfo(x,y)	remove_List((x), (y))
#define listdestroy_ZoneInfo(x)	listdestroy_List((x), destroy_ZoneInfo)
#define rewind_ZoneInfo(x)		(ZoneInfo *)rewind_List(x)
#define unwind_ZoneInfo(x)		(ZoneInfo *)unwind_List(x)

/*
	This is not the real number of time zones in the world ...
	it is only the number of time zones that the world clock in the bbs
	displays
*/
#define NUM_TIMEZONES	12

/*
	zoneinfo is the structure that knows about daylight saving time
	and when the transitions occur
*/
typedef struct ZoneInfo_tag ZoneInfo;

struct ZoneInfo_tag {
	List(ZoneInfo);

	time_t t;
	int gmtoff;
};

typedef struct {
	char *city;
	ZoneInfo *zoneinfo;
} TimeZone;

extern TimeZone timezones[NUM_TIMEZONES];


int init_ZoneInfo(void);
void deinit_ZoneInfo(void);

ZoneInfo *new_ZoneInfo(void);
void destroy_ZoneInfo(ZoneInfo *);

ZoneInfo *load_ZoneInfo(char *);

#endif	/* ZONEINFO_H_WJ103 */

/* EOB */
