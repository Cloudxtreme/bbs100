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
	Process.c	WJ99
*/

#include "config.h"
#include "defines.h"
#include "Process.h"
#include "PList.h"
#include "inet.h"
#include "util.h"
#include "log.h"
#include "Signals.h"
#include "cstring.h"
#include "Param.h"
#include "ConnResolv.h"
#include "cstrerror.h"
#include "sys_wait.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>


int ignore_sigchld = 0;

static PList *process_table = NULL;


int bbs_init_process(void) {
	set_Signal(SIGCHLD, process_sigchld);
	return 0;
}

int fork_process(Process *proc) {
	if (proc == NULL)
		return -1;

	proc->pid = (pid_t)-1L;
	proc->start_time = (time_t)0UL;

	proc->pid = fork();
	if (proc->pid == (pid_t)-1L) {
		log_err("failed to fork()");
		return -1;
	}
	if (!proc->pid) {
		execv(proc->path, proc->argv);
		log_err("exec(\"%s\") failed", proc->path);
		exit(-1);
	}
	waitpid(-1, NULL, WNOHANG);

	proc->start_time = time(NULL);
	process_table = add_PList(&process_table, new_PList(proc));
	log_msg("%s started, pid %u", proc->name, proc->pid);
	return 0;
}


int restart_process(Process *proc) {
	if (proc == NULL)
		return -1;

	kill(proc->pid, SIGKILL);			/* make sure it is dead */
	proc->pid = (pid_t)-1L;

	if (!(proc->flags & PROC_RESTART))
		return 1;

	log_msg("restarting %s", proc->name);

	if (((unsigned long)proc->start_time > 0UL)
		&& (((unsigned long)time(NULL) - (unsigned long)proc->start_time) < (unsigned long)SECS_IN_MIN)) {
		proc->died_times++;
		if (proc->died_times > 3) {
			log_warn("%s died too many times, continuing without", proc->name);
/*
	reset start_time, so the operator can restart the process by killing
	the bbs with a SIGCHLD
*/
			proc->start_time = (time_t)0UL;
			proc->died_times = 0;
			return -1;
		}
	} else
		proc->died_times = 0;

	if (fork_process(proc)) {
		log_warn("failed to restart %s, continuing without", proc->name);
		proc->died_times = 0;
		proc->start_time = (time_t)0UL;
		return -1;
	}
	return 0;
}

void wait_process(void) {
int status;
pid_t pid;
PList *pl, *pl_next;
Process *proc;
char errbuf[MAX_LINE];

	pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
	if (pid == (pid_t)-1L) {
		log_err("waitpid(): %s", cstrerror(errno, errbuf, MAX_LINE));
		return;
	}
	if (pid == (pid_t)0L) {
/*		log_err("SIGCHLD caught; waitpid() says no child available");	*/
		return;
	}
	for(pl = process_table; pl != NULL; pl = pl_next) {
		pl_next = pl->next;
		proc = (Process *)pl->p;

		if (pid == proc->pid) {
			char signame_buf[MAX_SIGNAME];

			if (WIFSIGNALED(status))
				log_msg("%s terminated on signal %s", proc->name, sig_name(WTERMSIG(status), signame_buf, MAX_SIGNAME));
			else
				if (WIFSTOPPED(status))
					log_msg("%s stopped on signal %s", proc->name, sig_name(WSTOPSIG(status), signame_buf, MAX_SIGNAME));
				else
					log_msg("%s terminated, exit code %d", proc->name, WEXITSTATUS(status));
/*
	destroy this plist, because restart_process() adds a new one if it succeeds
	Note: do not destroy pl->p; it's pointing to static memory
*/
			(void)remove_PList(&process_table, pl);
			destroy_PList(pl);

			restart_process(proc);
			return;
		}
	}
	log_msg("SIGCHLD caught; pid %lu not found in bbs internal process table", (unsigned long)pid);
}

/*
	SIGPIPE may be caused by lost connections, but also by a child that
	broke a pipe connection
*/
void process_sigpipe(int sig) {
	log_err("SIGPIPE caught");
	wait_process();
}

/* SIGCHLD was caused by an exiting child */
void process_sigchld(int sig) {
/*
	there are circumstances when we want to avoid this
	(like when taking a core dump)
	easiest portable solution is to simply use this variable and return
*/
	if (ignore_sigchld)
		return;

/*	log_msg("SIGCHLD caught");	*/
	wait_process();
}


void killall_process(void) {
PList *pl, *pl_next;
Process *proc;

	for(pl = process_table; pl != NULL; pl = pl_next) {
		pl_next = pl->next;

		proc = (Process *)pl->p;
		if ((long)proc->pid > -1L) {
			kill(proc->pid, SIGTERM);
			proc->pid = (pid_t)-1L;
		}
	}
	listdestroy_PList(process_table);
	process_table = NULL;
}

/* EOB */
