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
#include "OnlineUser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif

static sigset_t sig_pending;	/* pending signals */

int jump_set = 0;
jmp_buf jumper;			/* crash recovery trampoline */

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


void init_Signal(void) {
struct sigaction sa, old_sa;
int i, dumpcore;

	sigemptyset(&sig_pending);

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
	Note: signame_buf must be large enough (64 bytes should do)
*/
char *sig_name(int sig, char *signame_buf) {
int i;

	if (signame_buf == NULL)
		return NULL;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sig == sig_table[i].sig) {
			strcpy(signame_buf, sig_table[i].sig_name);
			return signame_buf;
		}
	}
	sprintf(signame_buf, "(unknown signal %d)", sig);
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
			add_SignalVector(&(sig_table[i].handlers), h);
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
					remove_SignalVector(&sig_table[i].handlers, h);
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
int i;

	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sigismember(&sig_pending, sig_table[i].sig)) {
			SignalVector *h, *h_next;

			for(h = sig_table[i].handlers; h != NULL; h = h_next) {
				h_next = h->next;
				if (h->handler != NULL)
					h->handler(sig_table[i].sig);
			}
			if (sig_table[i].default_handler != NULL
				&& sig_table[i].default_handler != SIG_IGN
				&& sig_table[i].default_handler != SIG_DFL) {
				sig_table[i].default_handler(sig_table[i].sig);
				sigdelset(&sig_pending, sig_table[i].sig);
			}
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

	sigpending(&pending);			/* these were received while blocked */
	for(i = 0; sig_table[i].sig > 0; i++) {
		if (sigismember(&pending, sig_table[i].sig))
			sigaddset(&sig_pending, sig_table[i].sig);
	}
	sigfillset(&all_signals);
	sigprocmask(SIG_UNBLOCK, &all_signals, NULL);
}


/*
	below are the 'signal handlers'
	They are not really signal handlers, because in bbs100 they are
	socalled SignalVectors which are the synchronous equivalents of
	signal handlers
	They are being run from the mainloop by handle_pending_signals()
*/

/*
	sig_fatal is an empty dummy function so that init_Signal()
	knows it should install catch_sigfatal()
	It's not very elegant, I know ...
	You will want to check out crash_recovery() below
*/
void sig_fatal(int sig) {
	;
}

