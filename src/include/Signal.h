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
	Signal.h	WJ97
*/

#ifndef SIGNAL_H_WJ97
#define SIGNAL_H_WJ97 1

#include <config.h>
#include "SignalVector.h"

#include <signal.h>
#include <setjmp.h>

/*
	the following is for brain-dead OSes and broken development environments
	e.g. Cygwin for Windows
	The code will compile, but you will miss some functionality, because
	the OS doesn't really support the defined signals
*/
#ifndef SIGINT
#define SIGINT	2
#endif

#ifndef SIGKILL
#define SIGKILL	9
#endif

#ifndef SIGTERM
#define SIGTERM	15
#endif

/*
	actually, SIGCHLD and SIGCONT can have other values on different implementations
	so defining them yourself is not a very solid solution
*/
#ifndef SIGCHLD
#define SIGCHLD	17
#endif

#ifndef SIGCLD
#define SIGCLD	SIGCHLD
#endif

#ifndef SIGCONT
#define SIGCONT	18
#endif

#ifndef SIG_DFL
#define SIG_DFL	((RETSIGTYPE (*)(int))0)		/* default action */
#endif

#ifndef SIG_IGN
#define SIG_IGN	((RETSIGTYPE (*)(int))1)		/* ignore action */
#endif

#ifndef SIG_ERR
#define SIG_ERR	((RETSIGTYPE (*)(int))-1)		/* error return */
#endif


typedef struct {
	int sig;
	void (*default_handler)(int);
	char *sig_name;
	SignalVector *handlers;
} SigTable;

extern int jump_set;
extern jmp_buf jumper;
extern SigTable sig_table[];

void init_Signal(void);
void deinit_Signal(void);
char *sig_name(int, char *);

int set_Signal(int, void (*)(int));
void remove_Signal(int, void (*)(int));

RETSIGTYPE catch_signal(int);
RETSIGTYPE catch_sigfatal(int);
void handle_pending_signals(void);

void block_all_signals(void);
void unblock_all_signals(void);

void sig_fatal(int);
void sig_reboot(int);
void sig_shutdown(int);
void sig_shutnow(int);
void sig_mail(int);
void sig_nologin(int);

void crash_recovery(void);

#endif	/* SIGNAL_H_WJ97 */

/* EOB */
