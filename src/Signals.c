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
	Signals.c	WJ97
*/

#include "config.h"
#include "Signals.h"
#include "debug.h"
#include "main.h"
#include "cstring.h"
#include "User.h"
#include "screens.h"
#include "util.h"
#include "log.h"
#include "inet.h"
#include "Process.h"
#include "main.h"
#include "state_room.h"
#include "Timer.h"
#include "timeout.h"
#include "Param.h"
#include "OnlineUser.h"
#include "Memory.h"
#include "bufprintf.h"
#include "my_fcntl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif


static sigset_t sig_pending;	/* pending signals */
static int crashing = 0;		/* we're crashing */
int jump_set = 0;
jmp_buf jumper;					/* crash recovery trampoline */


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
	{ SIGTERM,		sig_shutnow,"SIGTERM",			NULL },
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


void init_Signal(int debugger) {
struct sigaction sa, old_sa;
int i, dumpcore;

	sigemptyset(&sig_pending);

	if (debugger)
		return;

	dumpcore = 0;
	if (!cstricmp(PARAM_ONCRASH, "dumpcore"))
		dumpcore = 1;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig != SIGKILL && sig_table[i].sig != SIGSTOP
			&& sig_table[i].sig != SIGTRAP) {
/*
	if dumpcore, do not set a handler but use the default handler,
	which will make it dump core for us
*/
			if (dumpcore && sig_table[i].default_handler == sig_fatal)
				continue;

			sigfillset(&sa.sa_mask);
#ifdef SA_RESTART
			sa.sa_flags = SA_RESTART;
#endif
			if (sig_table[i].default_handler == sig_fatal)
				sa.sa_handler = catch_sigfatal;
			else
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
	Note: deinit_Signal() is only called before an exit() or abort()
	Therefore, bluntly restore all signal handlers to SIG_DFL
*/
void deinit_Signal(void) {
int i;

	block_all_signals();
	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig != SIGKILL && sig_table[i].sig != SIGSTOP)
			signal(sig_table[i].sig, SIG_DFL);
	}
	unblock_all_signals();
	sigemptyset(&sig_pending);
}

/*
	buflen should be at least MAX_SIGNAME in size
*/
char *sig_name(int sig, char *signame_buf, int buflen) {
int i;

	if (signame_buf == NULL)
		return NULL;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig == sig_table[i].sig) {
			cstrcpy(signame_buf, sig_table[i].sig_name, buflen);
			return signame_buf;
		}
	}
	bufprintf(signame_buf, buflen, "(unknown signal %d)", sig);
	return signame_buf;
}

/* add a signal handler */
int set_Signal(int sig, void (*handler)(int)) {
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig == sig) {
			SignalVector *h;

			if ((h = new_SignalVector(handler)) == NULL) {
				log_err("Out of memory allocating SignalVector for signal %d", sig);
				return -1;
			}
			(void)add_SignalVector(&(sig_table[i].handlers), h);
			return 0;
		}
	}
	log_err("unknown signal '%d'", sig);
	return -1;
}

void remove_Signal(int sig, void (*handler)(int)) {
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig_table[i].sig == sig) {
			SignalVector *h;

			for(h = sig_table[i].handlers; h != NULL; h = h->next) {
				if (h->handler == handler)
					(void)remove_SignalVector(&sig_table[i].handlers, h);
			}
			return;
		}
	}
}

/*
	catch signal only adds it to the pending set
	the signal handled later, _synchronously_, so that the signal handlers
	are much safer than they used to be
*/
RETSIGTYPE catch_signal(int sig) {
	sigaddset(&sig_pending, sig);
}

/*
	bloody fatal signals should be dealt with immediately
	otherwise we could keep bouncing on the instruction that causes SEGV,
	for example
	We just jump out, and hope that crash_recovery() can do something
	about it ... if the jump had not been set yet, then I don't know
	either, just abort!
*/
RETSIGTYPE catch_sigfatal(int sig) {
	if (crashing)								/* do not attempt recovery of crashes within crashes */
		abort();

	crashing = 1;
	if (jump_set) {
		sigaddset(&sig_pending, SIGSEGV);		/* this really is a flag for crash_recovery() */
		longjmp(jumper, 1);
	} else
		abort();
}

/*
	process the pending signals

	- call all registered signal handlers
	- also call default handler if defined

	Note: they are processed in the order of appearance,
	not in the order they were received
*/
void handle_pending_signals(void) {
sigset_t block;
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sigismember(&sig_pending, sig_table[i].sig)) {
			SignalVector *h, *h_next;

/*
	temporarily disable this signal while running its handler(s)
*/
			sigemptyset(&block);
			sigaddset(&block, sig_table[i].sig);
			sigprocmask(SIG_BLOCK, &block, NULL);

			for(h = sig_table[i].handlers; h != NULL; h = h_next) {
				h_next = h->next;
				if (h->handler != NULL)
					h->handler(sig_table[i].sig);
			}
			if (sig_table[i].default_handler != NULL
				&& sig_table[i].default_handler != SIG_IGN
				&& sig_table[i].default_handler != SIG_DFL)
				sig_table[i].default_handler(sig_table[i].sig);

			sigdelset(&sig_pending, sig_table[i].sig);

			sigfillset(&block);
			sigprocmask(SIG_UNBLOCK, &block, NULL);
/*
	if this signal is now pending because it was raised while being blocked,
	it will be seen by sigpending(), but not until the next loop in mainloop()
*/
		}
	}
}