/*
	SIGQUIT: reboot in 5
*/
void sig_reboot(int sig) {
char buf[128], total_buf[MAX_LINE];

	Enter(sig_reboot);
	log_msg("SIGQUIT received, rebooting in 5 minutes");

	if (reboot_timer != NULL) {
		reboot_timer->sleeptime = reboot_timer->maxtime = 4 * SECS_IN_MIN;	/* reboot in 5 mins */
		reboot_timer->restart = TIMEOUT_REBOOT;

		sprintf(buf, "The system is now rebooting in %s", 
			print_total_time(NULL, (unsigned long)reboot_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
		system_broadcast(0, buf);
		Return;
	}
	if ((reboot_timer = new_Timer(4 * SECS_IN_MIN, reboot_timeout, TIMEOUT_REBOOT)) == NULL) {
		log_msg("SIGQUIT: Out of memory, reboot cancelled");
		log_err("SIGQUIT: Out of memory, reboot cancelled");
		Return;
	}
	add_Timer(&timerq, reboot_timer);

	sprintf(buf, "The system is rebooting in %s",
		print_total_time(NULL, (unsigned long)reboot_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
	system_broadcast(0, buf);
	Return;
}

/*
	SIGABRT: shutdown in 5
*/
void sig_shutdown(int sig) {
char buf[128], total_buf[128];

	Enter(sig_shutdown);
	log_msg("SIGTERM received, shutting down in 5 minutes");

	if (shutdown_timer != NULL) {
		shutdown_timer->sleeptime = shutdown_timer->maxtime = 4 * SECS_IN_MIN;	/* shutdown in 5 mins */
		shutdown_timer->restart = TIMEOUT_REBOOT;

		sprintf(buf, "The system is now shutting down in %s",
			print_total_time(NULL, (unsigned long)shutdown_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
		system_broadcast(0, buf);
		Return;
	}
	if ((shutdown_timer = new_Timer(4 * SECS_IN_MIN, shutdown_timeout, TIMEOUT_REBOOT)) == NULL) {
		log_msg("SIGTERM: Out of memory, shutdown cancelled");
		log_err("SIGTERM: Out of memory, shutdown cancelled");
		Return;
	}
	add_Timer(&timerq, shutdown_timer);

	sprintf(buf, "The system is shutting down in %s",
		print_total_time(NULL, (unsigned long)shutdown_timer->sleeptime + (unsigned long)SECS_IN_MIN, total_buf));
	system_broadcast(0, buf);
	Return;
}

/*
	SIGTERM: immediate shutdown
*/
void sig_shutnow(int sig) {
StringList *screen, *sl;
User *u;
char signame_buf[MAX_LINE];

	Enter(sig_shutnow);

	log_info("*** shutting down on %s ***", sig_name(sig, signame_buf));

	if ((screen = load_StringList(PARAM_SHUTDOWN_SCREEN)) == NULL)
		screen = crash_screen;

	for(u = AllUsers; u != NULL; u = u->next) {
		for(sl = screen; sl != NULL; sl = sl->next)
			Print(u, "%s\n", sl->str);
		close_connection(u, "system shutdown");
	}
	exit_program(SHUTDOWN);
	Return;
}

/*
	SIGUSR1: rescan Mail> directory
*/
void sig_mail(int sig) {
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
				path_strip(buf);

				if (!stat(buf, &statbuf)) {
					new_mail = 1;
					u->mail->msgs = add_MsgIndex(&u->mail->msgs, new_MsgIndex(num));
				} else
					break;
			}
			u->mail->msgs = rewind_MsgIndex(u->mail->msgs);

			if (PARAM_HAVE_MAILROOM && new_mail) {
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
void sig_nologin(int sig) {
	Enter(sig_nologin);

	log_msg("SIGHUP caught; reset nologin");

	if (nologin_screen == NULL) {
		if ((nologin_screen = load_StringList(PARAM_NOLOGIN_SCREEN)) == NULL)
			log_err("failed to load nologin_screen %s", PARAM_NOLOGIN_SCREEN);
		else
			log_msg("nologin set ; users cannot login");
	} else {
		listdestroy_StringList(nologin_screen);
		nologin_screen = NULL;
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

	if (!sigismember(&sig_pending, SIGSEGV))		/* did it really crash? */
		return;

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

			Print(usr, "\n<beep><white>*** <yellow>System message received at %d<white>:<yellow>%02d<white> ***<red>\n"
				"<red>Something's wrong, the BBS made an illegal instruction\n"
				"You are automatically being disconnected <white>--<yellow> our apologies..!\n",
				tm->tm_hour, tm->tm_min);

			if (usr->name[0]) {
				remove_OnlineUser(usr);
				usr->name[0] = 0;
			}
			close_connection(usr, "user crashed too many times");
			return;
		}
		Print(usr, "\n<beep><white>*** <yellow>System message received at %d<white>:<yellow>%02d<white> ***<red>\n"
			"<red>Something's wrong, the BBS made an illegal instruction\n"
			"Attempting crash recovery...\n", tm->tm_hour, tm->tm_min);

		listdestroy_CallStack(usr->callstack);
		usr->callstack = NULL;
		CALL(usr, STATE_ROOM_PROMPT);
	} else {
		StringList *sl;
		User *u;

		deinit_Signal();		/* reset all signal handlers */

		log_err("CRASH *** program terminated ***");
#ifdef DEBUG
		dump_debug_stack();
#endif
		for(u = AllUsers; u != NULL; u = u->next) {
			for(sl = crash_screen; sl != NULL; sl = sl->next)
				Print(u, "%s\n", sl->str);
			close_connection(u, "system crash");
		}
		if (!cstricmp(PARAM_ONCRASH, "recover"))
			exit_program(REBOOT);
		abort();
	}
}

/* EOB */
