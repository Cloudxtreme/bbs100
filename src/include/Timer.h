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
	Timer.h	WJ99
*/

#ifndef TIMER_H_WJ99
#define TIMER_H_WJ99 1

#include "config.h"
#include "List.h"
#include "sys_time.h"

#define rewind_Timer(x)			(Timer *)rewind_List((x))
#define unwind_Timer(x)			(Timer *)unwind_List((x))
#define listdestroy_Timer(x)	listdestroy_List((x), destroy_Timer)

#define TIMER_ONESHOT	0
#define TIMER_RESTART	-1

typedef struct Timer_tag Timer;

struct Timer_tag {
	List(Timer);

	int sleeptime, maxtime, restart;
	void (*action)(void *);
};

extern time_t rtc;
extern Timer *timerq;

Timer *new_Timer(int, void (*)(void *), int);
void destroy_Timer(Timer *);
Timer *add_Timer(Timer **, Timer *);
Timer *remove_Timer(Timer **, Timer *);
void set_Timer(Timer **, Timer *, int);

int init_rtc(void);
void update_timers(void);
int shortest_timer(void);

#endif	/* TIMER_H_WJ99 */

/* EOB */