void block_all_signals(void) {
sigset_t all_signals;

	sigfillset(&all_signals);
	sigprocmask(SIG_BLOCK, &all_signals, NULL);
}

void unblock_all_signals(void) {
sigset_t all_signals, pending;
int i;

	sigfillset(&all_signals);
	sigprocmask(SIG_UNBLOCK, &all_signals, NULL);

	sigpending(&pending);			/* these were received while blocked */
	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sigismember(&pending, sig_table[i].sig))
			sigaddset(&sig_pending, sig_table[i].sig);
	}
}

/*
	block all signals that can modify the global timer queue, while running the global timer queue
*/
void block_timer_signals(int how) {
	if (how == SIG_BLOCK) {
		sigset_t signals;

		sigemptyset(&signals);
		sigaddset(&signals, SIGQUIT);
		sigaddset(&signals, SIGABRT);
		sigaddset(&signals, SIGTERM);
		sigprocmask(SIG_BLOCK, &signals, NULL);
	} else
		unblock_all_signals();
}

/*
	below are the 'signal handlers'
	They are not really signal handlers, because in bbs100 they are socalled SignalVectors
	which are the synchronous equivalents of signal handlers
	They are being run from the mainloop by handle_pending_signals()
*/

/*
	just call catch_sigfatal() ...
	(this thing is mostly used to avoid compile problems when RETSIGTYPE is an int)

	sig_fatal() can still be called directly, this happens when SEGV is received
	while we were blocking signals
*/
void sig_fatal(int sig) {
	catch_sigfatal(sig);
}

/*
	SIGQUIT: reboot in 5
*/
void sig_reboot(int sig) {
char buf[MAX_LONGLINE], total_buf[MAX_LINE];

	Enter(sig_reboot);
	log_msg("SIGQUIT received, rebooting in 5 minutes");

	if (reboot_timer != NULL) {
		if (reboot_timer->sleeptime > 4 * SECS_IN_MIN) {
			reboot_timer->maxtime = 4 * SECS_IN_MIN;	/* reboot in 5 mins */
			reboot_timer->restart = TIMEOUT_REBOOT;
			set_Timer(&timerq, reboot_timer, reboot_timer->maxtime);

			bufprintf(buf, sizeof(buf), "The system is now rebooting in %s", 
				print_total_time((unsigned long)reboot_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf, sizeof(total_buf)));
			system_broadcast(0, buf);
		}
		Return;
	}
	if ((reboot_timer = new_Timer(4 * SECS_IN_MIN, reboot_timeout, TIMEOUT_REBOOT)) == NULL) {
		log_msg("SIGQUIT: Out of memory, reboot cancelled");
		log_err("SIGQUIT: Out of memory, reboot cancelled");
		Return;
	}
	add_Timer(&timerq, reboot_timer);

	bufprintf(buf, sizeof(buf), "The system is rebooting in %s",
		print_total_time((unsigned long)reboot_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf, sizeof(total_buf)));
	system_broadcast(0, buf);
	Return;
}

/*
	SIGABRT: shutdown in 5
*/
void sig_shutdown(int sig) {
char buf[MAX_LONGLINE], total_buf[MAX_LINE];

	Enter(sig_shutdown);
	log_msg("SIGTERM received, shutting down in 5 minutes");

	if (shutdown_timer != NULL) {
		if (shutdown_timer->sleeptime > 4 * SECS_IN_MIN) {
			shutdown_timer->maxtime = 4 * SECS_IN_MIN;	/* shutdown in 5 mins */
			shutdown_timer->restart = TIMEOUT_REBOOT;
			set_Timer(&timerq, shutdown_timer, shutdown_timer->maxtime);

			bufprintf(buf, sizeof(buf), "The system is now shutting down in %s",
				print_total_time((unsigned long)shutdown_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf, sizeof(total_buf)));
			system_broadcast(0, buf);
		}
		Return;
	}
	if ((shutdown_timer = new_Timer(4 * SECS_IN_MIN, shutdown_timeout, TIMEOUT_REBOOT)) == NULL) {
		log_msg("SIGTERM: Out of memory, shutdown cancelled");
		log_err("SIGTERM: Out of memory, shutdown cancelled");
		Return;
	}
	add_Timer(&timerq, shutdown_timer);

	bufprintf(buf, sizeof(buf), "The system is shutting down in %s",
		print_total_time((unsigned long)shutdown_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf, sizeof(total_buf)));
	system_broadcast(0, buf);
	Return;
}

