/*
    bbs100 2.0 WJ104
    Copyright (C) 2004  Walter de Jong <walter@heiho.net>

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
	Timer.c	WJ99

	- the timerqueue is sorted by 'relative' time; e.g. the
	  event happens in 'n' seconds from now
	- timer functions are called synchronously (after select()
	  times out), so no locking needs to be done ever
*/

#include "config.h"
#include "Timer.h"
#include "User.h"
#include "Signal.h"
#include "sys_time.h"
#include "cstring.h"
#include "Memory.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

volatile time_t rtc = (time_t)0UL;

Timer *timerq = NULL;


Timer *new_Timer(int s, void (*func)(void *), int r) {
Timer *t;

	if ((t = (Timer *)Malloc(sizeof(Timer), TYPE_TIMER)) == NULL)
		return NULL;

	t->sleeptime = t->maxtime = s;
	t->restart = r;
	t->action = func;

	return t;
}

void destroy_Timer(Timer *t) {
	Free(t);
}

/*
	insertion-sort timers into the sorted timerqueue
	the timerqueue works with 'relative' time
*/
Timer *add_Timer(Timer **queue, Timer *t) {
Timer *q, *q_prev;

	if (queue == NULL || t == NULL)
		return NULL;

	t->prev = t->next = NULL;

	if (*queue == NULL) {
		*queue = t;
		return t;
	}
	for(q = *queue; q != NULL; q = q->next) {
		if (q->sleeptime > t->sleeptime || t->sleeptime <= 0) {
			if (q->sleeptime > t->sleeptime)
				q->sleeptime -= t->sleeptime;

			t->next = q;					/* insert before q */
			t->prev = q->prev;

			if (q->prev != NULL)
				q->prev->next = t;

			q->prev = t;
			if (t->prev == NULL)
				*queue = t;
			return t;
		} else
			t->sleeptime -= q->sleeptime;

		q_prev = q;
	}
	q_prev->next = t;						/* append at end */
	t->prev = q_prev;
	return t;
}


static int update_timerqueue(Timer **queue, void *arg, int tdiff) {
int nap;
Timer *t, *t_next;

	nap = 10 * SECS_IN_MIN;
	for(t = *queue; t != NULL; t = t_next) {
		t_next = t->next;

		if (tdiff > 0) {
			t->sleeptime -= tdiff;
			tdiff = 0;
		}
		if (t->sleeptime > 0) {
			if (t->sleeptime < nap)
				nap = t->sleeptime;
			break;
		}
		if (t->sleeptime <= 0) {
			t->sleeptime = t->maxtime;

			if (t->action != NULL)
				t->action((arg == NULL) ? t : arg);

			if (t->restart > 0)		/* restart -1 always restarts */
				t->restart--;

			remove_Timer(queue, t);
			if (!t->restart)
				destroy_Timer(t);
			else
				add_Timer(queue, t);
		}
	}
	return nap;
}

/*
	update timers
	called from mainloop() after the select() times out

	If select() does not time out, rtc will be updated too much,
	so I had to put an old_rtc in and compare the two clocks :P
*/
int update_timers(void) {
int n, nap;
static time_t old_rtc = (time_t)0UL;
int tdiff;
User *usr, *usr_next;

	rtc = time(NULL);
	if (!old_rtc) {
		old_rtc = rtc;
		return 1;
	}
	tdiff = (int)((unsigned long)rtc - (unsigned long)old_rtc);
	old_rtc = rtc;

	nap = 10 * SECS_IN_MIN;			/* 10 minutes */

/* update the user timers */
	for(usr = AllUsers; usr != NULL; usr = usr_next) {
		usr_next = usr->next;

		if (usr->timerq == NULL)
			continue;

		n = update_timerqueue(&usr->timerq, usr, tdiff);
		if (n < nap)
			nap = n;
	}

/* now process timers that are not bound to a user */

	n = update_timerqueue(&timerq, NULL, tdiff);
	if (n < nap)
		nap = n;

	return nap;
}

/* EOB */
