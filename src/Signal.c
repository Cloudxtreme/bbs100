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
	Signal.c	WJ97
*/

#include "config.h"
#include "Signal.h"
#include "debug.h"
#include "User.h"
#include "screens.h"
#include "util.h"
#include "log.h"
#include "inet.h"
#include "Process.h"
#include "main.h"
#include "state.h"
#include "Timer.h"
#include "timeout.h"
#include "Param.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif

/* Note: only used at shutdown */
int ignore_signals = 0;

jmp_buf jumper;


SigTable sig_table[] = {
	{ SIGHUP,		sig_nologin,"SIGHUP",			NULL },
	{ SIGINT,		sig_fatal,	"SIGINT",			NULL },
	{ SIGQUIT,		sig_reboot,	"SIGQUIT",			NULL },
	{ SIGILL,		sig_fatal,	"SIGILL",			NULL },
	{ SIGTRAP,		sig_fatal,	"SIGTRAP",			NULL },
	{ SIGABRT,		sig_shutdown,"SIGABRT/SIGIOT",	NULL },
	{ SIGBUS,		sig_fatal,	"SIGBUS",			NULL },
	{ SIGFPE,		sig_fatal,	"SIGFPE",			NULL },
	{ SIGKILL,		NULL,		"SIGKILL",			NULL },
	{ SIGUSR1,		sig_mail,	"SIGUSR1",			NULL },
	{ SIGSEGV,		sig_fatal,	"SIGSEGV",			NULL },
	{ SIGUSR2,		SIG_IGN,	"SIGUSR2",			NULL },
	{ SIGPIPE,		SIG_IGN,	"SIGPIPE",			NULL },
	{ SIGALRM,		NULL,		"SIGALRM",			NULL },
	{ SIGTERM,		sig_fatal,	"SIGTERM",			NULL },
	{ SIGCHLD,		SIG_IGN,	"SIGCHLD",			NULL },
	{ SIGCONT,		SIG_IGN,	"SIGCONT",			NULL },
	{ SIGSTOP,		NULL,		"SIGSTOP",			NULL },
	{ SIGTSTP,		NULL,		"SIGTSTP",			NULL },
	{ SIGTTIN,		NULL,		"SIGTTIN",			NULL },
	{ SIGTTOU,		NULL,		"SIGTTOU",			NULL },
	{ SIGURG,		SIG_IGN,	"SIGURG",			NULL },
	{ SIGWINCH,		SIG_IGN,	"SIGWINCH",			NULL },
	{ SIGIO,		SIG_IGN,	"SIGIO",			NULL },
#ifdef SIGPWR
	{ SIGPWR,		sig_fatal,	"SIGPWR",			NULL },
#endif
#ifdef SIGSTKFLT
	{ SIGSTKFLT,	SIG_IGN,	"SIGSTKFLT",		NULL },
#endif
#ifdef SIGXCPU
	{ SIGXCPU,		sig_fatal,	"SIGXCPU",			NULL },
#endif
#ifdef SIGXFSZ
	{ SIGXFSZ,		SIG_IGN,	"SIGXFSZ",			NULL },
#endif
#ifdef SIGVTALRM
	{ SIGVTALRM,	SIG_IGN,	"SIGVTALRM",		NULL },
#endif
#ifdef SIGPROF
	{ SIGPROF,		SIG_IGN,	"SIGPROF",			NULL },
#endif
#ifdef SIGERR
	{ SIGERR,		sig_fatal,	"SIGERR",			NULL },
#endif
#ifdef SIGPRE
	{ SIGPRE,		sig_fatal,	"SIGPRE",			NULL },
#endif
#ifdef SIGORE
	{ SIGORE,		sig_fatal,	"SIGORE",			NULL },
#endif
#ifdef SIGSYS
	{ SIGSYS,		sig_fatal,	"SIGSYS",			NULL },
#endif
#ifdef SIGUNUSED
	{ SIGUNUSED,	SIG_IGN,	"SIGUNUSED",		NULL },
#endif
	{ -1,			NULL,		"",					NULL },
};


void init_Signal(void) {
struct sigaction sa, old_sa;
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig != SIGKILL && sig_table[i].sig != SIGSTOP
			&& sig_table[i].sig != SIGTRAP) {
			sigfillset(&sa.sa_mask);
#ifdef SA_RESTART
			sa.sa_flags = SA_RESTART;
#endif
			sa.sa_handler = catch_signal;

/* restart interrupted system calls, except for SIGALRM */

#ifdef SA_INTERRUPT
			if (sig_table[i].sig == SIGALRM)
				sa.sa_flags = SA_INTERRUPT;		/* for SunOS 4.x */
#endif
#ifdef SA_NOCLDSTOP								/* don't do SIGCHLD on stop */
			if (sig_table[i].sig == SIGCHLD)
				sa.sa_flags |= SA_NOCLDSTOP;
#endif
			if (sigaction(sig_table[i].sig, &sa, &old_sa)) {
				log_err("sigaction(%s) failed, ignored", sig_table[i].sig_name);
			}
		}
	}
}


