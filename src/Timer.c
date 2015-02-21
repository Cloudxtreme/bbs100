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
	Timer.c	WJ99

	- the timerqueue is sorted by 'relative' time; e.g. the
	  event happens in 'n' seconds from now
	- timer functions are called synchronously (after select()
	  times out), so no locking needs to be done ever
*/

#include "config.h"
#include "Timer.h"
#include "User.h"
#include "Signals.h"
#include "sys_time.h"
#include "cstring.h"
#include "Memory.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

time_t rtc = (time_t)0UL;

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

	if (t->sleeptime < 0)
		t->sleeptime = 0;

	t->prev = t->next = NULL;

	if (*queue == NULL) {
		*queue = t;
		return t;
	}
	for(q = *queue; q != NULL; q = q->next) {
		if (q->sleeptime >= t->sleeptime) {
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

Timer *remove_Timer(Timer **queue, Timer *t) {
	if (queue == NULL || t == NULL)
		return NULL;

	if (*queue == NULL)
		return t;

	if (t->sleeptime > 0 && t->next != NULL)
		t->next->sleeptime += t->sleeptime;

	return (Timer *)remove_List(queue, t);
}

/*
	timer queues are sorted, and should remain sorted at all times
	
	Note: do NOT run this function from within a timer action ... it'll SEGV because
	you cannot manipulate the timer queue from within a timer action handler
*/
void set_Timer(Timer **queue, Timer *t, int sleeptime) {
	if (t == NULL)
		return;

	t->sleeptime = sleeptime;
	if (queue != NULL) {
		remove_Timer(queue, t);
		add_Timer(queue, t);
	}
}

static void update_timerqueue(Timer **queue, void *arg, int tdiff) {
Timer *t, *t_next;

	if (queue == NULL || *queue == NULL)
		return;

	for(t = *queue; t != NULL; t = t_next) {
		t_next = t->next;

		if (tdiff > 0) {
			t->sleeptime -= tdiff;
			tdiff = 0;
		}
		if (t->sleeptime > 0)
			break;

		if (t->sleeptime <= 0) {
			remove_Timer(queue, t);
			t->sleeptime = t->maxtime;

			if (t->action != NULL)
				t->action((arg == NULL) ? t : arg);

			if (t->restart > 0)		/* restart -1 always restarts */
				t->restart--;

			if (!t->restart)
				destroy_Timer(t);
			else
				add_Timer(queue, t);
		}
	}
}

/*
	update timers
	called from mainloop() after the select() times out

	If select() does not time out, rtc will be updated too much,
	so I had to put an old_rtc in and compare the two clocks :P
*/
void update_timers(void) {
static time_t old_rtc = (time_t)0UL;
int tdiff;
User *usr, *usr_next;

	rtc = time(NULL);
	if (!old_rtc) {
		old_rtc = rtc;
		return;
	}
	tdiff = (int)((unsigned long)rtc - (unsigned long)old_rtc);
	old_rtc = rtc;

/* update the user timers */
	for(usr = AllUsers; usr != NULL; usr = usr_next) {
		usr_next = usr->next;

		if (usr->timerq == NULL)
			continue;

		update_timerqueue(&usr->timerq, usr, tdiff);
	}
/*
	now process timers that are not bound to a user
	temporarily block signals, as there are signals that modify the global timerq
*/
	block_timer_signals(SIG_BLOCK);
	update_timerqueue(&timerq, NULL, tdiff);
	block_timer_signals(SIG_UNBLOCK);
	handle_pending_signals();
}

/*
	return the shortest next timer event
	this is fed to select() as timeout
*/
int shortest_timer(void) {
User *usr;
int nap;

	nap = SOME_TIME;

	for(usr = AllUsers; usr != NULL; usr = usr->next) {
		if (usr->timerq == NULL)
			continue;

		if (usr->timerq->sleeptime < nap)
			nap = usr->timerq->sleeptime;
	}
	if (timerq != NULL && timerq->sleeptime < nap)
		nap = timerq->sleeptime;

	return nap;
}

/* EOB */