/*
	SIGTERM: immediate shutdown
*/
void sig_shutnow(int sig) {
StringIO *screen;
User *u;
char signame_buf[MAX_SIGNAME];

	Enter(sig_shutnow);

	log_info("*** shutting down on %s ***", sig_name(sig, signame_buf, MAX_SIGNAME));

	if ((screen = new_StringIO()) != NULL && load_screen(screen, PARAM_SHUTDOWN_SCREEN) >= 0) {
		for(u = AllUsers; u != NULL; u = u->next) {
			display_text(u, screen);
			flush_Conn(u->conn);
			close_connection(u, "system shutdown");
		}
	}
	destroy_StringIO(screen);
	exit_program(SHUTDOWN);
	Return;
}

/*
	SIGUSR1: rescan Mail> directory
*/
void sig_mail(int sig) {
User *u;
unsigned long old_mail;

	if (!PARAM_HAVE_MAILROOM)
		return;

	Enter(sig_mail);

	for(u = AllUsers; u != NULL; u = u->next) {
		if (u->mail == NULL || !u->name[0] || u->conn == NULL || u->conn->state != CONN_ESTABLISHED)
			continue;

		old_mail = u->mail->head_msg;	

		room_readmaildir(u->mail, u->name);

		if (old_mail != u->mail->head_msg)
			Tell(u, "<beep><cyan>You have new mail\n");
	}
	Return;
}

/*
	SIGHUP: set/reset nologin status
*/
void sig_nologin(int sig) {
char filename[MAX_PATHLEN];

	Enter(sig_nologin);

	log_msg("SIGHUP caught; reset nologin");

	bufprintf(filename, sizeof(filename), "%s/%s", PARAM_CONFDIR, NOLOGIN_FILE);

	if (!nologin_active) {
		close(open(filename, O_CREAT|O_WRONLY|O_TRUNC, (mode_t)0660));
		nologin_active = 1;
		log_msg("nologin set ; users cannot login");
	} else {
		unlink(filename);
		nologin_active = 0;
		log_msg("nologin reset ; users can login");
	}
	Return;
}


/*
	do crash recovery for crashed users
	this is done out of the signal handler to prevent even more havoc

	The principle of crash recovery is jumping out of an error condition
	In reality, this is unsafe in all cases because the called routine does not get
	the chance to finish and cleanup nicely. It is wiser to configure 'oncrash dumpcore',
	which is unfriendly to the users ...
*/
void crash_recovery(void) {
User *usr;

	if (!sigismember(&sig_pending, SIGSEGV)) {		/* did it really crash? */
		crashing = 0;
		return;
	}
	usr = this_user;
	this_user = NULL;

	if (!cstricmp(PARAM_ONCRASH, "recover") && usr != NULL) {
		struct tm *tm;

		if (usr->name[0])
			log_err("CRASH *** user %s; recovering (%d) ***", usr->name, usr->crashed+1);
		else
			log_err("CRASH *** user <unknown>; recovering (%d) ***", usr->crashed+1);

#ifdef DEBUG
		dump_debug_stack();
		debug_stackp = 0;
		debug_stack[0] = 0UL;
#endif
		tm = user_time(usr, (time_t)0UL);
		if ((usr->flags & USR_12HRCLOCK) && (tm->tm_hour > 12))
			tm->tm_hour -= 12;

		usr->crashed++;
		if (usr->crashed >= 3) {
			if (usr->name[0])
				log_err("CRASH *** disconnecting user %s", usr->name);
			else
				log_err("CRASH *** disconnecting user");

			Print(usr, "\n<beep><white>*** <yellow>System message received at %d:%02d <white>***\n"
				"<red>Something's wrong, the BBS made an illegal instruction\n"
				"You are automatically being disconnected -- our apologies..!\n",
				tm->tm_hour, tm->tm_min);

			if (usr->name[0]) {
				remove_OnlineUser(usr);
				usr->name[0] = 0;
			}
			close_connection(usr, "user crashed too many times");
			crashing = 0;
			return;
		}
		Print(usr, "\n<beep><white>*** <yellow>System message received at %d:%02d <white>***\n"
			"<red>Something's wrong, the BBS made an illegal instruction\n"
			"Attempting crash recovery...\n", tm->tm_hour, tm->tm_min);

		listdestroy_CallStack(usr->conn->callstack);
		usr->conn->callstack = NULL;
		CALL(usr, STATE_ROOM_PROMPT);
	} else {
		User *u;

		deinit_Signal();		/* reset all signal handlers */

		log_err("CRASH *** program terminated ***");
#ifdef DEBUG
		dump_debug_stack();
#endif
		for(u = AllUsers; u != NULL; u = u->next) {
			display_text(u, crash_screen);
			Put(u, "<default>\n");
			flush_Conn(u->conn);
			close_connection(u, "system crash");
		}
/*
	try to nicely shut the sockets down, before the guard process
	reboots us
*/
		shut_allconns();
		sleep(2);
		abort();
	}
	crashing = 0;
}

/* EOB */
