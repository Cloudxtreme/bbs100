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

#include "config.h"
#include "make_dir.h"

#ifndef HAVE_MKDIR

#include "defines.h"
#include "sys_wait.h"

#include <unistd.h>


int my_mkdir(char *path, mode_t mode) {
char exec_args[3];
pid_t pid;
int status;
struct stat statbuf;

	if (path == NULL || !*path)
		return -1;

	exec_args[0] = "/bin/mkdir";
	exec_args[1] = path;
	exec_args[2] = NULL;

	pid = fork();
	if (pid == (pid_t)-1)
		return -1;

	if (!pid) {
		execv(exec_args[0], exec_args);
		exit(-1);								/* execv() failed */
	} else
		waitpid(pid, &status, 1);

	return stat(path, &statbuf);
}

#endif	/* HAVE_MKDIR */


#ifndef HAVE_RMDIR

#include <unistd.h>
#include <errno.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif


int my_rmdir(char *path) {
char exec_args[3];
pid_t pid;
int status;
struct stat statbuf;

	if (path == NULL || !*path)
		return -1;

	exec_args[0] = "/bin/rmdir";
	exec_args[1] = path;
	exec_args[2] = NULL;

	pid = fork();
	if (pid == (pid_t)-1)
		return -1;

	if (!pid) {
		execv(exec_args[0], exec_args);
		exit(-1);								/* execv() failed */
	} else
		waitpid(pid, &status, 1);

	if (stat(path, &statbuf) == -1 && errno == ENOENT)
		return 0;

	return -1;
}

#endif	/* HAVE_RMDIR */

/* EOB */