/*
	Note: deinit_Signal() is only called before an exit()
	Therefore, bluntly restore all signal handlers to SIG_DFL
*/
void deinit_Signal(void) {
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig != SIGKILL && sig_table[i].sig != SIGSTOP)
			signal(sig_table[i].sig, SIG_DFL);
	}
}

char *sig_name(int sig) {
static char unknown[80];
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig == sig_table[i].sig)
			return sig_table[i].sig_name;
	}
	sprintf(unknown, "(unknown signal %d)", sig);
	return unknown;
}

/*
	the crash handler
*/
RETSIGTYPE sig_fatal(int sig) {
StringList *sl, *screen;
User *u;

	Enter(sig_fatal);

	if (this_user != NULL) {
		time_t now;
		struct tm *tm;
		User *usr;

		usr = this_user;			/* prevent 'resonance' in case we crash again */
		this_user = NULL;

		if (usr->name[0]) {
			log_msg("CRASH *** user %s; recovering ***", usr->name);
			log_err("CRASH *** user %s; recovering ***", usr->name);
		} else {
			log_msg("CRASH *** user %s; recovering ***", usr->name);
			log_err("CRASH *** user %s; recovering ***", usr->name);
		}
#ifdef DEBUG
		dump_debug_stack();
		debug_stackp = 0;
		debug_stack[0] = 0UL;
#endif
		now = rtc + usr->time_disp;
		tm = gmtime(&now);
		if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
			tm->tm_hour -= 12;

		Print(usr, "\n<beep><white>*** <yellow>System message received at %d<white>:<yellow>%02d<white> ***<red>\n"
			"<red>Something's wrong, the BBS made an illegal instruction\n"
			"Attempting crash recovery...\n", tm->tm_hour, tm->tm_min);

		listdestroy_CallStack(usr->callstack);
		usr->callstack = NULL;
		CALL(usr, STATE_ROOM_PROMPT);

		longjmp(jumper, 1);
	}
	this_user = NULL;

	deinit_Signal();		/* reset all signal handlers */

	if (sig != SIGTERM) {
		log_msg("CRASH *** terminated on %s ***", sig_name(sig));
		log_err("CRASH *** terminated on %s ***", sig_name(sig));
#ifdef DEBUG
		dump_debug_stack();
#endif
		screen = crash_screen;
	} else {
		log_info("*** shutting down on %s ***", sig_name(sig));

		if ((screen = load_StringList(PARAM_SHUTDOWN_SCREEN)) == NULL)
			screen = crash_screen;
	}
	for(u = AllUsers; u != NULL; u = u->next) {
		for(sl = screen; sl != NULL; sl = sl->next)
			Print(u, "%s\n", sl->str);
		close_connection(u, "system crash");
	}
	if (sig == SIGTERM)
		exit_program(SHUTDOWN);
	else
		exit_program(REBOOT);
	Return;
}

/*
	SIGQUIT: reboot in 5
*/
RETSIGTYPE sig_reboot(int sig) {
char buf[128];

	Enter(sig_reboot);
	log_msg("SIGQUIT received, rebooting in 5 minutes");

	if (reboot_timer != NULL) {
		reboot_timer->sleeptime = reboot_timer->maxtime = 4*60;		/* reboot in 5 mins */
		reboot_timer->restart = TIMEOUT_REBOOT;

		sprintf(buf, "The system is now rebooting in %s", 
			print_total_time((unsigned long)reboot_timer->sleeptime + 60UL));
		system_broadcast(0, buf);
		Return;
	}
	if ((reboot_timer = new_Timer(4*60, reboot_timeout, TIMEOUT_REBOOT)) == NULL) {
		log_msg("SIGQUIT: Out of memory, reboot cancelled");
		log_err("SIGQUIT: Out of memory, reboot cancelled");
		Return;
	}
	add_Timer(&timerq, reboot_timer);

	sprintf(buf, "The system is rebooting in %s",
		print_total_time((unsigned long)reboot_timer->sleeptime + 60UL));
	system_broadcast(0, buf);
	Return;
}

