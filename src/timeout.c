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
	timeout.c	WJ99
*/

#include "config.h"
#include "timeout.h"
#include "User.h"
#include "inet.h"
#include "util.h"
#include "log.h"
#include "Stats.h"
#include "Process.h"
#include "main.h"
#include "copyright.h"
#include "Param.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

Timer *shutdown_timer = NULL, *reboot_timer = NULL;


void login_timeout(void *v) {
User *usr;

	if (v == NULL)
		return;

	usr = (User *)v;
	Put(usr, "\nConnection timed out\n");
	log_auth("TIMEOUT %s (%s)", usr->name, usr->from_ip);
	close_connection(usr, "%s got timed out", usr->name);
}

void user_timeout(void *v) {
User *usr;

	usr = (User *)v;
	if (usr == NULL || usr->idle_timer == NULL)
		return;

	switch(usr->idle_timer->restart) {
		case TIMEOUT_USER:
			Put(usr, "\n<beep><red>Hello? Is anybody out there?? You will be logged off in one minute unless\n"
				"you start looking more alive!\n");
			usr->idle_timer->sleeptime = SECS_IN_MIN;
			break;

		case (TIMEOUT_USER-1):
			Put(usr, "\n<beep><red>WAKE UP! You will be logged off NOW unless you show you're alive!\n");
			usr->idle_timer->sleeptime = 6;
			break;

		case (TIMEOUT_USER-2):
			Put(usr, "\n<beep><red>You are being logged off due to inactivity\n");
			notify_idle(usr);
			log_auth("IDLE %s (%s)", usr->name, usr->from_ip);
			close_connection(usr, "%s went idle", usr->name);
	}
}

void save_timeout(void *v) {
User *usr;

	usr = (User *)v;
	if (usr->name[0] && save_User(usr) == -1)
		log_err("failed to periodically save user %s", usr->name);
}

void reboot_timeout(void *v) {
Timer *t;
User *u;
StringList *screen, *sl;

	t = (Timer *)v;
	switch(t->restart) {
		case TIMEOUT_REBOOT:
			t->sleeptime = t->maxtime = 30;		/* one minute to go! */
			system_broadcast(0, "The system will reboot in one minute!");
			break;

		case (TIMEOUT_REBOOT-1):
			t->sleeptime = t->maxtime = 25;
			system_broadcast(OVERRULE, "The system will reboot in 30 seconds!");
			break;

		case (TIMEOUT_REBOOT-2):
			t->sleeptime = t->maxtime = 5;
			system_broadcast(OVERRULE, "The system will reboot in 5 seconds!");
			break;

		case (TIMEOUT_REBOOT-3):
			log_msg("rebooting, logging off all users");

			screen = load_StringList(PARAM_REBOOT_SCREEN);

			for(u = AllUsers; u != NULL; u = u->next) {
				for(sl = screen; sl != NULL; sl = sl->next)
					Print(u, "%s\n", sl->str);
				close_connection(u, "reboot");
			}
			log_msg("reboot procedure completed, exiting");
			exit_program(REBOOT);
	}
}

void shutdown_timeout(void *v) {
Timer *t;
User *u;
StringList *screen, *sl;

	t = (Timer *)v;
	switch(t->restart) {
		case TIMEOUT_SHUTDOWN:
			t->sleeptime = t->maxtime = 30;		/* one minute to go! */
			system_broadcast(0, "The system will shutdown in one minute!");
			break;

		case (TIMEOUT_SHUTDOWN-1):
			t->sleeptime = t->maxtime = 25;
			system_broadcast(OVERRULE, "The system will shutdown in 30 seconds!");
			break;

		case (TIMEOUT_SHUTDOWN-2):
			t->sleeptime = t->maxtime = 5;
			system_broadcast(OVERRULE, "The system will shutdown in 5 seconds!");
			break;

		case (TIMEOUT_SHUTDOWN-3):
			log_msg("shutting down, logging off all users");

			screen = load_StringList(PARAM_SHUTDOWN_SCREEN);

			for(u = AllUsers; u != NULL; u = u->next) {
				for(sl = screen; sl != NULL; sl = sl->next)
					Print(u, "%s\n", sl->str);
				close_connection(u, "shutdown");
			}
			log_msg("shutdown sequence completed, exiting");
			exit_program(SHUTDOWN);
	}
}

/* EOB */
