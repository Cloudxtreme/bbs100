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
	Process.h	WJ99

	for background processes
*/

#ifndef PROCESS_H_WJ99
#define PROCESS_H_WJ99 1

#include <config.h>
#include <sys/types.h>

#include "sys_time.h"

#define PROC_RESTART	1
#define PROC_LOGD		2
#define PROC_RESOLVER	4

typedef struct {
	char *name, *path, **argv;
	pid_t pid;
	int flags, died_times;
	time_t start_time;
} Process;

int fork_process(Process *);
void restart_process(Process *);
void wait_process(void);
void process_sigpipe(int);
void process_sigchld(int);
int init_process(void);
void kill_process(void);

#endif	/* PROCESS_H_WJ99 */

/* EOB */
