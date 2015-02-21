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
	timeout.h	WJ99
*/

#ifndef TIMEOUT_H_WJ99
#define TIMEOUT_H_WJ99 1

#include "Timer.h"

#define LOGIN_TIMEOUT		20		/* 20 secs at login */
#define LOGOUT_TIMEOUT		2		/* logout screen is shown this long */
#define IDLING_TIMEOUT		6		/* You are logged off NOW */

/*
	Note: these timers have a restart of 10 (way too large) because they are
	not removed in update_rtc() but in an other way (destroy_Timer in User and
	by exit())
*/
#define TIMEOUT_USER		10		/* restart value for 'timer states' */
#define TIMEOUT_REBOOT		10
#define TIMEOUT_SHUTDOWN	TIMEOUT_REBOOT		/* must be the same, for time_to_dd() to work correctly */

extern Timer *shutdown_timer;
extern Timer *reboot_timer;

void login_timeout(void *);
void user_timeout(void *);
void save_timeout(void *);
void reboot_timeout(void *);
void shutdown_timeout(void *);

int time_to_dd(Timer *);

#endif	/* TIMEOUT_H_WJ99 */

/* EOB */
