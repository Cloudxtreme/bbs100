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
	Process.c	WJ99
*/

#include "config.h"
#include "Process.h"
#include "inet.h"
#include "util.h"
#include "log.h"
#include "Signal.h"
#include "cstring.h"
#include "Param.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

static char *resolver_argv[3] =	{ "(bbs100 resolver)",	NULL,	NULL	};

static Process process_table[1] = {
	{	"resolver",	NULL,	resolver_argv,	(pid_t)-1L,	PROC_RESTART | PROC_RESOLVER,	0,	(time_t)0UL	},
};


int bbs_init_process(void) {
int i, num;

	set_Signal(SIGCHLD, process_sigchld);

	process_table[0].path = PARAM_PROGRAM_RESOLVER;

	num = sizeof(process_table)/sizeof(Process);	/* start all children */
	for(i = 0; i < num; i++) {
		if (fork_process(&process_table[i])) {
			log_err("failed to start %s", process_table[i].name);
			return -1;
		}
	}
	return 0;
}

/*
	If the process is the resolver, create a Unix domain socket
	If the process is the logger, create a stderr <-> stdin pipe
*/
int fork_process(Process *proc) {
	if (proc == NULL)
		return -1;

	proc->pid = (pid_t)-1L;
	proc->start_time = (time_t)0UL;

	if (proc->flags & PROC_RESOLVER) {
		char buf[MAX_LINE];

/* make pathname for the unix domain socket */
		sprintf(buf, "%s/resolver.%lu", PARAM_CONFDIR, (unsigned long)getpid());
		path_strip(buf);
		if ((dns_main_socket = unix_sock(buf)) < 0) {
			log_err("failed to create unix domain socket");
			return -1;
		}
		log_msg("starting resolver with socket %s", buf);
		proc->argv[1] = buf;
	}
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

	log_msg("%s started, pid %u", proc->name, proc->pid);
	return 0;
}


void restart_process(Process *proc) {
	if (proc == NULL)
		return;

	kill(proc->pid, SIGKILL);		/* make sure it is dead */
	proc->pid = (pid_t)-1L;

	if (proc->flags & PROC_RESOLVER) {
		if (dns_socket > 0) {
			shutdown(dns_socket, 2);
			close(dns_socket);
			dns_socket = -1;
		}
		shutdown(dns_main_socket, 2);
		close(dns_main_socket);			/* this should terminate the resolver for sure */
		dns_main_socket = -1;
	}
	if (!(proc->flags & PROC_RESTART))
		return;

	log_msg("restarting %s", proc->name);

	if (((unsigned long)proc->start_time > 0UL)
		&& (((unsigned long)time(NULL) - (unsigned long)proc->start_time) < (unsigned long)SECS_IN_MIN)) {
		proc->died_times++;
		if (proc->died_times > 3) {
			log_msg("%s died too many times, continuing without", proc->name);
/*
	reset start_time, so the operator can restart the process by killing
	the bbs with a SIGCHLD
*/
			proc->start_time = (time_t)0UL;
			proc->died_times = 0;
			return;
		}
	} else
		proc->died_times = 0;

	if (fork_process(proc)) {
		log_msg("failed to restart %s, continuing without", proc->name);
		proc->died_times = 0;
		proc->start_time = (time_t)0UL;
	}
}

void wait_process(void) {
int i, num, status;
pid_t pid;

	pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
	if (pid == (pid_t)-1L) {
		log_err("waitpid()");
		return;
	}
	if (pid == (pid_t)0L) {
		log_msg("SIGCHLD caught");
		return;
	}
	num = sizeof(process_table)/sizeof(Process);
	for(i = 0; i < num; i++) {
		if (pid == process_table[i].pid) {
			char signame_buf[80];

			if (WIFSIGNALED(status))
				log_msg("%s terminated on signal %s", process_table[i].name, sig_name(WTERMSIG(status), signame_buf));
			else
				if (WIFSTOPPED(status))
					log_msg("%s stopped on signal %s", process_table[i].name, sig_name(WSTOPSIG(status), signame_buf));
				else
					log_msg("%s terminated, exit code %d", process_table[i].name, WEXITSTATUS(status));

			restart_process(&process_table[i]);
			return;
		}
	}
	log_msg("SIGCHLD caught");
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
	log_err("SIGCHLD caught");
	wait_process();
}


void kill_process(void) {
int i, num;

	num = sizeof(process_table)/sizeof(Process);
	for(i = 0; i < num; i++) {
		if ((long)process_table[i].pid > -1L) {
			kill(process_table[i].pid, SIGTERM);
			process_table[i].pid = (pid_t)-1L;
		}
	}
}

/* EOB */
