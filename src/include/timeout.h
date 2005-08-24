/*
    bbs100 3.0 WJ105
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
	timeout.h	WJ99
*/

#ifndef TIMEOUT_H_WJ99
#define TIMEOUT_H_WJ99 1

#include "Timer.h"

#define LOGIN_TIMEOUT		20		/* 20 secs at login */

/*
	Note: these timers have a restart of 10 (way too large) because they are
	not removed in update_rtc() but in an other way (destroy_Timer in User and
	by exit())
*/
#define TIMEOUT_USER		10		/* restart value for 'timer states' */
#define TIMEOUT_REBOOT		10
#define TIMEOUT_SHUTDOWN	10

extern Timer *shutdown_timer;
extern Timer *reboot_timer;

void login_timeout(void *);
void user_timeout(void *);
void save_timeout(void *);
void reboot_timeout(void *);
void shutdown_timeout(void *);

#endif	/* TIMEOUT_H_WJ99 */

/* EOB */
