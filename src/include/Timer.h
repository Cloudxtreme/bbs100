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
	Timer.h	WJ99
*/

#ifndef TIMER_H_WJ99
#define TIMER_H_WJ99 1

#include "config.h"
#include "List.h"
#include "sys_time.h"

#define remove_Timer(x,y)		remove_List((x), (y))
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

Timer *new_Timer(int, void (*)(void *), int);
void destroy_Timer(Timer *);
Timer *add_Timer(Timer **, Timer *);
int init_rtc(void);
int update_timers(void);

extern volatile time_t rtc;
extern Timer *timerq;

#endif	/* TIMER_H_WJ99 */

/* EOB */
