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
	Timer.c	WJ99
*/

#include <config.h>

#include "Timer.h"
#include "User.h"
#include "Signal.h"
#include "sys_time.h"
#include "cstring.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

volatile time_t rtc = (time_t)0UL;

Timer *timerq = NULL;


Timer *new_Timer(int s, void (*func)(User *), int r) {
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
	update timers
	called from mainloop() after the select() times out

	If select() does not time out, rtc will be updated too much,
	so I had to put an old_rtc in and compare the two clocks :P
*/
void update_timers(void) {
static time_t old_rtc = (time_t)0UL;
User *usr, *usr_next;
Timer *t, *t_next;

	rtc = time(NULL);
	if (rtc == old_rtc)				/* rtc is moving too fast */
		return;

	old_rtc = rtc;

/* update the user timers */
	for(usr = AllUsers; usr != NULL; usr = usr_next) {
		usr_next = usr->next;

		for(t = usr->timer; t != NULL; t = t_next) {
			t_next = t->next;

			if (t->sleeptime > 0)
				t->sleeptime--;

			if (t->sleeptime <= 0) {
				t->sleeptime = t->maxtime;

				if (t->action != NULL)
					t->action(usr);
				if (t->restart > 0)		/* restart -1 always restarts */
					t->restart--;

				if (!t->restart) {
					remove_Timer(&usr->timer, t);
					destroy_Timer(t);
				}
			}
		}
	}

/* now process timers that are not bound to a user */

	for(t = timerq; t != NULL; t = t_next) {
		t_next = t->next;

		if (t->sleeptime > 0)
			t->sleeptime--;
		if (!t->sleeptime) {
			t->sleeptime = t->maxtime;

			if (t->action != NULL)
				t->action((User *)t);	/* user-less timers have 't' as argument(!) */

			if (t->restart > 0)			/* restart -1 always restarts */
				t->restart--;
			if (!t->restart) {
				remove_Timer(&timerq, t);
				destroy_Timer(t);
			}
		}
	}
}

/* EOB */
