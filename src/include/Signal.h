/*
    bbs100 2.1 WJ104
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
	Signal.h	WJ97
*/

#ifndef SIGNAL_H_WJ97
#define SIGNAL_H_WJ97 1

#include <config.h>
#include "SignalVector.h"

#include <setjmp.h>

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