/*
	SIGABRT: shutdown in 5
*/
RETSIGTYPE sig_shutdown(int sig) {
char buf[128];

	Enter(sig_shutdown);
	log_msg("SIGTERM received, shutting down in 5 minutes");

	if (shutdown_timer != NULL) {
		shutdown_timer->sleeptime = shutdown_timer->maxtime = 4*60;		/* shutdown in 5 mins */
		shutdown_timer->restart = TIMEOUT_REBOOT;

		sprintf(buf, "The system is now shutting down in %s",
			print_total_time((unsigned long)shutdown_timer->sleeptime + 60UL));
		system_broadcast(0, buf);
		Return;
	}
	if ((shutdown_timer = new_Timer(4*60, shutdown_timeout, TIMEOUT_REBOOT)) == NULL) {
		log_msg("SIGTERM: Out of memory, shutdown cancelled");
		log_err("SIGTERM: Out of memory, shutdown cancelled");
		Return;
	}
	add_Timer(&timerq, shutdown_timer);

	sprintf(buf, "The system is shutting down in %s",
		print_total_time((unsigned long)shutdown_timer->sleeptime + 60UL));
	system_broadcast(0, buf);
	Return;
}

/*
	SIGUSR1: rescan Mail> directory
*/
RETSIGTYPE sig_mail(int sig) {
User *u;
char buf[MAX_PATHLEN];
struct stat statbuf;
int new_mail = 0;
unsigned long num;

	Enter(sig_mail);
	for(u = AllUsers; u != NULL; u = u->next) {
		if (u->socket > 0 && u->name[0] && u->mail != NULL) {
			if (u->mail->msgs == NULL)
				num = 0UL;
			else {
				u->mail->msgs = unwind_MsgIndex(u->mail->msgs);
				num = u->mail->msgs->number;
			}
			for(;;) {
				num++;
				sprintf(buf, "%s/%c/%s/%lu", PARAM_USERDIR, u->name[0], u->name, num);

				if (!stat(buf, &statbuf)) {
					new_mail = 1;
					u->mail->msgs = add_MsgIndex(&u->mail->msgs, new_MsgIndex(num));
				} else
					break;
			}
			u->mail->msgs = rewind_MsgIndex(u->mail->msgs);

			if (new_mail) {
				Tell(u, "<beep><cyan>You have new mail\n");
				new_mail = 0;
			}
		}
	}
	Return;
}

/*
	SIGHUP: set/reset nologin status
*/
RETSIGTYPE sig_nologin(int sig) {
	Enter(sig_nologin);

	log_msg("SIGHUP caught; reset nologin");

	if (nologin_screen == NULL) {
		if ((nologin_screen = load_StringList(PARAM_NOLOGIN_SCREEN)) == NULL) {
			log_err("failed to load nologin_screen %s", PARAM_NOLOGIN_SCREEN);
		} else {
			log_msg("nologin set ; users cannot login");
		}
	} else {
		listdestroy_StringList(nologin_screen);
		nologin_screen = NULL;
		log_msg("nologin reset ; users can login");
	}
	Return;
}


/*
	catch signal:
		call all registered signal handlers
		also calls the default handler when it is done
*/
RETSIGTYPE catch_signal(int sig) {
int i;

	if (ignore_signals)
		return;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig == sig) {
			SignalVector *h, *h_next;

			for(h = sig_table[i].handlers; h != NULL; h = h_next) {
				h_next = h->next;
				if (h->handler != NULL)
					h->handler(sig);
			}
			if (sig_table[i].default_handler != NULL
				&& sig_table[i].default_handler != SIG_IGN
				&& sig_table[i].default_handler != SIG_DFL)
				sig_table[i].default_handler(sig);
			return;
		}
	}
	log_err("caught unknown signal %d", sig);
}

/* add a signal handler */
int set_Signal(int sig, RETSIGTYPE (*handler)(int)) {
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig == sig) {
			SignalVector *h;

			if ((h = new_SignalVector(handler)) == NULL) {
				log_err("Out of memory allocating SignalVector for signal %d", sig);
				return -1;
			}
			add_SignalVector(&(sig_table[i].handlers), h);
			return 0;
		}
	}
	log_err("unknown signal '%d'", sig);
	return -1;
}

void remove_Signal(int sig, RETSIGTYPE (*handler)(int)) {
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig == sig) {
			SignalVector *h;

			for(h = sig_table[i].handlers; h != NULL; h = h->next) {
				if (h->handler == handler)
					remove_SignalVector(&sig_table[i].handlers, h);
			}
			return;
		}
	}
}

/* EOB */
