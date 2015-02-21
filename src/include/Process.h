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
	Process.h	WJ99

	for background processes
*/

#ifndef PROCESS_H_WJ99
#define PROCESS_H_WJ99 1

#include <config.h>
#include <sys/types.h>

#include "sys_time.h"

#define PROC_RESTART	1

typedef struct {
	char *name, *path, **argv;
	pid_t pid;
	int flags, died_times;
	time_t start_time;
} Process;

extern int ignore_sigchld;

int fork_process(Process *);
int restart_process(Process *);
void wait_process(void);
void process_sigpipe(int);
void process_sigchld(int);
int bbs_init_process(void);
void killall_process(void);

#endif	/* PROCESS_H_WJ99 */

/* EOB */
