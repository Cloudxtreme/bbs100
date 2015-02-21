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
	Joined.h	WJ99

	the joined room
*/

#ifndef JOINED_H_WJ99
#define JOINED_H_WJ99 1

#include <config.h>

#include "List.h"
#include "sys_time.h"

#define add_Joined(x,y)			(Joined *)add_List((x), (y))
#define prepend_Joined(x,y)		(Joined *)prepend_List((x), (y))
#define concat_Joined(x,y)		(Joined *)concat_List((x), (y))
#define remove_Joined(x,y)		(Joined *)remove_List((x), (y))
#define rewind_Joined(x)		(Joined *)rewind_List((x))
#define unwind_Joined(x)		(Joined *)unwind_List((x))
#define listdestroy_Joined(x)	listdestroy_List((x), destroy_Joined)

typedef struct Joined_tag Joined;

struct Joined_tag {
	List(Joined);

	unsigned int zapped, number, roominfo_read;
	unsigned long generation;
	long last_read;
};

Joined *new_Joined(void);
void destroy_Joined(Joined *);

Joined *in_Joined(Joined *, unsigned int);

#endif	/* JOINED_H_WJ99 */

/* EOB */
